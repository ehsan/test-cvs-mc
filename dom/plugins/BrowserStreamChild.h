/* -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 8 -*- */
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
 * The Original Code is Mozilla Plugin App.
 *
 * The Initial Developer of the Original Code is
 *   Benjamin Smedberg <benjamin@smedbergs.us>
 * Portions created by the Initial Developer are Copyright (C) 2009
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

#ifndef mozilla_plugins_BrowserStreamChild_h
#define mozilla_plugins_BrowserStreamChild_h 1

#include "mozilla/plugins/PBrowserStreamChild.h"
#include "mozilla/plugins/AStream.h"

namespace mozilla {
namespace plugins {

class PluginInstanceChild;
class PStreamNotifyChild;

class BrowserStreamChild : public PBrowserStreamChild, public AStream
{
public:
  BrowserStreamChild(PluginInstanceChild* instance,
                     const nsCString& url,
                     const uint32_t& length,
                     const uint32_t& lastmodified,
                     const PStreamNotifyChild* notifyData,
                     const nsCString& headers,
                     const nsCString& mimeType,
                     const bool& seekable,
                     NPError* rv,
                     uint16_t* stype);
  virtual ~BrowserStreamChild() { }

  NS_OVERRIDE virtual bool IsBrowserStream() { return true; }

  NPError StreamConstructed(
            const nsCString& url,
            const uint32_t& length,
            const uint32_t& lastmodified,
            PStreamNotifyChild* notifyData,
            const nsCString& headers,
            const nsCString& mimeType,
            const bool& seekable,
            uint16_t* stype);

  virtual bool RecvWrite(const int32_t& offset,
                         const Buffer& data,
                         const uint32_t& newsize);
  virtual bool AnswerNPP_StreamAsFile(const nsCString& fname);
  virtual bool RecvNPP_DestroyStream(const NPReason& reason);
  virtual bool Recv__delete__();

  void EnsureCorrectInstance(PluginInstanceChild* i)
  {
    if (i != mInstance)
      NS_RUNTIMEABORT("Incorrect stream instance");
  }
  void EnsureCorrectStream(NPStream* s)
  {
    if (s != &mStream)
      NS_RUNTIMEABORT("Incorrect stream data");
  }

  NPError NPN_RequestRead(NPByteRange* aRangeList);
  void NPN_DestroyStream(NPReason reason);

private:
  using PBrowserStreamChild::SendNPN_DestroyStream;

  /**
   * Deliver the data currently in mPending, scheduling
   * or cancelling the suspended timer as needed.
   */
  void DeliverData();
  void MaybeDeliverNPP_DestroyStream();
  void SetSuspendedTimer();
  void ClearSuspendedTimer();

  PluginInstanceChild* mInstance;
  NPStream mStream;
  bool mClosed;
  enum {
    CONSTRUCTING,
    ALIVE,
    DYING,
    DELETING
  } mState;
  nsCString mURL;
  nsCString mHeaders;

  static const NPReason kDestroyNotPending = -1;

  /**
   * When NPP_DestroyStream is delivered with pending data, we postpone
   * delivering or acknowledging it until the pending data has been written.
   *
   * The default/initial value is kDestroyNotPending
   */
  NPReason mDestroyPending;

  struct PendingData
  {
    int32_t offset;
    Buffer data;
    int32_t curpos;
  };
  nsTArray<PendingData> mPendingData;

  /**
   * Asynchronous RecvWrite messages are never delivered to the plugin
   * immediately, because that may be in the midst of an unexpected RPC
   * stack frame. It instead posts a runnable using this tracker to cancel
   * in case we are destroyed.
   */
  ScopedRunnableMethodFactory<BrowserStreamChild> mDeliverDataTracker;
  base::RepeatingTimer<BrowserStreamChild> mSuspendedTimer;
};

} // namespace plugins
} // namespace mozilla

#endif /* mozilla_plugins_BrowserStreamChild_h */
