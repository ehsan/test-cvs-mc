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

#include "jsinfer.h"
#include "jsinferinlines.h"
#include "IonMacroAssembler.h"
#include "gc/Root.h"

using namespace js;
using namespace js::ion;

template <typename T> void
MacroAssembler::guardTypeSet(const T &address, types::TypeSet *types,
                             Register scratch, Label *mismatched)
{
    JS_ASSERT(!types->unknown());

    Label matched;
    Register tag = extractTag(address, scratch);

    if (types->hasType(types::Type::DoubleType())) {
        // The double type also implies Int32.
        JS_ASSERT(types->hasType(types::Type::Int32Type()));
        branchTestNumber(Equal, tag, &matched);
    } else if (types->hasType(types::Type::Int32Type())) {
        branchTestInt32(Equal, tag, &matched);
    }

    if (types->hasType(types::Type::UndefinedType()))
        branchTestUndefined(Equal, tag, &matched);
    if (types->hasType(types::Type::BooleanType()))
        branchTestBoolean(Equal, tag, &matched);
    if (types->hasType(types::Type::StringType()))
        branchTestString(Equal, tag, &matched);
    if (types->hasType(types::Type::NullType()))
        branchTestNull(Equal, tag, &matched);

    if (types->hasType(types::Type::AnyObjectType())) {
        branchTestObject(Equal, tag, &matched);
    } else if (types->getObjectCount()) {
        branchTestObject(NotEqual, tag, mismatched);
        Register obj = extractObject(address, scratch);

        unsigned count = types->getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            if (JSObject *object = types->getSingleObject(i))
                branchPtr(Equal, obj, ImmGCPtr(object), &matched);
        }

        loadPtr(Address(obj, JSObject::offsetOfType()), scratch);

        for (unsigned i = 0; i < count; i++) {
            if (types::TypeObject *object = types->getTypeObject(i))
                branchPtr(Equal, scratch, ImmGCPtr(object), &matched);
        }
    }

    jump(mismatched);
    bind(&matched);
}

template void MacroAssembler::guardTypeSet(const Address &address, types::TypeSet *types,
                                           Register scratch, Label *mismatched);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, types::TypeSet *types,
                                           Register scratch, Label *mismatched);

void
MacroAssembler::PushRegsInMask(RegisterSet set)
{
    size_t diff = set.gprs().size() * STACK_SLOT_SIZE +
                  set.fpus().size() * sizeof(double);

    reserveStack(diff);

    for (GeneralRegisterIterator iter(set.gprs()); iter.more(); iter++) {
        diff -= STACK_SLOT_SIZE;
        storePtr(*iter, Address(StackPointer, diff));
    }
    for (FloatRegisterIterator iter(set.fpus()); iter.more(); iter++) {
        diff -= sizeof(double);
        storeDouble(*iter, Address(StackPointer, diff));
    }
}

void
MacroAssembler::PopRegsInMask(RegisterSet set)
{
    size_t diff = set.gprs().size() * STACK_SLOT_SIZE +
                  set.fpus().size() * sizeof(double);
    size_t reserved = diff;

    for (GeneralRegisterIterator iter(set.gprs()); iter.more(); iter++) {
        diff -= STACK_SLOT_SIZE;
        loadPtr(Address(StackPointer, diff), *iter);
    }
    for (FloatRegisterIterator iter(set.fpus()); iter.more(); iter++) {
        diff -= sizeof(double);
        loadDouble(Address(StackPointer, diff), *iter);
    }

    freeStack(reserved);
}

void
MacroAssembler::branchTestValueTruthy(const ValueOperand &value, Label *ifTrue, FloatRegister fr)
{
    Register tag = splitTagForTest(value);
    Label ifFalse;
    Assembler::Condition cond;

    // Eventually we will want some sort of type filter here. For now, just
    // emit all easy cases. For speed we use the cached tag for all comparison,
    // except for doubles, which we test last (as the operation can clobber the
    // tag, which may be in ScratchReg).
    branchTestUndefined(Assembler::Equal, tag, &ifFalse);

    branchTestNull(Assembler::Equal, tag, &ifFalse);
    branchTestObject(Assembler::Equal, tag, ifTrue);

    Label notBoolean;
    branchTestBoolean(Assembler::NotEqual, tag, &notBoolean);
    branchTestBooleanTruthy(false, value, &ifFalse);
    jump(ifTrue);
    bind(&notBoolean);

    Label notInt32;
    branchTestInt32(Assembler::NotEqual, tag, &notInt32);
    cond = testInt32Truthy(false, value);
    j(cond, &ifFalse);
    jump(ifTrue);
    bind(&notInt32);

    // Test if a string is non-empty.
    Label notString;
    branchTestString(Assembler::NotEqual, tag, &notString);
    cond = testStringTruthy(false, value);
    j(cond, &ifFalse);
    jump(ifTrue);
    bind(&notString);

    // If we reach here the value is a double.
    unboxDouble(value, fr);
    cond = testDoubleTruthy(false, fr);
    j(cond, &ifFalse);
    jump(ifTrue);
    bind(&ifFalse);
}

template<typename T>
void
MacroAssembler::loadFromTypedArray(int arrayType, const T &src, AnyRegister dest, Register temp,
                                   Label *fail)
{
    switch (arrayType) {
      case TypedArray::TYPE_INT8:
        load8SignExtend(src, dest.gpr());
        break;
      case TypedArray::TYPE_UINT8:
      case TypedArray::TYPE_UINT8_CLAMPED:
        load8ZeroExtend(src, dest.gpr());
        break;
      case TypedArray::TYPE_INT16:
        load16SignExtend(src, dest.gpr());
        break;
      case TypedArray::TYPE_UINT16:
        load16ZeroExtend(src, dest.gpr());
        break;
      case TypedArray::TYPE_INT32:
        load32(src, dest.gpr());
        break;
      case TypedArray::TYPE_UINT32:
        if (dest.isFloat()) {
            load32(src, temp);
            convertUInt32ToDouble(temp, dest.fpu());
        } else {
            load32(src, dest.gpr());
            test32(dest.gpr(), dest.gpr());
            j(Assembler::Signed, fail);
        }
        break;
      case TypedArray::TYPE_FLOAT32:
      case TypedArray::TYPE_FLOAT64:
      {
        if (arrayType == js::TypedArray::TYPE_FLOAT32)
            loadFloatAsDouble(src, dest.fpu());
        else
            loadDouble(src, dest.fpu());

        // Make sure NaN gets canonicalized.
        Label notNaN;
        branchDouble(DoubleOrdered, dest.fpu(), dest.fpu(), &notNaN);
        {
            loadStaticDouble(&js_NaN, dest.fpu());
        }
        bind(&notNaN);
        break;
      }
      default:
        JS_NOT_REACHED("Invalid typed array type");
        break;
    }
}

template void MacroAssembler::loadFromTypedArray(int arrayType, const Address &src, AnyRegister dest,
                                                 Register temp, Label *fail);
template void MacroAssembler::loadFromTypedArray(int arrayType, const BaseIndex &src, AnyRegister dest,
                                                 Register temp, Label *fail);

template<typename T>
void
MacroAssembler::loadFromTypedArray(int arrayType, const T &src, const ValueOperand &dest,
                                   bool allowDouble, Label *fail)
{
    switch (arrayType) {
      case TypedArray::TYPE_INT8:
      case TypedArray::TYPE_UINT8:
      case TypedArray::TYPE_UINT8_CLAMPED:
      case TypedArray::TYPE_INT16:
      case TypedArray::TYPE_UINT16:
      case TypedArray::TYPE_INT32:
        loadFromTypedArray(arrayType, src, AnyRegister(dest.scratchReg()), InvalidReg, NULL);
        tagValue(JSVAL_TYPE_INT32, dest.scratchReg(), dest);
        break;
      case TypedArray::TYPE_UINT32:
        load32(src, dest.scratchReg());
        test32(dest.scratchReg(), dest.scratchReg());
        if (allowDouble) {
            // If the value fits in an int32, store an int32 type tag.
            // Else, convert the value to double and box it.
            Label done, isDouble;
            j(Assembler::Signed, &isDouble);
            {
                tagValue(JSVAL_TYPE_INT32, dest.scratchReg(), dest);
                jump(&done);
            }
            bind(&isDouble);
            {
                convertUInt32ToDouble(dest.scratchReg(), ScratchFloatReg);
                boxDouble(ScratchFloatReg, dest);
            }
            bind(&done);
        } else {
            // Bailout if the value does not fit in an int32.
            j(Assembler::Signed, fail);
            tagValue(JSVAL_TYPE_INT32, dest.scratchReg(), dest);
        }
        break;
      case TypedArray::TYPE_FLOAT32:
      case TypedArray::TYPE_FLOAT64:
        loadFromTypedArray(arrayType, src, AnyRegister(ScratchFloatReg), dest.scratchReg(), NULL);
        boxDouble(ScratchFloatReg, dest);
        break;
      default:
        JS_NOT_REACHED("Invalid typed array type");
        break;
    }
}

template void MacroAssembler::loadFromTypedArray(int arrayType, const Address &src, const ValueOperand &dest,
                                                 bool allowDouble, Label *fail);
template void MacroAssembler::loadFromTypedArray(int arrayType, const BaseIndex &src, const ValueOperand &dest,
                                                 bool allowDouble, Label *fail);

// Note: this function clobbers the input register.
void
MacroAssembler::clampDoubleToUint8(FloatRegister input, Register output)
{
    JS_ASSERT(input != ScratchFloatReg);
#ifdef JS_CPU_ARM
    Label notSplit;
    ma_vimm(0.5, ScratchFloatReg);
    ma_vadd(input, ScratchFloatReg, ScratchFloatReg);
    // Convert the double into an unsigned fixed point value with 24 bits of
    // precision. The resulting number will look like 0xII.DDDDDD
    as_vcvtFixed(ScratchFloatReg, false, 24, true);
    // Move the fixed point value into an integer register
    as_vxfer(output, InvalidReg, ScratchFloatReg, FloatToCore);
    // See if this value *might* have been an exact integer after adding 0.5
    // This tests the 1/2 through 1/16,777,216th places, but 0.5 needs to be tested out to
    // the 1/140,737,488,355,328th place.
    ma_tst(output, Imm32(0x00ffffff));
    // convert to a uint8 by shifting out all of the fraction bits
    ma_lsr(Imm32(24), output, output);
    // If any of the bottom 24 bits were non-zero, then we're good, since this number
    // can't be exactly XX.0
    ma_b(&notSplit, NonZero);
    as_vxfer(ScratchRegister, InvalidReg, input, FloatToCore);
    ma_cmp(ScratchRegister, Imm32(0));
    // If the lower 32 bits of the double were 0, then this was an exact number,
    // and it should be even.
    ma_bic(Imm32(1), output, NoSetCond, Zero);
    bind(&notSplit);
#else

    Label positive, done;

    // <= 0 or NaN --> 0
    zeroDouble(ScratchFloatReg);
    branchDouble(DoubleGreaterThan, input, ScratchFloatReg, &positive);
    {
        move32(Imm32(0), output);
        jump(&done);
    }

    bind(&positive);

    // Add 0.5 and truncate.
    static const double DoubleHalf = 0.5;
    loadStaticDouble(&DoubleHalf, ScratchFloatReg);
    addDouble(ScratchFloatReg, input);

    Label outOfRange;
    branchTruncateDouble(input, output, &outOfRange);
    branch32(Assembler::Above, output, Imm32(255), &outOfRange);
    {
        // Check if we had a tie.
        convertInt32ToDouble(output, ScratchFloatReg);
        branchDouble(DoubleNotEqual, input, ScratchFloatReg, &done);

        // It was a tie. Mask out the ones bit to get an even value.
        // See also js_TypedArray_uint8_clamp_double.
        and32(Imm32(~1), output);
        jump(&done);
    }

    // > 255 --> 255
    bind(&outOfRange);
    {
        move32(Imm32(255), output);
    }

    bind(&done);
#endif
}

void
MacroAssembler::newGCThing(JSContext *cx, const Register &result,
                           HandleObject templateObject, Label *fail)
{
    // Inlined equivalent of js::gc::NewGCThing() without failure case handling.

    gc::AllocKind allocKind = templateObject->getAllocKind();
    JS_ASSERT(allocKind >= gc::FINALIZE_OBJECT0 && allocKind <= gc::FINALIZE_OBJECT_LAST);
    int thingSize = (int)gc::Arena::thingSize(allocKind);

    JS_ASSERT(cx->typeInferenceEnabled());
    JS_ASSERT(!templateObject->hasDynamicElements());

#ifdef JS_GC_ZEAL
    // Don't execute the inline path if gcZeal is active.
    movePtr(ImmWord(cx->runtime), result);
    loadPtr(Address(result, offsetof(JSRuntime, gcZeal_)), result);
    branch32(Assembler::NotEqual, result, Imm32(0), fail);
#endif

    // Inline FreeSpan::allocate.
    // There is always exactly one FreeSpan per allocKind per JSCompartment.
    // If a FreeSpan is replaced, its members are updated in the freeLists table,
    // which the code below always re-reads.
    gc::FreeSpan *list = const_cast<gc::FreeSpan *>
                         (cx->compartment->arenas.getFreeList(allocKind));
    loadPtr(AbsoluteAddress(&list->first), result);
    branchPtr(Assembler::BelowOrEqual, AbsoluteAddress(&list->last), result, fail);

    addPtr(Imm32(thingSize), result);
    storePtr(result, AbsoluteAddress(&list->first));
    subPtr(Imm32(thingSize), result);
}

void
MacroAssembler::initGCThing(JSContext *cx, const Register &obj, HandleObject templateObject)
{
    // Fast initialization of an empty object returned by NewGCThing().

    storePtr(ImmGCPtr(templateObject->lastProperty()), Address(obj, JSObject::offsetOfShape()));
    storePtr(ImmGCPtr(templateObject->type()), Address(obj, JSObject::offsetOfType()));
    storePtr(ImmWord((void *)NULL), Address(obj, JSObject::offsetOfSlots()));

    if (templateObject->isDenseArray()) {
        JS_ASSERT(!templateObject->getDenseArrayInitializedLength());

        int elementsOffset = JSObject::offsetOfFixedElements();

        addPtr(Imm32(elementsOffset), obj);
        storePtr(obj, Address(obj, -elementsOffset + JSObject::offsetOfElements()));
        addPtr(Imm32(-elementsOffset), obj);

        // Fill in the elements header.
        store32(Imm32(templateObject->getDenseArrayCapacity()),
                Address(obj, elementsOffset + ObjectElements::offsetOfCapacity()));
        store32(Imm32(templateObject->getDenseArrayInitializedLength()),
                Address(obj, elementsOffset + ObjectElements::offsetOfInitializedLength()));
        store32(Imm32(templateObject->getArrayLength()),
                Address(obj, elementsOffset + ObjectElements::offsetOfLength()));
    } else {
        storePtr(ImmWord(emptyObjectElements), Address(obj, JSObject::offsetOfElements()));

        // Fixed slots of non-array objects are required to be initialized.
        // Use the values currently in the template object.
        size_t nslots = JS_MIN(templateObject->numFixedSlots(), templateObject->slotSpan());
        for (unsigned i = 0; i < nslots; i++) {
            storeValue(templateObject->getFixedSlot(i),
                       Address(obj, JSObject::getFixedSlotOffset(i)));
        }
    }

    if (templateObject->hasPrivate()) {
        uint32_t nfixed = templateObject->numFixedSlots();
        storePtr(ImmWord(templateObject->getPrivate()),
                 Address(obj, JSObject::getPrivateDataOffset(nfixed)));
    }
}

void
MacroAssembler::maybeRemoveOsrFrame(Register scratch)
{
    // Before we link an exit frame, check for an OSR frame, which is
    // indicative of working inside an existing bailout. In this case, remove
    // the OSR frame, so we don't explode the stack with repeated bailouts.
    Label osrRemoved;
    movePtr(Address(StackPointer, IonCommonFrameLayout::offsetOfDescriptor()), scratch);
    and32(Imm32(FRAMETYPE_MASK), scratch);
    branch32(Assembler::NotEqual, scratch, Imm32(IonFrame_Osr), &osrRemoved);
    addPtr(Imm32(sizeof(IonOsrFrameLayout)), StackPointer);
    bind(&osrRemoved);
}

void
MacroAssembler::performOsr()
{
    GeneralRegisterSet regs = GeneralRegisterSet::All();

    // This register must be fixed as it's used in the Osr prologue.
    regs.take(OsrFrameReg);

    // Remove any existing OSR frame so we don't create one per bailout.
    maybeRemoveOsrFrame(regs.getAny());

    const Register script = regs.takeAny();
    const Register calleeToken = regs.takeAny();

    // Grab fp.exec
    loadPtr(Address(OsrFrameReg, StackFrame::offsetOfExec()), script);
    mov(script, calleeToken);

    Label isFunction, performOsr;
    branchTest32(Assembler::NonZero,
                 Address(OsrFrameReg, StackFrame::offsetOfFlags()),
                 Imm32(StackFrame::FUNCTION),
                 &isFunction);

    {
        // Not a function - just tag the calleeToken now.
        orPtr(Imm32(CalleeToken_Script), calleeToken);
        jump(&performOsr);
    }

    bind(&isFunction);
    {
        // Function - create the callee token, then get the script.
        orPtr(Imm32(CalleeToken_Function), calleeToken);
        loadPtr(Address(script, JSFunction::offsetOfNativeOrScript()), script);
    }

    bind(&performOsr);

    const Register ionScript = regs.takeAny();
    const Register osrEntry = regs.takeAny();

    loadPtr(Address(script, offsetof(JSScript, ion)), ionScript);
    load32(Address(ionScript, IonScript::offsetOfOsrEntryOffset()), osrEntry);

    // Get ionScript->method->code, and scoot to the osrEntry.
    const Register code = ionScript;
    loadPtr(Address(ionScript, IonScript::offsetOfMethod()), code);
    loadPtr(Address(code, IonCode::OffsetOfCode()), code);
    addPtr(osrEntry, code);

    // To simplify stack handling, we create an intermediate OSR frame, that
    // looks like a JS frame with no argv.
    enterOsr(calleeToken, code);
    ret();
}

