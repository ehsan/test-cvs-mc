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

#ifndef jsion_ion_lowering_inl_h__
#define jsion_ion_lowering_inl_h__

#include "MIR.h"
#include "MIRGraph.h"

namespace js {
namespace ion {

template <size_t X, size_t Y> bool
LIRGenerator::define(LInstructionHelper<1, X, Y> *lir, MInstruction *mir, const LDefinition &def)
{
    uint32 vreg = nextVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    // Assign the definition and a virtual register. Then, propagate this
    // virtual register to the MIR, so we can map MIR to LIR during lowering.
    lir->setDef(0, def);
    lir->getDef(0)->setVirtualRegister(vreg);
    mir->setId(vreg);
    mir->setInWorklistUnchecked();
    return add(lir);
}

template <size_t X, size_t Y> bool
LIRGenerator::define(LInstructionHelper<1, X, Y> *lir, MInstruction *mir, LDefinition::Policy policy)
{
    LDefinition::Type type;
    switch (mir->type()) {
      case MIRType_Boolean:
      case MIRType_Int32:
        type = LDefinition::INTEGER;
        break;
      case MIRType_String:
      case MIRType_Object:
        type = LDefinition::OBJECT;
        break;
      case MIRType_Double:
        type = LDefinition::DOUBLE;
        break;
#if defined(JS_PUNBOX64)
      case MIRType_Value:
        type = LDefinition::BOX;
        break;
#endif
      default:
        JS_NOT_REACHED("unexpected type");
        return false;
    }

    return define(lir, mir, LDefinition(type, policy));
}

void
LIRGenerator::startUsing(MInstruction *mir)
{
    JS_ASSERT(mir->inWorklist());
    if (!mir->id()) {
        // Instruction is generated on-demand, near its uses.
        if (mir->accept(this))
            JS_ASSERT(mir->id());
        else
            gen->error();
        mir->setNotInWorklist();
    }
}

void
LIRGenerator::stopUsing(MInstruction *mir)
{
    if (!mir->inWorklist()) {
        mir->setInWorklist();
        mir->setId(0);
    }
}

LUse
LIRGenerator::useRegister(MInstruction *mir)
{
    return use(mir, LUse(LUse::REGISTER));
}

LUse
LIRGenerator::use(MInstruction *mir)
{
    return use(mir, LUse(LUse::ANY));
}

LAllocation
LIRGenerator::useOrConstant(MInstruction *mir)
{
    if (mir->isConstant())
        return LAllocation(mir->toConstant()->vp());
    return use(mir);
}

LAllocation
LIRGenerator::useRegisterOrConstant(MInstruction *mir)
{
    if (mir->isConstant())
        return LAllocation(mir->toConstant()->vp());
    return use(mir, LUse(LUse::REGISTER));
}

LUse
LIRGenerator::useFixed(MInstruction *mir, Register reg)
{
    return use(mir, LUse(reg));
}

LUse
LIRGenerator::useFixed(MInstruction *mir, FloatRegister reg)
{
    return use(mir, LUse(reg));
}

LDefinition
LIRGenerator::temp(LDefinition::Type type)
{
    uint32 vreg = nextVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS) {
        gen->error("max virtual registers");
        return LDefinition();
    }
    return LDefinition(vreg, type);
}

} // namespace js
} // namespace ion

#endif // jsion_ion_lowering_inl_h__

