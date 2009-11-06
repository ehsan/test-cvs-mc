/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=80:
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * The Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ben Newman <b{enjam,newma}n@mozilla.com> (original author)
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

#ifndef mozilla_jsipc_ObjectWrapperChild_h__
#define mozilla_jsipc_ObjectWrapperChild_h__

#include "mozilla/jsipc/PObjectWrapperChild.h"

using mozilla::jsipc::JSVariant;

namespace mozilla {
namespace jsipc {

class ContextWrapperChild;
  
class ObjectWrapperChild
    : public PObjectWrapperChild
{
public:

    ObjectWrapperChild(JSContext* cx, JSObject* obj);

    JSObject* GetJSObject() const { return mObj; }
    
private:

    JSObject* const mObj;

    bool JSObject_to_JSVariant(JSContext* cx, JSObject* from, JSVariant* to);
    bool jsval_to_JSVariant(JSContext* cx, jsval from, JSVariant* to);

    static bool JSObject_from_PObjectWrapperChild(JSContext* cx,
                                                  const PObjectWrapperChild* from,
                                                  JSObject** to);
    static bool JSObject_from_JSVariant(JSContext* cx, const JSVariant& from,
                                        JSObject** to);
    static bool jsval_from_JSVariant(JSContext* cx, const JSVariant& from,
                                     jsval* to);

    ContextWrapperChild* Manager();

protected:

    void ActorDestroy(ActorDestroyReason why);

    bool AnswerAddProperty(const nsString& id,
                           JSBool* ok);

    bool AnswerGetProperty(const nsString& id,
                           JSBool* ok, JSVariant* vp);

    bool AnswerSetProperty(const nsString& id, const JSVariant& v,
                           JSBool* ok, JSVariant* vp);

    bool AnswerDelProperty(const nsString& id,
                           JSBool* ok, JSVariant* vp);

    bool AnswerNewEnumerateInit(/* no in-parameters */
                                JSBool* ok, JSVariant* statep, int* idp);

    bool AnswerNewEnumerateNext(const JSVariant& in_state,
                                JSBool* ok, JSVariant* statep, nsString* idp);

    bool RecvNewEnumerateDestroy(const JSVariant& in_state);

    bool AnswerNewResolve(const nsString& id, const int& flags,
                          JSBool* ok, PObjectWrapperChild** obj2);

    bool AnswerConvert(const JSType& type,
                       JSBool* ok, JSVariant* vp);

    bool AnswerCall(PObjectWrapperChild* receiver, const nsTArray<JSVariant>& argv,
                    JSBool* ok, JSVariant* rval);

    bool AnswerConstruct(const nsTArray<JSVariant>& argv,
                         JSBool* ok, PObjectWrapperChild** rval);

    bool AnswerHasInstance(const JSVariant& v,
                           JSBool* ok, JSBool* bp);
};

}}
  
#endif
