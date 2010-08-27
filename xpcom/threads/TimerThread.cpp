/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
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
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
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

#include "nsTimerImpl.h"
#include "TimerThread.h"

#include "nsAutoLock.h"
#include "nsThreadUtils.h"
#include "pratom.h"

#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsIProxyObjectManager.h"
#include "mozilla/Services.h"

#include <math.h>

NS_IMPL_THREADSAFE_ISUPPORTS2(TimerThread, nsIRunnable, nsIObserver)

TimerThread::TimerThread() :
  mInitInProgress(0),
  mInitialized(PR_FALSE),
  mLock(nsnull),
  mCondVar(nsnull),
  mShutdown(PR_FALSE),
  mWaiting(PR_FALSE),
  mSleeping(PR_FALSE)
{
}

TimerThread::~TimerThread()
{
  if (mCondVar)
    PR_DestroyCondVar(mCondVar);
  if (mLock)
    PR_DestroyLock(mLock);

  mThread = nsnull;

  NS_ASSERTION(mTimers.IsEmpty(), "Timers remain in TimerThread::~TimerThread");
}

nsresult
TimerThread::InitLocks()
{
  NS_ASSERTION(!mLock, "InitLocks called twice?");
  mLock = PR_NewLock();
  if (!mLock)
    return NS_ERROR_OUT_OF_MEMORY;

  mCondVar = PR_NewCondVar(mLock);
  if (!mCondVar)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

nsresult TimerThread::Init()
{
  PR_LOG(gTimerLog, PR_LOG_DEBUG, ("TimerThread::Init [%d]\n", mInitialized));

  if (mInitialized) {
    if (!mThread)
      return NS_ERROR_FAILURE;

    return NS_OK;
  }

  if (PR_AtomicSet(&mInitInProgress, 1) == 0) {
    // We hold on to mThread to keep the thread alive.
    nsresult rv = NS_NewThread(getter_AddRefs(mThread), this);
    if (NS_FAILED(rv)) {
      mThread = nsnull;
    }
    else {
      nsCOMPtr<nsIObserverService> observerService =
          mozilla::services::GetObserverService();
      // We must not use the observer service from a background thread!
      if (observerService && !NS_IsMainThread()) {
        nsCOMPtr<nsIObserverService> result = nsnull;
        NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                             NS_GET_IID(nsIObserverService),
                             observerService, NS_PROXY_ASYNC,
                             getter_AddRefs(result));
        observerService.swap(result);
      }
      // We'll be released at xpcom shutdown
      if (observerService) {
        observerService->AddObserver(this, "sleep_notification", PR_FALSE);
        observerService->AddObserver(this, "wake_notification", PR_FALSE);
      }
    }

    PR_Lock(mLock);
    mInitialized = PR_TRUE;
    PR_NotifyAllCondVar(mCondVar);
    PR_Unlock(mLock);
  }
  else {
    PR_Lock(mLock);
    while (!mInitialized) {
      PR_WaitCondVar(mCondVar, PR_INTERVAL_NO_TIMEOUT);
    }
    PR_Unlock(mLock);
  }

  if (!mThread)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult TimerThread::Shutdown()
{
  PR_LOG(gTimerLog, PR_LOG_DEBUG, ("TimerThread::Shutdown begin\n"));

  if (!mThread)
    return NS_ERROR_NOT_INITIALIZED;

  nsTArray<nsTimerImpl*> timers;
  {   // lock scope
    nsAutoLock lock(mLock);

    mShutdown = PR_TRUE;

    // notify the cond var so that Run() can return
    if (mCondVar && mWaiting)
      PR_NotifyCondVar(mCondVar);

    // Need to copy content of mTimers array to a local array
    // because call to timers' ReleaseCallback() (and release its self)
    // must not be done under the lock. Destructor of a callback
    // might potentially call some code reentering the same lock
    // that leads to unexpected behavior or deadlock.
    // See bug 422472.
    timers.AppendElements(mTimers);
    mTimers.Clear();
  }

  PRUint32 timersCount = timers.Length();
  for (PRUint32 i = 0; i < timersCount; i++) {
    nsTimerImpl *timer = timers[i];
    timer->ReleaseCallback();
    ReleaseTimerInternal(timer);
  }

  mThread->Shutdown();    // wait for the thread to die

  PR_LOG(gTimerLog, PR_LOG_DEBUG, ("TimerThread::Shutdown end\n"));
  return NS_OK;
}

/* void Run(); */
NS_IMETHODIMP TimerThread::Run()
{
  nsAutoLock lock(mLock);

  while (!mShutdown) {
    // Have to use PRIntervalTime here, since PR_WaitCondVar takes it
    PRIntervalTime waitFor;

    if (mSleeping) {
      // Sleep for 0.1 seconds while not firing timers.
      waitFor = PR_MillisecondsToInterval(100);
    } else {
      waitFor = PR_INTERVAL_NO_TIMEOUT;
      TimeStamp now = TimeStamp::Now();
      nsTimerImpl *timer = nsnull;

      if (!mTimers.IsEmpty()) {
        timer = mTimers[0];

        if (now >= timer->mTimeout) {
    next:
          // NB: AddRef before the Release under RemoveTimerInternal to avoid
          // mRefCnt passing through zero, in case all other refs than the one
          // from mTimers have gone away (the last non-mTimers[i]-ref's Release
          // must be racing with us, blocked in gThread->RemoveTimer waiting
          // for TimerThread::mLock, under nsTimerImpl::Release.

          NS_ADDREF(timer);
          RemoveTimerInternal(timer);

          // We release mLock around the Fire call to avoid deadlock.
          lock.unlock();

#ifdef DEBUG_TIMERS
          if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
            PR_LOG(gTimerLog, PR_LOG_DEBUG,
                   ("Timer thread woke up %fms from when it was supposed to\n",
                    fabs((now - timer->mTimeout).ToMilliseconds())));
          }
#endif

          // We are going to let the call to PostTimerEvent here handle the
          // release of the timer so that we don't end up releasing the timer
          // on the TimerThread instead of on the thread it targets.
          if (NS_FAILED(timer->PostTimerEvent())) {
            nsrefcnt rc;
            NS_RELEASE2(timer, rc);
            
            // The nsITimer interface requires that its users keep a reference
            // to the timers they use while those timers are initialized but
            // have not yet fired.  If this ever happens, it is a bug in the
            // code that created and used the timer.
            //
            // Further, note that this should never happen even with a
            // misbehaving user, because nsTimerImpl::Release checks for a
            // refcount of 1 with an armed timer (a timer whose only reference
            // is from the timer thread) and when it hits this will remove the
            // timer from the timer thread and thus destroy the last reference,
            // preventing this situation from occurring.
            NS_ASSERTION(rc != 0, "destroyed timer off its target thread!");
          }
          timer = nsnull;

          lock.lock();
          if (mShutdown)
            break;

          // Update now, as PostTimerEvent plus the locking may have taken a
          // tick or two, and we may goto next below.
          now = TimeStamp::Now();
        }
      }

      if (!mTimers.IsEmpty()) {
        timer = mTimers[0];

        TimeStamp timeout = timer->mTimeout;

        // Don't wait at all (even for PR_INTERVAL_NO_WAIT) if the next timer
        // is due now or overdue.
        if (now >= timeout)
          goto next;
        waitFor = PR_MillisecondsToInterval((timeout - now).ToMilliseconds());
      }

#ifdef DEBUG_TIMERS
      if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
        if (waitFor == PR_INTERVAL_NO_TIMEOUT)
          PR_LOG(gTimerLog, PR_LOG_DEBUG,
                 ("waiting for PR_INTERVAL_NO_TIMEOUT\n"));
        else
          PR_LOG(gTimerLog, PR_LOG_DEBUG,
                 ("waiting for %u\n", PR_IntervalToMilliseconds(waitFor)));
      }
#endif
    }

    mWaiting = PR_TRUE;
    PR_WaitCondVar(mCondVar, waitFor);
    mWaiting = PR_FALSE;
  }

  return NS_OK;
}

nsresult TimerThread::AddTimer(nsTimerImpl *aTimer)
{
  nsAutoLock lock(mLock);

  // Add the timer to our list.
  PRInt32 i = AddTimerInternal(aTimer);
  if (i < 0)
    return NS_ERROR_OUT_OF_MEMORY;

  // Awaken the timer thread.
  if (mCondVar && mWaiting && i == 0)
    PR_NotifyCondVar(mCondVar);

  return NS_OK;
}

nsresult TimerThread::TimerDelayChanged(nsTimerImpl *aTimer)
{
  nsAutoLock lock(mLock);

  // Our caller has a strong ref to aTimer, so it can't go away here under
  // ReleaseTimerInternal.
  RemoveTimerInternal(aTimer);

  PRInt32 i = AddTimerInternal(aTimer);
  if (i < 0)
    return NS_ERROR_OUT_OF_MEMORY;

  // Awaken the timer thread.
  if (mCondVar && mWaiting && i == 0)
    PR_NotifyCondVar(mCondVar);

  return NS_OK;
}

nsresult TimerThread::RemoveTimer(nsTimerImpl *aTimer)
{
  nsAutoLock lock(mLock);

  // Remove the timer from our array.  Tell callers that aTimer was not found
  // by returning NS_ERROR_NOT_AVAILABLE.  Unlike the TimerDelayChanged case
  // immediately above, our caller may be passing a (now-)weak ref in via the
  // aTimer param, specifically when nsTimerImpl::Release loses a race with
  // TimerThread::Run, must wait for the mLock auto-lock here, and during the
  // wait Run drops the only remaining ref to aTimer via RemoveTimerInternal.

  if (!RemoveTimerInternal(aTimer))
    return NS_ERROR_NOT_AVAILABLE;

  // Awaken the timer thread.
  if (mCondVar && mWaiting)
    PR_NotifyCondVar(mCondVar);

  return NS_OK;
}

// This function must be called from within a lock
PRInt32 TimerThread::AddTimerInternal(nsTimerImpl *aTimer)
{
  if (mShutdown)
    return -1;

  TimeStamp now = TimeStamp::Now();
  PRUint32 count = mTimers.Length();
  PRUint32 i = 0;
  for (; i < count; i++) {
    nsTimerImpl *timer = mTimers[i];

    // Don't break till we have skipped any overdue timers.
    if (now < timer->mTimeout && aTimer->mTimeout < timer->mTimeout) {
      break;
    }
  }

  if (!mTimers.InsertElementAt(i, aTimer))
    return -1;

  aTimer->mArmed = PR_TRUE;
  NS_ADDREF(aTimer);
  return i;
}

PRBool TimerThread::RemoveTimerInternal(nsTimerImpl *aTimer)
{
  if (!mTimers.RemoveElement(aTimer))
    return PR_FALSE;

  ReleaseTimerInternal(aTimer);
  return PR_TRUE;
}

void TimerThread::ReleaseTimerInternal(nsTimerImpl *aTimer)
{
  // Order is crucial here -- see nsTimerImpl::Release.
  aTimer->mArmed = PR_FALSE;
  NS_RELEASE(aTimer);
}

void TimerThread::DoBeforeSleep()
{
  mSleeping = PR_TRUE;
}

void TimerThread::DoAfterSleep()
{
  mSleeping = PR_TRUE; // wake may be notified without preceding sleep notification
  for (PRUint32 i = 0; i < mTimers.Length(); i ++) {
    nsTimerImpl *timer = mTimers[i];
    // get and set the delay to cause its timeout to be recomputed
    PRUint32 delay;
    timer->GetDelay(&delay);
    timer->SetDelay(delay);
  }

  mSleeping = PR_FALSE;
}


/* void observe (in nsISupports aSubject, in string aTopic, in wstring aData); */
NS_IMETHODIMP
TimerThread::Observe(nsISupports* /* aSubject */, const char *aTopic, const PRUnichar* /* aData */)
{
  if (strcmp(aTopic, "sleep_notification") == 0)
    DoBeforeSleep();
  else if (strcmp(aTopic, "wake_notification") == 0)
    DoAfterSleep();

  return NS_OK;
}
