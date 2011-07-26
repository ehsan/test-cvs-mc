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

#include "ion/MIR.h"
#include "Lowering-x64.h"
#include "ion/IonLowering-inl.h"
#include "Assembler-x64.h"

using namespace js;
using namespace js::ion;

bool
LIRGeneratorX64::fillBoxUses(LInstruction *lir, size_t n, MDefinition *mir)
{
    lir->setOperand(n, useRegister(mir));
    return true;
}

bool
LIRGeneratorX64::visitConstant(MConstant *ins)
{
    if (!ins->isEmittedAtUses())
        return emitAtUses(ins);

    return LIRGenerator::visitConstant(ins);
}

bool
LIRGeneratorX64::visitBox(MBox *box)
{
    MDefinition *opd = box->getOperand(0);

    // If the operand is a constant, emit near its uses.
    if (opd->isConstant() && !box->isEmittedAtUses())
        return emitAtUses(box);

    if (opd->isConstant())
        return define(new LValue(opd->toConstant()->value()), box, LDefinition(LDefinition::BOX));

    LBox *ins = new LBox(opd->type(), useRegister(opd));
    return define(ins, box, LDefinition(LDefinition::BOX));
}

bool
LIRGeneratorX64::visitUnbox(MUnbox *unbox)
{
    MDefinition *box = unbox->getOperand(0);

    switch (unbox->type()) {
      // Integers, booleans, and strings both need two outputs: the payload
      // and the type, the type of which is temporary and thrown away.
      case MIRType_Boolean: {
        LUnboxBoolean *ins = new LUnboxBoolean(useRegister(box), temp(LDefinition::INTEGER));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      case MIRType_Int32: {
        LUnboxInteger *ins = new LUnboxInteger(useRegister(box));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      case MIRType_String: {
        LUnboxString *ins = new LUnboxString(useRegister(box), temp(LDefinition::INTEGER));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      case MIRType_Object: {
        // Objects don't need a temporary.
        LDefinition out(LDefinition::POINTER);
        LUnboxObject *ins = new LUnboxObject(useRegister(box));
        return define(ins, unbox, out) && assignSnapshot(ins);
      }
      case MIRType_Double: {
        // Doubles don't need a temporary.
        LUnboxDouble *ins = new LUnboxDouble(useRegister(box));
        return define(ins, unbox) && assignSnapshot(ins);
      }
      default:
        JS_NOT_REACHED("cannot unbox a value with no payload");
    }

    return false;
}

bool
LIRGeneratorX64::visitReturn(MReturn *ret)
{
    MDefinition *opd = ret->getOperand(0);
    JS_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new LReturn;
    ins->setOperand(0, useFixed(opd, JSReturnReg));
    return add(ins);
}

bool
LIRGeneratorX64::preparePhi(MPhi *phi)
{
    uint32 vreg = getVirtualRegister();
    if (vreg >= MAX_VIRTUAL_REGISTERS)
        return false;

    phi->setId(vreg);
    return true;
}

void
LIRGeneratorX64::fillSnapshot(LSnapshot *snapshot)
{
    MSnapshot *mir = snapshot->mir();
    for (size_t i = 0; i < mir->numOperands(); i++) {
        MDefinition *ins = mir->getOperand(i);
        LAllocation *a = snapshot->getEntry(i);
        *a = useKeepaliveOrConstant(ins);
    }
}

bool
LIRGeneratorX64::lowerForALU(LMathI *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useOrConstant(rhs));
    return defineReuseInput(ins, mir);
}

