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
 *   David Mandelin <dmandelin@mozilla.com>
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
#if !defined jsjaeger_compiler_h__ && defined JS_METHODJIT
#define jsjaeger_compiler_h__

#include "jscntxt.h"
#include "jstl.h"
#include "BytecodeAnalyzer.h"
#include "MethodJIT.h"
#include "CodeGenIncludes.h"
#include "StubCompiler.h"
#include "MonoIC.h"
#include "PolyIC.h"

namespace js {
namespace mjit {

class Compiler
{
    typedef JSC::MacroAssembler::Label Label;
    typedef JSC::MacroAssembler::Imm32 Imm32;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::BaseIndex BaseIndex;
    typedef JSC::MacroAssembler::Jump Jump;
    typedef JSC::MacroAssembler::Call Call;
    typedef JSC::MacroAssembler::DataLabelPtr DataLabelPtr;

    struct BranchPatch {
        BranchPatch(const Jump &j, jsbytecode *pc)
          : jump(j), pc(pc)
        { }

        Jump jump;
        jsbytecode *pc;
    };

#if defined JS_MONOIC
    struct MICGenInfo {
        MICGenInfo(ic::MICInfo::Kind kind) : kind(kind)
        { }
        Label entry;
        Label stubEntry;
        DataLabelPtr shapeVal;
        Label load;
        Call call;
        ic::MICInfo::Kind kind;
        bool typeConst;
        bool dataConst;
        bool dataWrite;
    };
#endif

#if defined JS_POLYIC
    struct PICGenInfo {
        PICGenInfo(ic::PICInfo::Kind kind) : kind(kind)
        { }
        ic::PICInfo::Kind kind;
        Label hotPathBegin;
        Label storeBack;
        Label typeCheck;
        Label slowPathStart;
        RegisterID shapeReg;
        RegisterID objReg;
        RegisterID typeReg;
        Label shapeGuard;
        JSAtom *atom;
        StateRemat objRemat;
        Call callReturn;
        bool hasTypeCheck;
        ValueRemat vr;
    };
#endif

    struct Defs {
        Defs(uint32 ndefs)
          : ndefs(ndefs)
        { }
        uint32 ndefs;
    };

    class MaybeRegisterID {
      public:
        MaybeRegisterID()
          : reg(Registers::ReturnReg), set(false)
        { }

        inline RegisterID getReg() const { JS_ASSERT(set); return reg; }
        inline void setReg(const RegisterID r) { reg = r; set = true; }
        inline bool isSet() const { return set; }

      private:
        RegisterID reg;
        bool set;
    };

    class MaybeJump {
      public:
        MaybeJump()
          : set(false)
        { }

        inline Jump getJump() const { JS_ASSERT(set); return jump; }
        inline void setJump(const Jump &j) { jump = j; set = true; }
        inline bool isSet() const { return set; }

        inline MaybeJump &operator=(Jump j) { setJump(j); return *this; }

      private:
        Jump jump;
        bool set;
    };

    JSContext *cx;
    JSScript *script;
    JSObject *scopeChain;
    JSObject *globalObj;
    JSFunction *fun;
    BytecodeAnalyzer analysis;
    Label *jumpMap;
    jsbytecode *PC;
    Assembler masm;
    FrameState frame;
    js::Vector<BranchPatch, 64> branchPatches;
#if defined JS_MONOIC
    js::Vector<MICGenInfo, 64> mics;
#endif
#if defined JS_POLYIC
    js::Vector<PICGenInfo, 64> pics;
#endif
    StubCompiler stubcc;
    Label invokeLabel;

  public:
    // Special atom index used to indicate that the atom is 'length'. This
    // follows interpreter usage in JSOP_LENGTH.
    enum { LengthAtomIndex = uint32(-2) };

    Compiler(JSContext *cx, JSScript *script, JSFunction *fun, JSObject *scopeChain);
    ~Compiler();

    CompileStatus Compile();

    jsbytecode *getPC() { return PC; }
    Label getLabel() { return masm.label(); }
    bool knownJump(jsbytecode *pc);
    Label labelOf(jsbytecode *target);

  private:
    CompileStatus generatePrologue();
    CompileStatus generateMethod();
    CompileStatus generateEpilogue();
    CompileStatus finishThisUp();

    /* Non-emitting helpers. */
    uint32 fullAtomIndex(jsbytecode *pc);
    void jumpInScript(Jump j, jsbytecode *pc);
    JSC::ExecutablePool *getExecPool(size_t size);
    bool compareTwoValues(JSContext *cx, JSOp op, const Value &lhs, const Value &rhs);

    /* Emitting helpers. */
    void restoreFrameRegs();
    void emitStubCmpOp(BoolStub stub, jsbytecode *target, JSOp fused);
    void iterNext();
    void iterMore();

    /* Opcode handlers. */
    void jsop_bindname(uint32 index);
    void jsop_setglobal(uint32 index);
    void jsop_getglobal(uint32 index);
    void jsop_getprop_slow();
    void jsop_getarg(uint32 index);
    void jsop_this();
    void emitReturn();
    void dispatchCall(VoidPtrStubUInt32 stub, uint32 argc);
    void inlineCallHelper(uint32 argc, bool callingNew);
    void jsop_gnameinc(JSOp op, VoidStubAtom stub, uint32 index);
    void jsop_nameinc(JSOp op, VoidStubAtom stub, uint32 index);
    void jsop_propinc(JSOp op, VoidStubAtom stub, uint32 index);
    void jsop_eleminc(JSOp op, VoidStub);
    void jsop_getgname(uint32 index);
    void jsop_getgname_slow(uint32 index);
    void jsop_setgname(uint32 index);
    void jsop_setgname_slow(uint32 index);
    void jsop_bindgname();
    void jsop_setelem_slow();
    void jsop_getelem_slow();
    void jsop_unbrand();
    void jsop_getprop(JSAtom *atom, bool typeCheck = true);
    void jsop_length();
    void jsop_setprop(JSAtom *atom);
    void jsop_setprop_slow(JSAtom *atom);
    bool jsop_callprop_slow(JSAtom *atom);
    bool jsop_callprop(JSAtom *atom);
    bool jsop_callprop_obj(JSAtom *atom);
    bool jsop_callprop_str(JSAtom *atom);
    bool jsop_callprop_generic(JSAtom *atom);
    void jsop_instanceof();
    void jsop_name(JSAtom *atom);

    /* Fast arithmetic. */
    void jsop_binary(JSOp op, VoidStub stub);
    void jsop_binary_intmath(JSOp op, RegisterID *returnReg,
                             MaybeJump &jmpOverflow);
    void jsop_binary_dblmath(JSOp op, FPRegisterID rfp, FPRegisterID lfp);
    void slowLoadConstantDouble(Assembler &masm, FrameEntry *fe,
                                FPRegisterID fpreg);
    void maybeJumpIfNotInt32(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                             MaybeRegisterID &mreg);
    void maybeJumpIfNotDouble(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                              MaybeRegisterID &mreg);

    /* Fast opcodes. */
    void jsop_bitop(JSOp op);
    void jsop_globalinc(JSOp op, uint32 index);
    void jsop_relational(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_neg();
    void jsop_bitnot();
    void jsop_objtostr();
    void jsop_not();
    void jsop_typeof();
    void booleanJumpScript(JSOp op, jsbytecode *target);
    void jsop_ifneq(JSOp op, jsbytecode *target);
    void jsop_andor(JSOp op, jsbytecode *target);
    void jsop_arginc(JSOp op, uint32 slot, bool popped);
    void jsop_localinc(JSOp op, uint32 slot, bool popped);
    void jsop_setelem();
    void jsop_getelem();
    void jsop_stricteq(JSOp op);
    void jsop_equality(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_pos();

#define STUB_CALL_TYPE(type)                                            \
    Call stubCall(type stub) {                                          \
        return stubCall(JS_FUNC_TO_DATA_PTR(void *, stub));             \
    }

    STUB_CALL_TYPE(JSObjStub);
    STUB_CALL_TYPE(VoidStubUInt32);
    STUB_CALL_TYPE(VoidStub);
    STUB_CALL_TYPE(VoidPtrStubUInt32);
    STUB_CALL_TYPE(VoidPtrStub);
    STUB_CALL_TYPE(BoolStub);
    STUB_CALL_TYPE(JSObjStubUInt32);
    STUB_CALL_TYPE(JSObjStubFun);
    STUB_CALL_TYPE(JSObjStubJSObj);
    STUB_CALL_TYPE(VoidStubAtom);
    STUB_CALL_TYPE(JSStrStub);
    STUB_CALL_TYPE(JSStrStubUInt32);
    STUB_CALL_TYPE(VoidStubJSObj);
    STUB_CALL_TYPE(VoidPtrStubPC);
    STUB_CALL_TYPE(VoidVpStub);

#undef STUB_CALL_TYPE
    void prepareStubCall(Uses uses);
    Call stubCall(void *ptr);
};

} /* namespace js */
} /* namespace mjit */

#endif

