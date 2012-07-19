/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=40: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_bluetooth_bluetoothdevice_h__
#define mozilla_dom_bluetooth_bluetoothdevice_h__

#include "BluetoothCommon.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMBluetoothDevice.h"
#include "nsString.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothNamedValue;
class BluetoothSignal;

class BluetoothDevice : public nsDOMEventTargetHelper
                      , public nsIDOMBluetoothDevice
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMBLUETOOTHDEVICE

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothDevice,
                                           nsDOMEventTargetHelper)

  static already_AddRefed<BluetoothDevice>
  Create(nsPIDOMWindow* aOwner, const BluetoothSignal& aSignal);

  nsIDOMEventTarget*
  ToIDOMEventTarget() const
  {
    return static_cast<nsDOMEventTargetHelper*>(
      const_cast<BluetoothDevice*>(this));
  }

  nsISupports*
  ToISupports() const
  {
    return ToIDOMEventTarget();
  }
  
private:
  BluetoothDevice(const BluetoothSignal& aSignal);
  ~BluetoothDevice() {}  
  void SetPropertyByValue(const BluetoothNamedValue& aValue);
  
  nsString mAddress;
  nsString mName;
  PRUint32 mClass;
  bool mConnected;
  bool mPaired;
  nsTArray<nsString> mUuids;

  NS_DECL_EVENT_HANDLER(propertychanged)
};

END_BLUETOOTH_NAMESPACE

#endif
