/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=79:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Anderson <danderson@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonSpew.h"
#include "IonAnalysis.h"
#include "IonLowering.h"
#include "jsemit.h"
#include "jsscriptinlines.h"

#ifdef JS_THREADSAFE
# include "prthread.h"
#endif

using namespace js;
using namespace js::ion;

bool
ion::Go(JSContext *cx, JSScript *script, StackFrame *fp)
{
    TempAllocator temp(&cx->tempPool);
    IonContext ictx(cx, &temp);

    JSFunction *fun = fp->isFunctionFrame() ? fp->fun() : NULL;

    MIRGraph graph;

    DummyOracle oracle;
    C1Spewer spew(graph, script);
    IonBuilder analyzer(cx, script, fun, temp, graph, &oracle);
    spew.enable("/tmp/ion.cfg");

    if (!analyzer.analyze())
        return false;
    spew.spew("Build SSA");

    if (!ReorderBlocks(graph))
        return false;
    spew.spew("Reorder Blocks");

    if (!ApplyTypeInformation(graph))
        return false;
    if (!Lower(graph))
        return false;
    spew.spew("Lower");

    RenumberInstructions(graph);
    spew.spew("Renumber Instructions");

    return false;
}

IonBuilder::IonBuilder(JSContext *cx, JSScript *script, JSFunction *fun, TempAllocator &temp,
                       MIRGraph &graph, TypeOracle *oracle)
  : MIRGenerator(temp, script, fun, graph),
    cx(cx),
    oracle(oracle)
{
    pc = script->code;
    atoms = script->atomMap.vector;
}

static inline int32
GetJumpOffset(jsbytecode *pc)
{
    JSOp op = JSOp(*pc);
    JS_ASSERT(js_CodeSpec[op].type() == JOF_JUMP ||
              js_CodeSpec[op].type() == JOF_JUMPX);
    return (js_CodeSpec[op].type() == JOF_JUMP)
           ? GET_JUMP_OFFSET(pc)
           : GET_JUMPX_OFFSET(pc);
}

static inline jsbytecode *
GetNextPc(jsbytecode *pc)
{
    return pc + js_CodeSpec[JSOp(*pc)].length;
}

uint32
IonBuilder::readIndex(jsbytecode *pc)
{
    return (atoms - script->atomMap.vector) + GET_INDEX(pc);
}

IonBuilder::CFGState
IonBuilder::CFGState::If(jsbytecode *join, MBasicBlock *ifFalse)
{
    CFGState state;
    state.state = IF_TRUE;
    state.stopAt = join;
    state.branch.ifFalse = ifFalse;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::IfElse(jsbytecode *trueEnd, jsbytecode *falseEnd, MBasicBlock *ifFalse) 
{
    CFGState state;
    // If the end of the false path is the same as the start of the
    // false path, then the "else" block is empty and we can devolve
    // this to the IF_TRUE case. We handle this here because there is
    // still an extra GOTO on the true path and we want stopAt to point
    // there, whereas the IF_TRUE case does not have the GOTO.
    state.state = (falseEnd == ifFalse->pc())
                  ? IF_TRUE_EMPTY_ELSE
                  : IF_ELSE_TRUE;
    state.stopAt = trueEnd;
    state.branch.falseEnd = falseEnd;
    state.branch.ifFalse = ifFalse;
    return state;
}

void
IonBuilder::popCfgStack()
{
    if (cfgStack_.back().isLoop())
        loops_.popBack();
    cfgStack_.popBack();
}

bool
IonBuilder::pushLoop(CFGState::State initial, jsbytecode *stopAt, MBasicBlock *entry,
                     jsbytecode *bodyStart, jsbytecode *bodyEnd, jsbytecode *exitpc,
                     jsbytecode *continuepc)
{
    if (!continuepc)
        continuepc = entry->pc();

    LoopInfo loop(cfgStack_.length(), continuepc);
    if (!loops_.append(loop))
        return false;

    CFGState state;
    state.state = initial;
    state.stopAt = stopAt;
    state.loop.bodyStart = bodyStart;
    state.loop.bodyEnd = bodyEnd;
    state.loop.exitpc = exitpc;
    state.loop.entry = entry;
    state.loop.successor = NULL;
    state.loop.breaks = NULL;
    state.loop.continues = NULL;
    return cfgStack_.append(state);
}

bool
IonBuilder::analyze()
{
    current = newBlock(pc);
    if (!current)
        return false;

    // Initialize argument references if inside a function frame.
    if (fun()) {
        MParameter *param = MParameter::New(MParameter::CALLEE_SLOT);
        current->add(param);
        current->initSlot(calleeSlot(), param);

        param = MParameter::New(MParameter::THIS_SLOT);
        current->add(param);
        current->initSlot(thisSlot(), param);

        for (uint32 i = 0; i < nargs(); i++) {
            param = MParameter::New(int(i));
            current->add(param);
            current->initSlot(argSlot(i), param);
        }
    }

    // Initialize local variables.
    for (uint32 i = 0; i < nlocals(); i++) {
        MConstant *undef = MConstant::New(UndefinedValue());
        current->add(undef);
        current->initSlot(localSlot(i), undef);
    }

    if (!traverseBytecode())
        return false;

    return true;
}

// We try to build a control-flow graph in the order that it would be built as
// if traversing the AST. This leads to a nice ordering and lets us build SSA
// in one pass, since the bytecode is structured.
//
// We traverse the bytecode iteratively, maintaining a current basic block.
// Each basic block has a mapping of local slots to instructions, as well as a
// stack depth. As we encounter instructions we mutate this mapping in the
// current block.
//
// Things get interesting when we encounter a control structure. This can be
// either an IFEQ, downward GOTO, or a decompiler hint stashed away in source
// notes. Once we encounter such an opcode, we recover the structure of the
// control flow (its branches and bounds), and push it on a stack.
//
// As we continue traversing the bytecode, we look for points that would
// terminate the topmost control flow path pushed on the stack. These are:
//  (1) The bounds of the current structure (end of a loop or join/edge of a
//      branch).
//  (2) A "return", "break", or "continue" statement.
//
// For (1), we expect that there is a current block in the progress of being
// built, and we complete the necessary edges in the CFG. For (2), we expect
// that there is no active block.
//
// For normal diamond join points, we construct Phi nodes as we add
// predecessors. For loops, care must be taken to propagate Phi nodes back
// through uses in the loop body.
bool
IonBuilder::traverseBytecode()
{
    for (;;) {
        JS_ASSERT(pc < script->code + script->length);

        for (;;) {
            if (!temp().ensureBallast())
                return false;

            // Check if we've hit an expected join point or edge in the bytecode.
            // Leaving one control structure could place us at the edge of another,
            // thus |while| instead of |if| so we don't skip any opcodes.
            if (!cfgStack_.empty() && cfgStack_.back().stopAt == pc) {
                ControlStatus status = processCfgStack();
                if (status == ControlStatus_Error)
                    return false;
                if (!current)
                    return true;
                continue;
            }

            // Some opcodes need to be handled early because they affect control
            // flow, terminating the current basic block and/or instructing the
            // traversal algorithm to continue from a new pc.
            //
            //   (1) If the opcode does not affect control flow, then the opcode
            //       is inspected and transformed to IR. This is the process_opcode
            //       label.
            //   (2) A loop could be detected via a forward GOTO. In this case,
            //       we don't want to process the GOTO, but the following
            //       instruction.
            //   (3) A RETURN, STOP, BREAK, or CONTINUE may require processing the
            //       CFG stack to terminate open branches.
            //
            // Similar to above, snooping control flow could land us at another
            // control flow point, so we iterate until it's time to inspect a real
            // opcode.
            ControlStatus status;
            if ((status = snoopControlFlow(JSOp(*pc))) == ControlStatus_None)
                break;
            if (status == ControlStatus_Error)
                return false;
            if (!current)
                return true;
        }

        // Nothing in inspectOpcode() is allowed to advance the pc.
        JSOp op = JSOp(*pc);
        if (!inspectOpcode(op))
            return false;

        pc += js_CodeSpec[op].length;
    }

    return true;
}

IonBuilder::ControlStatus
IonBuilder::snoopControlFlow(JSOp op)
{
    switch (op) {
      case JSOP_NOP:
        return maybeLoop(op, js_GetSrcNote(script, pc));

      case JSOP_POP:
        return maybeLoop(op, js_GetSrcNote(script, pc));

      case JSOP_RETURN:
      case JSOP_STOP:
        return processReturn(op);

      case JSOP_GOTO:
      case JSOP_GOTOX:
      {
        jssrcnote *sn = js_GetSrcNote(script, pc);
        switch (sn ? SN_TYPE(sn) : SRC_NULL) {
          case SRC_BREAK:
          case SRC_BREAK2LABEL:
            return processBreak(op, sn);

          case SRC_CONTINUE:
          case SRC_CONT2LABEL:
            return processContinue(op, sn);

          case SRC_WHILE:
            // while (cond) { }
            if (!whileLoop(op, sn))
              return ControlStatus_Error;
            return ControlStatus_Jumped;

          case SRC_FOR_IN:
            // for (x in y) { }
            if (!forInLoop(op, sn))
              return ControlStatus_Error;
            return ControlStatus_Jumped;

          default:
            // Hard assert for now - make an error later.
            JS_NOT_REACHED("unknown goto case");
            break;
        }
        break;
      }

      case JSOP_IFNE:
      case JSOP_IFNEX:
        // We should never reach an IFNE, it's a stopAt point, which will
        // trigger closing the loop.
        JS_NOT_REACHED("we should never reach an ifne!");
        return ControlStatus_Error;

      default:
        break;
    }
    return ControlStatus_None;
}

bool
IonBuilder::inspectOpcode(JSOp op)
{
    switch (op) {
      case JSOP_NOP:
        return true;

      case JSOP_PUSH:
        return pushConstant(UndefinedValue());

      case JSOP_IFEQ:
        return jsop_ifeq(JSOP_IFEQ);

      case JSOP_BITAND:
        return jsop_binary(op);

      case JSOP_ADD:
      	return jsop_binary(op);

      case JSOP_DOUBLE:
        return pushConstant(script->getConst(readIndex(pc)));

      case JSOP_STRING:
        return pushConstant(StringValue(atoms[GET_INDEX(pc)]));

      case JSOP_ZERO:
        return pushConstant(Int32Value(0));

      case JSOP_ONE:
        return pushConstant(Int32Value(1));

      case JSOP_NULL:
        return pushConstant(NullValue());

      case JSOP_FALSE:
        return pushConstant(BooleanValue(false));

      case JSOP_TRUE:
        return pushConstant(BooleanValue(true));

      case JSOP_GETARG:
        current->pushArg(GET_SLOTNO(pc));
        return true;

      case JSOP_GETLOCAL:
        current->pushLocal(GET_SLOTNO(pc));
        return true;

      case JSOP_SETLOCAL:
        return current->setLocal(GET_SLOTNO(pc));

      case JSOP_POP:
        current->pop();
        return true;

      case JSOP_IFEQX:
        return jsop_ifeq(JSOP_IFEQX);

      case JSOP_NULLBLOCKCHAIN:
        return true;

      case JSOP_INT8:
        return pushConstant(Int32Value(GET_INT8(pc)));

      case JSOP_TRACE:
        assertValidTraceOp(op);
        return true;

      default:
        fprintf(stdout, "%d\n", op);
        JS_NOT_REACHED("what");
        return false;
    }
}


// Given that the current control flow structure has ended forcefully,
// via a return, break, or continue (rather than joining), propagate the
// termination up. For example, a return nested 5 loops deep may terminate
// every outer loop at once, if there are no intervening conditionals:
//
// for (...) {
//   for (...) {
//     return x;
//   }
// }
//
// If |current| is NULL when this function returns, then there is no more
// control flow to be processed.
IonBuilder::ControlStatus
IonBuilder::processControlEnd()
{
    JS_ASSERT(!current);

    if (cfgStack_.empty()) {
        // If there is no more control flow to process, then this is the
        // last return in the function.
        return ControlStatus_Ended;
    }

    return processCfgStack();
}

// Processes the top of the CFG stack. This is used from two places:
// (1) processControlEnd(), whereby a break, continue, or return may interrupt
//     an in-progress CFG structure before reaching its actual termination
//     point in the bytecode.
// (2) traverseBytecode(), whereby we reach the last instruction in a CFG
//     structure.
IonBuilder::ControlStatus
IonBuilder::processCfgStack()
{
    ControlStatus status = processCfgEntry(cfgStack_.back());

    // If this terminated a CFG structure, act like processControlEnd() and
    // keep propagating upward.
    while (status == ControlStatus_Ended) {
        popCfgStack();
        if (cfgStack_.empty())
            return status;
        status = processCfgEntry(cfgStack_.back());
    }

    // If some join took place, the current structure is finished.
    if (status == ControlStatus_Joined)
        popCfgStack();

    return status;
}

IonBuilder::ControlStatus
IonBuilder::processCfgEntry(CFGState &state)
{
    switch (state.state) {
      case CFGState::IF_TRUE:
      case CFGState::IF_TRUE_EMPTY_ELSE:
        return processIfEnd(state);

      case CFGState::IF_ELSE_TRUE:
        return processIfElseTrueEnd(state);

      case CFGState::IF_ELSE_FALSE:
        return processIfElseFalseEnd(state);

      case CFGState::DO_WHILE_LOOP:
        return processDoWhileEnd(state);

      case CFGState::WHILE_LOOP_COND:
        return processWhileCondEnd(state);

      case CFGState::WHILE_LOOP_BODY:
        return processWhileBodyEnd(state);

      case CFGState::FOR_LOOP_COND:
        return processForCondEnd(state);

      case CFGState::FOR_LOOP_BODY:
        return processForBodyEnd(state);

      case CFGState::FOR_LOOP_UPDATE:
        return processForUpdateEnd(state);

      default:
        JS_NOT_REACHED("unknown cfgstate");
    }
    return ControlStatus_Error;
}

IonBuilder::ControlStatus
IonBuilder::processIfEnd(CFGState &state)
{
    if (current) {
        // Here, the false block is the join point. Create an edge from the
        // current block to the false block. Note that a RETURN opcode
        // could have already ended the block.
        current->end(MGoto::New(state.branch.ifFalse));

        if (!current->addPredecessor(state.branch.ifFalse))
            return ControlStatus_Error;
    }

    current = state.branch.ifFalse;
    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processIfElseTrueEnd(CFGState &state)
{
    // We've reached the end of the true branch of an if-else. Don't
    // create an edge yet, just transition to parsing the false branch.
    state.state = CFGState::IF_ELSE_FALSE;
    state.branch.ifTrue = current;
    state.stopAt = state.branch.falseEnd;
    pc = state.branch.ifFalse->pc();
    current = state.branch.ifFalse;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processIfElseFalseEnd(CFGState &state)
{
    // Update the state to have the latest block from the false path.
    state.branch.ifFalse = current;
  
    // To create the join node, we need an incoming edge that has not been
    // terminated yet.
    MBasicBlock *pred = state.branch.ifTrue
                        ? state.branch.ifTrue
                        : state.branch.ifFalse;
    MBasicBlock *other = (pred == state.branch.ifTrue) ? state.branch.ifFalse : state.branch.ifTrue;
  
    if (!pred)
        return ControlStatus_Ended;
  
    // Create a new block to represent the join.
    MBasicBlock *join = newBlock(pred, pc);
    if (!join)
        return ControlStatus_Error;
  
    // Create edges from the true and false blocks as needed.
    pred->end(MGoto::New(join));

    if (other) {
        other->end(MGoto::New(join));
        if (!join->addPredecessor(other))
            return ControlStatus_Error;
    }
  
    // Continue parsing at the join point.
    current = join;
    return ControlStatus_Joined;
}

bool
IonBuilder::finalizeLoop(CFGState &state, MInstruction *last)
{
    JS_ASSERT(!state.loop.continues);

    // Create a block to catch all breaks.
    MBasicBlock *breaks = NULL;
    if (state.loop.breaks) {
        breaks = newBlock(state.loop.exitpc);
        if (!breaks)
            return false;

        DeferredEdge *edge = state.loop.breaks;
        while (edge) {
            edge->block->end(MGoto::New(breaks));
            if (!breaks->addPredecessor(edge->block))
                return false;
            edge = edge->next;
        }
    }

    // If there is no successor, but there are breaks, treat the catch-all
    // break block as the actual successor.
    MBasicBlock *successor = state.loop.successor
                             ? state.loop.successor
                             : breaks;

    // Place phis in the loop header, propagating their assignment to the
    // successor block. If there is no |current|, then the successor is still
    // reachable via the original test branch, and thus its state is coherent.
    if (current) {
        JS_ASSERT_IF(last, successor);

        MControlInstruction *ins;
        if (last)
            ins = MTest::New(last, state.loop.entry, successor);
        else
            ins = MGoto::New(state.loop.entry);
        current->end(ins);

        if (!state.loop.entry->setBackedge(current, successor))
            return false;
    }

    // Now that phis are computed in the successor block, see if we need to
    // link the successor and break blocks together.
    if (successor && breaks && (successor != breaks)) {
        breaks->end(MGoto::New(successor));
        if (!successor->addPredecessor(breaks))
            return false;
    }

    state.loop.successor = successor;
    return true;
}

IonBuilder::ControlStatus
IonBuilder::processDoWhileEnd(CFGState &state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;

    MInstruction *last = NULL;
    if (current) {
        last = current->pop();
        state.loop.successor = newBlock(current, GetNextPc(pc));
        if (!state.loop.successor)
            return ControlStatus_Error;
    }

    if (!finalizeLoop(state, last))
        return ControlStatus_Error;

    current = state.loop.successor;
    if (!current)
        return ControlStatus_Ended;

    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processWhileCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);

    // Balance the stack past the IFNE.
    MInstruction *ins = current->pop();

    // Create the body and successor blocks.
    MBasicBlock *body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc);
    if (!body || !state.loop.successor)
        return ControlStatus_Error;

    MTest *test = MTest::New(ins, body, state.loop.successor);
    current->end(test);

    state.state = CFGState::WHILE_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    current = body;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processWhileBodyEnd(CFGState &state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;
    if (!finalizeLoop(state, NULL))
        return ControlStatus_Error;

    current = state.loop.successor;
    if (!current)
        return ControlStatus_Ended;

    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processForCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);

    // Balance the stack past the IFNE.
    MInstruction *ins = current->pop();

    // Create the body and successor blocks.
    MBasicBlock *body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc);
    if (!body || !state.loop.successor)
        return ControlStatus_Error;

    MTest *test = MTest::New(ins, body, state.loop.successor);
    current->end(test);

    state.state = CFGState::FOR_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    current = body;
    return ControlStatus_Jumped;
}

bool
IonBuilder::processDeferredContinues(CFGState &state)
{
    // If there are any continues for this loop, and there is an update block,
    // then we need to create a new basic block to house the update.
    if (state.loop.continues) {
        MBasicBlock *update = newBlock(pc);
        if (!update)
            return false;

        if (current) {
            current->end(MGoto::New(update));
            if (!update->addPredecessor(current))
                return ControlStatus_Error;
        }

        DeferredEdge *edge = state.loop.continues;
        while (edge) {
            edge->block->end(MGoto::New(update));
            if (!update->addPredecessor(edge->block))
                return ControlStatus_Error;
            edge = edge->next;
        }
        state.loop.continues = NULL;

        current = update;
    }

    return true;
}

IonBuilder::ControlStatus
IonBuilder::processForBodyEnd(CFGState &state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;

    if (!state.loop.updatepc)
        return processForUpdateEnd(state);

    pc = state.loop.updatepc;

    state.state = CFGState::FOR_LOOP_UPDATE;
    state.stopAt = state.loop.updateEnd;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processForUpdateEnd(CFGState &state)
{
    if (!finalizeLoop(state, NULL))
        return ControlStatus_Error;

    current = state.loop.successor;
    if (!current)
        return ControlStatus_Ended;

    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processBreak(JSOp op, jssrcnote *sn)
{
    JS_ASSERT(op == JSOP_GOTO || op == JSOP_GOTOX);

    // Find the target loop.
    CFGState *found = NULL;
    jsbytecode *target = pc + GetJumpOffset(pc);
    for (size_t i = loops_.length() - 1; i < loops_.length(); i--) {
        CFGState &cfg = cfgStack_[loops_[i].cfgEntry];
        if (cfg.loop.exitpc == target) {
            found = &cfg;
            break;
        }
    }

    // There must always be a valid target loop structure. If not, there's
    // probably an off-by-something error in which pc we track.
    JS_ASSERT(found);
    CFGState &state = *found;

    state.loop.breaks = new DeferredEdge(current, state.loop.breaks);

    current = NULL;
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::processContinue(JSOp op, jssrcnote *sn)
{
    JS_ASSERT(op == JSOP_GOTO || op == JSOP_GOTOX);

    // Find the target loop.
    CFGState *found = NULL;
    jsbytecode *target = pc + GetJumpOffset(pc);
    for (size_t i = loops_.length() - 1; i < loops_.length(); i--) {
        if (loops_[i].continuepc == target) {
            found = &cfgStack_[loops_[i].cfgEntry];
            break;
        }
    }

    // There must always be a valid target loop structure. If not, there's
    // probably an off-by-something error in which pc we track.
    JS_ASSERT(found);
    CFGState &state = *found;

    state.loop.continues = new DeferredEdge(current, state.loop.continues);

    current = NULL;
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::maybeLoop(JSOp op, jssrcnote *sn)
{
    // This function looks at the opcode and source note and tries to
    // determine the structure of the loop. For some opcodes, like
    // POP/NOP which are not explicitly control flow, this source note is
    // optional. For opcodes with control flow, like GOTO, an unrecognized
    // or not-present source note is a compilation failure.
    switch (op) {
      case JSOP_POP:
        // for (init; ; update?) ...
        if (sn && SN_TYPE(sn) == SRC_FOR) {
            current->pop();
            return forLoop(op, sn);
        }
        break;

      case JSOP_NOP:
        if (sn) {
            // do { } while (cond)
            if (SN_TYPE(sn) == SRC_WHILE)
                return doWhileLoop(op, sn);

            // for (; ; update?)
            if (SN_TYPE(sn) == SRC_FOR)
                return forLoop(op, sn);
        }
        break;

      default:
        JS_NOT_REACHED("unexpected opcode");
        return ControlStatus_Error;
    }

    return ControlStatus_None;
}

void
IonBuilder::assertValidTraceOp(JSOp op)
{
#ifdef DEBUG
    jssrcnote *sn = js_GetSrcNote(script, pc);
    jsbytecode *ifne = pc + js_GetSrcNoteOffset(sn, 0);
    CFGState &state = cfgStack_.back();

    // Make sure this is the next opcode after the loop header.
    JS_ASSERT(GetNextPc(state.loop.entry->pc()) == pc);

    jsbytecode *expected_ifne;
    switch (state.state) {
      case CFGState::DO_WHILE_LOOP:
        expected_ifne = state.stopAt;
        break;

      default:
        JS_NOT_REACHED("JSOP_TRACE appeared in unknown control flow construct");
        return;
    }

    // Make sure this trace op goes to the same ifne as the loop header's
    // source notes or GOTO.
    JS_ASSERT(ifne == expected_ifne);
#endif
}

IonBuilder::ControlStatus
IonBuilder::doWhileLoop(JSOp op, jssrcnote *sn)
{
    int offset = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *ifne = pc + offset;
    JS_ASSERT(ifne > pc);

    // Verify that the IFNE goes back to a trace op.
    JS_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_TRACE);
    JS_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    // do { } while() loops have the following structure:
    //    NOP         ; SRC_WHILE (offset to IFNE)
    //    TRACE       ; SRC_WHILE (offset to IFNE)
    //    ...
    //    IFNE ->     ; goes to TRACE
    MBasicBlock *header = newLoopHeader(current, pc);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(header));

    current = header;
    jsbytecode *bodyStart = GetNextPc(GetNextPc(pc));
    jsbytecode *exitpc = GetNextPc(ifne);
    if (!pushLoop(CFGState::DO_WHILE_LOOP, ifne, header, bodyStart, ifne, exitpc))
        return ControlStatus_Error;

    pc = GetNextPc(GetNextPc(pc));
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::whileLoop(JSOp op, jssrcnote *sn)
{
    // while (cond) { } loops have the following structure:
    //    GOTO cond   ; SRC_WHILE (offset to IFNE)
    //    TRACE       ; SRC_WHILE (offset to IFNE)
    //    ...
    //  cond:
    //    ...
    //    IFNE        ; goes to TRACE
    int ifneOffset = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *ifne = pc + ifneOffset;
    JS_ASSERT(ifne > pc);

    // Verify that the IFNE goes back to a trace op.
    JS_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_TRACE);
    JS_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    MBasicBlock *header = newLoopHeader(current, pc);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(header));

    // Skip past the JSOP_TRACE for the body start.
    jsbytecode *bodyStart = GetNextPc(GetNextPc(pc));
    jsbytecode *bodyEnd = pc + GetJumpOffset(pc);
    jsbytecode *exitpc = GetNextPc(ifne);
    if (!pushLoop(CFGState::WHILE_LOOP_COND, ifne, header, bodyStart, bodyEnd, exitpc))
        return ControlStatus_Error;

    // Parse the condition first.
    pc = bodyEnd;
    current = header;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::forLoop(JSOp op, jssrcnote *sn)
{
    // Skip the NOP or POP.
    JS_ASSERT(op == JSOP_POP || op == JSOP_NOP);
    pc = GetNextPc(pc);

    jsbytecode *condpc = pc + js_GetSrcNoteOffset(sn, 0);
    jsbytecode *updatepc = pc + js_GetSrcNoteOffset(sn, 1);
    jsbytecode *ifne = pc + js_GetSrcNoteOffset(sn, 2);
    jsbytecode *exitpc = GetNextPc(ifne);

    // for loops have the following structures:
    //
    //   NOP or POP
    //   [GOTO cond]
    //   TRACE
    // body:
    //    ; [body]
    // [increment:]
    //    ; [increment]
    // [cond:]
    //   GOTO body
    //
    // If there is a condition (condpc != ifne), this acts similar to a while
    // loop otherwise, it acts like a do-while loop.
    jsbytecode *bodyStart = pc;
    jsbytecode *bodyEnd = updatepc;
    if (condpc != ifne) {
        JS_ASSERT(JSOp(*bodyStart) == JSOP_GOTO || JSOp(*bodyStart) == JSOP_GOTOX);
        JS_ASSERT(bodyStart + GetJumpOffset(bodyStart) == condpc);
        bodyStart = GetNextPc(bodyStart);
    }
    JS_ASSERT(JSOp(*bodyStart) == JSOP_TRACE);
    JS_ASSERT(ifne + GetJumpOffset(ifne) == bodyStart);
    bodyStart = GetNextPc(bodyStart);

    MBasicBlock *header = newLoopHeader(current, pc);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(header));

    // If there is no condition, we immediately parse the body. Otherwise, we
    // parse the condition.
    jsbytecode *stopAt;
    CFGState::State initial;
    if (condpc != ifne) {
        pc = condpc;
        stopAt = ifne;
        initial = CFGState::FOR_LOOP_COND;
    } else {
        pc = bodyStart;
        stopAt = bodyEnd;
        initial = CFGState::FOR_LOOP_BODY;
    }

    if (!pushLoop(initial, ifne, header, bodyStart, bodyEnd, exitpc, updatepc))
        return ControlStatus_Error;

    CFGState &state = cfgStack_.back();
    state.loop.condpc = (condpc != ifne) ? condpc : NULL;
    state.loop.updatepc = (updatepc != condpc) ? updatepc : NULL;
    if (state.loop.updatepc)
        state.loop.updateEnd = condpc;

    current = header;
    return ControlStatus_Jumped;
}

bool
IonBuilder::jsop_ifeq(JSOp op)
{
    // IFEQ always has a forward offset.
    jsbytecode *trueStart = pc + js_CodeSpec[op].length;
    jsbytecode *falseStart = pc + GetJumpOffset(pc);
    JS_ASSERT(falseStart > pc);

    // We only handle cases that emit source notes.
    jssrcnote *sn = js_GetSrcNote(script, pc);
    if (!sn) {
        // :FIXME: log this.
        return false;
    }

    MInstruction *ins = current->pop();

    // Create true and false branches.
    MBasicBlock *ifTrue = newBlock(current, trueStart);
    MBasicBlock *ifFalse = newBlock(current, falseStart);
    if (!ifTrue || !ifFalse)
        return false;

    current->end(MTest::New(ins, ifTrue, ifFalse));

    // The bytecode for if/ternary gets emitted either like this:
    //
    //    IFEQ X  ; src note (IF_ELSE, COND) points to the GOTO
    //    ...
    //    GOTO Z
    // X: ...     ; else/else if
    //    ...
    // Z:         ; join
    //
    // Or like this:
    //
    //    IFEQ X  ; src note (IF) has no offset
    //    ...
    // Z: ...     ; join
    //
    // We want to parse the bytecode as if we were parsing the AST, so for the
    // IF_ELSE/COND cases, we use the source note and follow the GOTO. For the
    // IF case, the IFEQ offset is the join point.
    switch (SN_TYPE(sn)) {
      case SRC_IF:
        if (!cfgStack_.append(CFGState::If(falseStart, ifFalse)))
            return false;
        break;

      case SRC_IF_ELSE:
      case SRC_COND:
      {
        // Infer the join point from the JSOP_GOTO[X] sitting here, then
        // assert as we much we can that this is the right GOTO.
        jsbytecode *trueEnd = pc + js_GetSrcNoteOffset(sn, 0);
        JS_ASSERT(trueEnd > pc);
        JS_ASSERT(trueEnd < falseStart);
        JS_ASSERT(JSOp(*trueEnd) == JSOP_GOTO || JSOp(*trueEnd) == JSOP_GOTOX);
        JS_ASSERT(!js_GetSrcNote(script, trueEnd));

        jsbytecode *falseEnd = trueEnd + GetJumpOffset(trueEnd);
        JS_ASSERT(falseEnd > trueEnd);
        JS_ASSERT(falseEnd >= falseStart);

        if (!cfgStack_.append(CFGState::IfElse(trueEnd, falseEnd, ifFalse)))
            return false;
        break;
      }

      default:
        JS_NOT_REACHED("unexpected source note type");
        break;
    }

    // Switch to parsing the true branch. Note that no PC update is needed,
    // it's the next instruction.
    current = ifTrue;

    return true;
}

IonBuilder::ControlStatus
IonBuilder::processReturn(JSOp op)
{
    MInstruction *ins;
    switch (op) {
      case JSOP_RETURN:
        ins = current->pop();
        break;

      case JSOP_STOP:
        ins = MConstant::New(UndefinedValue());
        current->add(ins);
        break;

      default:
        ins = NULL;
        JS_NOT_REACHED("unknown return op");
        break;
    }

    MReturn *ret = MReturn::New(ins);
    current->end(ret);

    // Make sure no one tries to use this block now.
    current = NULL;
    return processControlEnd();
}

bool
IonBuilder::pushConstant(const Value &v)
{
    MConstant *ins = MConstant::New(v);
    current->add(ins);
    current->push(ins);
    return true;
}

bool
IonBuilder::jsop_binary(JSOp op)
{
    // Take snapshot before killing inputs.
    MSnapshot *snapshot = takeSnapshot();

    // Pop inputs.
    MInstruction *right = current->pop();
    MInstruction *left = current->pop();

    MBinaryInstruction *ins;
    switch (op) {
      case JSOP_BITAND:
        ins = MBitAnd::New(left, right);
        break;
        
      case JSOP_ADD:
        ins = MAdd::New(left, right);
        break;

      default:
        JS_NOT_REACHED("unexpected binary opcode");
        return false;
    }

    current->add(ins);
    ins->infer(oracle->binaryOp(script, pc));
    ins->assignSnapshot(snapshot);

    current->push(ins);
    return true;
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::New(this, predecessor, pc);
    if (!graph().addBlock(block))
        return NULL;
    return block;
}

MBasicBlock *
IonBuilder::newLoopHeader(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::NewLoopHeader(this, predecessor, pc);
    if (!graph().addBlock(block))
        return NULL;
    return block;
}

MSnapshot *
IonBuilder::takeSnapshot()
{
    MSnapshot *snapshot = MSnapshot::New(current, pc);
    if (!snapshot)
        return NULL;
    current->add(snapshot);
    return snapshot;
}

