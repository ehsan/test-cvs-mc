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

#ifndef jsion_ion_lowering_x86_h__
#define jsion_ion_lowering_x86_h__

#include "ion/IonLowering.h"

namespace js {
namespace ion {

class LIRGeneratorX86 : public LIRGenerator
{
  public:
    LIRGeneratorX86(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGenerator(gen, graph, lirGraph)
    { }

  protected:
    // Uses components of a nunbox. Must be in a use request (startUsing,
    // stopUsing).
    LUse useType(MInstruction *mir);
    LUse useTypeOrConstant(MInstruction *mir);
    LUse usePayload(MInstruction *mir, LUse::Policy);
    LUse usePayloadInRegister(MInstruction *mir);

    // Adds a box input to an instruction, setting operand |n| to the type and
    // |n+1| to the payload. Does not modify the operands, instead expecting a
    // policy to already be set.
    bool fillBoxUses(LInstruction *lir, size_t n, MInstruction *mir);

    void fillSnapshot(LSnapshot *snapshot);
    bool preparePhi(MPhi *phi);

    bool lowerForALU(LMathI *ins, MInstruction *mir, MInstruction *lhs, MInstruction *rhs);

  public:
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitConstant(MConstant *ins);
    bool visitReturn(MReturn *ret);
    bool visitPhi(MPhi *phi);
};

typedef LIRGeneratorX86 LIRBuilder;

} // namespace js
} // namespace ion

#endif // jsion_ion_lowering_x86_h__

