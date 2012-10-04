/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef nsPagePrintTimer_h___
#define nsPagePrintTimer_h___

// Timer Includes
#include "nsITimer.h"

#include "nsIDocumentViewerPrint.h"
#include "nsPrintObject.h"
#include "mozilla/Attributes.h"
#include "nsThreadUtils.h"

class nsPrintEngine;

//---------------------------------------------------
//-- Page Timer Class
//---------------------------------------------------
class nsPagePrintTimer MOZ_FINAL : public nsRunnable,
                                   public nsITimerCallback
{
public:

  NS_DECL_ISUPPORTS

  nsPagePrintTimer(nsPrintEngine* aPrintEngine,
                   nsIDocumentViewerPrint* aDocViewerPrint,
                   uint32_t aDelay)
    : mPrintEngine(aPrintEngine)
    , mDocViewerPrint(aDocViewerPrint)
    , mDelay(aDelay)
    , mFiringCount(0)
    , mPrintObj(nullptr)
    , mWatchDogCount(0)
    , mDone(false)
  {
    mDocViewerPrint->IncrementDestroyRefCount();
  }
  ~nsPagePrintTimer();

  NS_DECL_NSITIMERCALLBACK

  nsresult Start(nsPrintObject* aPO);

  NS_IMETHOD Run();

  void Stop();

private:
  nsresult StartTimer(bool aUseDelay);
  nsresult StartWatchDogTimer();
  void     StopWatchDogTimer();
  void     Fail();

  nsPrintEngine*             mPrintEngine;
  nsCOMPtr<nsIDocumentViewerPrint> mDocViewerPrint;
  nsCOMPtr<nsITimer>         mTimer;
  nsCOMPtr<nsITimer>         mWatchDogTimer;
  uint32_t                   mDelay;
  uint32_t                   mFiringCount;
  nsPrintObject *            mPrintObj;
  uint32_t                   mWatchDogCount;
  bool                       mDone;

  static const uint32_t WATCH_DOG_INTERVAL  = 1000;
  static const uint32_t WATCH_DOG_MAX_COUNT = 10;
};


nsresult
NS_NewPagePrintTimer(nsPagePrintTimer **aResult);

#endif /* nsPagePrintTimer_h___ */
