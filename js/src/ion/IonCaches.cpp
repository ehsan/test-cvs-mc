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
 *   Brian Hackett <bhackett@mozilla.com>
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

#include "jsscope.h"

#include "CodeGenerator.h"
#include "Ion.h"
#include "IonCaches.h"
#include "IonLinker.h"
#include "IonSpewer.h"

#include "vm/Stack.h"

using namespace js;
using namespace js::ion;

void
CodeLocationJump::repoint(IonCode *code, MacroAssembler *masm)
{
    JS_ASSERT(!absolute);
    size_t new_off = (size_t)raw_;
    if (masm != NULL) {
#ifdef JS_CPU_X64
        JS_ASSERT((uint64_t)raw_ <= UINT32_MAX);
#endif
        new_off = masm->actualOffset((uintptr_t)raw_);
    }
    raw_ = code->raw() + new_off;
#ifdef JS_CPU_X64
    jumpTableEntry_ = Assembler::PatchableJumpAddress(code, (size_t) jumpTableEntry_);
#endif
    markAbsolute(true);
}

void
CodeLocationLabel::repoint(IonCode *code, MacroAssembler *masm)
{
     JS_ASSERT(!absolute);
     size_t new_off = (size_t)raw_;
     if (masm != NULL) {
#ifdef JS_CPU_X64
        JS_ASSERT((uint64_t)raw_ <= UINT32_MAX);
#endif
        new_off = masm->actualOffset((uintptr_t)raw_);
     }
     JS_ASSERT(new_off < code->instructionsSize());

     raw_ = code->raw() + new_off;
     markAbsolute(true);
}

void
CodeOffsetLabel::fixup(MacroAssembler *masm)
{
     offset_ = masm->actualOffset(offset_);
}

static const size_t MAX_STUBS = 16;

static void
GeneratePrototypeGuards(JSContext *cx, MacroAssembler &masm, JSObject *obj, JSObject *holder,
                        Register objectReg, Register scratchReg, Label *failures)
{
    JS_ASSERT(obj != holder);

    if (obj->hasUncacheableProto()) {
        // Note: objectReg and scratchReg may be the same register, so we cannot
        // use objectReg in the rest of this function.
        masm.loadPtr(Address(objectReg, JSObject::offsetOfType()), scratchReg);
        Address proto(scratchReg, offsetof(types::TypeObject, proto));
        masm.branchPtr(Assembler::NotEqual, proto, ImmGCPtr(obj->getProto()), failures);
    }

    JSObject *pobj = obj->getProto();
    while (pobj != holder) {
        if (pobj->hasUncacheableProto()) {
            if (pobj->hasSingletonType()) {
                types::TypeObject *type = pobj->getType(cx);
                masm.movePtr(ImmGCPtr(type), scratchReg);
                Address proto(scratchReg, offsetof(types::TypeObject, proto));
                masm.branchPtr(Assembler::NotEqual, proto, ImmGCPtr(obj->getProto()), failures);
            } else {
                masm.movePtr(ImmGCPtr(pobj), scratchReg);
                Address objType(scratchReg, JSObject::offsetOfType());
                masm.branchPtr(Assembler::NotEqual, objType, ImmGCPtr(pobj->type()), failures);
            }
        }
        pobj = pobj->getProto();
    }
}

bool
IonCacheGetProperty::attachNative(JSContext *cx, JSObject *obj, JSObject *holder, const Shape *shape)
{
    MacroAssembler masm;

    Label failures;
    CodeOffsetJump exitOffset =
        masm.branchPtrWithPatch(Assembler::NotEqual,
                                Address(object(), JSObject::offsetOfShape()),
                                ImmGCPtr(obj->lastProperty()),
                                &failures);

    bool restoreScratch = false;
    Register scratchReg = Register::FromCode(0); // Quell compiler warning.

    // If we need a scratch register, use either an output register or the object
    // register (and restore it afterwards). After this point, we cannot jump
    // directly to |failures| since we may still have to pop the object register.
    Label prototypeFailures;
    if (obj != holder || !holder->isFixedSlot(shape->slot())) {
        if (output().hasValue()) {
            scratchReg = output().valueReg().scratchReg();
        } else if (output().type() == MIRType_Double) {
            scratchReg = object();
            masm.push(scratchReg);
            restoreScratch = true;
        } else {
            scratchReg = output().typedReg().gpr();
        }
    }

    Register holderReg;
    if (obj != holder) {
        // Note: this may clobber the object register if it's used as scratch.
        GeneratePrototypeGuards(cx, masm, obj, holder, object(), scratchReg, &prototypeFailures);

        // Guard on the holder's shape.
        holderReg = scratchReg;
        masm.movePtr(ImmGCPtr(holder), holderReg);
        masm.branchPtr(Assembler::NotEqual,
                       Address(holderReg, JSObject::offsetOfShape()),
                       ImmGCPtr(holder->lastProperty()),
                       &prototypeFailures);
    } else {
        holderReg = object();
    }

    if (holder->isFixedSlot(shape->slot())) {
        Address addr(holderReg, JSObject::getFixedSlotOffset(shape->slot()));
        masm.loadTypedOrValue(addr, output());
    } else {
        masm.loadPtr(Address(holderReg, JSObject::offsetOfSlots()), scratchReg);

        Address addr(scratchReg, holder->dynamicSlotIndex(shape->slot()) * sizeof(Value));
        masm.loadTypedOrValue(addr, output());
    }

    if (restoreScratch)
        masm.pop(scratchReg);

    Label rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    if (obj != holder) {
        masm.bind(&prototypeFailures);
        if (restoreScratch)
            masm.pop(scratchReg);
        masm.bind(&failures);
        Label exit_;
        exitOffset = masm.jumpWithPatch(&exit_);
        masm.bind(&exit_);
    } else {
        masm.bind(&failures);
    }

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump exitJump(code, exitOffset);

    PatchJump(lastJump(), CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated native GETPROP stub at %p", code->raw());

    return true;
}

static bool
IsCacheableProtoChain(JSObject *obj, JSObject *holder)
{
    while (obj != holder) {
        /*
         * We cannot assume that we find the holder object on the prototype
         * chain and must check for null proto. The prototype chain can be
         * altered during the lookupProperty call.
         */
        JSObject *proto = obj->getProto();
        if (!proto || !proto->isNative())
            return false;
        obj = proto;
    }
    return true;
}

bool
js::ion::GetPropertyCache(JSContext *cx, size_t cacheIndex, JSObject *obj, Value *vp)
{
    JSScript *script = GetTopIonJSScript(cx);
    IonScript *ion = script->ionScript();

    IonCacheGetProperty &cache = ion->getCache(cacheIndex).toGetProperty();
    JSAtom *atom = cache.atom();

    // For now, just stop generating new stubs once we hit the stub count
    // limit. Once we can make calls from within generated stubs, a new call
    // stub will be generated instead and the previous stubs unlinked.
    if (cache.stubCount() < MAX_STUBS && obj->isNative()) {
        cache.incrementStubCount();

        jsid id = ATOM_TO_JSID(atom);
        id = js_CheckForStringIndex(id);

        JSObject *holder;
        JSProperty *prop;
        if (!obj->lookupProperty(cx, atom->asPropertyName(), &holder, &prop))
            return false;

        const Shape *shape = (const Shape *)prop;
        if (shape && IsCacheableProtoChain(obj, holder) &&
            shape->hasSlot() && shape->hasDefaultGetter() && !shape->isMethod())
        {
            if (!cache.attachNative(cx, obj, holder, shape))
                return false;
        }
    }

    if (!obj->getGeneric(cx, obj, ATOM_TO_JSID(atom), vp))
        return false;

    {
        JSScript *script;
        jsbytecode *pc;
        cache.getScriptedLocation(&script, &pc);
        types::TypeScript::Monitor(cx, script, pc, *vp);
    }

    // If we've been invalidated, override the return value (bug 728188).
    if (script->ion != ion)
        cx->runtime->setIonReturnOverride(*vp);

    return true;
}

void
IonCache::updateBaseAddress(IonCode *code, MacroAssembler &masm)
{
    initialJump_.repoint(code, &masm);
    lastJump_.repoint(code, &masm);
    cacheLabel_.repoint(code, &masm);
}

bool
IonCacheSetProperty::attachNativeExisting(JSContext *cx, JSObject *obj, const Shape *shape)
{
    MacroAssembler masm;

    Label exit_;
    CodeOffsetJump exitOffset =
        masm.branchPtrWithPatch(Assembler::NotEqual,
                                Address(object(), JSObject::offsetOfShape()),
                                ImmGCPtr(obj->lastProperty()),
                                &exit_);
    masm.bind(&exit_);

    if (obj->isFixedSlot(shape->slot())) {
        Address addr(object(), JSObject::getFixedSlotOffset(shape->slot()));
        masm.storeConstantOrRegister(value(), addr);
    } else {
        Register slotsReg = object();

        masm.loadPtr(Address(object(), JSObject::offsetOfSlots()), slotsReg);

        Address addr(slotsReg, obj->dynamicSlotIndex(shape->slot()) * sizeof(Value));
        masm.storeConstantOrRegister(value(), addr);
    }

    Label rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump exitJump(code, exitOffset);

    PatchJump(lastJump(), CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated native SETPROP stub at %p", code->raw());

    return true;
}

bool
js::ion::SetPropertyCache(JSContext *cx, size_t cacheIndex, JSObject *obj, const Value& value)
{
    IonScript *ion = GetTopIonJSScript(cx)->ion;
    IonCacheSetProperty &cache = ion->getCache(cacheIndex).toSetProperty();
    JSAtom *atom = cache.atom();
    Value v = value;
    // Stop generating new stubs once we hit the stub count limit, see
    // GetPropertyCache.
    if (cache.stubCount() < MAX_STUBS &&
        obj->isNative() &&
        !obj->watched())
    {
        cache.incrementStubCount();

        jsid id = ATOM_TO_JSID(atom);
        id = js_CheckForStringIndex(id);

        const Shape *shape = obj->nativeLookup(cx, id);
        if (shape && shape->hasSlot() && shape->hasDefaultSetter() && !shape->isMethod()) {
            if (!cache.attachNativeExisting(cx, obj, shape))
                return false;
        }
    }

    return obj->setGeneric(cx, ATOM_TO_JSID(atom), &v, cache.strict());
}

bool
IonCacheBindName::attachGlobal(JSContext *cx, JSObject *scopeChain)
{
    JS_ASSERT(scopeChain->isGlobal());

    MacroAssembler masm;

    // Guard on the scope chain.
    Label exit_;
    CodeOffsetJump exitOffset = masm.branchPtrWithPatch(Assembler::NotEqual, scopeChainReg(),
                                                        ImmGCPtr(scopeChain), &exit_);
    masm.bind(&exit_);
    masm.movePtr(ImmGCPtr(scopeChain), outputReg());

    Label rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump exitJump(code, exitOffset);

    PatchJump(lastJump(), CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated BINDNAME global stub at %p", code->raw());
    return true;
}

static void
GenerateScopeChainGuards(MacroAssembler &masm, JSObject *scopeChain, JSObject *holder,
                         Register scopeChainReg, Register outputReg, Label *failures)
{
    JS_ASSERT(scopeChain != holder);

    // Load the parent of the scope object into outputReg.
    JSObject *tobj = &scopeChain->asScope().enclosingScope();
    masm.extractObject(Address(scopeChainReg, ScopeObject::offsetOfEnclosingScope()), outputReg);

    // Walk up the scope chain. Note that IsCacheableScopeChain guarantees the
    // |tobj == holder| condition terminates the loop.
    while (true) {
        JS_ASSERT(IsCacheableNonGlobalScope(tobj));

        // Test intervening shapes.
        Address shapeAddr(outputReg, JSObject::offsetOfShape());
        masm.branchPtr(Assembler::NotEqual, shapeAddr, ImmGCPtr(tobj->lastProperty()), failures);
        if (tobj == holder)
            break;

        // Load the next link.
        tobj = &tobj->asScope().enclosingScope();
        masm.extractObject(Address(outputReg, ScopeObject::offsetOfEnclosingScope()), outputReg);
    }
}

bool
IonCacheBindName::attachNonGlobal(JSContext *cx, JSObject *scopeChain, JSObject *holder)
{
    JS_ASSERT(IsCacheableNonGlobalScope(scopeChain));

    MacroAssembler masm;

    // Guard on the shape of the scope chain.
    Label failures;
    CodeOffsetJump exitOffset =
        masm.branchPtrWithPatch(Assembler::NotEqual,
                                Address(scopeChainReg(), JSObject::offsetOfShape()),
                                ImmGCPtr(scopeChain->lastProperty()),
                                &failures);

    if (holder != scopeChain)
        GenerateScopeChainGuards(masm, scopeChain, holder, scopeChainReg(), outputReg(), &failures);
    else
        masm.movePtr(scopeChainReg(), outputReg());

    // At this point outputReg holds the object on which the property
    // was found, so we're done.
    Label rejoin_;
    CodeOffsetJump rejoinOffset = masm.jumpWithPatch(&rejoin_);
    masm.bind(&rejoin_);

    // All failures flow to here, so there is a common point to patch.
    masm.bind(&failures);
    if (holder != scopeChain) {
        Label exit_;
        exitOffset = masm.jumpWithPatch(&exit_);
        masm.bind(&exit_);
    }

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    CodeLocationJump rejoinJump(code, rejoinOffset);
    CodeLocationJump exitJump(code, exitOffset);

    PatchJump(lastJump(), CodeLocationLabel(code));
    PatchJump(rejoinJump, rejoinLabel());
    PatchJump(exitJump, cacheLabel());
    updateLastJump(exitJump);

    IonSpew(IonSpew_InlineCaches, "Generated BINDNAME non-global stub at %p", code->raw());
    return true;
}

static bool
IsCacheableScopeChain(JSObject *scopeChain, JSObject *holder)
{
    while (true) {
        if (!IsCacheableNonGlobalScope(scopeChain)) {
            IonSpew(IonSpew_InlineCaches, "Non-cacheable object on scope chain");
            return false;
        }

        if (scopeChain == holder)
            return true;

        scopeChain = &scopeChain->asScope().enclosingScope();
        if (!scopeChain) {
            IonSpew(IonSpew_InlineCaches, "Scope chain indirect hit");
            return false;
        }
    }

    JS_NOT_REACHED("Shouldn't get here");
    return false;
}

JSObject *
js::ion::BindNameCache(JSContext *cx, size_t cacheIndex, JSObject *scopeChain)
{
    IonScript *ion = GetTopIonJSScript(cx)->ionScript();
    IonCacheBindName &cache = ion->getCache(cacheIndex).toBindName();
    PropertyName *name = cache.name();

    JSObject *holder;
    if (scopeChain->isGlobal()) {
        holder = scopeChain;
    } else {
        holder = FindIdentifierBase(cx, scopeChain, name);
        if (!holder)
            return NULL;
    }

    // Stop generating new stubs once we hit the stub count limit, see
    // GetPropertyCache.
    if (cache.stubCount() < MAX_STUBS) {
        cache.incrementStubCount();

        if (scopeChain->isGlobal()) {
            if (!cache.attachGlobal(cx, scopeChain))
                return NULL;
        } else if (IsCacheableScopeChain(scopeChain, holder)) {
            if (!cache.attachNonGlobal(cx, scopeChain, holder))
                return NULL;
        } else {
            IonSpew(IonSpew_InlineCaches, "BINDNAME uncacheable scope chain");
        }
    }

    return holder;
}
