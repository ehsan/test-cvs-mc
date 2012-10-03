/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/histogram.h"
#include "ImageLogging.h"
#include "nsComponentManagerUtils.h"
#include "imgIContainerObserver.h"
#include "nsError.h"
#include "Decoder.h"
#include "imgIDecoderObserver.h"
#include "RasterImage.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"
#include "nsStringStream.h"
#include "prmem.h"
#include "prenv.h"
#include "ImageContainer.h"
#include "Layers.h"

#include "nsPNGDecoder.h"
#include "nsGIFDecoder2.h"
#include "nsJPEGDecoder.h"
#include "nsBMPDecoder.h"
#include "nsICODecoder.h"
#include "nsIconDecoder.h"

#include "gfxContext.h"
#include "gfx2DGlue.h"

#include "mozilla/Preferences.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/gfx/Scale.h"

// The high-quality scaler requires Skia.
#ifdef MOZ_ENABLE_SKIA

static bool
ScaleFrameImage(imgFrame *aSrcFrame, imgFrame *aDstFrame,
                const gfxSize &aScaleFactors)
{
  if (aScaleFactors.width <= 0 || aScaleFactors.height <= 0)
    return false;

  imgFrame *srcFrame = aSrcFrame;
  nsIntRect srcRect = srcFrame->GetRect();
  uint32_t dstWidth = NSToIntRoundUp(srcRect.width * aScaleFactors.width);
  uint32_t dstHeight = NSToIntRoundUp(srcRect.height * aScaleFactors.height);

  // Destination is unconditionally ARGB32 because that's what the scaler
  // outputs.
  nsresult rv = aDstFrame->Init(0, 0, dstWidth, dstHeight,
                                gfxASurface::ImageFormatARGB32);
  if (!NS_FAILED(rv)) {
    uint8_t* srcData;
    uint32_t srcDataLength;
    // Source frame data is locked/unlocked on the main thread.
    srcFrame->GetImageData(&srcData, &srcDataLength);
    NS_ASSERTION(srcData != nullptr, "Source data is unavailable! Is it locked?");

    uint8_t* dstData;
    uint32_t dstDataLength;
    aDstFrame->LockImageData();
    aDstFrame->GetImageData(&dstData, &dstDataLength);

    // This returns an SkBitmap backed by dstData; since it wrote to dstData,
    // we don't need to look at that SkBitmap.
    mozilla::gfx::Scale(srcData, srcRect.width, srcRect.height, aSrcFrame->GetImageBytesPerRow(),
                        dstData, dstWidth, dstHeight, aDstFrame->GetImageBytesPerRow(),
                        mozilla::gfx::ImageFormatToSurfaceFormat(aSrcFrame->GetFormat()));

    aDstFrame->UnlockImageData();
    return true;
  }

  return false;
}
#else // MOZ_ENABLE_SKIA
static bool
ScaleFrameImage(imgFrame *aSrcFrame, imgFrame *aDstFrame,
                const gfxSize &aScaleFactors)
{
  return false;
}
#endif // MOZ_ENABLE_SKIA

using namespace mozilla;
using namespace mozilla::image;
using namespace mozilla::layers;

// a mask for flags that will affect the decoding
#define DECODE_FLAGS_MASK (imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA | imgIContainer::FLAG_DECODE_NO_COLORSPACE_CONVERSION)
#define DECODE_FLAGS_DEFAULT 0

/* Accounting for compressed data */
#if defined(PR_LOGGING)
static PRLogModuleInfo *gCompressedImageAccountingLog = PR_NewLogModule ("CompressedImageAccounting");
#else
#define gCompressedImageAccountingLog
#endif

// Tweakable progressive decoding parameters.  These are initialized to 0 here
// because otherwise, we have to initialize them in a static initializer, which
// makes us slower to start up.
static uint32_t gDecodeBytesAtATime = 0;
static uint32_t gMaxMSBeforeYield = 0;
static bool gHQDownscaling = false;
// This is interpreted as a floating-point value / 1000
static uint32_t gHQDownscalingMinFactor = 1000;

static void
InitPrefCaches()
{
  Preferences::AddUintVarCache(&gDecodeBytesAtATime,
                               "image.mem.decode_bytes_at_a_time", 200000);
  Preferences::AddUintVarCache(&gMaxMSBeforeYield,
                               "image.mem.max_ms_before_yield", 400);
  Preferences::AddBoolVarCache(&gHQDownscaling,
                               "image.high_quality_downscaling.enabled", false);
  Preferences::AddUintVarCache(&gHQDownscalingMinFactor,
                               "image.high_quality_downscaling.min_factor", 1000);
}

/* We define our own error checking macros here for 2 reasons:
 *
 * 1) Most of the failures we encounter here will (hopefully) be
 * the result of decoding failures (ie, bad data) and not code
 * failures. As such, we don't want to clutter up debug consoles
 * with spurious messages about NS_ENSURE_SUCCESS failures.
 *
 * 2) We want to set the internal error flag, shutdown properly,
 * and end up in an error state.
 *
 * So this macro should be called when the desired failure behavior
 * is to put the container into an error state and return failure.
 * It goes without saying that macro won't compile outside of a
 * non-static RasterImage method.
 */
#define LOG_CONTAINER_ERROR                      \
  PR_BEGIN_MACRO                                 \
  PR_LOG (gImgLog, PR_LOG_ERROR,                 \
          ("RasterImage: [this=%p] Error "      \
           "detected at line %u for image of "   \
           "type %s\n", this, __LINE__,          \
           mSourceDataMimeType.get()));          \
  PR_END_MACRO

#define CONTAINER_ENSURE_SUCCESS(status)      \
  PR_BEGIN_MACRO                              \
  nsresult _status = status; /* eval once */  \
  if (NS_FAILED(_status)) {                   \
    LOG_CONTAINER_ERROR;                      \
    DoError();                                \
    return _status;                           \
  }                                           \
 PR_END_MACRO

#define CONTAINER_ENSURE_TRUE(arg, rv)  \
  PR_BEGIN_MACRO                        \
  if (!(arg)) {                         \
    LOG_CONTAINER_ERROR;                \
    DoError();                          \
    return rv;                          \
  }                                     \
  PR_END_MACRO



static int num_containers;
static int num_discardable_containers;
static int64_t total_source_bytes;
static int64_t discardable_source_bytes;

/* Are we globally disabling image discarding? */
static bool
DiscardingEnabled()
{
  static bool inited;
  static bool enabled;

  if (!inited) {
    inited = true;

    enabled = (PR_GetEnv("MOZ_DISABLE_IMAGE_DISCARD") == nullptr);
  }

  return enabled;
}

namespace mozilla {
namespace image {

/* static */ StaticRefPtr<RasterImage::DecodeWorker> RasterImage::DecodeWorker::sSingleton;
/* static */ nsRefPtr<RasterImage::ScaleWorker> RasterImage::ScaleWorker::sSingleton;
/* static */ nsRefPtr<RasterImage::DrawWorker> RasterImage::DrawWorker::sSingleton;
static nsCOMPtr<nsIThread> sScaleWorkerThread = nullptr;

#ifndef DEBUG
NS_IMPL_ISUPPORTS3(RasterImage, imgIContainer, nsIProperties,
                   nsISupportsWeakReference)
#else
NS_IMPL_ISUPPORTS4(RasterImage, imgIContainer, nsIProperties,
                   imgIContainerDebug, nsISupportsWeakReference)
#endif

//******************************************************************************
RasterImage::RasterImage(imgStatusTracker* aStatusTracker) :
  Image(aStatusTracker), // invoke superclass's constructor
  mSize(0,0),
  mFrameDecodeFlags(DECODE_FLAGS_DEFAULT),
  mAnim(nullptr),
  mLoopCount(-1),
  mObserver(nullptr),
  mLockCount(0),
  mDecoder(nullptr),
  mDecodeRequest(this),
  mBytesDecoded(0),
  mDecodeCount(0),
#ifdef DEBUG
  mFramesNotified(0),
#endif
  mHasSize(false),
  mDecodeOnDraw(false),
  mMultipart(false),
  mDiscardable(false),
  mHasSourceData(false),
  mDecoded(false),
  mHasBeenDecoded(false),
  mInDecoder(false),
  mAnimationFinished(false),
  mFinishing(false),
  mInUpdateImageContainer(false),
  mScaleRequest(this)
{
  // Set up the discard tracker node.
  mDiscardTrackerNode.img = this;
  Telemetry::GetHistogramById(Telemetry::IMAGE_DECODE_COUNT)->Add(0);

  // Statistics
  num_containers++;

}

//******************************************************************************
RasterImage::~RasterImage()
{
  ScaleRequest::Stop(mScaleRequest.image);

  delete mAnim;

  for (unsigned int i = 0; i < mFrames.Length(); ++i)
    delete mFrames[i];

  // Discardable statistics
  if (mDiscardable) {
    num_discardable_containers--;
    discardable_source_bytes -= mSourceData.Length();

    PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
            ("CompressedImageAccounting: destroying RasterImage %p.  "
             "Total Containers: %d, Discardable containers: %d, "
             "Total source bytes: %lld, Source bytes for discardable containers %lld",
             this,
             num_containers,
             num_discardable_containers,
             total_source_bytes,
             discardable_source_bytes));
    DiscardTracker::Remove(&mDiscardTrackerNode);
  }

  // If we have a decoder open, shut it down
  if (mDecoder) {
    nsresult rv = ShutdownDecoder(eShutdownIntent_Interrupted);
    if (NS_FAILED(rv))
      NS_WARNING("Failed to shut down decoder in destructor!");
  }

  // Total statistics
  num_containers--;
  total_source_bytes -= mSourceData.Length();
}

void
RasterImage::Initialize()
{
  InitPrefCaches();

  // Create our singletons now, so we don't have to worry about what thread
  // they're created on.
  DecodeWorker::Singleton();
  DrawWorker::Singleton();
  ScaleWorker::Singleton();
}

nsresult
RasterImage::Init(imgIDecoderObserver *aObserver,
                  const char* aMimeType,
                  const char* aURIString,
                  uint32_t aFlags)
{
  // We don't support re-initialization
  if (mInitialized)
    return NS_ERROR_ILLEGAL_VALUE;

  // Not sure an error can happen before init, but be safe
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aMimeType);

  // We must be non-discardable and non-decode-on-draw for
  // multipart channels
  NS_ABORT_IF_FALSE(!(aFlags & INIT_FLAG_MULTIPART) ||
                    (!(aFlags & INIT_FLAG_DISCARDABLE) &&
                     !(aFlags & INIT_FLAG_DECODE_ON_DRAW)),
                    "Can't be discardable or decode-on-draw for multipart");

  // Store initialization data
  mObserver = do_GetWeakReference(aObserver);
  mSourceDataMimeType.Assign(aMimeType);
  mURIString.Assign(aURIString);
  mDiscardable = !!(aFlags & INIT_FLAG_DISCARDABLE);
  mDecodeOnDraw = !!(aFlags & INIT_FLAG_DECODE_ON_DRAW);
  mMultipart = !!(aFlags & INIT_FLAG_MULTIPART);

  // Statistics
  if (mDiscardable) {
    num_discardable_containers++;
    discardable_source_bytes += mSourceData.Length();
  }

  // If we're being called from ExtractFrame (used by borderimage),
  // we don't actually do any decoding. Bail early.
  // XXX - This should be removed when we fix borderimage
  if (mSourceDataMimeType.Length() == 0) {
    mInitialized = true;
    return NS_OK;
  }

  // Instantiate the decoder
  //
  // If we're doing decode-on-draw, we want to do a quick first pass to get
  // the size but nothing else. We instantiate another decoder later to do
  // the full decoding.
  nsresult rv = InitDecoder(/* aDoSizeDecode = */ mDecodeOnDraw);
  CONTAINER_ENSURE_SUCCESS(rv);

  // Mark us as initialized
  mInitialized = true;

  return NS_OK;
}

bool
RasterImage::AdvanceFrame(TimeStamp aTime, nsIntRect* aDirtyRect)
{
  NS_ASSERTION(aTime <= TimeStamp::Now(),
               "Given time appears to be in the future");

  imgFrame* nextFrame = nullptr;
  uint32_t currentFrameIndex = mAnim->currentAnimationFrameIndex;
  uint32_t nextFrameIndex = mAnim->currentAnimationFrameIndex + 1;
  uint32_t timeout = 0;

  // Figure out if we have the next full frame. This is more complicated than
  // just checking for mFrames.Length() because decoders append their frames
  // before they're filled in.
  NS_ABORT_IF_FALSE(mDecoder || nextFrameIndex <= mFrames.Length(),
                    "How did we get 2 indices too far by incrementing?");

  // If we don't have a decoder, we know we've got everything we're going to
  // get. If we do, we only display fully-downloaded frames; everything else
  // gets delayed.
  bool haveFullNextFrame = (mMultipart && mBytesDecoded == 0) || !mDecoder ||
                           nextFrameIndex < mDecoder->GetCompleteFrameCount();

  // If we're done decoding the next frame, go ahead and display it now and
  // reinit with the next frame's delay time.
  if (haveFullNextFrame) {
    if (mFrames.Length() == nextFrameIndex) {
      // End of Animation, unless we are looping forever

      // If animation mode is "loop once", it's time to stop animating
      if (mAnimationMode == kLoopOnceAnimMode || mLoopCount == 0) {
        mAnimationFinished = true;
        EvaluateAnimation();
      }

      // We may have used compositingFrame to build a frame, and then copied
      // it back into mFrames[..].  If so, delete composite to save memory
      if (mAnim->compositingFrame && mAnim->lastCompositedFrameIndex == -1) {
        mAnim->compositingFrame = nullptr;
      }

      nextFrameIndex = 0;

      if (mLoopCount > 0) {
        mLoopCount--;
      }

      if (!mAnimating) {
        // break out early if we are actually done animating
        return false;
      }
    }

    if (!(nextFrame = mFrames[nextFrameIndex])) {
      // something wrong with the next frame, skip it
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      return false;
    }

    timeout = nextFrame->GetTimeout();

  } else {
    // Uh oh, the frame we want to show is currently being decoded (partial)
    // Wait until the next refresh driver tick and try again
    return false;
  }

  if (!(timeout > 0)) {
    mAnimationFinished = true;
    EvaluateAnimation();
  }

  if (nextFrameIndex == 0) {
    *aDirtyRect = mAnim->firstFrameRefreshArea;
  } else {
    imgFrame *curFrame = mFrames[currentFrameIndex];

    if (!curFrame) {
      return false;
    }

    // Change frame
    if (NS_FAILED(DoComposite(aDirtyRect, curFrame,
                              nextFrame, nextFrameIndex))) {
      // something went wrong, move on to next
      NS_WARNING("RasterImage::AdvanceFrame(): Compositing of frame failed");
      nextFrame->SetCompositingFailed(true);
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      mAnim->currentAnimationFrameTime = aTime;
      return false;
    }

    nextFrame->SetCompositingFailed(false);
  }

  // Set currentAnimationFrameIndex at the last possible moment
  mAnim->currentAnimationFrameIndex = nextFrameIndex;
  mAnim->currentAnimationFrameTime = aTime;

  return true;
}

//******************************************************************************
// [notxpcom] void requestRefresh ([const] in TimeStamp aTime);
NS_IMETHODIMP_(void)
RasterImage::RequestRefresh(const mozilla::TimeStamp& aTime)
{
  if (!mAnimating || !ShouldAnimate()) {
    return;
  }

  EnsureAnimExists();

  // only advance the frame if the current time is greater than or
  // equal to the current frame's end time.
  TimeStamp currentFrameEndTime = GetCurrentImgFrameEndTime();
  bool frameAdvanced = false;

  // The dirtyRect variable will contain an accumulation of the sub-rectangles
  // that are dirty for each frame we advance in AdvanceFrame().
  nsIntRect dirtyRect;

  while (currentFrameEndTime <= aTime) {
    TimeStamp oldFrameEndTime = currentFrameEndTime;
    nsIntRect frameDirtyRect;
    bool didAdvance = AdvanceFrame(aTime, &frameDirtyRect);
    frameAdvanced = frameAdvanced || didAdvance;
    currentFrameEndTime = GetCurrentImgFrameEndTime();

    // Accumulate the dirty area.
    dirtyRect = dirtyRect.Union(frameDirtyRect);

    // if we didn't advance a frame, and our frame end time didn't change,
    // then we need to break out of this loop & wait for the frame(s)
    // to finish downloading
    if (!frameAdvanced && (currentFrameEndTime == oldFrameEndTime)) {
      break;
    }
  }

  if (frameAdvanced) {
    nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));

    if (!observer) {
      NS_ERROR("Refreshing image after its imgRequest is gone");
      StopAnimation();
      return;
    }

    // Notify listeners that our frame has actually changed, but do this only
    // once for all frames that we've now passed (if AdvanceFrame() was called
    // more than once).
    #ifdef DEBUG
      mFramesNotified++;
    #endif

    UpdateImageContainer();
    observer->FrameChanged(nullptr, this, &dirtyRect);
  }
}

//******************************************************************************
/* [noscript] imgIContainer extractFrame(uint32_t aWhichFrame,
 *                                       [const] in nsIntRect aRegion,
 *                                       in uint32_t aFlags); */
NS_IMETHODIMP
RasterImage::ExtractFrame(uint32_t aWhichFrame,
                          const nsIntRect &aRegion,
                          uint32_t aFlags,
                          imgIContainer **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  // Disallowed in the API
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  // Make a new container. This should switch to another class with bug 505959.
  nsRefPtr<RasterImage> img(new RasterImage());

  // We don't actually have a mimetype in this case. The empty string tells the
  // init routine not to try to instantiate a decoder. This should be fixed in
  // bug 505959.
  img->Init(nullptr, "", "", INIT_FLAG_NONE);
  img->SetSize(aRegion.width, aRegion.height);
  img->mDecoded = true; // Also, we need to mark the image as decoded
  img->mHasBeenDecoded = true;
  img->mFrameDecodeFlags = aFlags & DECODE_FLAGS_MASK;

  if (!ApplyDecodeFlags(aFlags))
    return NS_ERROR_NOT_AVAILABLE;
  
  // If a synchronous decode was requested, do it
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // Get the frame. If it's not there, it's probably the caller's fault for
  // not waiting for the data to be loaded from the network or not passing
  // FLAG_SYNC_DECODE
  uint32_t frameIndex = (aWhichFrame == FRAME_FIRST) ?
                        0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    *_retval = nullptr;
    return NS_ERROR_FAILURE;
  }

  // The frame can be smaller than the image. We want to extract only the part
  // of the frame that actually exists.
  nsIntRect framerect = frame->GetRect();
  framerect.IntersectRect(framerect, aRegion);

  if (framerect.IsEmpty())
    return NS_ERROR_NOT_AVAILABLE;

  nsAutoPtr<imgFrame> subframe;
  rv = frame->Extract(framerect, getter_Transfers(subframe));
  if (NS_FAILED(rv))
    return rv;

  img->mFrames.AppendElement(subframe.forget());

  img->mStatusTracker->RecordLoaded();
  img->mStatusTracker->RecordDecoded();

  *_retval = img.forget().get();

  return NS_OK;
}

//******************************************************************************
/* readonly attribute int32_t width; */
NS_IMETHODIMP
RasterImage::GetWidth(int32_t *aWidth)
{
  NS_ENSURE_ARG_POINTER(aWidth);

  if (mError) {
    *aWidth = 0;
    return NS_ERROR_FAILURE;
  }

  *aWidth = mSize.width;
  return NS_OK;
}

//******************************************************************************
/* readonly attribute int32_t height; */
NS_IMETHODIMP
RasterImage::GetHeight(int32_t *aHeight)
{
  NS_ENSURE_ARG_POINTER(aHeight);

  if (mError) {
    *aHeight = 0;
    return NS_ERROR_FAILURE;
  }

  *aHeight = mSize.height;
  return NS_OK;
}

//******************************************************************************
/* unsigned short GetType(); */
NS_IMETHODIMP
RasterImage::GetType(uint16_t *aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = GetType();
  return NS_OK;
}

//******************************************************************************
/* [noscript, notxpcom] uint16_t GetType(); */
NS_IMETHODIMP_(uint16_t)
RasterImage::GetType()
{
  return imgIContainer::TYPE_RASTER;
}

imgFrame*
RasterImage::GetImgFrameNoDecode(uint32_t framenum)
{
  if (!mAnim) {
    NS_ASSERTION(framenum == 0, "Don't ask for a frame > 0 if we're not animated!");
    return mFrames.SafeElementAt(0, nullptr);
  }
  if (mAnim->lastCompositedFrameIndex == int32_t(framenum))
    return mAnim->compositingFrame;
  return mFrames.SafeElementAt(framenum, nullptr);
}

imgFrame*
RasterImage::GetImgFrame(uint32_t framenum)
{
  nsresult rv = WantDecodedFrames();
  CONTAINER_ENSURE_TRUE(NS_SUCCEEDED(rv), nullptr);
  return GetImgFrameNoDecode(framenum);
}

imgFrame*
RasterImage::GetDrawableImgFrame(uint32_t framenum)
{
  imgFrame *frame = GetImgFrame(framenum);

  // We will return a paletted frame if it's not marked as compositing failed
  // so we can catch crashes for reasons we haven't investigated.
  if (frame && frame->GetCompositingFailed())
    return nullptr;
  return frame;
}

uint32_t
RasterImage::GetCurrentImgFrameIndex() const
{
  if (mAnim)
    return mAnim->currentAnimationFrameIndex;

  return 0;
}

TimeStamp
RasterImage::GetCurrentImgFrameEndTime() const
{
  imgFrame* currentFrame = mFrames[mAnim->currentAnimationFrameIndex];
  TimeStamp currentFrameTime = mAnim->currentAnimationFrameTime;
  int64_t timeout = currentFrame->GetTimeout();

  if (timeout < 0) {
    // We need to return a sentinel value in this case, because our logic
    // doesn't work correctly if we have a negative timeout value. The reason
    // this positive infinity was chosen was because it works with the loop in
    // RequestRefresh() above.
    return TimeStamp() + TimeDuration::FromMilliseconds(UINT64_MAX);
  }

  TimeDuration durationOfTimeout = TimeDuration::FromMilliseconds(timeout);
  TimeStamp currentFrameEndTime = currentFrameTime + durationOfTimeout;

  return currentFrameEndTime;
}

imgFrame*
RasterImage::GetCurrentImgFrame()
{
  return GetImgFrame(GetCurrentImgFrameIndex());
}

imgFrame*
RasterImage::GetCurrentDrawableImgFrame()
{
  return GetDrawableImgFrame(GetCurrentImgFrameIndex());
}

//******************************************************************************
/* readonly attribute boolean currentFrameIsOpaque; */
NS_IMETHODIMP
RasterImage::GetCurrentFrameIsOpaque(bool *aIsOpaque)
{
  NS_ENSURE_ARG_POINTER(aIsOpaque);

  if (mError)
    return NS_ERROR_FAILURE;

  // See if we can get an image frame
  imgFrame *curframe = GetCurrentImgFrame();

  // If we don't get a frame, the safe answer is "not opaque"
  if (!curframe)
    *aIsOpaque = false;

  // Otherwise, we can make a more intelligent decision
  else {
    *aIsOpaque = !curframe->GetNeedsBackground();

    // We are also transparent if the current frame's size doesn't cover our
    // entire area.
    nsIntRect framerect = curframe->GetRect();
    *aIsOpaque = *aIsOpaque && framerect.IsEqualInterior(nsIntRect(0, 0, mSize.width, mSize.height));
  }

  return NS_OK;
}

void
RasterImage::GetCurrentFrameRect(nsIntRect& aRect)
{
  // Get the current frame
  imgFrame* curframe = GetCurrentImgFrame();

  // If we have the frame, use that rectangle
  if (curframe) {
    aRect = curframe->GetRect();
  } else {
    // If the frame doesn't exist, we pass the empty rectangle. It's not clear
    // whether this is appropriate in general, but at the moment the only
    // consumer of this method is imgStatusTracker (when it wants to figure out
    // dirty rectangles to send out batched observer updates). This should
    // probably be revisited when we fix bug 503973.
    aRect.MoveTo(0, 0);
    aRect.SizeTo(0, 0);
  }
}

uint32_t
RasterImage::GetCurrentFrameIndex()
{
  return GetCurrentImgFrameIndex();
}

uint32_t
RasterImage::GetNumFrames()
{
  return mFrames.Length();
}

//******************************************************************************
/* readonly attribute boolean animated; */
NS_IMETHODIMP
RasterImage::GetAnimated(bool *aAnimated)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aAnimated);

  // If we have mAnim, we can know for sure
  if (mAnim) {
    *aAnimated = true;
    return NS_OK;
  }

  // Otherwise, we need to have been decoded to know for sure, since if we were
  // decoded at least once mAnim would have been created for animated images
  if (!mHasBeenDecoded)
    return NS_ERROR_NOT_AVAILABLE;

  // We know for sure
  *aAnimated = false;

  return NS_OK;
}


//******************************************************************************
/* [noscript] gfxImageSurface copyFrame(in uint32_t aWhichFrame,
 *                                      in uint32_t aFlags); */
NS_IMETHODIMP
RasterImage::CopyFrame(uint32_t aWhichFrame,
                       uint32_t aFlags,
                       gfxImageSurface **_retval)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  // Disallowed in the API
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  nsresult rv;

  if (!ApplyDecodeFlags(aFlags))
    return NS_ERROR_NOT_AVAILABLE;

  // If requested, synchronously flush any data we have lying around to the decoder
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  NS_ENSURE_ARG_POINTER(_retval);

  // Get the frame. If it's not there, it's probably the caller's fault for
  // not waiting for the data to be loaded from the network or not passing
  // FLAG_SYNC_DECODE
  uint32_t frameIndex = (aWhichFrame == FRAME_FIRST) ?
                        0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    *_retval = nullptr;
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<gfxPattern> pattern;
  frame->GetPattern(getter_AddRefs(pattern));
  nsIntRect intframerect = frame->GetRect();
  gfxRect framerect(intframerect.x, intframerect.y, intframerect.width, intframerect.height);

  // Create a 32-bit image surface of our size, but draw using the frame's
  // rect, implicitly padding the frame out to the image's size.
  nsRefPtr<gfxImageSurface> imgsurface = new gfxImageSurface(gfxIntSize(mSize.width, mSize.height),
                                                             gfxASurface::ImageFormatARGB32);
  gfxContext ctx(imgsurface);
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx.Rectangle(framerect);
  ctx.Translate(framerect.TopLeft());
  ctx.SetPattern(pattern);
  ctx.Fill();

  *_retval = imgsurface.forget().get();
  return NS_OK;
}

//******************************************************************************
/* [noscript] gfxASurface getFrame(in uint32_t aWhichFrame,
 *                                 in uint32_t aFlags); */
NS_IMETHODIMP
RasterImage::GetFrame(uint32_t aWhichFrame,
                      uint32_t aFlags,
                      gfxASurface **_retval)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  // Disallowed in the API
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;

  if (!ApplyDecodeFlags(aFlags))
    return NS_ERROR_NOT_AVAILABLE;

  // If the caller requested a synchronous decode, do it
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // Get the frame. If it's not there, it's probably the caller's fault for
  // not waiting for the data to be loaded from the network or not passing
  // FLAG_SYNC_DECODE
  uint32_t frameIndex = (aWhichFrame == FRAME_FIRST) ?
                          0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    *_retval = nullptr;
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<gfxASurface> framesurf;

  // If this frame covers the entire image, we can just reuse its existing
  // surface.
  nsIntRect framerect = frame->GetRect();
  if (framerect.x == 0 && framerect.y == 0 &&
      framerect.width == mSize.width &&
      framerect.height == mSize.height)
    rv = frame->GetSurface(getter_AddRefs(framesurf));

  // The image doesn't have a surface because it's been optimized away. Create
  // one.
  if (!framesurf) {
    nsRefPtr<gfxImageSurface> imgsurf;
    rv = CopyFrame(aWhichFrame, aFlags, getter_AddRefs(imgsurf));
    framesurf = imgsurf;
  }

  *_retval = framesurf.forget().get();

  return rv;
}

already_AddRefed<layers::Image>
RasterImage::GetCurrentImage()
{
  nsRefPtr<gfxASurface> imageSurface;
  nsresult rv = GetFrame(FRAME_CURRENT, FLAG_SYNC_DECODE, getter_AddRefs(imageSurface));
  NS_ENSURE_SUCCESS(rv, nullptr);

  if (!mImageContainer) {
    mImageContainer = LayerManager::CreateImageContainer();
  }

  CairoImage::Data cairoData;
  cairoData.mSurface = imageSurface;
  GetWidth(&cairoData.mSize.width);
  GetHeight(&cairoData.mSize.height);

  ImageFormat cairoFormat = CAIRO_SURFACE;
  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(&cairoFormat, 1);
  NS_ASSERTION(image, "Failed to create Image");
  
  NS_ASSERTION(image->GetFormat() == cairoFormat, "Wrong format");
  static_cast<CairoImage*>(image.get())->SetData(cairoData);

  return image.forget();
}


NS_IMETHODIMP
RasterImage::GetImageContainer(ImageContainer **_retval)
{
  if (mImageContainer) {
    *_retval = mImageContainer;
    NS_ADDREF(*_retval);
    return NS_OK;
  }
  
  nsRefPtr<layers::Image> image = GetCurrentImage();
  if (!image) {
    return NS_ERROR_FAILURE;
  }
  mImageContainer->SetCurrentImageInTransaction(image);

  *_retval = mImageContainer;
  NS_ADDREF(*_retval);
  return NS_OK;
}

void
RasterImage::UpdateImageContainer()
{
  if (!mImageContainer || IsInUpdateImageContainer()) {
    return;
  }

  SetInUpdateImageContainer(true);

  nsRefPtr<layers::Image> image = GetCurrentImage();
  if (!image) {
    return;
  }
  mImageContainer->SetCurrentImage(image);
  SetInUpdateImageContainer(false);
}

size_t
RasterImage::HeapSizeOfSourceWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const
{
  // n == 0 is possible for two reasons. 
  // - This is a zero-length image.
  // - We're on a platform where moz_malloc_size_of always returns 0.
  // In either case the fallback works appropriately.
  size_t n = mSourceData.SizeOfExcludingThis(aMallocSizeOf);
  if (n == 0) {
    n = mSourceData.Length();
    NS_ABORT_IF_FALSE(StoringSourceData() || (n == 0),
                      "Non-zero source data size when we aren't storing it?");
  }
  return n;
}

static size_t
SizeOfDecodedWithComputedFallbackIfHeap(
  const nsTArray<imgFrame*>& aFrames, gfxASurface::MemoryLocation aLocation,
  nsMallocSizeOfFun aMallocSizeOf)
{
  size_t n = 0;
  for (uint32_t i = 0; i < aFrames.Length(); ++i) {
    imgFrame* frame = aFrames.SafeElementAt(i, nullptr);
    NS_ABORT_IF_FALSE(frame, "Null frame in frame array!");
    n += frame->SizeOfExcludingThisWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);
  }

  return n;
}

size_t
RasterImage::HeapSizeOfDecodedWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const
{
  return SizeOfDecodedWithComputedFallbackIfHeap(
           mFrames, gfxASurface::MEMORY_IN_PROCESS_HEAP, aMallocSizeOf);
}

size_t
RasterImage::NonHeapSizeOfDecoded() const
{
  return SizeOfDecodedWithComputedFallbackIfHeap(mFrames, gfxASurface::MEMORY_IN_PROCESS_NONHEAP, NULL);
}

size_t
RasterImage::OutOfProcessSizeOfDecoded() const
{
  return SizeOfDecodedWithComputedFallbackIfHeap(mFrames, gfxASurface::MEMORY_OUT_OF_PROCESS, NULL);
}

void
RasterImage::DeleteImgFrame(uint32_t framenum)
{
  NS_ABORT_IF_FALSE(framenum < mFrames.Length(), "Deleting invalid frame!");

  delete mFrames[framenum];
  mFrames[framenum] = nullptr;
}

nsresult
RasterImage::InternalAddFrameHelper(uint32_t framenum, imgFrame *aFrame,
                                    uint8_t **imageData, uint32_t *imageLength,
                                    uint32_t **paletteData, uint32_t *paletteLength)
{
  NS_ABORT_IF_FALSE(framenum <= mFrames.Length(), "Invalid frame index!");
  if (framenum > mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  nsAutoPtr<imgFrame> frame(aFrame);

  // We are in the middle of decoding. This will be unlocked when we finish the
  // decoder->Write() call.
  frame->LockImageData();

  if (paletteData && paletteLength)
    frame->GetPaletteData(paletteData, paletteLength);

  frame->GetImageData(imageData, imageLength);

  mFrames.InsertElementAt(framenum, frame.forget());

  return NS_OK;
}
                                  
nsresult
RasterImage::InternalAddFrame(uint32_t framenum,
                              int32_t aX, int32_t aY,
                              int32_t aWidth, int32_t aHeight,
                              gfxASurface::gfxImageFormat aFormat,
                              uint8_t aPaletteDepth,
                              uint8_t **imageData,
                              uint32_t *imageLength,
                              uint32_t **paletteData,
                              uint32_t *paletteLength)
{
  // We assume that we're in the middle of decoding because we unlock the
  // previous frame when we create a new frame, and only when decoding do we
  // lock frames.
  NS_ABORT_IF_FALSE(mInDecoder, "Only decoders may add frames!");

  NS_ABORT_IF_FALSE(framenum <= mFrames.Length(), "Invalid frame index!");
  if (framenum > mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  nsAutoPtr<imgFrame> frame(new imgFrame());

  nsresult rv = frame->Init(aX, aY, aWidth, aHeight, aFormat, aPaletteDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  // We know we are in a decoder. Therefore, we must unlock the previous frame
  // when we move on to decoding into the next frame.
  if (mFrames.Length() > 0) {
    imgFrame *prevframe = mFrames.ElementAt(mFrames.Length() - 1);
    prevframe->UnlockImageData();
  }

  if (mFrames.Length() == 0) {
    return InternalAddFrameHelper(framenum, frame.forget(), imageData, imageLength, 
                                  paletteData, paletteLength);
  }

  if (mFrames.Length() == 1) {
    // Since we're about to add our second frame, initialize animation stuff
    EnsureAnimExists();
    
    // If we dispose of the first frame by clearing it, then the
    // First Frame's refresh area is all of itself.
    // RESTORE_PREVIOUS is invalid (assumed to be DISPOSE_CLEAR)
    int32_t frameDisposalMethod = mFrames[0]->GetFrameDisposalMethod();
    if (frameDisposalMethod == kDisposeClear ||
        frameDisposalMethod == kDisposeRestorePrevious)
      mAnim->firstFrameRefreshArea = mFrames[0]->GetRect();
  }

  // Calculate firstFrameRefreshArea
  // Some gifs are huge but only have a small area that they animate
  // We only need to refresh that small area when Frame 0 comes around again
  nsIntRect frameRect = frame->GetRect();
  mAnim->firstFrameRefreshArea.UnionRect(mAnim->firstFrameRefreshArea, 
                                         frameRect);
  
  rv = InternalAddFrameHelper(framenum, frame.forget(), imageData, imageLength,
                              paletteData, paletteLength);
  
  // We may be able to start animating, if we now have enough frames
  EvaluateAnimation();
  
  return rv;
}

bool
RasterImage::ApplyDecodeFlags(uint32_t aNewFlags)
{
  if (mFrameDecodeFlags == (aNewFlags & DECODE_FLAGS_MASK))
    return true; // Not asking very much of us here.

  if (mDecoded) {
    // if we can't discard, then we're screwed; we have no way
    // to re-decode.  Similarly if we aren't allowed to do a sync
    // decode.
    if (!(aNewFlags & FLAG_SYNC_DECODE))
      return false;
    if (!CanForciblyDiscard() || mDecoder || mAnim)
      return false;
    ForceDiscard();
  }

  mFrameDecodeFlags = aNewFlags & DECODE_FLAGS_MASK;
  return true;
}

nsresult
RasterImage::SetSize(int32_t aWidth, int32_t aHeight)
{
  if (mError)
    return NS_ERROR_FAILURE;

  // Ensure that we have positive values
  // XXX - Why isn't the size unsigned? Should this be changed?
  if ((aWidth < 0) || (aHeight < 0))
    return NS_ERROR_INVALID_ARG;

  // if we already have a size, check the new size against the old one
  if (!mMultipart && mHasSize &&
      ((aWidth != mSize.width) || (aHeight != mSize.height))) {
    NS_WARNING("Image changed size on redecode! This should not happen!");

    // Make the decoder aware of the error so that it doesn't try to call
    // FinishInternal during ShutdownDecoder.
    if (mDecoder)
      mDecoder->PostResizeError();

    DoError();
    return NS_ERROR_UNEXPECTED;
  }

  // Set the size and flag that we have it
  mSize.SizeTo(aWidth, aHeight);
  mHasSize = true;

  return NS_OK;
}

nsresult
RasterImage::EnsureFrame(uint32_t aFrameNum, int32_t aX, int32_t aY,
                         int32_t aWidth, int32_t aHeight,
                         gfxASurface::gfxImageFormat aFormat,
                         uint8_t aPaletteDepth,
                         uint8_t **imageData, uint32_t *imageLength,
                         uint32_t **paletteData, uint32_t *paletteLength)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(imageData);
  NS_ENSURE_ARG_POINTER(imageLength);
  NS_ABORT_IF_FALSE(aFrameNum <= mFrames.Length(), "Invalid frame index!");

  if (aPaletteDepth > 0) {
    NS_ENSURE_ARG_POINTER(paletteData);
    NS_ENSURE_ARG_POINTER(paletteLength);
  }

  if (aFrameNum > mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  // Adding a frame that doesn't already exist.
  if (aFrameNum == mFrames.Length())
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                            aPaletteDepth, imageData, imageLength,
                            paletteData, paletteLength);

  imgFrame *frame = GetImgFrame(aFrameNum);
  if (!frame)
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                            aPaletteDepth, imageData, imageLength,
                            paletteData, paletteLength);

  // See if we can re-use the frame that already exists.
  nsIntRect rect = frame->GetRect();
  if (rect.x == aX && rect.y == aY && rect.width == aWidth &&
      rect.height == aHeight && frame->GetFormat() == aFormat &&
      frame->GetPaletteDepth() == aPaletteDepth) {
    frame->GetImageData(imageData, imageLength);
    if (paletteData) {
      frame->GetPaletteData(paletteData, paletteLength);
    }

    // We can re-use the frame if it has image data.
    if (*imageData && paletteData && *paletteData) {
      return NS_OK;
    }
    if (*imageData && !paletteData) {
      return NS_OK;
    }
  }

  // Not reusable, so replace the frame directly.

  // We know this frame is already locked, because it's the one we're currently
  // writing to.
  frame->UnlockImageData();

  DeleteImgFrame(aFrameNum);
  mFrames.RemoveElementAt(aFrameNum);
  nsAutoPtr<imgFrame> newFrame(new imgFrame());
  nsresult rv = newFrame->Init(aX, aY, aWidth, aHeight, aFormat, aPaletteDepth);
  NS_ENSURE_SUCCESS(rv, rv);
  return InternalAddFrameHelper(aFrameNum, newFrame.forget(), imageData,
                                imageLength, paletteData, paletteLength);
}

nsresult
RasterImage::EnsureFrame(uint32_t aFramenum, int32_t aX, int32_t aY,
                         int32_t aWidth, int32_t aHeight,
                         gfxASurface::gfxImageFormat aFormat,
                         uint8_t** imageData, uint32_t* imageLength)
{
  return EnsureFrame(aFramenum, aX, aY, aWidth, aHeight, aFormat,
                     /* aPaletteDepth = */ 0, imageData, imageLength,
                     /* aPaletteData = */ nullptr,
                     /* aPaletteLength = */ nullptr);
}

void
RasterImage::FrameUpdated(uint32_t aFrameNum, nsIntRect &aUpdatedRect)
{
  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");

  imgFrame *frame = GetImgFrameNoDecode(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling FrameUpdated on frame that doesn't exist!");

  frame->ImageUpdated(aUpdatedRect);
    
  if (aFrameNum == GetCurrentImgFrameIndex() &&
      !IsInUpdateImageContainer()) {
    mImageContainer = nullptr;
  }
}

nsresult
RasterImage::SetFrameDisposalMethod(uint32_t aFrameNum,
                                    int32_t aDisposalMethod)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame,
                    "Calling SetFrameDisposalMethod on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetFrameDisposalMethod(aDisposalMethod);

  return NS_OK;
}

nsresult
RasterImage::SetFrameTimeout(uint32_t aFrameNum, int32_t aTimeout)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameTimeout on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetTimeout(aTimeout);

  return NS_OK;
}

nsresult
RasterImage::SetFrameBlendMethod(uint32_t aFrameNum, int32_t aBlendMethod)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameBlendMethod on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetBlendMethod(aBlendMethod);

  return NS_OK;
}

nsresult
RasterImage::SetFrameHasNoAlpha(uint32_t aFrameNum)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameHasNoAlpha on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetHasNoAlpha();

  return NS_OK;
}

nsresult
RasterImage::SetFrameAsNonPremult(uint32_t aFrameNum, bool aIsNonPremult)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame* frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameAsNonPremult on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetAsNonPremult(aIsNonPremult);

  return NS_OK;
}

nsresult
RasterImage::DecodingComplete()
{
  if (mError)
    return NS_ERROR_FAILURE;

  // Flag that we're done decoding.
  // XXX - these should probably be combined when we fix animated image
  // discarding with bug 500402.
  mDecoded = true;
  mHasBeenDecoded = true;

  nsresult rv;

  // We now have one of the qualifications for discarding. Re-evaluate.
  if (CanDiscard()) {
    NS_ABORT_IF_FALSE(!DiscardingActive(),
                      "We shouldn't have been discardable before this");
    rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // If there's only 1 frame, optimize it. Optimizing animated images
  // is not supported.
  //
  // We don't optimize the frame for multipart images because we reuse
  // the frame.
  if ((mFrames.Length() == 1) && !mMultipart) {
    rv = mFrames[0]->Optimize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//******************************************************************************
/* void StartAnimation () */
nsresult
RasterImage::StartAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(ShouldAnimate(), "Should not animate!");

  EnsureAnimExists();

  imgFrame* currentFrame = GetCurrentImgFrame();
  if (currentFrame) {
    if (currentFrame->GetTimeout() < 0) { // -1 means display this frame forever
      mAnimationFinished = true;
      return NS_ERROR_ABORT;
    }

    // We need to set the time that this initial frame was first displayed, as
    // this is used in AdvanceFrame().
    mAnim->currentAnimationFrameTime = TimeStamp::Now();
  }
  
  return NS_OK;
}

//******************************************************************************
/* void stopAnimation (); */
nsresult
RasterImage::StopAnimation()
{
  NS_ABORT_IF_FALSE(mAnimating, "Should be animating!");

  if (mError)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

//******************************************************************************
/* void resetAnimation (); */
NS_IMETHODIMP
RasterImage::ResetAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  if (mAnimationMode == kDontAnimMode || 
      !mAnim || mAnim->currentAnimationFrameIndex == 0)
    return NS_OK;

  mAnimationFinished = false;

  if (mAnimating)
    StopAnimation();

  mAnim->lastCompositedFrameIndex = -1;
  mAnim->currentAnimationFrameIndex = 0;
  UpdateImageContainer();

  // Note - We probably want to kick off a redecode somewhere around here when
  // we fix bug 500402.

  // Update display if we were animating before
  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (mAnimating && observer)
    observer->FrameChanged(nullptr, this, &(mAnim->firstFrameRefreshArea));

  if (ShouldAnimate()) {
    StartAnimation();
    // The animation may not have been running before, if mAnimationFinished
    // was false (before we changed it to true in this function). So, mark the
    // animation as running.
    mAnimating = true;
  }

  return NS_OK;
}

void
RasterImage::SetLoopCount(int32_t aLoopCount)
{
  if (mError)
    return;

  // -1  infinite
  //  0  no looping, one iteration
  //  1  one loop, two iterations
  //  ...
  mLoopCount = aLoopCount;
}

nsresult
RasterImage::AddSourceData(const char *aBuffer, uint32_t aCount)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aBuffer);
  nsresult rv = NS_OK;

  // We should not call this if we're not initialized
  NS_ABORT_IF_FALSE(mInitialized, "Calling AddSourceData() on uninitialized "
                                  "RasterImage!");

  // We should not call this if we're already finished adding source data
  NS_ABORT_IF_FALSE(!mHasSourceData, "Calling AddSourceData() after calling "
                                     "sourceDataComplete()!");

  // This call should come straight from necko - no reentrancy allowed
  NS_ABORT_IF_FALSE(!mInDecoder, "Re-entrant call to AddSourceData!");

  // Image is already decoded, we shouldn't be getting data, but it could
  // be extra garbage data at the end of a file.
  if (mDecoded) {
    return NS_OK;
  }

  // Starting a new part's frames, let's clean up before we add any
  // This needs to happen just before we start getting EnsureFrame() call(s),
  // so that there's no gap for anything to miss us.
  if (mMultipart && mBytesDecoded == 0) {
    // Our previous state may have been animated, so let's clean up
    if (mAnimating) {
      StopAnimation();
      mAnimating = false;
    }
    mAnimationFinished = false;
    if (mAnim) {
      delete mAnim;
      mAnim = nullptr;
    }
    // If there's only one frame, this could cause flickering
    int old_frame_count = mFrames.Length();
    if (old_frame_count > 1) {
      for (int i = 0; i < old_frame_count; ++i) {
        DeleteImgFrame(i);
      }
      mFrames.Clear();
    }
  }

  // If we're not storing source data, write it directly to the decoder
  if (!StoringSourceData()) {
    rv = WriteToDecoder(aBuffer, aCount);
    CONTAINER_ENSURE_SUCCESS(rv);

    // We're not storing source data, so this data is probably coming straight
    // from the network. In this case, we want to display data as soon as we
    // get it, so we want to flush invalidations after every write.
    nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
    mInDecoder = true;
    mDecoder->FlushInvalidations();
    mInDecoder = false;
  }

  // Otherwise, we're storing data in the source buffer
  else {

    // Store the data
    char *newElem = mSourceData.AppendElements(aBuffer, aCount);
    if (!newElem)
      return NS_ERROR_OUT_OF_MEMORY;

    // If there's a decoder open, that means we want to do more decoding.
    // Wake up the worker.
    if (mDecoder) {
      DecodeWorker::Singleton()->RequestDecode(this);
    }
  }

  // Statistics
  total_source_bytes += aCount;
  if (mDiscardable)
    discardable_source_bytes += aCount;
  PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
          ("CompressedImageAccounting: Added compressed data to RasterImage %p (%s). "
           "Total Containers: %d, Discardable containers: %d, "
           "Total source bytes: %lld, Source bytes for discardable containers %lld",
           this,
           mSourceDataMimeType.get(),
           num_containers,
           num_discardable_containers,
           total_source_bytes,
           discardable_source_bytes));

  return NS_OK;
}

/* Note!  buf must be declared as char buf[9]; */
// just used for logging and hashing the header
static void
get_header_str (char *buf, char *data, size_t data_len)
{
  int i;
  int n;
  static char hex[] = "0123456789abcdef";

  n = data_len < 4 ? data_len : 4;

  for (i = 0; i < n; i++) {
    buf[i * 2]     = hex[(data[i] >> 4) & 0x0f];
    buf[i * 2 + 1] = hex[data[i] & 0x0f];
  }

  buf[i * 2] = 0;
}

nsresult
RasterImage::SourceDataComplete()
{
  if (mError)
    return NS_ERROR_FAILURE;

  // If we've been called before, ignore. Otherwise, flag that we have everything
  if (mHasSourceData)
    return NS_OK;
  mHasSourceData = true;

  // This call should come straight from necko - no reentrancy allowed
  NS_ABORT_IF_FALSE(!mInDecoder, "Re-entrant call to AddSourceData!");

  // If we're not storing any source data, then all the data was written
  // directly to the decoder in the AddSourceData() calls. This means we're
  // done, so we can shut down the decoder.
  if (!StoringSourceData()) {
    nsresult rv = ShutdownDecoder(eShutdownIntent_Done);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // If there's a decoder open, synchronously decode the beginning of the image
  // to check for errors and get the image's size.  (If we already have the
  // image's size, this does nothing.)  Then kick off an async decode of the
  // rest of the image.
  if (mDecoder) {
    nsresult rv = DecodeWorker::Singleton()->DecodeUntilSizeAvailable(this);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // If DecodeUntilSizeAvailable didn't finish the decode, let the decode worker
  // finish decoding this image.
  if (mDecoder) {
    DecodeWorker::Singleton()->RequestDecode(this);
  }

  // Free up any extra space in the backing buffer
  mSourceData.Compact();

  // Log header information
  if (PR_LOG_TEST(gCompressedImageAccountingLog, PR_LOG_DEBUG)) {
    char buf[9];
    get_header_str(buf, mSourceData.Elements(), mSourceData.Length());
    PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
            ("CompressedImageAccounting: RasterImage::SourceDataComplete() - data "
             "is done for container %p (%s) - header %p is 0x%s (length %d)",
             this,
             mSourceDataMimeType.get(),
             mSourceData.Elements(),
             buf,
             mSourceData.Length()));
  }

  // We now have one of the qualifications for discarding. Re-evaluate.
  if (CanDiscard()) {
    nsresult rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }
  return NS_OK;
}

nsresult
RasterImage::NewSourceData()
{
  nsresult rv;

  if (mError)
    return NS_ERROR_FAILURE;

  // The source data should be complete before calling this
  NS_ABORT_IF_FALSE(mHasSourceData,
                    "Calling NewSourceData before SourceDataComplete!");
  if (!mHasSourceData)
    return NS_ERROR_ILLEGAL_VALUE;

  // Only supported for multipart channels. It wouldn't be too hard to change this,
  // but it would involve making sure that things worked for decode-on-draw and
  // discarding. Presently there's no need for this, so we don't.
  NS_ABORT_IF_FALSE(mMultipart, "NewSourceData not supported for multipart");
  if (!mMultipart)
    return NS_ERROR_ILLEGAL_VALUE;

  // We're multipart, so we shouldn't be storing source data
  NS_ABORT_IF_FALSE(!StoringSourceData(),
                    "Shouldn't be storing source data for multipart");

  // We're not storing the source data and we got SourceDataComplete. We should
  // have shut down the previous decoder
  NS_ABORT_IF_FALSE(!mDecoder, "Shouldn't have a decoder in NewSourceData");

  // The decoder was shut down and we didn't flag an error, so we should be decoded
  NS_ABORT_IF_FALSE(mDecoded, "Should be decoded in NewSourceData");

  // Reset some flags
  mDecoded = false;
  mHasSourceData = false;

  // We're decode-on-load here. Open up a new decoder just like what happens when
  // we call Init() for decode-on-load images.
  rv = InitDecoder(/* aDoSizeDecode = */ false);
  CONTAINER_ENSURE_SUCCESS(rv);

  return NS_OK;
}

nsresult
RasterImage::SetSourceSizeHint(uint32_t sizeHint)
{
  if (sizeHint && StoringSourceData())
    return mSourceData.SetCapacity(sizeHint) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

//******************************************************************************
// DoComposite gets called when the timer for animation get fired and we have to
// update the composited frame of the animation.
nsresult
RasterImage::DoComposite(nsIntRect* aDirtyRect,
                         imgFrame* aPrevFrame,
                         imgFrame* aNextFrame,
                         int32_t aNextFrameIndex)
{
  NS_ENSURE_ARG_POINTER(aDirtyRect);
  NS_ENSURE_ARG_POINTER(aPrevFrame);
  NS_ENSURE_ARG_POINTER(aNextFrame);

  int32_t prevFrameDisposalMethod = aPrevFrame->GetFrameDisposalMethod();
  if (prevFrameDisposalMethod == kDisposeRestorePrevious &&
      !mAnim->compositingPrevFrame)
    prevFrameDisposalMethod = kDisposeClear;

  nsIntRect prevFrameRect = aPrevFrame->GetRect();
  bool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                          prevFrameRect.width == mSize.width &&
                          prevFrameRect.height == mSize.height);

  // Optimization: DisposeClearAll if the previous frame is the same size as
  //               container and it's clearing itself
  if (isFullPrevFrame && 
      (prevFrameDisposalMethod == kDisposeClear))
    prevFrameDisposalMethod = kDisposeClearAll;

  int32_t nextFrameDisposalMethod = aNextFrame->GetFrameDisposalMethod();
  nsIntRect nextFrameRect = aNextFrame->GetRect();
  bool isFullNextFrame = (nextFrameRect.x == 0 && nextFrameRect.y == 0 &&
                          nextFrameRect.width == mSize.width &&
                          nextFrameRect.height == mSize.height);

  if (!aNextFrame->GetIsPaletted()) {
    // Optimization: Skip compositing if the previous frame wants to clear the
    //               whole image
    if (prevFrameDisposalMethod == kDisposeClearAll) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      return NS_OK;
    }
  
    // Optimization: Skip compositing if this frame is the same size as the
    //               container and it's fully drawing over prev frame (no alpha)
    if (isFullNextFrame &&
        (nextFrameDisposalMethod != kDisposeRestorePrevious) &&
        !aNextFrame->GetHasAlpha()) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      return NS_OK;
    }
  }

  // Calculate area that needs updating
  switch (prevFrameDisposalMethod) {
    default:
    case kDisposeNotSpecified:
    case kDisposeKeep:
      *aDirtyRect = nextFrameRect;
      break;

    case kDisposeClearAll:
      // Whole image container is cleared
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;

    case kDisposeClear:
      // Calc area that needs to be redrawn (the combination of previous and
      // this frame)
      // XXX - This could be done with multiple framechanged calls
      //       Having prevFrame way at the top of the image, and nextFrame
      //       way at the bottom, and both frames being small, we'd be
      //       telling framechanged to refresh the whole image when only two
      //       small areas are needed.
      aDirtyRect->UnionRect(nextFrameRect, prevFrameRect);
      break;

    case kDisposeRestorePrevious:
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;
  }

  // Optimization:
  //   Skip compositing if the last composited frame is this frame
  //   (Only one composited frame was made for this animation.  Example:
  //    Only Frame 3 of a 10 frame image required us to build a composite frame
  //    On the second loop, we do not need to rebuild the frame
  //    since it's still sitting in compositingFrame)
  if (mAnim->lastCompositedFrameIndex == aNextFrameIndex) {
    return NS_OK;
  }

  bool needToBlankComposite = false;

  // Create the Compositing Frame
  if (!mAnim->compositingFrame) {
    mAnim->compositingFrame = new imgFrame();
    nsresult rv = mAnim->compositingFrame->Init(0, 0, mSize.width, mSize.height,
                                                gfxASurface::ImageFormatARGB32);
    if (NS_FAILED(rv)) {
      mAnim->compositingFrame = nullptr;
      return rv;
    }
    needToBlankComposite = true;
  } else if (aNextFrameIndex != mAnim->lastCompositedFrameIndex+1) {

    // If we are not drawing on top of last composited frame, 
    // then we are building a new composite frame, so let's clear it first.
    needToBlankComposite = true;
  }

  // More optimizations possible when next frame is not transparent
  // But if the next frame has kDisposeRestorePrevious,
  // this "no disposal" optimization is not possible, 
  // because the frame in "after disposal operation" state 
  // needs to be stored in compositingFrame, so it can be 
  // copied into compositingPrevFrame later.
  bool doDisposal = true;
  if (!aNextFrame->GetHasAlpha() &&
      nextFrameDisposalMethod != kDisposeRestorePrevious) {
    if (isFullNextFrame) {
      // Optimization: No need to dispose prev.frame when 
      // next frame is full frame and not transparent.
      doDisposal = false;
      // No need to blank the composite frame
      needToBlankComposite = false;
    } else {
      if ((prevFrameRect.x >= nextFrameRect.x) &&
          (prevFrameRect.y >= nextFrameRect.y) &&
          (prevFrameRect.x + prevFrameRect.width <= nextFrameRect.x + nextFrameRect.width) &&
          (prevFrameRect.y + prevFrameRect.height <= nextFrameRect.y + nextFrameRect.height)) {
        // Optimization: No need to dispose prev.frame when 
        // next frame fully overlaps previous frame.
        doDisposal = false;
      }
    }      
  }

  if (doDisposal) {
    // Dispose of previous: clear, restore, or keep (copy)
    switch (prevFrameDisposalMethod) {
      case kDisposeClear:
        if (needToBlankComposite) {
          // If we just created the composite, it could have anything in it's
          // buffer. Clear whole frame
          ClearFrame(mAnim->compositingFrame);
        } else {
          // Only blank out previous frame area (both color & Mask/Alpha)
          ClearFrame(mAnim->compositingFrame, prevFrameRect);
        }
        break;
  
      case kDisposeClearAll:
        ClearFrame(mAnim->compositingFrame);
        break;
  
      case kDisposeRestorePrevious:
        // It would be better to copy only the area changed back to
        // compositingFrame.
        if (mAnim->compositingPrevFrame) {
          CopyFrameImage(mAnim->compositingPrevFrame, mAnim->compositingFrame);
  
          // destroy only if we don't need it for this frame's disposal
          if (nextFrameDisposalMethod != kDisposeRestorePrevious)
            mAnim->compositingPrevFrame = nullptr;
        } else {
          ClearFrame(mAnim->compositingFrame);
        }
        break;
      
      default:
        // Copy previous frame into compositingFrame before we put the new frame on top
        // Assumes that the previous frame represents a full frame (it could be
        // smaller in size than the container, as long as the frame before it erased
        // itself)
        // Note: Frame 1 never gets into DoComposite(), so (aNextFrameIndex - 1) will
        // always be a valid frame number.
        if (mAnim->lastCompositedFrameIndex != aNextFrameIndex - 1) {
          if (isFullPrevFrame && !aPrevFrame->GetIsPaletted()) {
            // Just copy the bits
            CopyFrameImage(aPrevFrame, mAnim->compositingFrame);
          } else {
            if (needToBlankComposite) {
              // Only blank composite when prev is transparent or not full.
              if (aPrevFrame->GetHasAlpha() || !isFullPrevFrame) {
                ClearFrame(mAnim->compositingFrame);
              }
            }
            DrawFrameTo(aPrevFrame, mAnim->compositingFrame, prevFrameRect);
          }
        }
    }
  } else if (needToBlankComposite) {
    // If we just created the composite, it could have anything in it's
    // buffers. Clear them
    ClearFrame(mAnim->compositingFrame);
  }

  // Check if the frame we are composing wants the previous image restored afer
  // it is done. Don't store it (again) if last frame wanted its image restored
  // too
  if ((nextFrameDisposalMethod == kDisposeRestorePrevious) &&
      (prevFrameDisposalMethod != kDisposeRestorePrevious)) {
    // We are storing the whole image.
    // It would be better if we just stored the area that nextFrame is going to
    // overwrite.
    if (!mAnim->compositingPrevFrame) {
      mAnim->compositingPrevFrame = new imgFrame();
      nsresult rv = mAnim->compositingPrevFrame->Init(0, 0, mSize.width, mSize.height,
                                                      gfxASurface::ImageFormatARGB32);
      if (NS_FAILED(rv)) {
        mAnim->compositingPrevFrame = nullptr;
        return rv;
      }
    }

    CopyFrameImage(mAnim->compositingFrame, mAnim->compositingPrevFrame);
  }

  // blit next frame into it's correct spot
  DrawFrameTo(aNextFrame, mAnim->compositingFrame, nextFrameRect);

  // Set timeout of CompositeFrame to timeout of frame we just composed
  // Bug 177948
  int32_t timeout = aNextFrame->GetTimeout();
  mAnim->compositingFrame->SetTimeout(timeout);

  // Tell the image that it is fully 'downloaded'.
  nsresult rv = mAnim->compositingFrame->ImageUpdated(mAnim->compositingFrame->GetRect());
  if (NS_FAILED(rv)) {
    return rv;
  }

  // We don't want to keep composite images for 8bit frames.
  // Also this optimization won't work if the next frame has 
  // kDisposeRestorePrevious, because it would need to be restored 
  // into "after prev disposal but before next blend" state, 
  // not into empty frame.
  if (isFullNextFrame && mAnimationMode == kNormalAnimMode && mLoopCount != 0 &&
      nextFrameDisposalMethod != kDisposeRestorePrevious &&
      !aNextFrame->GetIsPaletted()) {
    // We have a composited full frame
    // Store the composited frame into the mFrames[..] so we don't have to
    // continuously re-build it
    // Then set the previous frame's disposal to CLEAR_ALL so we just draw the
    // frame next time around
    if (CopyFrameImage(mAnim->compositingFrame, aNextFrame)) {
      aPrevFrame->SetFrameDisposalMethod(kDisposeClearAll);
      mAnim->lastCompositedFrameIndex = -1;
      return NS_OK;
    }
  }

  mAnim->lastCompositedFrameIndex = aNextFrameIndex;

  return NS_OK;
}

//******************************************************************************
// Fill aFrame with black. Does also clears the mask.
void
RasterImage::ClearFrame(imgFrame *aFrame)
{
  if (!aFrame)
    return;

  nsresult rv = aFrame->LockImageData();
  if (NS_FAILED(rv))
    return;

  nsRefPtr<gfxASurface> surf;
  aFrame->GetSurface(getter_AddRefs(surf));

  // Erase the surface to transparent
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Paint();

  aFrame->UnlockImageData();
}

//******************************************************************************
void
RasterImage::ClearFrame(imgFrame *aFrame, nsIntRect &aRect)
{
  if (!aFrame || aRect.width <= 0 || aRect.height <= 0)
    return;

  nsresult rv = aFrame->LockImageData();
  if (NS_FAILED(rv))
    return;

  nsRefPtr<gfxASurface> surf;
  aFrame->GetSurface(getter_AddRefs(surf));

  // Erase the destination rectangle to transparent
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Rectangle(gfxRect(aRect.x, aRect.y, aRect.width, aRect.height));
  ctx.Fill();

  aFrame->UnlockImageData();
}


//******************************************************************************
// Whether we succeed or fail will not cause a crash, and there's not much
// we can do about a failure, so there we don't return a nsresult
bool
RasterImage::CopyFrameImage(imgFrame *aSrcFrame,
                            imgFrame *aDstFrame)
{
  uint8_t* aDataSrc;
  uint8_t* aDataDest;
  uint32_t aDataLengthSrc;
  uint32_t aDataLengthDest;

  if (!aSrcFrame || !aDstFrame)
    return false;

  AutoFrameLocker dstLock(aDstFrame);
  AutoFrameLocker srcLock(aSrcFrame);

  if (!srcLock.Succeeded() || !dstLock.Succeeded()) {
    return false;
  }

  // Copy Image Over
  aSrcFrame->GetImageData(&aDataSrc, &aDataLengthSrc);
  aDstFrame->GetImageData(&aDataDest, &aDataLengthDest);
  if (!aDataDest || !aDataSrc || aDataLengthDest != aDataLengthSrc) {
    return false;
  }

  memcpy(aDataDest, aDataSrc, aDataLengthSrc);

  return true;
}

//******************************************************************************
/* 
 * aSrc is the current frame being drawn,
 * aDst is the composition frame where the current frame is drawn into.
 * aSrcRect is the size of the current frame, and the position of that frame
 *          in the composition frame.
 */
nsresult
RasterImage::DrawFrameTo(imgFrame *aSrc,
                         imgFrame *aDst,
                         nsIntRect& aSrcRect)
{
  NS_ENSURE_ARG_POINTER(aSrc);
  NS_ENSURE_ARG_POINTER(aDst);

  AutoFrameLocker srcLock(aSrc);
  AutoFrameLocker dstLock(aDst);

  nsIntRect dstRect = aDst->GetRect();

  // According to both AGIF and APNG specs, offsets are unsigned
  if (aSrcRect.x < 0 || aSrcRect.y < 0) {
    NS_WARNING("RasterImage::DrawFrameTo: negative offsets not allowed");
    return NS_ERROR_FAILURE;
  }
  // Outside the destination frame, skip it
  if ((aSrcRect.x > dstRect.width) || (aSrcRect.y > dstRect.height)) {
    return NS_OK;
  }

  if (aSrc->GetIsPaletted()) {
    // Larger than the destination frame, clip it
    int32_t width = NS_MIN(aSrcRect.width, dstRect.width - aSrcRect.x);
    int32_t height = NS_MIN(aSrcRect.height, dstRect.height - aSrcRect.y);

    // The clipped image must now fully fit within destination image frame
    NS_ASSERTION((aSrcRect.x >= 0) && (aSrcRect.y >= 0) &&
                 (aSrcRect.x + width <= dstRect.width) &&
                 (aSrcRect.y + height <= dstRect.height),
                "RasterImage::DrawFrameTo: Invalid aSrcRect");

    // clipped image size may be smaller than source, but not larger
    NS_ASSERTION((width <= aSrcRect.width) && (height <= aSrcRect.height),
                 "RasterImage::DrawFrameTo: source must be smaller than dest");

    if (!srcLock.Succeeded() || !dstLock.Succeeded())
      return NS_ERROR_FAILURE;

    // Get pointers to image data
    uint32_t size;
    uint8_t *srcPixels;
    uint32_t *colormap;
    uint32_t *dstPixels;

    aSrc->GetImageData(&srcPixels, &size);
    aSrc->GetPaletteData(&colormap, &size);
    aDst->GetImageData((uint8_t **)&dstPixels, &size);
    if (!srcPixels || !dstPixels || !colormap) {
      return NS_ERROR_FAILURE;
    }

    // Skip to the right offset
    dstPixels += aSrcRect.x + (aSrcRect.y * dstRect.width);
    if (!aSrc->GetHasAlpha()) {
      for (int32_t r = height; r > 0; --r) {
        for (int32_t c = 0; c < width; c++) {
          dstPixels[c] = colormap[srcPixels[c]];
        }
        // Go to the next row in the source resp. destination image
        srcPixels += aSrcRect.width;
        dstPixels += dstRect.width;
      }
    } else {
      for (int32_t r = height; r > 0; --r) {
        for (int32_t c = 0; c < width; c++) {
          const uint32_t color = colormap[srcPixels[c]];
          if (color)
            dstPixels[c] = color;
        }
        // Go to the next row in the source resp. destination image
        srcPixels += aSrcRect.width;
        dstPixels += dstRect.width;
      }
    }

    return NS_OK;
  }

  nsRefPtr<gfxPattern> srcPatt;
  aSrc->GetPattern(getter_AddRefs(srcPatt));

  nsRefPtr<gfxASurface> dstSurf;
  aDst->GetSurface(getter_AddRefs(dstSurf));

  gfxContext dst(dstSurf);
  dst.Translate(gfxPoint(aSrcRect.x, aSrcRect.y));
  dst.Rectangle(gfxRect(0, 0, aSrcRect.width, aSrcRect.height), true);
  
  // first clear the surface if the blend flag says so
  int32_t blendMethod = aSrc->GetBlendMethod();
  if (blendMethod == kBlendSource) {
    gfxContext::GraphicsOperator defaultOperator = dst.CurrentOperator();
    dst.SetOperator(gfxContext::OPERATOR_CLEAR);
    dst.Fill();
    dst.SetOperator(defaultOperator);
  }
  dst.SetPattern(srcPatt);
  dst.Paint();

  return NS_OK;
}


/********* Methods to implement lazy allocation of nsIProperties object *************/
NS_IMETHODIMP
RasterImage::Get(const char *prop, const nsIID & iid, void * *result)
{
  if (!mProperties)
    return NS_ERROR_FAILURE;
  return mProperties->Get(prop, iid, result);
}

NS_IMETHODIMP
RasterImage::Set(const char *prop, nsISupports *value)
{
  if (!mProperties)
    mProperties = do_CreateInstance("@mozilla.org/properties;1");
  if (!mProperties)
    return NS_ERROR_OUT_OF_MEMORY;
  return mProperties->Set(prop, value);
}

NS_IMETHODIMP
RasterImage::Has(const char *prop, bool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if (!mProperties) {
    *_retval = false;
    return NS_OK;
  }
  return mProperties->Has(prop, _retval);
}

NS_IMETHODIMP
RasterImage::Undefine(const char *prop)
{
  if (!mProperties)
    return NS_ERROR_FAILURE;
  return mProperties->Undefine(prop);
}

NS_IMETHODIMP
RasterImage::GetKeys(uint32_t *count, char ***keys)
{
  if (!mProperties) {
    *count = 0;
    *keys = nullptr;
    return NS_OK;
  }
  return mProperties->GetKeys(count, keys);
}

void
RasterImage::Discard(bool force)
{
  // We should be ok for discard
  NS_ABORT_IF_FALSE(force ? CanForciblyDiscard() : CanDiscard(), "Asked to discard but can't!");

  // We should never discard when we have an active decoder
  NS_ABORT_IF_FALSE(!mDecoder, "Asked to discard with open decoder!");

  // As soon as an image becomes animated, it becomes non-discardable and any
  // timers are cancelled.
  NS_ABORT_IF_FALSE(!mAnim, "Asked to discard for animated image!");

  // For post-operation logging
  int old_frame_count = mFrames.Length();

  // Delete all the decoded frames, then clear the array.
  for (int i = 0; i < old_frame_count; ++i)
    delete mFrames[i];
  mFrames.Clear();

  // Flag that we no longer have decoded frames for this image
  mDecoded = false;

  // Notify that we discarded
  nsCOMPtr<imgIDecoderObserver> observer(do_QueryReferent(mObserver));
  if (observer)
    observer->OnDiscard(nullptr);

  if (force)
    DiscardTracker::Remove(&mDiscardTrackerNode);

  // Log
  PR_LOG(gCompressedImageAccountingLog, PR_LOG_DEBUG,
         ("CompressedImageAccounting: discarded uncompressed image "
          "data from RasterImage %p (%s) - %d frames (cached count: %d); "
          "Total Containers: %d, Discardable containers: %d, "
          "Total source bytes: %lld, Source bytes for discardable containers %lld",
          this,
          mSourceDataMimeType.get(),
          old_frame_count,
          mFrames.Length(),
          num_containers,
          num_discardable_containers,
          total_source_bytes,
          discardable_source_bytes));
}

// Helper method to determine if we can discard an image
bool
RasterImage::CanDiscard() {
  return (DiscardingEnabled() && // Globally enabled...
          mDiscardable &&        // ...Enabled at creation time...
          (mLockCount == 0) &&   // ...not temporarily disabled...
          mHasSourceData &&      // ...have the source data...
          mDecoded);             // ...and have something to discard.
}

bool
RasterImage::CanForciblyDiscard() {
  return mDiscardable &&         // ...Enabled at creation time...
         mHasSourceData;         // ...have the source data...
}

// Helper method to tell us whether the clock is currently running for
// discarding this image. Mainly for assertions.
bool
RasterImage::DiscardingActive() {
  return mDiscardTrackerNode.isInList();
}

// Helper method to determine if we're storing the source data in a buffer
// or just writing it directly to the decoder
bool
RasterImage::StoringSourceData() const {
  return (mDecodeOnDraw || mDiscardable);
}


// Sets up a decoder for this image. It is an error to call this function
// when decoding is already in process (ie - when mDecoder is non-null).
nsresult
RasterImage::InitDecoder(bool aDoSizeDecode)
{
  // Ensure that the decoder is not already initialized
  NS_ABORT_IF_FALSE(!mDecoder, "Calling InitDecoder() while already decoding!");
  
  // We shouldn't be firing up a decoder if we already have the frames decoded
  NS_ABORT_IF_FALSE(!mDecoded, "Calling InitDecoder() but already decoded!");

  // Since we're not decoded, we should not have a discard timer active
  NS_ABORT_IF_FALSE(!DiscardingActive(), "Discard Timer active in InitDecoder()!");

  // Figure out which decoder we want
  eDecoderType type = GetDecoderType(mSourceDataMimeType.get());
  CONTAINER_ENSURE_TRUE(type != eDecoderType_unknown, NS_IMAGELIB_ERROR_NO_DECODER);

  nsCOMPtr<imgIDecoderObserver> observer(do_QueryReferent(mObserver));
  // Instantiate the appropriate decoder
  switch (type) {
    case eDecoderType_png:
      mDecoder = new nsPNGDecoder(*this, observer);
      break;
    case eDecoderType_gif:
      mDecoder = new nsGIFDecoder2(*this, observer);
      break;
    case eDecoderType_jpeg:
      mDecoder = new nsJPEGDecoder(*this, observer);
      break;
    case eDecoderType_bmp:
      mDecoder = new nsBMPDecoder(*this, observer);
      break;
    case eDecoderType_ico:
      mDecoder = new nsICODecoder(*this, observer);
      break;
    case eDecoderType_icon:
      mDecoder = new nsIconDecoder(*this, observer);
      break;
    default:
      NS_ABORT_IF_FALSE(0, "Shouldn't get here!");
  }

  // Initialize the decoder
  mDecoder->SetSizeDecode(aDoSizeDecode);
  mDecoder->SetDecodeFlags(mFrameDecodeFlags);
  mDecoder->Init();
  CONTAINER_ENSURE_SUCCESS(mDecoder->GetDecoderError());

  if (!aDoSizeDecode) {
    Telemetry::GetHistogramById(Telemetry::IMAGE_DECODE_COUNT)->Subtract(mDecodeCount);
    mDecodeCount++;
    Telemetry::GetHistogramById(Telemetry::IMAGE_DECODE_COUNT)->Add(mDecodeCount);
  }

  return NS_OK;
}

// Flushes, closes, and nulls-out a decoder. Cleans up any related decoding
// state. It is an error to call this function when there is no initialized
// decoder.
// 
// aIntent specifies the intent of the shutdown. If aIntent is
// eShutdownIntent_Done, an error is flagged if we didn't get what we should
// have out of the decode. If aIntent is eShutdownIntent_Interrupted, we don't
// check this. If aIntent is eShutdownIntent_Error, we shut down in error mode.
nsresult
RasterImage::ShutdownDecoder(eShutdownIntent aIntent)
{
  // Ensure that our intent is valid
  NS_ABORT_IF_FALSE((aIntent >= 0) || (aIntent < eShutdownIntent_AllCount),
                    "Invalid shutdown intent");

  // Ensure that the decoder is initialized
  NS_ABORT_IF_FALSE(mDecoder, "Calling ShutdownDecoder() with no active decoder!");

  // Figure out what kind of decode we were doing before we get rid of our decoder
  bool wasSizeDecode = mDecoder->IsSizeDecode();

  // Finalize the decoder
  // null out mDecoder, _then_ check for errors on the close (otherwise the
  // error routine might re-invoke ShutdownDecoder)
  nsRefPtr<Decoder> decoder = mDecoder;
  mDecoder = nullptr;

  mFinishing = true;
  mInDecoder = true;
  decoder->Finish();
  mInDecoder = false;
  mFinishing = false;

  // Kill off our decode request, if it's pending.  (If not, this call is
  // harmless.)
  DecodeWorker::Singleton()->StopDecoding(this);

  nsresult decoderStatus = decoder->GetDecoderError();
  if (NS_FAILED(decoderStatus)) {
    DoError();
    return decoderStatus;
  }

  // We just shut down the decoder. If we didn't get what we want, but expected
  // to, flag an error
  bool failed = false;
  if (wasSizeDecode && !mHasSize)
    failed = true;
  if (!wasSizeDecode && !mDecoded)
    failed = true;
  if ((aIntent == eShutdownIntent_Done) && failed) {
    DoError();
    return NS_ERROR_FAILURE;
  }

  // Reset number of decoded bytes
  mBytesDecoded = 0;

  return NS_OK;
}

// Writes the data to the decoder, updating the total number of bytes written.
nsresult
RasterImage::WriteToDecoder(const char *aBuffer, uint32_t aCount)
{
  // We should have a decoder
  NS_ABORT_IF_FALSE(mDecoder, "Trying to write to null decoder!");

  // The decoder will start decoding into the current frame (if we have one).
  // When it needs to add another frame, we will unlock this frame and lock the
  // new frame.
  // Our invariant is that, while in the decoder, the last frame is always
  // locked, and all others are unlocked.
  if (mFrames.Length() > 0) {
    imgFrame *curframe = mFrames.ElementAt(mFrames.Length() - 1);
    curframe->LockImageData();
  }

  // Write
  nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
  mInDecoder = true;
  mDecoder->Write(aBuffer, aCount);
  mInDecoder = false;

  // We unlock the current frame, even if that frame is different from the
  // frame we entered the decoder with. (See above.)
  if (mFrames.Length() > 0) {
    imgFrame *curframe = mFrames.ElementAt(mFrames.Length() - 1);
    curframe->UnlockImageData();
  }

  if (!mDecoder)
    return NS_ERROR_FAILURE;
    
  CONTAINER_ENSURE_SUCCESS(mDecoder->GetDecoderError());

  // Keep track of the total number of bytes written over the lifetime of the
  // decoder
  mBytesDecoded += aCount;

  return NS_OK;
}

// This function is called in situations where it's clear that we want the
// frames in decoded form (Draw, GetFrame, CopyFrame, ExtractFrame, etc).
// If we're completely decoded, this method resets the discard timer (if
// we're discardable), since wanting the frames now is a good indicator of
// wanting them again soon. If we're not decoded, this method kicks off
// asynchronous decoding to generate the frames.
nsresult
RasterImage::WantDecodedFrames()
{
  nsresult rv;

  // If we can discard, the clock should be running. Reset it.
  if (CanDiscard()) {
    NS_ABORT_IF_FALSE(DiscardingActive(),
                      "Decoded and discardable but discarding not activated!");
    rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // Request a decode (no-op if we're decoded)
  return RequestDecode();
}

//******************************************************************************
/* void requestDecode() */
NS_IMETHODIMP
RasterImage::RequestDecode()
{
  nsresult rv;

  if (mError)
    return NS_ERROR_FAILURE;

  // If we're fully decoded, we have nothing to do
  if (mDecoded)
    return NS_OK;

  // If we're not storing source data, we have nothing to do
  if (!StoringSourceData())
    return NS_OK;

  // If we've already got a full decoder running, we have nothing to do
  if (mDecoder && !mDecoder->IsSizeDecode())
    return NS_OK;

  // mFinishing protects against the case when we enter RequestDecode from
  // ShutdownDecoder -- in that case, we're done with the decode, we're just
  // not quite ready to admit it.  See bug 744309.
  if (mFinishing)
    return NS_OK;

  // If our callstack goes through a size decoder, we have a problem.
  // We need to shutdown the size decode and replace it with  a full
  // decoder, but can't do that from within the decoder itself. Thus, we post
  // an asynchronous event to the event loop to do it later. Since
  // RequestDecode() is an asynchronous function this works fine (though it's
  // a little slower).
  if (mInDecoder) {
    nsRefPtr<imgDecodeRequestor> requestor = new imgDecodeRequestor(this);
    return NS_DispatchToCurrentThread(requestor);
  }


  // If we have a size decode open, interrupt it and shut it down; or if
  // the decoder has different flags than what we need
  if (mDecoder &&
      (mDecoder->IsSizeDecode() || mDecoder->GetDecodeFlags() != mFrameDecodeFlags))
  {
    rv = ShutdownDecoder(eShutdownIntent_Interrupted);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // If we don't have a decoder, create one 
  if (!mDecoder) {
    NS_ABORT_IF_FALSE(mFrames.IsEmpty(), "Trying to decode to non-empty frame-array");
    rv = InitDecoder(/* aDoSizeDecode = */ false);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // If we've read all the data we have, we're done
  if (mBytesDecoded == mSourceData.Length())
    return NS_OK;

  // If we can do decoding now, do so.  Small images will decode completely,
  // large images will decode a bit and post themselves to the event loop
  // to finish decoding.
  if (!mDecoded && !mInDecoder && mHasSourceData) {
    DecodeWorker::Singleton()->DecodeABitOf(this);
    return NS_OK;
  }

  // If we get this far, dispatch the worker. We do this instead of starting
  // any immediate decoding to guarantee that all our decode notifications are
  // dispatched asynchronously, and to ensure we stay responsive.
  DecodeWorker::Singleton()->RequestDecode(this);

  return NS_OK;
}

// Synchronously decodes as much data as possible
nsresult
RasterImage::SyncDecode()
{
  nsresult rv;

  // If we're decoded already, no worries
  if (mDecoded)
    return NS_OK;

  // If we're not storing source data, there isn't much to do here
  if (!StoringSourceData())
    return NS_OK;

  // We really have no good way of forcing a synchronous decode if we're being
  // called in a re-entrant manner (ie, from an event listener fired by a
  // decoder), because the decoding machinery is already tied up. We thus explicitly
  // disallow this type of call in the API, and check for it in API methods.
  NS_ABORT_IF_FALSE(!mInDecoder, "Yikes, forcing sync in reentrant call!");

  // If we have a size decoder open, or one with different flags than
  // what we need, shut it down
  if (mDecoder &&
      (mDecoder->IsSizeDecode() || mDecoder->GetDecodeFlags() != mFrameDecodeFlags))
  {
    rv = ShutdownDecoder(eShutdownIntent_Interrupted);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // If we don't have a decoder, create one 
  if (!mDecoder) {
    NS_ABORT_IF_FALSE(mFrames.IsEmpty(), "Trying to decode to non-empty frame-array");
    rv = InitDecoder(/* aDoSizeDecode = */ false);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // Write everything we have
  rv = WriteToDecoder(mSourceData.Elements() + mBytesDecoded,
                      mSourceData.Length() - mBytesDecoded);
  CONTAINER_ENSURE_SUCCESS(rv);

  // When we're doing a sync decode, we want to get as much information from the
  // image as possible. We've send the decoder all of our data, so now's a good
  // time  to flush any invalidations (in case we don't have all the data and what
  // we got left us mid-frame).
  nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
  mInDecoder = true;
  mDecoder->FlushInvalidations();
  mInDecoder = false;

  // If we finished the decode, shutdown the decoder
  if (mDecoder && IsDecodeFinished()) {
    rv = ShutdownDecoder(eShutdownIntent_Done);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  // All good if no errors!
  return mError ? NS_ERROR_FAILURE : NS_OK;
}

/* static */ RasterImage::ScaleWorker*
RasterImage::ScaleWorker::Singleton()
{
  if (!sSingleton) {
    sSingleton = new ScaleWorker();
    ClearOnShutdown(&sSingleton);
  }

  return sSingleton;
}

nsresult
RasterImage::ScaleWorker::Run()
{
  if (!mInitialized) {
    PR_SetCurrentThreadName("Image Scaler");
    mInitialized = true;
  }

  ScaleRequest* request;
  gfxSize scale;
  imgFrame* frame;
  {
    MutexAutoLock lock(ScaleWorker::Singleton()->mRequestsMutex);
    request = mScaleRequests.popFirst();
    if (!request)
      return NS_OK;

    scale = request->scale;
    frame = request->srcFrame;
  }

  nsAutoPtr<imgFrame> scaledFrame(new imgFrame());
  bool scaled = ScaleFrameImage(frame, scaledFrame, scale);

  // OK, we've got a new scaled image. Let's get the main thread to unlock and
  // redraw it.
  {
    MutexAutoLock lock(ScaleWorker::Singleton()->mRequestsMutex);
    if (scaled && scale == request->scale && !request->isInList()) {
      request->dstFrame = scaledFrame;
      request->done = true;
    }

    DrawWorker::Singleton()->RequestDraw(request->image);
  }
  return NS_OK;
}

// Note: you MUST call RequestScale with the ScaleWorker mutex held.
void
RasterImage::ScaleWorker::RequestScale(RasterImage* aImg)
{
  mRequestsMutex.AssertCurrentThreadOwns();

  ScaleRequest* request = &aImg->mScaleRequest;
  if (request->isInList())
    return;

  // While the request is outstanding, we hold a reference to it so it won't be
  // deleted from under us (and, since it owns us, so we won't be deleted).
  request->kungFuDeathGrip = request->image;

  mScaleRequests.insertBack(request);

  if (!sScaleWorkerThread) {
    NS_NewThread(getter_AddRefs(sScaleWorkerThread), this, NS_DISPATCH_NORMAL);
    ClearOnShutdown(&sScaleWorkerThread);
  }
  else {
    sScaleWorkerThread->Dispatch(this, NS_DISPATCH_NORMAL);
  }
}

/* static */ RasterImage::DrawWorker*
RasterImage::DrawWorker::Singleton()
{
  if (!sSingleton) {
    sSingleton = new DrawWorker();
    ClearOnShutdown(&sSingleton);
  }

  return sSingleton;
}

nsresult
RasterImage::DrawWorker::Run()
{
  ScaleRequest* request;
  {
    MutexAutoLock lock(ScaleWorker::Singleton()->mRequestsMutex);
    request = mDrawRequests.popFirst();
  }
  if (request) {
    // ScaleWorker is finished with this request, so we can unlock the data now.
    request->UnlockSourceData();
    // We have to reset dstFrame if request was stopped while ScaleWorker was scaling.
    if (request->stopped) {
      ScaleRequest::Stop(request->image);
    }
    nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(request->image->mObserver));
    if (request->done && observer) {
      imgFrame *scaledFrame = request->dstFrame.get();
      scaledFrame->ImageUpdated(scaledFrame->GetRect());
      nsIntRect frameRect = request->srcFrame->GetRect();
      observer->FrameChanged(nullptr, request->image, &frameRect);
    }
    if (request->done) {
      // We are now done with this image, so we can release our reference.
      // THIS CAN DELETE THE REQUEST!
      request->kungFuDeathGrip = nullptr;
    }
  }

  return NS_OK;
}

void
RasterImage::DrawWorker::RequestDraw(RasterImage* aImg)
{
  ScaleRequest* request = &aImg->mScaleRequest;
  mDrawRequests.insertBack(request);
  NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL);
}

void
RasterImage::ScaleRequest::Stop(RasterImage* aImg)
{
  ScaleRequest* request = &aImg->mScaleRequest;
  // It's safe to unlock source image data only if request is in the list.
  // Otherwise we may be reading from the source while performing scaling
  // and can't interrupt immediately.
  if (request->isInList()) {
    request->remove();
    request->UnlockSourceData();
  }
  // We have to check if request is finished before dropping the destination
  // frame. Otherwise we may be writing to the dest while performing scaling.
  if (request->done) {
    request->done = false;
    request->dstFrame = nullptr;
    request->scale.width = 0;
    request->scale.height = 0;
  }
  request->stopped = true;
}

bool
RasterImage::CanScale(gfxPattern::GraphicsFilter aFilter,
                      gfxSize aScale)
{
// The high-quality scaler requires Skia.
#ifdef MOZ_ENABLE_SKIA
  if (gHQDownscaling && aFilter == gfxPattern::FILTER_GOOD &&
      !mAnim && mDecoded &&
      (aScale.width <= 1.0 && aScale.height <= 1.0)) {
    gfxFloat factor = gHQDownscalingMinFactor / 1000.0;
    return (aScale.width < factor || aScale.height < factor);
  }
#endif

  return false;
}

void
RasterImage::DrawWithPreDownscaleIfNeeded(imgFrame *aFrame,
                                          gfxContext *aContext,
                                          gfxPattern::GraphicsFilter aFilter,
                                          const gfxMatrix &aUserSpaceToImageSpace,
                                          const gfxRect &aFill,
                                          const nsIntRect &aSubimage)
{
  imgFrame *frame = aFrame;
  nsIntRect framerect = frame->GetRect();
  gfxMatrix userSpaceToImageSpace = aUserSpaceToImageSpace;
  gfxMatrix imageSpaceToUserSpace = aUserSpaceToImageSpace;
  imageSpaceToUserSpace.Invert();
  gfxSize scale = imageSpaceToUserSpace.ScaleFactors(true);
  nsIntRect subimage = aSubimage;

  if (CanScale(aFilter, scale)) {
    MutexAutoLock lock(ScaleWorker::Singleton()->mRequestsMutex);
    // If scale factor is still the same that we scaled for and
    // ScaleWorker has done it's job, then we can use pre-downscaled frame.
    // If scale factor has changed, order new request.
    if (mScaleRequest.scale == scale) {
      if (mScaleRequest.done) {
        frame = mScaleRequest.dstFrame.get();
        userSpaceToImageSpace.Multiply(gfxMatrix().Scale(scale.width, scale.height));

        // Since we're switching to a scaled image, we we need to transform the
        // area of the subimage to draw accordingly, since imgFrame::Draw()
        // doesn't know about scaled frames.
        subimage.ScaleRoundOut(scale.width, scale.height);
      }
    } else {
      // FIXME: Current implementation doesn't support pre-downscale
      // mechanism for multiple images from same src, since we cache
      // pre-downscaled frame only for the latest requested scale.
      // The solution is to cache more than one scaled image frame
      // for each RasterImage.
      int scaling = mScaleRequest.srcDataLocked ? 1 : 0;
      if (mLockCount - scaling == 1) {
        ScaleRequest::Stop(this);
        mScaleRequest.srcFrame = frame;
        mScaleRequest.scale = scale;
        mScaleRequest.stopped = false;

        // We need to make sure that source data is available before asking to scale.
        if (mScaleRequest.LockSourceData()) {
          ScaleWorker::Singleton()->RequestScale(this);
        }
      }
    }
  }

  nsIntMargin padding(framerect.x, framerect.y,
                      mSize.width - framerect.XMost(),
                      mSize.height - framerect.YMost());

  frame->Draw(aContext, aFilter, userSpaceToImageSpace, aFill, padding, subimage);
}

//******************************************************************************
/* [noscript] void draw(in gfxContext aContext,
 *                      in gfxGraphicsFilter aFilter,
 *                      [const] in gfxMatrix aUserSpaceToImageSpace,
 *                      [const] in gfxRect aFill,
 *                      [const] in nsIntRect aSubimage,
 *                      [const] in nsIntSize aViewportSize,
 *                      in uint32_t aFlags); */
NS_IMETHODIMP
RasterImage::Draw(gfxContext *aContext,
                  gfxPattern::GraphicsFilter aFilter,
                  const gfxMatrix &aUserSpaceToImageSpace,
                  const gfxRect &aFill,
                  const nsIntRect &aSubimage,
                  const nsIntSize& /*aViewportSize - ignored*/,
                  uint32_t aFlags)
{
  if (mError)
    return NS_ERROR_FAILURE;

  // Disallowed in the API
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  // Illegal -- you can't draw with non-default decode flags.
  // (Disabling colorspace conversion might make sense to allow, but
  // we don't currently.)
  if ((aFlags & DECODE_FLAGS_MASK) != DECODE_FLAGS_DEFAULT)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aContext);

  // We can only draw with the default decode flags
  if (mFrameDecodeFlags != DECODE_FLAGS_DEFAULT) {
    if (!CanForciblyDiscard())
      return NS_ERROR_NOT_AVAILABLE;
    ForceDiscard();

    mFrameDecodeFlags = DECODE_FLAGS_DEFAULT;
  }

  // If this image is a candidate for discarding, reset its position in the
  // discard tracker so we're less likely to discard it right away.
  //
  // (We don't normally draw unlocked images, so this conditition will usually
  // be false.  But we will draw unlocked images if image locking is globally
  // disabled via the content.image.allow_locking pref.)
  if (DiscardingActive()) {
    DiscardTracker::Reset(&mDiscardTrackerNode);
  }

  // We use !mDecoded && mHasSourceData to mean discarded.
  if (!mDecoded && mHasSourceData) {
      mDrawStartTime = TimeStamp::Now();

      // We're drawing this image, so indicate that we should decode it as soon
      // as possible.
      DecodeWorker::Singleton()->MarkAsASAP(this);
  }

  // If a synchronous draw is requested, flush anything that might be sitting around
  if (aFlags & FLAG_SYNC_DECODE) {
    nsresult rv = SyncDecode();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  imgFrame *frame = GetCurrentDrawableImgFrame();
  if (!frame) {
    return NS_OK; // Getting the frame (above) touches the image and kicks off decoding
  }

  DrawWithPreDownscaleIfNeeded(frame, aContext, aFilter, aUserSpaceToImageSpace, aFill, aSubimage);

  if (mDecoded && !mDrawStartTime.IsNull()) {
      TimeDuration drawLatency = TimeStamp::Now() - mDrawStartTime;
      Telemetry::Accumulate(Telemetry::IMAGE_DECODE_ON_DRAW_LATENCY, int32_t(drawLatency.ToMicroseconds()));
      // clear the value of mDrawStartTime
      mDrawStartTime = TimeStamp();
  }

  return NS_OK;
}

//******************************************************************************
/* [notxpcom] nsIFrame GetRootLayoutFrame() */
nsIFrame*
RasterImage::GetRootLayoutFrame()
{
  return nullptr;
}

//******************************************************************************
/* void lockImage() */
NS_IMETHODIMP
RasterImage::LockImage()
{
  if (mError)
    return NS_ERROR_FAILURE;

  // Cancel the discard timer if it's there
  DiscardTracker::Remove(&mDiscardTrackerNode);

  // Increment the lock count
  mLockCount++;

  return NS_OK;
}

//******************************************************************************
/* void unlockImage() */
NS_IMETHODIMP
RasterImage::UnlockImage()
{
  if (mError)
    return NS_ERROR_FAILURE;

  // It's an error to call this function if the lock count is 0
  NS_ABORT_IF_FALSE(mLockCount > 0,
                    "Calling UnlockImage with mLockCount == 0!");
  if (mLockCount == 0)
    return NS_ERROR_ABORT;

  // We're locked, so discarding should not be active
  NS_ABORT_IF_FALSE(!DiscardingActive(), "Locked, but discarding activated");

  // Decrement our lock count
  mLockCount--;

  if (ScaleWorker::sSingleton && mLockCount == 0) {
    MutexAutoLock lock(ScaleWorker::Singleton()->mRequestsMutex);
    ScaleRequest::Stop(this);
  }

  // If we've decoded this image once before, we're currently decoding again,
  // and our lock count is now zero (so nothing is forcing us to keep the
  // decoded data around), try to cancel the decode and throw away whatever
  // we've decoded.
  if (mHasBeenDecoded && mDecoder &&
      mLockCount == 0 && CanForciblyDiscard()) {
    PR_LOG(gCompressedImageAccountingLog, PR_LOG_DEBUG,
           ("RasterImage[0x%p] canceling decode because image "
            "is now unlocked.", this));
    ShutdownDecoder(eShutdownIntent_Interrupted);
    ForceDiscard();
    return NS_OK;
  }

  // Otherwise, we might still be a candidate for discarding in the future.  If
  // we are, add ourselves to the discard tracker.
  if (CanDiscard()) {
    nsresult rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  return NS_OK;
}

//******************************************************************************
/* void requestDiscard() */
NS_IMETHODIMP
RasterImage::RequestDiscard()
{
  if (CanDiscard()) {
    ForceDiscard();
  }

  return NS_OK;
}

// Flushes up to aMaxBytes to the decoder.
nsresult
RasterImage::DecodeSomeData(uint32_t aMaxBytes)
{
  // We should have a decoder if we get here
  NS_ABORT_IF_FALSE(mDecoder, "trying to decode without decoder!");

  // If we have nothing to decode, return
  if (mBytesDecoded == mSourceData.Length())
    return NS_OK;


  // write the proper amount of data
  uint32_t bytesToDecode = NS_MIN(aMaxBytes,
                                  mSourceData.Length() - mBytesDecoded);
  nsresult rv = WriteToDecoder(mSourceData.Elements() + mBytesDecoded,
                               bytesToDecode);

  return rv;
}

// There are various indicators that tell us we're finished with the decode
// task at hand and can shut down the decoder.
//
// This method may not be called if there is no decoder.
bool
RasterImage::IsDecodeFinished()
{
  // Precondition
  NS_ABORT_IF_FALSE(mDecoder, "Can't call IsDecodeFinished() without decoder!");

  // Assume it's not finished
  bool decodeFinished = false;

  // There shouldn't be any reason to call this if we're not storing
  // source data
  NS_ABORT_IF_FALSE(StoringSourceData(),
                    "just shut down on SourceDataComplete!");

  // The decode is complete if we got what we wanted...
  if (mDecoder->IsSizeDecode()) {
    if (mHasSize)
      decodeFinished = true;
  }
  else {
    if (mDecoded)
      decodeFinished = true;
  }

  // ...or if we have all the source data and wrote all the source data.
  //
  // (NB - This can be distinct from the above case even for non-erroneous
  // images because the decoder might not call DecodingComplete() until we
  // call Close() in ShutdownDecoder())
  if (mHasSourceData && (mBytesDecoded == mSourceData.Length()))
    decodeFinished = true;

  return decodeFinished;
}

// Indempotent error flagging routine. If a decoder is open, shuts it down.
void
RasterImage::DoError()
{
  // If we've flagged an error before, we have nothing to do
  if (mError)
    return;

  // If we're mid-decode, shut down the decoder.
  if (mDecoder)
    ShutdownDecoder(eShutdownIntent_Error);

  // Put the container in an error state
  mError = true;

  // Log our error
  LOG_CONTAINER_ERROR;
}

// nsIInputStream callback to copy the incoming image data directly to the
// RasterImage without processing. The RasterImage is passed as the closure.
// Always reads everything it gets, even if the data is erroneous.
NS_METHOD
RasterImage::WriteToRasterImage(nsIInputStream* /* unused */,
                                void*          aClosure,
                                const char*    aFromRawSegment,
                                uint32_t       /* unused */,
                                uint32_t       aCount,
                                uint32_t*      aWriteCount)
{
  // Retrieve the RasterImage
  RasterImage* image = static_cast<RasterImage*>(aClosure);

  // Copy the source data. Unless we hit OOM, we squelch the return value
  // here, because returning an error means that ReadSegments stops
  // reading data, violating our invariant that we read everything we get.
  // If we hit OOM then we fail and the load is aborted.
  nsresult rv = image->AddSourceData(aFromRawSegment, aCount);
  if (rv == NS_ERROR_OUT_OF_MEMORY) {
    image->DoError();
    return rv;
  }

  // We wrote everything we got
  *aWriteCount = aCount;

  return NS_OK;
}

bool
RasterImage::ShouldAnimate()
{
  return Image::ShouldAnimate() && mFrames.Length() >= 2 &&
         !mAnimationFinished;
}

/* readonly attribute uint32_t framesNotified; */
#ifdef DEBUG
NS_IMETHODIMP
RasterImage::GetFramesNotified(uint32_t *aFramesNotified)
{
  NS_ENSURE_ARG_POINTER(aFramesNotified);

  *aFramesNotified = mFramesNotified;

  return NS_OK;
}
#endif

/* static */ RasterImage::DecodeWorker*
RasterImage::DecodeWorker::Singleton()
{
  if (!sSingleton) {
    sSingleton = new DecodeWorker();
    ClearOnShutdown(&sSingleton);
  }

  return sSingleton;
}

void
RasterImage::DecodeWorker::MarkAsASAP(RasterImage* aImg)
{
  DecodeRequest* request = &aImg->mDecodeRequest;

  // If we're already an ASAP request, there's nothing to do here.
  if (request->mIsASAP) {
    return;
  }

  request->mIsASAP = true;

  if (request->isInList()) {
    // If the decode request is in a list, it must be in the normal decode
    // requests list -- if it had been in the ASAP list, then mIsASAP would
    // have been true above.  Move the request to the ASAP list.
    request->remove();
    mASAPDecodeRequests.insertBack(request);

    // Since request is in a list, one of the decode worker's lists is
    // non-empty, so the worker should be pending in the event loop.
    //
    // (Note that this invariant only holds while we are not in Run(), because
    // DecodeSomeOfImage adds requests to the decode worker using
    // AddDecodeRequest, not RequestDecode, and AddDecodeRequest does not call
    // EnsurePendingInEventLoop.  Therefore, it is an error to call MarkAsASAP
    // from within DecodeWorker::Run.)
    MOZ_ASSERT(mPendingInEventLoop);
  }
}

void
RasterImage::DecodeWorker::AddDecodeRequest(DecodeRequest* aRequest)
{
  if (aRequest->isInList()) {
    // The image is already in our list of images to decode, so we don't have
    // to do anything here.
    return;
  }

  if (aRequest->mIsASAP) {
    mASAPDecodeRequests.insertBack(aRequest);
  } else {
    mNormalDecodeRequests.insertBack(aRequest);
  }
}

void
RasterImage::DecodeWorker::RequestDecode(RasterImage* aImg)
{
  AddDecodeRequest(&aImg->mDecodeRequest);
  EnsurePendingInEventLoop();
}

void
RasterImage::DecodeWorker::DecodeABitOf(RasterImage* aImg)
{
  DecodeSomeOfImage(aImg);

  // If we aren't yet finished decoding and we have more data in hand, add
  // this request to the back of the priority list.
  if (aImg->mDecoder &&
      !aImg->mError &&
      !aImg->IsDecodeFinished() &&
      aImg->mSourceData.Length() > aImg->mBytesDecoded) {
    RequestDecode(aImg);
  }
}

void
RasterImage::DecodeWorker::EnsurePendingInEventLoop()
{
  if (!mPendingInEventLoop) {
    mPendingInEventLoop = true;
    NS_DispatchToCurrentThread(this);
  }
}

void
RasterImage::DecodeWorker::StopDecoding(RasterImage* aImg)
{
  DecodeRequest* request = &aImg->mDecodeRequest;
  if (request->isInList()) {
    request->remove();
  }
  request->mDecodeTime = TimeDuration(0);
  request->mIsASAP = false;
}

NS_IMETHODIMP
RasterImage::DecodeWorker::Run()
{
  // We just got called back by the event loop; therefore, we're no longer
  // pending.
  mPendingInEventLoop = false;

  TimeStamp eventStart = TimeStamp::Now();

  // Now decode until we either run out of time or run out of images.
  do {
    // Try to get an ASAP request to handle.  If there isn't one, try to get a
    // normal request.  If no normal request is pending either, then we're done
    // here.
    DecodeRequest* request = mASAPDecodeRequests.popFirst();
    if (!request)
      request = mNormalDecodeRequests.popFirst();
    if (!request)
      break;

    // This has to be a strong pointer, because DecodeSomeOfImage may destroy
    // image->mDecoder, which may be holding the only other reference to image.
    nsRefPtr<RasterImage> image = request->mImage;
    DecodeSomeOfImage(image);

    // If we aren't yet finished decoding and we have more data in hand, add
    // this request to the back of the list.
    if (image->mDecoder &&
        !image->mError &&
        !image->IsDecodeFinished() &&
        image->mSourceData.Length() > image->mBytesDecoded) {
      AddDecodeRequest(request);
    }

  } while ((TimeStamp::Now() - eventStart).ToMilliseconds() <= gMaxMSBeforeYield);

  // If decode requests are pending, re-post ourself to the event loop.
  if (!mASAPDecodeRequests.isEmpty() || !mNormalDecodeRequests.isEmpty()) {
    EnsurePendingInEventLoop();
  }

  Telemetry::Accumulate(Telemetry::IMAGE_DECODE_LATENCY,
                        uint32_t((TimeStamp::Now() - eventStart).ToMilliseconds()));

  return NS_OK;
}

nsresult
RasterImage::DecodeWorker::DecodeUntilSizeAvailable(RasterImage* aImg)
{
  return DecodeSomeOfImage(aImg, DECODE_TYPE_UNTIL_SIZE);
}

nsresult
RasterImage::DecodeWorker::DecodeSomeOfImage(
  RasterImage* aImg,
  DecodeType aDecodeType /* = DECODE_TYPE_NORMAL */)
{
  NS_ABORT_IF_FALSE(aImg->mInitialized,
                    "Worker active for uninitialized container!");

  // If an error is flagged, it probably happened while we were waiting
  // in the event queue.
  if (aImg->mError)
    return NS_OK;

  // If mDecoded or we don't have a decoder, we must have finished already (for
  // example, a synchronous decode request came while the worker was pending).
  if (!aImg->mDecoder || aImg->mDecoded)
    return NS_OK;

  nsRefPtr<Decoder> decoderKungFuDeathGrip = aImg->mDecoder;

  uint32_t maxBytes;
  if (aImg->mDecoder->IsSizeDecode()) {
    // Decode all available data if we're a size decode; they're cheap, and we
    // want them to be more or less synchronous.
    maxBytes = aImg->mSourceData.Length();
  } else {
    // We're only guaranteed to decode this many bytes, so in particular,
    // gDecodeBytesAtATime should be set high enough for us to read the size
    // from most images.
    maxBytes = gDecodeBytesAtATime;
  }

  int32_t chunkCount = 0;
  TimeStamp start = TimeStamp::Now();
  TimeStamp deadline = start + TimeDuration::FromMilliseconds(gMaxMSBeforeYield);

  // We keep decoding chunks until:
  //  * we don't have any data left to decode,
  //  * the decode completes,
  //  * we're an UNTIL_SIZE decode and we get the size, or
  //  * we run out of time.
  while (aImg->mSourceData.Length() > aImg->mBytesDecoded &&
         !aImg->IsDecodeFinished() &&
         !(aDecodeType == DECODE_TYPE_UNTIL_SIZE && aImg->mHasSize)) {
    chunkCount++;
    nsresult rv = aImg->DecodeSomeData(maxBytes);
    if (NS_FAILED(rv)) {
      aImg->DoError();
      return rv;
    }

    // Yield if we've been decoding for too long. We check this _after_ decoding
    // a chunk to ensure that we don't yield without doing any decoding.
    if (TimeStamp::Now() >= deadline)
      break;
  }

  aImg->mDecodeRequest.mDecodeTime += (TimeStamp::Now() - start);

  if (chunkCount && !aImg->mDecoder->IsSizeDecode()) {
    Telemetry::Accumulate(Telemetry::IMAGE_DECODE_CHUNKS, chunkCount);
  }

  // Flush invalidations (and therefore paint) now that we've decoded all the
  // chunks we're going to.
  //
  // However, don't paint if:
  //
  //  * This was an until-size decode.  Until-size decodes are always followed
  //    by normal decodes, so don't bother painting.
  //
  //  * The decoder flagged an error.  The decoder may have written garbage
  //    into the output buffer; don't paint it to the screen.
  //
  //  * We have all the source data.  This disables progressive display of
  //    previously-decoded images, thus letting us finish decoding faster,
  //    since we don't waste time painting while we decode.
  //    Decoder::PostFrameStop() will flush invalidations once the decode is
  //    done.

  if (aDecodeType != DECODE_TYPE_UNTIL_SIZE &&
      !aImg->mDecoder->HasError() &&
      !aImg->mHasSourceData) {
    aImg->mInDecoder = true;
    aImg->mDecoder->FlushInvalidations();
    aImg->mInDecoder = false;
  }

  // If the decode finished, shut down the decoder.
  if (aImg->mDecoder && aImg->IsDecodeFinished()) {

    // Do some telemetry if this isn't a size decode.
    DecodeRequest* request = &aImg->mDecodeRequest;
    if (!aImg->mDecoder->IsSizeDecode()) {
      Telemetry::Accumulate(Telemetry::IMAGE_DECODE_TIME,
                            int32_t(request->mDecodeTime.ToMicroseconds()));

      // We record the speed for only some decoders. The rest have
      // SpeedHistogram return HistogramCount.
      Telemetry::ID id = aImg->mDecoder->SpeedHistogram();
      if (id < Telemetry::HistogramCount) {
          int32_t KBps = int32_t(request->mImage->mBytesDecoded /
                                 (1024 * request->mDecodeTime.ToSeconds()));
          Telemetry::Accumulate(id, KBps);
      }
    }

    nsresult rv = aImg->ShutdownDecoder(RasterImage::eShutdownIntent_Done);
    if (NS_FAILED(rv)) {
      aImg->DoError();
      return rv;
    }
  }

  return NS_OK;
}

} // namespace image
} // namespace mozilla
