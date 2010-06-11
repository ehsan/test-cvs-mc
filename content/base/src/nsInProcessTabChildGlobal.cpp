/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 8; -*- */
/* vim: set sw=4 ts=8 et tw=80 : */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla Content App.
 *
 * The Initial Developer of the Original Code is
 *   The Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsInProcessTabChildGlobal.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsEventDispatcher.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIJSRuntimeService.h"
#include "nsComponentManagerUtils.h"
#include "nsNetUtil.h"
#include "nsScriptLoader.h"
#include "nsIJSContextStack.h"
#include "nsFrameLoader.h"

bool SendSyncMessageToParent(void* aCallbackData,
                             const nsAString& aMessage,
                             const nsAString& aJSON,
                             nsTArray<nsString>* aJSONRetVal)
{
  nsInProcessTabChildGlobal* tabChild =
    static_cast<nsInProcessTabChildGlobal*>(aCallbackData);
  nsCOMPtr<nsIContent> owner = tabChild->mOwner;
  nsTArray<nsCOMPtr<nsIRunnable> > asyncMessages;
  asyncMessages.SwapElements(tabChild->mASyncMessages);
  PRUint32 len = asyncMessages.Length();
  for (PRInt32 i = 0; i < len; ++i) {
    nsCOMPtr<nsIRunnable> async = asyncMessages[i];
    async->Run();
  }
  if (tabChild->mChromeMessageManager) {
    tabChild->mChromeMessageManager->ReceiveMessage(owner, aMessage, PR_TRUE,
                                                    aJSON, nsnull, aJSONRetVal);
  }
  return true;
}

class nsAsyncMessageToParent : public nsRunnable
{
public:
  nsAsyncMessageToParent(nsInProcessTabChildGlobal* aTabChild,
                         const nsAString& aMessage, const nsAString& aJSON)
    : mTabChild(aTabChild), mMessage(aMessage), mJSON(aJSON) {}

  NS_IMETHOD Run()
  {
    mTabChild->mASyncMessages.RemoveElement(this);
    if (mTabChild->mChromeMessageManager) {
      mTabChild->mChromeMessageManager->ReceiveMessage(mTabChild->mOwner, mMessage,
                                                       PR_FALSE,
                                                       mJSON, nsnull, nsnull);
    }
    return NS_OK;
  }
  nsRefPtr<nsInProcessTabChildGlobal> mTabChild;
  nsString mMessage;
  nsString mJSON;
};

bool SendAsyncMessageToParent(void* aCallbackData,
                              const nsAString& aMessage,
                              const nsAString& aJSON)
{
  nsInProcessTabChildGlobal* tabChild =
    static_cast<nsInProcessTabChildGlobal*>(aCallbackData);
  nsCOMPtr<nsIRunnable> ev =
    new nsAsyncMessageToParent(tabChild, aMessage, aJSON);
  tabChild->mASyncMessages.AppendElement(ev);
  NS_DispatchToCurrentThread(ev);
  return true;
}

static int tabChildC = 0;
nsInProcessTabChildGlobal::nsInProcessTabChildGlobal(nsIDocShell* aShell,
                                                     nsIContent* aOwner,
                                                     nsFrameMessageManager* aChrome)
: mCx(nsnull), mDocShell(aShell), mInitialized(PR_FALSE), mLoadingScript(PR_FALSE),
  mDelayedDisconnect(PR_FALSE), mOwner(aOwner), mChromeMessageManager(aChrome)
{
}

nsInProcessTabChildGlobal::~nsInProcessTabChildGlobal()
{
  Disconnect();
  NS_ASSERTION(!mCx, "Couldn't release JSContext?!?");
}

nsresult
nsInProcessTabChildGlobal::Init()
{
  nsresult rv = InitTabChildGlobal();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                   "Couldn't initialize nsInProcessTabChildGlobal");
  mMessageManager = new nsFrameMessageManager(PR_FALSE,
                                              SendSyncMessageToParent,
                                              SendAsyncMessageToParent,
                                              nsnull,
                                              this,
                                              nsnull,
                                              mCx);
  return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsInProcessTabChildGlobal)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsInProcessTabChildGlobal,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mMessageManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mGlobal)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsInProcessTabChildGlobal,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mMessageManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mGlobal)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsInProcessTabChildGlobal)
  NS_INTERFACE_MAP_ENTRY(nsIFrameMessageManager)
  NS_INTERFACE_MAP_ENTRY(nsIContentFrameMessageManager)
  NS_INTERFACE_MAP_ENTRY(nsIInProcessContentFrameMessageManager)
  NS_INTERFACE_MAP_ENTRY(nsIScriptContextPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ContentFrameMessageManager)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(nsInProcessTabChildGlobal, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(nsInProcessTabChildGlobal, nsDOMEventTargetHelper)

NS_IMETHODIMP
nsInProcessTabChildGlobal::GetContent(nsIDOMWindow** aContent)
{
  *aContent = nsnull;
  nsCOMPtr<nsIDOMWindow> window = do_GetInterface(mDocShell);
  window.swap(*aContent);
  return NS_OK;
}

NS_IMETHODIMP
nsInProcessTabChildGlobal::GetDocShell(nsIDocShell** aDocShell)
{
  NS_IF_ADDREF(*aDocShell = mDocShell);
  return NS_OK;
}

void
nsInProcessTabChildGlobal::Disconnect()
{
  mDocShell = nsnull;
  mOwner = nsnull;
  mChromeMessageManager = nsnull;
  if (mMessageManager) {
    static_cast<nsFrameMessageManager*>(mMessageManager.get())->Disconnect();
    mMessageManager = nsnull;
  }
  if (!mLoadingScript) {
    if (mCx) {
      JS_DestroyContext(mCx);
      mCx = nsnull;
    }
  } else {
    mDelayedDisconnect = PR_TRUE;
  }
}

NS_IMETHODIMP_(nsIContent *)
nsInProcessTabChildGlobal::GetOwnerContent()
{
  return mOwner;
}

nsresult
nsInProcessTabChildGlobal::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = PR_TRUE;
  aVisitor.mParentTarget = mOwner;

#ifdef DEBUG
  if (mOwner) {
    nsCOMPtr<nsIFrameLoaderOwner> owner = do_QueryInterface(mOwner);
    nsRefPtr<nsFrameLoader> fl = owner->GetFrameLoader();
    if (fl) {
      NS_ASSERTION(this == fl->GetTabChildGlobalAsEventTarget(),
                   "Wrong event target!");
      NS_ASSERTION(fl->mMessageManager == mChromeMessageManager,
                   "Wrong message manager!");
    }
  }
#endif

  return NS_OK;
}

nsresult
nsInProcessTabChildGlobal::InitTabChildGlobal()
{
  nsCOMPtr<nsIJSRuntimeService> runtimeSvc = 
    do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
  NS_ENSURE_STATE(runtimeSvc);

  JSRuntime* rt = nsnull;
  runtimeSvc->GetRuntime(&rt);
  NS_ENSURE_STATE(rt);

  JSContext* cx = JS_NewContext(rt, 8192);
  NS_ENSURE_STATE(cx);

  mCx = cx;

  nsContentUtils::XPConnect()->SetSecurityManagerForJSContext(cx, nsContentUtils::GetSecurityManager(), 0);
  nsContentUtils::GetSecurityManager()->GetSystemPrincipal(getter_AddRefs(mPrincipal));

  PRUint32 stackDummy;
  jsuword stackLimit, currentStackAddr = (jsuword)&stackDummy;

  // 256k stack space.
  const jsuword kStackSize = 0x40000;

#if JS_STACK_GROWTH_DIRECTION < 0
  stackLimit = (currentStackAddr > kStackSize) ?
               currentStackAddr - kStackSize :
               0;
#else
  stackLimit = (currentStackAddr + kStackSize > currentStackAddr) ?
               currentStackAddr + kStackSize :
               (jsuword) -1;
#endif

  JS_SetThreadStackLimit(cx, stackLimit);
  JS_SetScriptStackQuota(cx, 100*1024*1024);

  JS_SetOptions(cx, JS_GetOptions(cx) | JSOPTION_JIT | JSOPTION_ANONFUNFIX | JSOPTION_PRIVATE_IS_NSISUPPORTS);
  JS_SetVersion(cx, JSVERSION_LATEST);
  JS_SetGCParameterForThread(cx, JSGC_MAX_CODE_CACHE_BYTES, 1 * 1024 * 1024);

  JSAutoRequest ar(cx);
  nsIXPConnect* xpc = nsContentUtils::XPConnect();
  const PRUint32 flags = nsIXPConnect::INIT_JS_STANDARD_CLASSES |
                         /*nsIXPConnect::OMIT_COMPONENTS_OBJECT ?  |*/
                         nsIXPConnect::FLAG_SYSTEM_GLOBAL_OBJECT;

  nsISupports* scopeSupports =
    NS_ISUPPORTS_CAST(nsPIDOMEventTarget*, this);
  JS_SetContextPrivate(cx, scopeSupports);

  nsresult rv =
    xpc->InitClassesWithNewWrappedGlobal(cx, scopeSupports,
                                         NS_GET_IID(nsISupports), flags,
                                         getter_AddRefs(mGlobal));
  NS_ENSURE_SUCCESS(rv, false);

  JSObject* global = nsnull;
  rv = mGlobal->GetJSObject(&global);
  NS_ENSURE_SUCCESS(rv, false);

  JS_SetGlobalObject(cx, global);

  return NS_OK;
}

void
nsInProcessTabChildGlobal::LoadFrameScript(const nsAString& aURL)
{
  if (!mInitialized) {
    mInitialized = PR_TRUE;
    Init();
  }
  if (!mGlobal || !mCx) {
    return;
  }

  nsCString url = NS_ConvertUTF16toUTF8(aURL);
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), url);
  if (NS_FAILED(rv)) {
    return;
  }
  nsCOMPtr<nsIChannel> channel;
  NS_NewChannel(getter_AddRefs(channel), uri);
  if (!channel) {
    return;
  }

  nsCOMPtr<nsIInputStream> input;
  channel->Open(getter_AddRefs(input));
  nsString dataString;
  if (input) {
    const PRUint32 bufferSize = 256;
    char buffer[bufferSize];
    nsCString data;
    PRUint32 avail = 0;
    input->Available(&avail);
    PRUint32 read = 0;
    if (avail) {
      while (NS_SUCCEEDED(input->Read(buffer, bufferSize, &read)) && read) {
        data.Append(buffer, read);
        read = 0;
      }
    }
    nsScriptLoader::ConvertToUTF16(channel, (PRUint8*)data.get(), data.Length(),
                                   EmptyString(), nsnull, dataString);
  }

  if (!dataString.IsEmpty()) {
    JSAutoRequest ar(mCx);
    jsval retval;
    JSObject* global = nsnull;
    mGlobal->GetJSObject(&global);
    if (!global) {
      return;
    }

    JSPrincipals* jsprin = nsnull;
    mPrincipal->GetJSPrincipals(mCx, &jsprin);
    nsContentUtils::XPConnect()->FlagSystemFilenamePrefix(url.get(), PR_TRUE);
    nsContentUtils::ThreadJSContextStack()->Push(mCx);
    PRBool tmp = mLoadingScript;
    mLoadingScript = PR_TRUE;
    JS_EvaluateUCScriptForPrincipals(mCx, global, jsprin,
                                     (jschar*)dataString.get(),
                                     dataString.Length(),
                                     url.get(), 1, &retval);
    //XXX Argh, JSPrincipals are manually refcounted!
    JSPRINCIPALS_DROP(mCx, jsprin);
    mLoadingScript = tmp;
    JSContext* unused;
    nsContentUtils::ThreadJSContextStack()->Pop(&unused);
  }
  if (!mLoadingScript && mDelayedDisconnect) {
    mDelayedDisconnect = PR_FALSE;
    Disconnect();
  }
}
