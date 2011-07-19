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
 *   Andrew Drake <adrake@adrake.org>
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

#include "Ion.h"
#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "IonSpewer.h"
#include "IonLIR.h"
#include "GreedyAllocator.h"
#include "LICM.h"
#include "ValueNumbering.h"
#include "LinearScan.h"

#if defined(JS_CPU_X86)
# include "x86/Lowering-x86.h"
# include "x86/CodeGenerator-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/Lowering-x64.h"
# include "x64/CodeGenerator-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/Lowering-arm.h"
# include "arm/CodeGenerator-arm.h"
#endif
#include "jsgcmark.h"
#include "jsgcinlines.h"

using namespace js;
using namespace js::ion;

IonOptions ion::js_IonOptions;

// Assert that IonCode is gc::Cell aligned.
JS_STATIC_ASSERT(sizeof(IonCode) % gc::Cell::CellSize == 0);

#ifdef JS_THREADSAFE
static bool IonTLSInitialized = false;
static PRUintn IonTLSIndex;
#else
static IonContext *GlobalIonContext;
#endif

IonContext::IonContext(JSContext *cx, TempAllocator *temp)
  : cx(cx),
    temp(temp)
{
    SetIonContext(this);
}

IonContext::~IonContext()
{
    SetIonContext(NULL);
}

bool
ion::InitializeIon()
{
#ifdef JS_THREADSAFE
    if (!IonTLSInitialized) {
        PRStatus status = PR_NewThreadPrivateIndex(&IonTLSIndex, NULL);
        if (status != PR_SUCCESS)
            return false;
        IonTLSInitialized = true;
    }
#endif
    IonBuilder::SetupOpcodeFlags();
    CheckLogging();
    return true;
}

#ifdef JS_THREADSAFE
IonContext *
ion::GetIonContext()
{
    return (IonContext *)PR_GetThreadPrivate(IonTLSIndex);
}

bool
ion::SetIonContext(IonContext *ctx)
{
    return PR_SetThreadPrivate(IonTLSIndex, ctx) == PR_SUCCESS;
}
#else
IonContext *
ion::GetIonContext()
{
    JS_ASSERT(GlobalIonContext);
    return GlobalIonContext;
}

bool
ion::SetIonContext(IonContext *ctx)
{
    GlobalIonContext = ctx;
    return true;
}
#endif

IonCode *
IonCode::New(JSContext *cx, uint8 *code, uint32 size, JSC::ExecutablePool *pool)
{
    IonCode *codeObj = NewGCThing<IonCode>(cx, gc::FINALIZE_IONCODE, sizeof(IonCode));
    if (!codeObj) {
        pool->release();
        return NULL;
    }

    new (codeObj) IonCode(code, size, pool);
    return codeObj;
}

void
IonCode::finalize(JSContext *cx)
{
    if (pool_)
        pool_->release();
}

void
IonScript::trace(JSTracer *trc, JSScript *script)
{
    if (method)
        MarkIonCode(trc, method, "method");
}

void
IonScript::Destroy(JSContext *cx, JSScript *script)
{
    if (!script->ion || script->ion == ION_DISABLED_SCRIPT)
        return;

    cx->free_(script->ion);
}

static bool
TestCompiler(IonBuilder &builder, MIRGraph &graph)
{
    IonSpewer spew(&graph, builder.script);
    spew.init();

    if (!builder.build())
        return false;
    spew.spewPass("Build SSA");

    if (!SplitCriticalEdges(&builder, graph))
        return false;
    spew.spewPass("Split Critical Edges");

    if (!ReorderBlocks(graph))
        return false;
    spew.spewPass("Reorder Blocks");

    if (!BuildDominatorTree(graph))
        return false;
    spew.spewPass("Dominator tree");

    if (!BuildPhiReverseMapping(graph))
        return false;
    // No spew, graph not changed.

    if (!ApplyTypeInformation(graph))
        return false;
    spew.spewPass("Apply types");

    if (js_IonOptions.gvn) {
        ValueNumberer gvn(graph, js_IonOptions.gvnIsOptimistic);
        if (!gvn.analyze())
            return false;
        spew.spewPass("GVN");
    }

    if (js_IonOptions.licm) {
        LICM licm(graph);
        if (!licm.analyze())
            return false;
        spew.spewPass("LICM");
    }

    LIRGraph lir;
    LIRBuilder lirgen(&builder, graph, lir);
    if (!lirgen.generate())
        return false;
    spew.spewPass("Generate LIR");

    if (js_IonOptions.lsra) {
        LinearScanAllocator regalloc(&lirgen, lir);
        if (!regalloc.go())
            return false;
        spew.spewPass("Allocate Registers", &regalloc);
    } else {
        GreedyAllocator greedy(&builder, lir);
        if (!greedy.allocate())
            return false;
        spew.spewPass("Allocate Registers");
    }

    CodeGenerator codegen(&builder, lir);
    if (!codegen.generate())
        return false;
    spew.spewPass("Code generation");

    spew.finish();

    return true;
}

static bool
IonCompile(JSContext *cx, JSScript *script, StackFrame *fp)
{
    TempAllocator temp(&cx->tempPool);
    IonContext ictx(cx, &temp);

    MIRGraph graph;
    DummyOracle oracle;

    JSFunction *fun = fp->isFunctionFrame() ? fp->fun() : NULL;
    IonBuilder builder(cx, script, fun, temp, graph, &oracle);
    if (!TestCompiler(builder, graph))
        return false;

    return true;
}

MethodStatus
ion::Compile(JSContext *cx, JSScript *script, js::StackFrame *fp)
{
    JS_ASSERT(ion::IsEnabled());

    if (script->ion) {
        if (script->ion == ION_DISABLED_SCRIPT)
            return Method_CantCompile;

        // This will be enabled once we can run code.
        return Method_CantCompile;
    }

    if (!IonCompile(cx, script, fp)) {
        script->ion = ION_DISABLED_SCRIPT;
        return Method_CantCompile;
    }

    // This will be enabled once we can run code.
    return Method_CantCompile;
}

bool
ion::FireMahLaser(JSContext *cx)
{
    JS_ASSERT(ion::IsEnabled());

    // This will be enabled once we run code.
    return true;
}

