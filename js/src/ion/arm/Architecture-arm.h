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

#ifndef jsion_architecture_arm_h__
#define jsion_architecture_arm_h__

#include "assembler/assembler/ARMAssembler.h"
#include "ion/shared/Assembler-shared.h"

namespace js {
namespace ion {

static const ptrdiff_t STACK_SLOT_SIZE       = 4;
static const uint32 DOUBLE_STACK_ALIGNMENT   = 2;

// In bytes: slots needed for potential memory->memory move spills.
//   +8 for cycles
//   +4 for gpr spills
//   +8 for double spills
static const uint32 ION_FRAME_SLACK_SIZE    = 20;

// An offset that is illegal for a local variable's stack allocation.
static const int32 INVALID_STACK_SLOT       = -1;

////
// These offsets are related to bailouts.
////

// Size of each bailout table entry. On arm, this is at most a ldr, then a branch
static const uint32 BAILOUT_TABLE_ENTRY_SIZE    = 8;

class Registers
{
  public:
    typedef JSC::ARMRegisters::RegisterID Code;

    static const char *GetName(Code code) {
        static const char *Names[] = { "r0", "r1", "r2", "r3",
                                       "r4", "r5", "r6", "r7",
                                       "r8", "r9", "r10", "r11",
                                       "r12", "sp", "r14", "pc"};
        return Names[code];
    }

    static const Code StackPointer = JSC::ARMRegisters::sp;
    static const Code Invalid = JSC::ARMRegisters::invalid_reg;

    static const uint32 Total = 16;
    static const uint32 Allocatable = 13;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 VolatileMask =
        (1 << JSC::ARMRegisters::r0) |
        (1 << JSC::ARMRegisters::r1) |
        (1 << JSC::ARMRegisters::r2) |
        (1 << JSC::ARMRegisters::r3);

    static const uint32 NonVolatileMask =
        (1 << JSC::ARMRegisters::r4) |
        (1 << JSC::ARMRegisters::r5) |
        (1 << JSC::ARMRegisters::r6) |
        (1 << JSC::ARMRegisters::r7) |
        (1 << JSC::ARMRegisters::r8) |
        (1 << JSC::ARMRegisters::r9) |
        (1 << JSC::ARMRegisters::r10) |
        (1 << JSC::ARMRegisters::r11) |
        (1 << JSC::ARMRegisters::r12) |
        (1 << JSC::ARMRegisters::r14);

    static const uint32 SingleByteRegs =
        VolatileMask | NonVolatileMask;
    // we should also account for any scratch registers that we care about.x
    // possibly the stack as well.
    static const uint32 NonAllocatableMask =
        (1 << JSC::ARMRegisters::sp) |
        (1 << JSC::ARMRegisters::r12) | // r12 = ip = scratch
        (1 << JSC::ARMRegisters::pc);

    static const uint32 JSCallClobberMask =
              (1 << JSC::ARMRegisters::r0) |
              (1 << JSC::ARMRegisters::r1) |
              (1 << JSC::ARMRegisters::r2) |
              (1 << JSC::ARMRegisters::r3);
    static const uint32 AllocatableMask = AllMask & ~NonAllocatableMask;
};

class FloatRegisters
{
  public:
    typedef JSC::ARMRegisters::FPRegisterID Code;

    static const char *GetName(Code code) {
        static const char *Names[] = { "d0", "d1", "d2", "d3",
                                       "d4", "d5", "d6", "d7",
                                       "d8", "d9", "d10", "d11",
                                       "d12", "d13", "d14", "d15"};
        return Names[code];
    }

    static const Code Invalid = JSC::ARMRegisters::invalid_freg;

    static const uint32 Total = 16;
    static const uint32 Allocatable = 15;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 VolatileMask = AllMask;
    static const uint32 NonVolatileMask = 0;

    static const uint32 NonAllocatableMask =
        // the scratch float register for ARM.
        (1 << JSC::ARMRegisters::SD0);

    // Registers that can be allocated without being saved, generally.
    static const uint32 TempMask = VolatileMask & ~NonAllocatableMask;

    static const uint32 AllocatableMask = AllMask & ~NonAllocatableMask;
    static const uint32 JSCallClobberMask = AllocatableMask;

};

bool hasMOVWT();
bool hasVFPv3();

} // namespace ion
} // namespace js

#endif // jsion_architecture_arm_h__
