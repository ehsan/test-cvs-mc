/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99 ft=cpp:
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
 * The Original Code is mozilla.org code, released
 * June 24, 2010.
 *
 * The Initial Developer of the Original Code is
 *    The Mozilla Foundation
 *
 * Contributor(s):
 *    Andreas Gal <gal@mozilla.com>
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

#include "jsapi.h"
#include "jswrapper.h"

#include "XPCWrapper.h"

#include "nsJSPrincipals.h"

#include "AccessCheck.h"

namespace xpc {

nsIPrincipal *
GetCompartmentPrincipal(JSCompartment *compartment)
{
    return static_cast<nsJSPrincipals *>(compartment->principals)->nsIPrincipalPtr;
}

bool
AccessCheck::isPrivileged(JSCompartment *compartment)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if(!ssm) {
        return true;
    }

    PRBool privileged;
    if(NS_SUCCEEDED(ssm->IsSystemPrincipal(GetCompartmentPrincipal(compartment), &privileged))
       && privileged) {
        return true;
    }
    if(NS_SUCCEEDED(ssm->IsCapabilityEnabled("UniversalXPConnect", &privileged)) && privileged) {
        return true;
    }
    return false;
}

void
AccessCheck::deny(JSContext *cx, jsid id)
{
    if(id == JSID_VOID) {
        JS_ReportError(cx, "Permission denied to access object");
    } else {
        jsval idval;
        if (!JS_IdToValue(cx, id, &idval))
            return;
        JSString *str = JS_ValueToString(cx, idval);
        if (!str)
            return;
        JS_ReportError(cx, "Permission denied to access property '%hs'", str);
    }
}

bool
AccessCheck::enter(JSContext *cx, JSObject *wrapper, JSObject *wrappedObject, jsid id,
                   JSCrossCompartmentWrapper::Mode mode)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if(!ssm) {
        return true;
    }
    JSStackFrame *fp = NULL;
    nsresult rv = ssm->PushContextPrincipal(cx, JS_FrameIterator(cx, &fp),
                                            GetCompartmentPrincipal(wrappedObject->getCompartment(cx)));
    if(NS_FAILED(rv)) {
        NS_WARNING("Not allowing call because we're out of memory");
        JS_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

void
AccessCheck::leave(JSContext *cx, JSObject *wrapper, JSObject *wrappedObject)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if(ssm) {
        ssm->PopContextPrincipal(cx);
    }
}

}
