/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsDOMWifiEvent_h__
#define nsDOMWifiEvent_h__

#include "nsIWifi.h"
#include "nsIWifiEventInits.h"
#include "nsDOMEvent.h"

class nsDOMMozWifiConnectionInfoEvent : public nsDOMEvent,
                                        public nsIDOMMozWifiConnectionInfoEvent
{
public:
  nsDOMMozWifiConnectionInfoEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent) {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMMozWifiConnectionInfoEvent, nsDOMEvent)
  // Forward to base class
  NS_FORWARD_TO_NSDOMEVENT

  NS_DECL_NSIDOMMOZWIFICONNECTIONINFOEVENT

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx, jsval* aVal);
private:
  nsCOMPtr<nsIVariant> mNetwork;
  int16_t mSignalStrength;
  int16_t mRelSignalStrength;
  int32_t mLinkSpeed;
  nsString mIpAddress;
};

#endif // nsDOMWifiEvent_h__
