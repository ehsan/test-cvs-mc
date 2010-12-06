/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99:
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
 * The Original Code is Mozilla SpiderMonkey JavaScript 1.9 code, released
 * May 28, 2008.
 *
 * The Initial Developer of the Original Code is
 *   Brendan Eich <brendan@mozilla.org>
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

#if !defined jsjaeger_regstate_h__ && defined JS_METHODJIT
#define jsjaeger_regstate_h__

#include "jsbit.h"
#include "assembler/assembler/MacroAssembler.h"

namespace js {

namespace mjit {

/* Common handling for both general purpose and floating point registers. */

struct AnyRegisterID {
    unsigned reg_;

    AnyRegisterID()
        : reg_((unsigned)-1)
    {}

    AnyRegisterID(const AnyRegisterID &o)
        : reg_(o.reg_)
    {}

    AnyRegisterID(JSC::MacroAssembler::RegisterID reg)
        : reg_((unsigned)reg)
    {}

    AnyRegisterID(JSC::MacroAssembler::FPRegisterID reg)
        : reg_(JSC::MacroAssembler::TotalRegisters + (unsigned)reg)
    {}

    static inline AnyRegisterID fromRaw(unsigned reg);

    inline JSC::MacroAssembler::RegisterID reg();
    inline JSC::MacroAssembler::FPRegisterID fpreg();

    bool isReg() { return reg_ < JSC::MacroAssembler::TotalRegisters; }
    bool isSet() { return reg_ != unsigned(-1); }

    inline const char * name();
};

struct Registers {

    /* General purpose registers. */

    static const uint32 TotalRegisters = JSC::MacroAssembler::TotalRegisters;

    enum CallConvention {
        NormalCall,
        FastCall
    };

    typedef JSC::MacroAssembler::RegisterID RegisterID;

    // Homed and scratch registers for working with Values on x64.
#if defined(JS_CPU_X64)
    static const RegisterID TypeMaskReg = JSC::X86Registers::r13;
    static const RegisterID PayloadMaskReg = JSC::X86Registers::r14;
    static const RegisterID ValueReg = JSC::X86Registers::r10;
#endif

    // Register that homes the current JSStackFrame.
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const RegisterID JSFrameReg = JSC::X86Registers::ebx;
#elif defined(JS_CPU_ARM)
    static const RegisterID JSFrameReg = JSC::ARMRegisters::r11;
#endif

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const RegisterID ReturnReg = JSC::X86Registers::eax;
# if defined(JS_CPU_X86) || defined(_MSC_VER)
    static const RegisterID ArgReg0 = JSC::X86Registers::ecx;
    static const RegisterID ArgReg1 = JSC::X86Registers::edx;
#  if defined(JS_CPU_X64)
    static const RegisterID ArgReg2 = JSC::X86Registers::r8;
#  endif
# else
    static const RegisterID ArgReg0 = JSC::X86Registers::edi;
    static const RegisterID ArgReg1 = JSC::X86Registers::esi;
    static const RegisterID ArgReg2 = JSC::X86Registers::edx;
# endif
#elif JS_CPU_ARM
    static const RegisterID ReturnReg = JSC::ARMRegisters::r0;
    static const RegisterID ArgReg0 = JSC::ARMRegisters::r0;
    static const RegisterID ArgReg1 = JSC::ARMRegisters::r1;
    static const RegisterID ArgReg2 = JSC::ARMRegisters::r2;
#endif

    static const RegisterID StackPointer = JSC::MacroAssembler::stackPointerRegister;

    static inline uint32 maskReg(RegisterID reg) {
        return (1 << reg);
    }

    static inline uint32 mask2Regs(RegisterID reg1, RegisterID reg2) {
        return maskReg(reg1) | maskReg(reg2);
    }

    static inline uint32 mask3Regs(RegisterID reg1, RegisterID reg2, RegisterID reg3) {
        return maskReg(reg1) | maskReg(reg2) | maskReg(reg3);
    }

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const uint32 TempRegs =
          (1 << JSC::X86Registers::eax)
        | (1 << JSC::X86Registers::ecx)
        | (1 << JSC::X86Registers::edx)
# if defined(JS_CPU_X64)
        | (1 << JSC::X86Registers::r8)
        | (1 << JSC::X86Registers::r9)
#  if !defined(_MSC_VER)
        | (1 << JSC::X86Registers::esi)
        | (1 << JSC::X86Registers::edi)
#  endif
# endif
        ;

# if defined(JS_CPU_X64)
    static const uint32 SavedRegs =
        /* r11 is scratchRegister, used by JSC. */
          (1 << JSC::X86Registers::r12)
    // r13 is TypeMaskReg.
    // r14 is PayloadMaskReg.
        | (1 << JSC::X86Registers::r15)
#  if defined(_MSC_VER)
        | (1 << JSC::X86Registers::esi)
        | (1 << JSC::X86Registers::edi)
#  endif
# else
    static const uint32 SavedRegs =
          (1 << JSC::X86Registers::esi)
        | (1 << JSC::X86Registers::edi)
# endif
        ;

    static const uint32 SingleByteRegs = (TempRegs | SavedRegs) &
        ~((1 << JSC::X86Registers::esi) |
          (1 << JSC::X86Registers::edi) |
          (1 << JSC::X86Registers::ebp) |
          (1 << JSC::X86Registers::esp));

#elif defined(JS_CPU_ARM)
    static const uint32 TempRegs =
          (1 << JSC::ARMRegisters::r0)
        | (1 << JSC::ARMRegisters::r1)
        | (1 << JSC::ARMRegisters::r2);
    // r3 is reserved as a scratch register for the assembler.

    static const uint32 SavedRegs =
          (1 << JSC::ARMRegisters::r4)
        | (1 << JSC::ARMRegisters::r5)
        | (1 << JSC::ARMRegisters::r6)
        | (1 << JSC::ARMRegisters::r7)
    // r8 is reserved as a scratch register for the assembler.
        | (1 << JSC::ARMRegisters::r9)
        | (1 << JSC::ARMRegisters::r10);
    // r11 is reserved for JSFrameReg.
    // r12 is IP, and is used for stub calls.
    // r13 is SP and must always point to VMFrame whilst in generated code.
    // r14 is LR and is used for return sequences.
    // r15 is PC (program counter).

    static const uint32 SingleByteRegs = TempRegs | SavedRegs;
#else
# error "Unsupported platform"
#endif

    static const uint32 AvailRegs = SavedRegs | TempRegs;

    static bool isAvail(RegisterID reg) {
        uint32 mask = maskReg(reg);
        return bool(mask & AvailRegs);
    }

    static bool isSaved(RegisterID reg) {
        uint32 mask = maskReg(reg);
        JS_ASSERT(mask & AvailRegs);
        return bool(mask & SavedRegs);
    }

    static inline uint32 numArgRegs(CallConvention convention) {
#if defined(JS_CPU_X86)
# if defined(JS_NO_FASTCALL)
        return 0;
# else
        return (convention == FastCall) ? 2 : 0;
# endif
#elif defined(JS_CPU_X64)
# ifdef _WIN64
        return 4;
# else
        return 6;
# endif
#elif defined(JS_CPU_ARM)
        return 4;
#endif
    }

    static inline bool regForArg(CallConvention conv, uint32 i, RegisterID *reg) {
#if defined(JS_CPU_X86)
        static const RegisterID regs[] = {
            JSC::X86Registers::ecx,
            JSC::X86Registers::edx
        };

# if defined(JS_NO_FASTCALL)
        return false;
# else
        if (conv == NormalCall)
            return false;
# endif
#elif defined(JS_CPU_X64)
# ifdef _WIN64
        static const RegisterID regs[] = {
            JSC::X86Registers::ecx,
            JSC::X86Registers::edx,
            JSC::X86Registers::r8,
            JSC::X86Registers::r9
        };
# else
        static const RegisterID regs[] = {
            JSC::X86Registers::edi,
            JSC::X86Registers::esi,
            JSC::X86Registers::edx,
            JSC::X86Registers::ecx,
            JSC::X86Registers::r8,
            JSC::X86Registers::r9
        };
# endif
#elif defined(JS_CPU_ARM)
        static const RegisterID regs[] = {
            JSC::ARMRegisters::r0,
            JSC::ARMRegisters::r1,
            JSC::ARMRegisters::r2,
            JSC::ARMRegisters::r3
        };
#endif
        JS_ASSERT(numArgRegs(conv) == JS_ARRAY_LENGTH(regs));
        if (i > JS_ARRAY_LENGTH(regs))
            return false;
        *reg = regs[i];
        return true;
    }

    /* Floating point registers. */

    typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const uint32 TotalFPRegisters = 7;
    static const uint32 TempFPRegs = (
          (1 << JSC::X86Registers::xmm0)
        | (1 << JSC::X86Registers::xmm1)
        | (1 << JSC::X86Registers::xmm2)
        | (1 << JSC::X86Registers::xmm3)
        | (1 << JSC::X86Registers::xmm4)
        | (1 << JSC::X86Registers::xmm5)
        | (1 << JSC::X86Registers::xmm6)
        ) << TotalRegisters;
    /* For shuffling FP values around, or loading GPRs into a FP reg. */
    static const FPRegisterID FPConversionTemp = JSC::X86Registers::xmm7;
#elif defined(JS_CPU_ARM)
    static const uint32 TotalFPRegisters = 3;
    static const uint32 TempFPRegs = (
          (1 << JSC::ARMRegisters::d0)
        | (1 << JSC::ARMRegisters::d1)
        | (1 << JSC::ARMRegisters::d2);
        ) << TotalRegisters;
    static const FPRegisterID FPConversionTemp = JSC::ARMRegisters::d3;
#else
# error "Unsupported platform"
#endif

    static const uint32 AvailFPRegs = TempFPRegs;

    static inline uint32 maskReg(FPRegisterID reg) {
        return (1 << reg) << TotalRegisters;
    }

    /* Common code. */

    static const uint32 TotalAnyRegisters = TotalRegisters + TotalFPRegisters;
    static const uint32 TempAnyRegs = TempRegs | TempFPRegs;
    static const uint32 AvailAnyRegs = AvailRegs | AvailFPRegs;

    static inline uint32 maskReg(AnyRegisterID reg) {
        return (1 << reg.reg_);
    }

    Registers(uint32 freeMask)
      : freeMask(freeMask)
    { }

    Registers(const Registers &other)
      : freeMask(other.freeMask)
    { }

    Registers & operator =(const Registers &other)
    {
        freeMask = other.freeMask;
        return *this;
    }

    bool empty(uint32 mask) const {
        return !(freeMask & mask);
    }

    bool empty() const {
        return !freeMask;
    }

    AnyRegisterID peekReg(uint32 mask) {
        JS_ASSERT(!empty(mask));
        int ireg;
        JS_FLOOR_LOG2(ireg, freeMask & mask);
        RegisterID reg = (RegisterID)ireg;
        return reg;
    }

    AnyRegisterID peekReg() {
        return peekReg(freeMask);
    }

    AnyRegisterID takeAnyReg(uint32 mask) {
        AnyRegisterID reg = peekReg(mask);
        takeReg(reg);
        return reg;
    }

    AnyRegisterID takeAnyReg() {
        return takeAnyReg(freeMask);
    }

    bool hasReg(AnyRegisterID reg) const {
        return !!(freeMask & (1 << reg.reg_));
    }

    bool hasRegInMask(uint32 mask) const {
        return !!(freeMask & mask);
    }

    void putRegUnchecked(AnyRegisterID reg) {
        freeMask |= (1 << reg.reg_);
    }

    void putReg(AnyRegisterID reg) {
        JS_ASSERT(!hasReg(reg));
        putRegUnchecked(reg);
    }

    void takeReg(AnyRegisterID reg) {
        JS_ASSERT(hasReg(reg));
        freeMask &= ~(1 << reg.reg_);
    }

    bool operator ==(const Registers &other) {
        return freeMask == other.freeMask;
    }

    uint32 freeMask;
};

static const JSC::MacroAssembler::RegisterID JSFrameReg = Registers::JSFrameReg;

AnyRegisterID
AnyRegisterID::fromRaw(unsigned reg_)
{
    JS_ASSERT(reg_ < Registers::TotalAnyRegisters);
    AnyRegisterID reg;
    reg.reg_ = reg_;
    return reg;
}

JSC::MacroAssembler::RegisterID
AnyRegisterID::reg()
{
    JS_ASSERT(reg_ < Registers::TotalRegisters);
    return (JSC::MacroAssembler::RegisterID) reg_;
}

JSC::MacroAssembler::FPRegisterID
AnyRegisterID::fpreg()
{
    JS_ASSERT(reg_ >= Registers::TotalRegisters &&
              reg_ < Registers::TotalAnyRegisters);
    return (JSC::MacroAssembler::FPRegisterID) (reg_ - Registers::TotalRegisters);
}

const char *
AnyRegisterID::name()
{
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    return isReg() ? JSC::X86Registers::nameIReg(reg()) : JSC::X86Registers::nameFPReg(fpreg());
#elif defined(JS_CPU_ARM)
    return isreg() ? JSC::ARMAssembler::nameGpReg(reg()) : JSC::ARMAssembler::nameFpReg(fpreg());
#else
    return "???";
#endif
}

} /* namespace mjit */

} /* namespace js */

#endif /* jsjaeger_regstate_h__ */

