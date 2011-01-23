/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 *  The Mozilla Foundation
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jason Duell <jduell.mcbugs@gmail.com>
 *   Honza Bambas <honzab@firemni.cz>
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

#include "mozilla/net/HttpChannelParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/net/NeckoParent.h"
#include "mozilla/unused.h"
#include "HttpChannelParentListener.h"
#include "nsHttpChannel.h"
#include "nsHttpHandler.h"
#include "nsNetUtil.h"
#include "nsISupportsPriority.h"
#include "nsIAuthPromptProvider.h"
#include "nsIDocShellTreeItem.h"
#include "nsIBadCertListener2.h"
#include "nsICacheEntryDescriptor.h"
#include "nsSerializationHelper.h"
#include "nsISerializable.h"
#include "nsIAssociatedContentSecurity.h"
#include "nsIApplicationCacheService.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsIRedirectChannelRegistrar.h"

namespace mozilla {
namespace net {

HttpChannelParent::HttpChannelParent(PBrowserParent* iframeEmbedding)
: mIPCClosed(false)
{
  // Ensure gHttpHandler is initialized: we need the atom table up and running.
  nsIHttpProtocolHandler* handler;
  CallGetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &handler);
  NS_ASSERTION(handler, "no http handler");

  mTabParent = do_QueryInterface(static_cast<nsITabParent*>(
      static_cast<TabParent*>(iframeEmbedding)));
}

HttpChannelParent::~HttpChannelParent()
{
  gHttpHandler->Release();
}

void
HttpChannelParent::ActorDestroy(ActorDestroyReason why)
{
  // We may still have refcount>0 if nsHttpChannel hasn't called OnStopRequest
  // yet, but we must not send any more msgs to child.
  mIPCClosed = true;

  // As we know the child channel has finished, let the cache entry close.
  mCacheClosePreventer = 0;
}

//-----------------------------------------------------------------------------
// HttpChannelParent::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_ISUPPORTS6(HttpChannelParent,
                   nsIInterfaceRequestor,
                   nsIProgressEventSink,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIParentChannel,
                   nsIParentRedirectingChannel)

//-----------------------------------------------------------------------------
// HttpChannelParent::nsIInterfaceRequestor
//-----------------------------------------------------------------------------

NS_IMETHODIMP
HttpChannelParent::GetInterface(const nsIID& aIID, void **result)
{
  if (aIID.Equals(NS_GET_IID(nsIAuthPromptProvider)) ||
      aIID.Equals(NS_GET_IID(nsISecureBrowserUI))) {
    if (!mTabParent)
      return NS_NOINTERFACE;

    return mTabParent->QueryInterface(aIID, result);
  }

  return QueryInterface(aIID, result);
}

//-----------------------------------------------------------------------------
// HttpChannelParent::PHttpChannelParent
//-----------------------------------------------------------------------------

bool 
HttpChannelParent::RecvAsyncOpen(const IPC::URI&            aURI,
                                 const IPC::URI&            aOriginalURI,
                                 const IPC::URI&            aDocURI,
                                 const IPC::URI&            aReferrerURI,
                                 const PRUint32&            loadFlags,
                                 const RequestHeaderTuples& requestHeaders,
                                 const nsHttpAtom&          requestMethod,
                                 const IPC::InputStream&    uploadStream,
                                 const PRBool&              uploadStreamHasHeaders,
                                 const PRUint16&            priority,
                                 const PRUint8&             redirectionLimit,
                                 const PRBool&              allowPipelining,
                                 const PRBool&              forceAllowThirdPartyCookie,
                                 const bool&                doResumeAt,
                                 const PRUint64&            startPos,
                                 const nsCString&           entityID,
                                 const bool&                chooseApplicationCache,
                                 const nsCString&           appCacheClientID)
{
  nsCOMPtr<nsIURI> uri(aURI);
  nsCOMPtr<nsIURI> originalUri(aOriginalURI);
  nsCOMPtr<nsIURI> docUri(aDocURI);
  nsCOMPtr<nsIURI> referrerUri(aReferrerURI);

  nsCString uriSpec;
  uri->GetSpec(uriSpec);
  LOG(("HttpChannelParent RecvAsyncOpen [this=%x uri=%s]\n", 
       this, uriSpec.get()));

  nsresult rv;

  nsCOMPtr<nsIIOService> ios(do_GetIOService(&rv));
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  rv = NS_NewChannel(getter_AddRefs(mChannel), uri, ios, nsnull, nsnull, loadFlags);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());

  if (doResumeAt)
    httpChan->ResumeAt(startPos, entityID);

  if (originalUri)
    httpChan->SetOriginalURI(originalUri);
  if (docUri)
    httpChan->SetDocumentURI(docUri);
  if (referrerUri)
    httpChan->SetReferrerInternal(referrerUri);
  if (loadFlags != nsIRequest::LOAD_NORMAL)
    httpChan->SetLoadFlags(loadFlags);

  for (PRUint32 i = 0; i < requestHeaders.Length(); i++) {
    httpChan->SetRequestHeader(requestHeaders[i].mHeader,
                               requestHeaders[i].mValue,
                               requestHeaders[i].mMerge);
  }

  nsRefPtr<HttpChannelParentListener> channelListener =
      new HttpChannelParentListener(this);

  httpChan->SetNotificationCallbacks(channelListener);

  httpChan->SetRequestMethod(nsDependentCString(requestMethod.get()));

  nsCOMPtr<nsIInputStream> stream(uploadStream);
  if (stream) {
    httpChan->InternalSetUploadStream(stream);
    httpChan->SetUploadStreamHasHeaders(uploadStreamHasHeaders);
  }

  if (priority != nsISupportsPriority::PRIORITY_NORMAL)
    httpChan->SetPriority(priority);
  httpChan->SetRedirectionLimit(redirectionLimit);
  httpChan->SetAllowPipelining(allowPipelining);
  httpChan->SetForceAllowThirdPartyCookie(forceAllowThirdPartyCookie);

  nsCOMPtr<nsIApplicationCacheChannel> appCacheChan =
    do_QueryInterface(mChannel);
  nsCOMPtr<nsIApplicationCacheService> appCacheService =
    do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID);

  PRBool setChooseApplicationCache = chooseApplicationCache;
  if (appCacheChan && appCacheService) {
    // We might potentially want to drop this flag (that is TRUE by default)
    // after we succefully associate the channel with an application cache
    // reported by the channel child.  Dropping it here may be too early.
    appCacheChan->SetInheritApplicationCache(PR_FALSE);
    if (!appCacheClientID.IsEmpty()) {
      nsCOMPtr<nsIApplicationCache> appCache;
      rv = appCacheService->GetApplicationCache(appCacheClientID,
                                                getter_AddRefs(appCache));
      if (NS_SUCCEEDED(rv)) {
        appCacheChan->SetApplicationCache(appCache);
        setChooseApplicationCache = PR_FALSE;
      }
    }

    if (setChooseApplicationCache) {
      nsCOMPtr<nsIOfflineCacheUpdateService> offlineUpdateService =
        do_GetService("@mozilla.org/offlinecacheupdate-service;1", &rv);
      if (NS_SUCCEEDED(rv)) {
        rv = offlineUpdateService->OfflineAppAllowedForURI(uri,
                                                           nsnull,
                                                           &setChooseApplicationCache);

        if (setChooseApplicationCache && NS_SUCCEEDED(rv))
          appCacheChan->SetChooseApplicationCache(PR_TRUE);
      }
    }
  }

  rv = httpChan->AsyncOpen(channelListener, nsnull);
  if (NS_FAILED(rv))
    return SendCancelEarly(rv);

  return true;
}

bool
HttpChannelParent::RecvConnectChannel(const PRUint32& channelId)
{
  nsresult rv;

  LOG(("Looking for a registered channel [this=%p, id=%d]", this, channelId));
  rv = NS_LinkRedirectChannels(channelId, this, getter_AddRefs(mChannel));
  LOG(("  found channel %p, rv=%08x", mChannel.get(), rv));

  return true;
}

bool 
HttpChannelParent::RecvSetPriority(const PRUint16& priority)
{
  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
  httpChan->SetPriority(priority);

  nsCOMPtr<nsISupportsPriority> priorityRedirectChannel =
      do_QueryInterface(mRedirectChannel);
  if (priorityRedirectChannel)
    priorityRedirectChannel->SetPriority(priority);

  return true;
}

bool
HttpChannelParent::RecvSuspend()
{
  mChannel->Suspend();
  return true;
}

bool
HttpChannelParent::RecvResume()
{
  mChannel->Resume();
  return true;
}

bool
HttpChannelParent::RecvCancel(const nsresult& status)
{
  // May receive cancel before channel has been constructed!
  if (mChannel) {
    nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
    httpChan->Cancel(status);
  }
  return true;
}


bool
HttpChannelParent::RecvSetCacheTokenCachedCharset(const nsCString& charset)
{
  nsHttpChannel *chan = static_cast<nsHttpChannel *>(mChannel.get());

  nsresult rv;

  nsCOMPtr<nsICacheEntryDescriptor> cacheDescriptor;
  rv = chan->GetCacheToken(getter_AddRefs(cacheDescriptor));
  if (NS_SUCCEEDED(rv))
    cacheDescriptor->SetMetaDataElement("charset",
                                        PromiseFlatCString(charset).get());
  return true;
}

bool
HttpChannelParent::RecvSetResponseHeader(const nsCString& header,
                                         const nsCString& value,
                                         const bool& merge)
{
  nsHttpChannel *chan = static_cast<nsHttpChannel *>(mChannel.get());
  chan->SetResponseHeader(header, value, merge);
  return true;
}

bool
HttpChannelParent::RecvUpdateAssociatedContentSecurity(const PRInt32& high,
                                                       const PRInt32& low,
                                                       const PRInt32& broken,
                                                       const PRInt32& no)
{
  nsHttpChannel *chan = static_cast<nsHttpChannel *>(mChannel.get());

  nsCOMPtr<nsISupports> secInfo;
  chan->GetSecurityInfo(getter_AddRefs(secInfo));

  nsCOMPtr<nsIAssociatedContentSecurity> assoc = do_QueryInterface(secInfo);
  if (!assoc)
    return true;

  assoc->SetCountSubRequestsHighSecurity(high);
  assoc->SetCountSubRequestsLowSecurity(low);
  assoc->SetCountSubRequestsBrokenSecurity(broken);
  assoc->SetCountSubRequestsNoSecurity(no);

  return true;
}

bool
HttpChannelParent::RecvRedirect2Verify(const nsresult& result, 
                                       const RequestHeaderTuples& changedHeaders)
{
  if (NS_SUCCEEDED(result)) {
    nsCOMPtr<nsIHttpChannel> newHttpChannel =
        do_QueryInterface(mRedirectChannel);

    if (newHttpChannel) {
      for (PRUint32 i = 0; i < changedHeaders.Length(); i++) {
        newHttpChannel->SetRequestHeader(changedHeaders[i].mHeader,
                                         changedHeaders[i].mValue,
                                         changedHeaders[i].mMerge);
      }
    }
  }

  mRedirectCallback->OnRedirectVerifyCallback(result);
  mRedirectCallback = nsnull;
  return true;
}

bool
HttpChannelParent::RecvDocumentChannelCleanup()
{
  // We must clear the cache entry here, else we'll block other channels from
  // reading it if we've got it open for writing.  
  mCacheClosePreventer = 0;

  return true;
}

bool 
HttpChannelParent::RecvMarkOfflineCacheEntryAsForeign()
{
  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
  httpChan->MarkOfflineCacheEntryAsForeign();
  return true;
}

//-----------------------------------------------------------------------------
// HttpChannelParent::nsIRequestObserver
//-----------------------------------------------------------------------------

NS_IMETHODIMP
HttpChannelParent::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  LOG(("HttpChannelParent::OnStartRequest [this=%x]\n", this));

  nsHttpChannel *chan = static_cast<nsHttpChannel *>(aRequest);
  nsHttpResponseHead *responseHead = chan->GetResponseHead();
  nsHttpRequestHead  *requestHead = chan->GetRequestHead();
  PRBool isFromCache = false;
  chan->IsFromCache(&isFromCache);
  PRUint32 expirationTime = nsICache::NO_EXPIRATION_TIME;
  chan->GetCacheTokenExpirationTime(&expirationTime);
  nsCString cachedCharset;
  chan->GetCacheTokenCachedCharset(cachedCharset);

  PRBool loadedFromApplicationCache;
  chan->GetLoadedFromApplicationCache(&loadedFromApplicationCache);
  if (loadedFromApplicationCache) {
    nsCOMPtr<nsIApplicationCache> appCache;
    chan->GetApplicationCache(getter_AddRefs(appCache));
    nsCString appCacheGroupId;
    nsCString appCacheClientId;
    appCache->GetGroupID(appCacheGroupId);
    appCache->GetClientID(appCacheClientId);
    if (mIPCClosed || 
        !SendAssociateApplicationCache(appCacheGroupId, appCacheClientId))
    {
      return NS_ERROR_UNEXPECTED;
    }
  }

  nsCOMPtr<nsIEncodedChannel> encodedChannel = do_QueryInterface(aRequest);
  if (encodedChannel)
    encodedChannel->SetApplyConversion(PR_FALSE);

  // Prevent cache entry from being closed during HttpChannel::OnStopRequest:
  // - We need the cache entry for RecvSetCacheTokenCachedCharset()
  // - The child channel may call GetCacheEntryClosePreventer, so we have to
  //   call it now (otherwise we could hit OnStopRequest and close entry
  //   before child gets a chance to keep it open).
  // We close entry either when RecvDocumentChannelCleanup is called, or the
  // IPDL channel is deleted.
  chan->GetCacheEntryClosePreventer(getter_AddRefs(mCacheClosePreventer));

  nsCString secInfoSerialization;
  nsCOMPtr<nsISupports> secInfoSupp;
  chan->GetSecurityInfo(getter_AddRefs(secInfoSupp));
  if (secInfoSupp) {
    nsCOMPtr<nsISerializable> secInfoSer = do_QueryInterface(secInfoSupp);
    if (secInfoSer)
      NS_SerializeToString(secInfoSer, secInfoSerialization);
  }

  RequestHeaderTuples headers;
  nsHttpHeaderArray harray = requestHead->Headers();

  for (PRUint32 i = 0; i < harray.Count(); i++) {
    RequestHeaderTuple* tuple = headers.AppendElement();
    tuple->mHeader = harray.Headers()[i].header;
    tuple->mValue  = harray.Headers()[i].value;
    tuple->mMerge  = false;
  }

  nsCOMPtr<nsICacheEntryDescriptor> cacheDescriptor;
  chan->GetCacheToken(getter_AddRefs(cacheDescriptor));

  if (mIPCClosed || 
      !SendOnStartRequest(responseHead ? *responseHead : nsHttpResponseHead(), 
                          !!responseHead,
                          headers,
                          isFromCache,
                          cacheDescriptor ? PR_TRUE : PR_FALSE,
                          expirationTime, cachedCharset, secInfoSerialization)) 
  {
    return NS_ERROR_UNEXPECTED; 
  }
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelParent::OnStopRequest(nsIRequest *aRequest, 
                                 nsISupports *aContext, 
                                 nsresult aStatusCode)
{
  LOG(("HttpChannelParent::OnStopRequest: [this=%x status=%ul]\n", 
       this, aStatusCode));

  if (mIPCClosed || !SendOnStopRequest(aStatusCode))
    return NS_ERROR_UNEXPECTED; 
  return NS_OK;
}

//-----------------------------------------------------------------------------
// HttpChannelParent::nsIStreamListener
//-----------------------------------------------------------------------------

NS_IMETHODIMP
HttpChannelParent::OnDataAvailable(nsIRequest *aRequest, 
                                   nsISupports *aContext, 
                                   nsIInputStream *aInputStream, 
                                   PRUint32 aOffset, 
                                   PRUint32 aCount)
{
  LOG(("HttpChannelParent::OnDataAvailable [this=%x]\n", this));
 
  nsCString data;
  nsresult rv = NS_ReadInputStreamToString(aInputStream, data, aCount);
  if (NS_FAILED(rv))
    return rv;

  if (mIPCClosed || !SendOnDataAvailable(data, aOffset, aCount))
    return NS_ERROR_UNEXPECTED; 

  return NS_OK;
}

//-----------------------------------------------------------------------------
// HttpChannelParent::nsIProgressEventSink
//-----------------------------------------------------------------------------

NS_IMETHODIMP
HttpChannelParent::OnProgress(nsIRequest *aRequest, 
                              nsISupports *aContext, 
                              PRUint64 aProgress, 
                              PRUint64 aProgressMax)
{
  if (mIPCClosed || !SendOnProgress(aProgress, aProgressMax))
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelParent::OnStatus(nsIRequest *aRequest, 
                            nsISupports *aContext, 
                            nsresult aStatus, 
                            const PRUnichar *aStatusArg)
{
  if (mIPCClosed || !SendOnStatus(aStatus, nsString(aStatusArg)))
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}

//-----------------------------------------------------------------------------
// HttpChannelParent::nsIParentChannel
//-----------------------------------------------------------------------------

NS_IMETHODIMP
HttpChannelParent::Delete()
{
  if (!mIPCClosed)
    SendDeleteSelf();

  return NS_OK;
}

//-----------------------------------------------------------------------------
// HttpChannelParent::nsIParentRedirectingChannel
//-----------------------------------------------------------------------------

NS_IMETHODIMP
HttpChannelParent::StartRedirect(PRUint32 newChannelId,
                                 nsIChannel* newChannel,
                                 PRUint32 redirectFlags,
                                 nsIAsyncVerifyRedirectCallback* callback)
{
  if (mIPCClosed)
    return NS_BINDING_ABORTED;

  nsCOMPtr<nsIURI> newURI;
  newChannel->GetURI(getter_AddRefs(newURI));

  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
  nsHttpResponseHead *responseHead = httpChan->GetResponseHead();
  bool result = SendRedirect1Begin(newChannelId,
                                   IPC::URI(newURI),
                                   redirectFlags,
                                   responseHead ? *responseHead
                                                : nsHttpResponseHead());
  if (!result)
    return NS_BINDING_ABORTED;

  // Result is handled in RecvRedirect2Verify above

  mRedirectChannel = newChannel;
  mRedirectCallback = callback;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelParent::CompleteRedirect(PRBool succeeded)
{
  if (succeeded && !mIPCClosed) {
    // TODO: check return value: assume child dead if failed
    unused << SendRedirect3Complete();
  }

  mRedirectChannel = nsnull;
  return NS_OK;
}

}} // mozilla::net
