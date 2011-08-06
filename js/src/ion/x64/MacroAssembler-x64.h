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

#ifndef jsion_macro_assembler_x64_h__
#define jsion_macro_assembler_x64_h__

#include "ion/shared/MacroAssembler-x86-shared.h"

namespace js {
namespace ion {

class MacroAssemblerX64 : public MacroAssemblerX86Shared
{
    static const uint32 StackAlignment = 16;

#ifdef _WIN64
    static const uint32 ShadowStackSpace = 32;
#else
    static const uint32 ShadowStackSpace = 0;
#endif

  protected:
    uint32 alignStackForCall(uint32 stackForArgs) {
        uint32 total = stackForArgs + ShadowStackSpace;
        uint32 displacement = total + framePushed_;
        return total + ComputeByteAlignment(displacement, StackAlignment);
    }

    uint32 alignStackForCall(uint32 stackForArgs, const Register &scratch) {
        // framePushed_ is bogus or we don't know it for sure, so instead, save
        // the original value of esp and then chop off its low bits. Then, we
        // push the original value of esp.
        movq(rsp, scratch);
        andq(Imm32(~(StackAlignment - 1)), rsp);
        push(scratch);
        uint32 total = stackForArgs + ShadowStackSpace;
        uint32 displacement = total + STACK_SLOT_SIZE;
        return total + ComputeByteAlignment(displacement, StackAlignment);
    }

    void restoreStackFromDynamicAlignment() {
        pop(rsp);
    }

  public:
    void reserveStack(uint32 amount) {
        if (amount)
            subq(Imm32(amount), rsp);
        framePushed_ += amount;
    }
    void freeStack(uint32 amount) {
        JS_ASSERT(amount <= framePushed_);
        if (amount)
            addq(Imm32(amount), rsp);
        framePushed_ -= amount;
    }
    void movePtr(ImmWord imm, const Register &dest) {
        movq(imm, dest);
    }
    void setStackArg(const Register &reg, uint32 arg) {
        movq(reg, Operand(rsp, (arg - NumArgRegs) * STACK_SLOT_SIZE + ShadowStackSpace));
    }
    void checkCallAlignment() {
#ifdef DEBUG
        Label good;
        movl(rsp, rax);
        testq(Imm32(StackAlignment - 1), rax);
        j(Equal, &good);
        breakpoint();
        bind(&good);
#endif
    }
};

typedef MacroAssemblerX64 MacroAssemblerSpecific;

} // namespace ion
} // namespace js

#endif // jsion_macro_assembler_x64_h__

