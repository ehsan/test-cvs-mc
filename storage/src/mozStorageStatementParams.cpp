/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=2 et lcs=trail\:.,tab\:>~ :
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsMemory.h"
#include "nsString.h"

#include "mozStoragePrivateHelpers.h"
#include "mozStorageStatementParams.h"
#include "mozIStorageStatement.h"

namespace mozilla {
namespace storage {

////////////////////////////////////////////////////////////////////////////////
//// StatementParams

StatementParams::StatementParams(mozIStorageStatement *aStatement) :
    mStatement(aStatement)
{
  NS_ASSERTION(mStatement != nullptr, "mStatement is null");
  (void)mStatement->GetParameterCount(&mParamCount);
}

NS_IMPL_ISUPPORTS2(
  StatementParams,
  mozIStorageStatementParams,
  nsIXPCScriptable
)

////////////////////////////////////////////////////////////////////////////////
//// nsIXPCScriptable

#define XPC_MAP_CLASSNAME StatementParams
#define XPC_MAP_QUOTED_CLASSNAME "StatementParams"
#define XPC_MAP_WANT_SETPROPERTY
#define XPC_MAP_WANT_NEWENUMERATE
#define XPC_MAP_WANT_NEWRESOLVE
#define XPC_MAP_FLAGS nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h"

NS_IMETHODIMP
StatementParams::SetProperty(nsIXPConnectWrappedNative *aWrapper,
                             JSContext *aCtx,
                             JSObject *aScopeObj,
                             jsid aId,
                             jsval *_vp,
                             bool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

  if (JSID_IS_INT(aId)) {
    int idx = JSID_TO_INT(aId);

    nsCOMPtr<nsIVariant> variant(convertJSValToVariant(aCtx, *_vp));
    NS_ENSURE_TRUE(variant, NS_ERROR_UNEXPECTED);
    nsresult rv = mStatement->BindByIndex(idx, variant);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (JSID_IS_STRING(aId)) {
    JSString *str = JSID_TO_STRING(aId);
    size_t length;
    const jschar *chars = JS_GetStringCharsAndLength(aCtx, str, &length);
    NS_ENSURE_TRUE(chars, NS_ERROR_UNEXPECTED);
    NS_ConvertUTF16toUTF8 name(chars, length);

    // check to see if there's a parameter with this name
    nsCOMPtr<nsIVariant> variant(convertJSValToVariant(aCtx, *_vp));
    NS_ENSURE_TRUE(variant, NS_ERROR_UNEXPECTED);
    nsresult rv = mStatement->BindByName(name, variant);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  *_retval = true;
  return NS_OK;
}

NS_IMETHODIMP
StatementParams::NewEnumerate(nsIXPConnectWrappedNative *aWrapper,
                              JSContext *aCtx,
                              JSObject *aScopeObj,
                              uint32_t aEnumOp,
                              jsval *_statep,
                              jsid *_idp,
                              bool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);

  switch (aEnumOp) {
    case JSENUMERATE_INIT:
    case JSENUMERATE_INIT_ALL:
    {
      // Start our internal index at zero.
      *_statep = JSVAL_ZERO;

      // And set our length, if needed.
      if (_idp)
        *_idp = INT_TO_JSID(mParamCount);

      break;
    }
    case JSENUMERATE_NEXT:
    {
      NS_ASSERTION(*_statep != JSVAL_NULL, "Internal state is null!");

      // Make sure we are in range first.
      uint32_t index = static_cast<uint32_t>(JSVAL_TO_INT(*_statep));
      if (index >= mParamCount) {
        *_statep = JSVAL_NULL;
        return NS_OK;
      }

      // Get the name of our parameter.
      nsAutoCString name;
      nsresult rv = mStatement->GetParameterName(index, name);
      NS_ENSURE_SUCCESS(rv, rv);

      // But drop the first character, which is going to be a ':'.
      JSString *jsname = ::JS_NewStringCopyN(aCtx, &(name.get()[1]),
                                             name.Length() - 1);
      NS_ENSURE_TRUE(jsname, NS_ERROR_OUT_OF_MEMORY);

      // Set our name.
      if (!::JS_ValueToId(aCtx, STRING_TO_JSVAL(jsname), _idp)) {
        *_retval = false;
        return NS_OK;
      }

      // And increment our index.
      *_statep = INT_TO_JSVAL(++index);

      break;
    }
    case JSENUMERATE_DESTROY:
    {
      // Clear our state.
      *_statep = JSVAL_NULL;

      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
StatementParams::NewResolve(nsIXPConnectWrappedNative *aWrapper,
                            JSContext *aCtx,
                            JSObject *aScopeObj,
                            jsid aId,
                            uint32_t aFlags,
                            JSObject **_objp,
                            bool *_retval)
{
  NS_ENSURE_TRUE(mStatement, NS_ERROR_NOT_INITIALIZED);
  // We do not throw at any point after this unless our index is out of range
  // because we want to allow the prototype chain to be checked for the
  // property.

  bool resolved = false;
  bool ok = true;
  if (JSID_IS_INT(aId)) {
    uint32_t idx = JSID_TO_INT(aId);

    // Ensure that our index is within range.  We do not care about the
    // prototype chain being checked here.
    if (idx >= mParamCount)
      return NS_ERROR_INVALID_ARG;

    ok = ::JS_DefineElement(aCtx, aScopeObj, idx, JSVAL_VOID, nullptr,
                            nullptr, JSPROP_ENUMERATE);
    resolved = true;
  }
  else if (JSID_IS_STRING(aId)) {
    JSString *str = JSID_TO_STRING(aId);
    size_t nameLength;
    const jschar *nameChars = JS_GetStringCharsAndLength(aCtx, str, &nameLength);
    NS_ENSURE_TRUE(nameChars, NS_ERROR_UNEXPECTED);

    // Check to see if there's a parameter with this name, and if not, let
    // the rest of the prototype chain be checked.
    NS_ConvertUTF16toUTF8 name(nameChars, nameLength);
    uint32_t idx;
    nsresult rv = mStatement->GetParameterIndex(name, &idx);
    if (NS_SUCCEEDED(rv)) {
      ok = ::JS_DefinePropertyById(aCtx, aScopeObj, aId, JSVAL_VOID, nullptr,
                                   nullptr, JSPROP_ENUMERATE);
      resolved = true;
    }
  }

  *_retval = ok;
  *_objp = resolved && ok ? aScopeObj : nullptr;
  return NS_OK;
}

} // namespace storage
} // namespace mozilla
