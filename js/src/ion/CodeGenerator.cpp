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
 *   David Anderson <dvander@alliedmods.net>
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

#include "CodeGenerator.h"
#include "IonLinker.h"
#include "MIRGenerator.h"

using namespace js;
using namespace js::ion;

CodeGenerator::CodeGenerator(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorSpecific(gen, graph)
{
}

bool
CodeGenerator::generateBody()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        current = graph.getBlock(i);
        masm.bind(current->label());
        for (LInstructionIterator iter = current->begin(); iter != current->end(); iter++) {
            if (!iter->accept(this))
                return false;
        }
        if (masm.oom())
            return false;
    }
    return true;
}

bool
CodeGenerator::generate()
{
    JSContext *cx = gen->cx;

    if (frameClass_ != FrameSizeClass::None()) {
        deoptTable_ = cx->compartment->ionCompartment()->getBailoutTable(cx, frameClass_);
        if (!deoptTable_)
            return false;
    }

    if (!generatePrologue())
        return false;
    if (!generateBody())
        return false;
    if (!generateEpilogue())
        return false;
    if (!generateOutOfLineCode())
        return false;

    if (masm.oom())
        return false;

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    JS_ASSERT(!gen->script->ion);

    gen->script->ion = IonScript::New(cx, snapshots_.length(), bailouts_.length(),
                                      graph.numConstants());
    if (!gen->script->ion)
        return false;

    gen->script->ion->setMethod(code);
    gen->script->ion->setDeoptTable(deoptTable_);
    if (snapshots_.length())
        gen->script->ion->copySnapshots(&snapshots_);
    if (bailouts_.length())
        gen->script->ion->copyBailoutTable(&bailouts_[0]);
    if (graph.numConstants())
        gen->script->ion->copyConstants(graph.constantPool());

    linkAbsoluteLabels();

    return true;
}

