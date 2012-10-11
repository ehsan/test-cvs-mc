/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef imgRequest_h__
#define imgRequest_h__

#include "imgIDecoderObserver.h"

#include "nsIChannelEventSink.h"
#include "nsIContentSniffer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIRequest.h"
#include "nsIProperties.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"
#include "nsITimedChannel.h"

#include "nsCategoryCache.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsTObserverArray.h"
#include "nsWeakReference.h"
#include "nsError.h"
#include "imgIRequest.h"
#include "nsIAsyncVerifyRedirectCallback.h"

class imgCacheValidator;
class imgStatusTracker;
class imgLoader;
class imgRequestProxy;
class imgCacheEntry;
class imgMemoryReporter;
class imgRequestNotifyRunnable;

namespace mozilla {
namespace image {
class Image;
} // namespace image
} // namespace mozilla

class imgRequest : public imgIDecoderObserver,
                   public nsIStreamListener,
                   public nsSupportsWeakReference,
                   public nsIChannelEventSink,
                   public nsIInterfaceRequestor,
                   public nsIAsyncVerifyRedirectCallback
{
public:
  imgRequest(imgLoader* aLoader);
  virtual ~imgRequest();

  NS_DECL_ISUPPORTS

  nsresult Init(nsIURI *aURI,
                nsIURI *aCurrentURI,
                nsIRequest *aRequest,
                nsIChannel *aChannel,
                imgCacheEntry *aCacheEntry,
                void *aLoadId,
                nsIPrincipal* aLoadingPrincipal,
                int32_t aCORSMode);

  // Callers must call imgRequestProxy::Notify later.
  void AddProxy(imgRequestProxy *proxy);

  nsresult RemoveProxy(imgRequestProxy *proxy, nsresult aStatus);

  void SniffMimeType(const char *buf, uint32_t len, nsACString& newType);

  // Cancel, but also ensure that all work done in Init() is undone. Call this
  // only when the channel has failed to open, and so calling Cancel() on it
  // won't be sufficient.
  void CancelAndAbort(nsresult aStatus);

  // Methods that get forwarded to the Image, or deferred until it's
  // instantiated.
  nsresult LockImage();
  nsresult UnlockImage();
  nsresult StartDecoding();
  nsresult RequestDecode();

  inline void SetInnerWindowID(uint64_t aInnerWindowId) {
    mInnerWindowId = aInnerWindowId;
  }

  inline uint64_t InnerWindowID() const {
    return mInnerWindowId;
  }

  // Set the cache validation information (expiry time, whether we must
  // validate, etc) on the cache entry based on the request information.
  // If this function is called multiple times, the information set earliest
  // wins.
  static void SetCacheValidation(imgCacheEntry* aEntry, nsIRequest* aRequest);

  bool GetMultipart() const { return mIsMultiPartChannel; }

  // The CORS mode for which we loaded this image.
  int32_t GetCORSMode() const { return mCORSMode; }

  // The principal for the document that loaded this image. Used when trying to
  // validate a CORS image load.
  already_AddRefed<nsIPrincipal> GetLoadingPrincipal() const
  {
    nsCOMPtr<nsIPrincipal> principal = mLoadingPrincipal;
    return principal.forget();
  }

private:
  friend class imgCacheEntry;
  friend class imgRequestProxy;
  friend class imgLoader;
  friend class imgCacheValidator;
  friend class imgStatusTracker;
  friend class imgCacheExpirationTracker;
  friend class imgRequestNotifyRunnable;

  inline void SetLoadId(void *aLoadId) {
    mLoadId = aLoadId;
  }
  void Cancel(nsresult aStatus);
  void RemoveFromCache();

  nsresult GetURI(nsIURI **aURI);
  nsresult GetSecurityInfo(nsISupports **aSecurityInfo);

  inline const char *GetMimeType() const {
    return mContentType.get();
  }
  inline nsIProperties *Properties() {
    return mProperties;
  }

  // Return the imgStatusTracker associated with this imgRequest.  It may live
  // in |mStatusTracker| or in |mImage.mStatusTracker|, depending on whether
  // mImage has been instantiated yet..
  imgStatusTracker& GetStatusTracker();
    
  // Reset the cache entry after we've dropped our reference to it. Used by the
  // imgLoader when our cache entry is re-requested after we've dropped our
  // reference to it.
  void SetCacheEntry(imgCacheEntry *entry);

  // Returns whether we've got a reference to the cache entry.
  bool HasCacheEntry() const;

  // Return true if at least one of our proxies, excluding
  // aProxyToIgnore, has an observer.  aProxyToIgnore may be null.
  bool HaveProxyWithObserver(imgRequestProxy* aProxyToIgnore) const;

  // Return the priority of the underlying network request, or return
  // PRIORITY_NORMAL if it doesn't support nsISupportsPriority.
  int32_t Priority() const;

  // Adjust the priority of the underlying network request by the given delta
  // on behalf of the given proxy.
  void AdjustPriority(imgRequestProxy *aProxy, int32_t aDelta);

  // Return whether we've seen some data at this point
  bool HasTransferredData() const { return mGotData; }

  // Set whether this request is stored in the cache. If it isn't, regardless
  // of whether this request has a non-null mCacheEntry, this imgRequest won't
  // try to update or modify the image cache.
  void SetIsInCache(bool cacheable);

  // Update the cache entry size based on the image container
  void UpdateCacheEntrySize();

public:
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_IMGICONTAINEROBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

private:
  friend class imgMemoryReporter;

  // Weak reference to parent loader; this request cannot outlive its owner.
  imgLoader* mLoader;
  nsCOMPtr<nsIRequest> mRequest;
  // The original URI we were loaded with. This is the same as the URI we are
  // keyed on in the cache.
  nsCOMPtr<nsIURI> mURI;
  // The URI of the resource we ended up loading after all redirects, etc.
  nsCOMPtr<nsIURI> mCurrentURI;
  // The principal of the document which loaded this image. Used when validating for CORS.
  nsCOMPtr<nsIPrincipal> mLoadingPrincipal;
  // The principal of this image.
  nsCOMPtr<nsIPrincipal> mPrincipal;
  // Status-tracker -- transferred to mImage, when it gets instantiated
  nsAutoPtr<imgStatusTracker> mStatusTracker;
  nsRefPtr<mozilla::image::Image> mImage;
  nsCOMPtr<nsIProperties> mProperties;
  nsCOMPtr<nsISupports> mSecurityInfo;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIInterfaceRequestor> mPrevChannelSink;

  nsTObserverArray<imgRequestProxy*> mObservers;

  nsCOMPtr<nsITimedChannel> mTimedChannel;

  nsCString mContentType;

  nsRefPtr<imgCacheEntry> mCacheEntry; /* we hold on to this to this so long as we have observers */

  void *mLoadId;

  imgCacheValidator *mValidator;
  nsCategoryCache<nsIContentSniffer> mImageSniffers;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;

  // The ID of the inner window origin, used for error reporting.
  uint64_t mInnerWindowId;

  // The CORS mode (defined in imgIRequest) this image was loaded with. By
  // default, imgIRequest::CORS_NONE.
  int32_t mCORSMode;

  // Sometimes consumers want to do things before the image is ready. Let them,
  // and apply the action when the image becomes available.
  bool mDecodeRequested : 1;

  bool mIsMultiPartChannel : 1;
  bool mGotData : 1;
  bool mIsInCache : 1;
  bool mBlockingOnload : 1;
  bool mResniffMimeType : 1;
};

#endif
