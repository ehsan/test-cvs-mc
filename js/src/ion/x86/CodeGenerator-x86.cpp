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
#include "CodeGenerator-x86.h"
#include "ion/shared/CodeGenerator-shared-inl.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "jsnum.h"
#include "jsscope.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

CodeGeneratorX86::CodeGeneratorX86(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorX86Shared(gen, graph)
{
}

// The first two size classes are 128 and 256 bytes respectively. After that we
// increment by 512.
static const uint32 LAST_FRAME_SIZE = 512;
static const uint32 LAST_FRAME_INCREMENT = 512;
static const uint32 FrameSizes[] = { 128, 256, LAST_FRAME_SIZE };

FrameSizeClass
FrameSizeClass::FromDepth(uint32 frameDepth)
{
    for (uint32 i = 0; i < JS_ARRAY_LENGTH(FrameSizes); i++) {
        if (frameDepth < FrameSizes[i])
            return FrameSizeClass(i);
    }

    uint32 newFrameSize = frameDepth - LAST_FRAME_SIZE;
    uint32 sizeClass = (newFrameSize / LAST_FRAME_INCREMENT) + 1;

    return FrameSizeClass(JS_ARRAY_LENGTH(FrameSizes) + sizeClass);
}
uint32
FrameSizeClass::frameSize() const
{
    JS_ASSERT(class_ != NO_FRAME_SIZE_CLASS_ID);

    if (class_ < JS_ARRAY_LENGTH(FrameSizes))
        return FrameSizes[class_];

    uint32 step = class_ - JS_ARRAY_LENGTH(FrameSizes);
    return LAST_FRAME_SIZE + step * LAST_FRAME_INCREMENT;
}

ValueOperand
CodeGeneratorX86::ToValue(LInstruction *ins, size_t pos)
{
    Register typeReg = ToRegister(ins->getOperand(pos + TYPE_INDEX));
    Register payloadReg = ToRegister(ins->getOperand(pos + PAYLOAD_INDEX));
    return ValueOperand(typeReg, payloadReg);
}

bool
CodeGeneratorX86::visitValue(LValue *value)
{
    LDefinition *type = value->getDef(TYPE_INDEX);
    LDefinition *payload = value->getDef(PAYLOAD_INDEX);

    masm.moveValue(value->value(), ToRegister(type), ToRegister(payload));
    return true;
}

bool
CodeGeneratorX86::visitOsrValue(LOsrValue *value)
{
    const LAllocation *frame   = value->getOperand(0);
    const LDefinition *type    = value->getDef(TYPE_INDEX);
    const LDefinition *payload = value->getDef(PAYLOAD_INDEX);

    const ptrdiff_t frameOffset = value->mir()->frameOffset();

    const ptrdiff_t payloadOffset = frameOffset + NUNBOX32_PAYLOAD_OFFSET;
    const ptrdiff_t typeOffset    = frameOffset + NUNBOX32_TYPE_OFFSET;

    masm.movl(Operand(ToRegister(frame), payloadOffset), ToRegister(payload));
    masm.movl(Operand(ToRegister(frame), typeOffset), ToRegister(type));

    return true;
}

static inline JSValueTag
MIRTypeToTag(MIRType type)
{
    switch (type) {
      case MIRType_Boolean:
        return JSVAL_TAG_BOOLEAN;
      case MIRType_Int32:
        return JSVAL_TAG_INT32;
      case MIRType_String:
        return JSVAL_TAG_STRING;
      case MIRType_Object:
        return JSVAL_TAG_OBJECT;
      default:
        JS_NOT_REACHED("no payload...");
    }
    return JSVAL_TAG_NULL;
}

bool
CodeGeneratorX86::visitBox(LBox *box)
{
    const LDefinition *type = box->getDef(TYPE_INDEX);

    DebugOnly<const LAllocation *> a = box->getOperand(0);
    JS_ASSERT(!a->isConstant());

    // On x86, the input operand and the output payload have the same
    // virtual register. All that needs to be written is the type tag for
    // the type definition.
    masm.movl(Imm32(MIRTypeToTag(box->type())), ToRegister(type));
    return true;
}

bool
CodeGeneratorX86::visitBoxDouble(LBoxDouble *box)
{
    const LDefinition *payload = box->getDef(PAYLOAD_INDEX);
    const LDefinition *type = box->getDef(TYPE_INDEX);
    const LAllocation *in = box->getOperand(0);

    masm.movd(ToFloatRegister(in), ToRegister(payload));
    masm.psrlq(Imm32(4), ToFloatRegister(in));
    masm.movd(ToFloatRegister(in), ToRegister(type));
    return true;
}

bool
CodeGeneratorX86::visitUnbox(LUnbox *unbox)
{
    // Note that for unbox, the type and payload indexes are switched on the
    // inputs.
    MUnbox *mir = unbox->mir();
    if (mir->fallible()) {
        masm.cmpl(ToOperand(unbox->type()), Imm32(MIRTypeToTag(mir->type())));
        if (!bailoutIf(Assembler::NotEqual, unbox->snapshot()))
            return false;
    }
    return true;
}

void
CodeGeneratorX86::linkAbsoluteLabels()
{
    JSScript *script = gen->info().script();
    IonCode *method = script->ion->method();

    for (size_t i = 0; i < deferredDoubles_.length(); i++) {
        DeferredDouble *d = deferredDoubles_[i];
        const Value &v = script->ion->getConstant(d->index());
        MacroAssembler::Bind(method, d->label(), &v);
    }
}

bool
CodeGeneratorX86::visitDouble(LDouble *ins)
{
    const LDefinition *out = ins->getDef(0);
    const LConstantIndex *cindex = ins->getOperand(0)->toConstantIndex();
    const Value &v = graph.getConstant(cindex->index());

    jsdpun dpun;
    dpun.d = v.toDouble();

    if (dpun.u64 == 0) {
        masm.xorpd(ToFloatRegister(out), ToFloatRegister(out));
        return true;
    }

    DeferredDouble *d = new DeferredDouble(cindex->index());
    if (!deferredDoubles_.append(d))
        return false;

    masm.movsd(d->label(), ToFloatRegister(out));
    return true;
}

Assembler::Condition
CodeGeneratorX86::testStringTruthy(bool truthy, const ValueOperand &value)
{
    Register string = value.payloadReg();
    Operand lengthAndFlags(string, JSString::offsetOfLengthAndFlags());

    size_t mask = (0xFFFFFFFF << JSString::LENGTH_SHIFT);
    masm.testl(lengthAndFlags, Imm32(mask));
    return truthy ? Assembler::NonZero : Assembler::Zero;
}

bool
CodeGeneratorX86::visitLoadSlotV(LLoadSlotV *load)
{
    Register type = ToRegister(load->getDef(TYPE_INDEX));
    Register payload = ToRegister(load->getDef(PAYLOAD_INDEX));
    Register base = ToRegister(load->input());
    int32 offset = load->mir()->slot() * sizeof(js::Value);

    masm.movl(Operand(base, offset + NUNBOX32_TYPE_OFFSET), type);
    masm.movl(Operand(base, offset + NUNBOX32_PAYLOAD_OFFSET), payload);
    return true;
}

bool
CodeGeneratorX86::visitLoadSlotT(LLoadSlotT *load)
{
    Register base = ToRegister(load->input());
    int32 offset = load->mir()->slot() * sizeof(js::Value);

    if (load->mir()->type() == MIRType_Double)
        masm.loadInt32OrDouble(Operand(base, offset), ToFloatRegister(load->output()));
    else
        masm.movl(Operand(base, offset + NUNBOX32_PAYLOAD_OFFSET), ToRegister(load->output()));
    return true;
}

bool
CodeGeneratorX86::visitStoreSlotT(LStoreSlotT *store)
{
    Register base = ToRegister(store->slots());
    int32 offset = store->mir()->slot() * sizeof(js::Value);

    const LAllocation *value = store->value();
    MIRType valueType = store->mir()->value()->type();

    if (valueType == MIRType_Double) {
        masm.movsd(ToFloatRegister(value), Operand(base, offset));
        return true;
    }

    // Store the type tag if needed.
    if (valueType != store->mir()->slotType())
        masm.storeTypeTag(ImmType(ValueTypeFromMIRType(valueType)), Operand(base, offset));

    // Store the payload.
    if (value->isConstant())
        masm.storePayload(*value->toConstant(), Operand(base, offset));
    else
        masm.storePayload(ToRegister(value), Operand(base, offset));

    return true;
}

bool
CodeGeneratorX86::visitLoadElementV(LLoadElementV *load)
{
    Operand source = createArrayElementOperand(ToRegister(load->elements()), load->index());
    Register type = ToRegister(load->getDef(TYPE_INDEX));
    Register payload = ToRegister(load->getDef(PAYLOAD_INDEX));

    masm.movl(masm.ToType(source), type);
    masm.movl(masm.ToPayload(source), payload);

    if (load->mir()->needsHoleCheck()) {
        masm.cmpl(type, ImmTag(JSVAL_TAG_MAGIC));
        return bailoutIf(Assembler::Equal, load->snapshot());
    }

    return true;
}

bool
CodeGeneratorX86::visitLoadElementT(LLoadElementT *load)
{
    Operand source = createArrayElementOperand(ToRegister(load->elements()), load->index());

    if (load->mir()->type() == MIRType_Double)
        masm.loadInt32OrDouble(source, ToFloatRegister(load->output()));
    else
        masm.movl(masm.ToPayload(source), ToRegister(load->output()));

    JS_ASSERT(!load->mir()->needsHoleCheck());
    return true;
}

bool
CodeGeneratorX86::visitStoreElementV(LStoreElementV *store)
{
    Operand dest = createArrayElementOperand(ToRegister(store->elements()), store->index());
    const ValueOperand value = ToValue(store, LStoreElementV::Value);

    masm.storeValue(value, dest);
    return true;
}

bool
CodeGeneratorX86::visitStoreElementT(LStoreElementT *store)
{
    Operand dest = createArrayElementOperand(ToRegister(store->elements()), store->index());

    const LAllocation *value = store->value();
    MIRType valueType = store->mir()->value()->type();

    if (valueType == MIRType_Double) {
        masm.movsd(ToFloatRegister(value), dest);
        return true;
    }

    // Store the type tag if needed.
    if (valueType != store->mir()->elementType())
        masm.storeTypeTag(ImmType(ValueTypeFromMIRType(valueType)), dest);

    // Store the payload.
    if (value->isConstant())
        masm.storePayload(*value->toConstant(), dest);
    else
        masm.storePayload(ToRegister(value), dest);

    return true;
}

bool
CodeGeneratorX86::visitWriteBarrierV(LWriteBarrierV *barrier)
{
    // TODO: Perform C++ call to some WriteBarrier stub.
    // For now, we just guard and breakpoint on failure.

    const ValueOperand value = ToValue(barrier, LWriteBarrierV::Input);

    Label skipBarrier;
    masm.branchTestGCThing(Assembler::NotEqual, value, &skipBarrier);
    {
        masm.breakpoint();
        masm.breakpoint();
    }
    masm.bind(&skipBarrier);

    return true;
}

bool
CodeGeneratorX86::visitWriteBarrierT(LWriteBarrierT *barrier)
{
    // TODO: Perform C++ call to some WriteBarrier stub.
    // For now, we just breakpoint.
    masm.breakpoint();
    masm.breakpoint();
    return true;
}

bool
CodeGeneratorX86::visitImplicitThis(LImplicitThis *lir)
{
    Register callee = ToRegister(lir->callee());
    Register type = ToRegister(lir->getDef(TYPE_INDEX));
    Register payload = ToRegister(lir->getDef(PAYLOAD_INDEX));

    // The implicit |this| is always |undefined| if the function's environment
    // is the current global.
    JSObject *global = gen->info().script()->global();
    masm.cmpPtr(Operand(callee, JSFunction::offsetOfEnvironment()), ImmGCPtr(global));

    // TODO: OOL stub path.
    if (!bailoutIf(Assembler::NotEqual, lir->snapshot()))
        return false;

    masm.moveValue(UndefinedValue(), type, payload);
    return true;
}
