/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/basictypes.h"
#include "BluetoothAdapter.h"
#include "BluetoothDevice.h"
#include "BluetoothPropertyEvent.h"
#include "BluetoothService.h"
#include "BluetoothTypes.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothUtils.h"
#include "GeneratedEvents.h"

#include "nsContentUtils.h"
#include "nsDOMClassInfo.h"
#include "nsDOMEvent.h"
#include "nsIDOMBluetoothAuthorizeEvent.h"
#include "nsIDOMBluetoothDeviceEvent.h"
#include "nsIDOMBluetoothDeviceAddressEvent.h"
#include "nsIDOMBluetoothPairingEvent.h"
#include "nsIDOMDOMRequest.h"
#include "nsThreadUtils.h"
#include "nsXPCOMCIDInternal.h"

#include "mozilla/LazyIdleThread.h"
#include "mozilla/Util.h"

using namespace mozilla;

USING_BLUETOOTH_NAMESPACE

DOMCI_DATA(BluetoothAdapter, BluetoothAdapter)

NS_IMPL_CYCLE_COLLECTION_CLASS(BluetoothAdapter)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(BluetoothAdapter,
                                               nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mJsUuids)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mJsDeviceAddresses)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BluetoothAdapter, 
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(devicefound)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(devicedisappeared)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(propertychanged)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(requestconfirmation)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(requestpincode)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(requestpasskey)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(authorize)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(cancel)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BluetoothAdapter, 
                                                nsDOMEventTargetHelper)
  tmp->Unroot();
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(devicefound)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(devicedisappeared)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(propertychanged)  
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(requestconfirmation)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(requestpincode)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(requestpasskey)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(authorize)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(cancel)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BluetoothAdapter)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBluetoothAdapter)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BluetoothAdapter)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothAdapter, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothAdapter, nsDOMEventTargetHelper)

class GetPairedDevicesTask : public BluetoothReplyRunnable
{
public:
  GetPairedDevicesTask(BluetoothAdapter* aAdapterPtr,
                       nsIDOMDOMRequest* aReq) :
    mAdapterPtr(aAdapterPtr),
    BluetoothReplyRunnable(aReq)
  {
    MOZ_ASSERT(aReq && aAdapterPtr);
  }

  virtual bool ParseSuccessfulReply(jsval* aValue)
  {
    *aValue = JSVAL_VOID;
    BluetoothValue& v = mReply->get_BluetoothReplySuccess().value();
    if (v.type() != BluetoothValue::TArrayOfBluetoothNamedValue) {
      NS_WARNING("Not a BluetoothNamedValue array!");
      return false;
    }

    const InfallibleTArray<BluetoothNamedValue>& reply =
      mReply->get_BluetoothReplySuccess().value().get_ArrayOfBluetoothNamedValue();
    nsTArray<nsRefPtr<BluetoothDevice> > devices;
    JSObject* JsDevices;
    for (uint32_t i = 0; i < reply.Length(); i++) {
      if (reply[i].value().type() != BluetoothValue::TArrayOfBluetoothNamedValue) {
        NS_WARNING("Not a BluetoothNamedValue array!");
        return false;
      }
      nsRefPtr<BluetoothDevice> d = BluetoothDevice::Create(mAdapterPtr->GetOwner(),
                                                            mAdapterPtr->GetPath(),
                                                            reply[i].value());
      devices.AppendElement(d);
    }

    nsresult rv;
    nsIScriptContext* sc = mAdapterPtr->GetContextForEventHandlers(&rv);
    if (!sc) {
      NS_WARNING("Cannot create script context!");
      return false;
    }

    rv = BluetoothDeviceArrayToJSArray(sc->GetNativeContext(),
                                       sc->GetNativeGlobal(), devices, &JsDevices);

    if (JsDevices) {
      aValue->setObject(*JsDevices);
    }
    else {
      NS_WARNING("Paird not yet set!");
      return false;
    }
    return true;
  }

  void
  ReleaseMembers()
  {
    BluetoothReplyRunnable::ReleaseMembers();
    mAdapterPtr = nullptr;
  }
private:
  nsRefPtr<BluetoothAdapter> mAdapterPtr;
};

static int kCreatePairedDeviceTimeout = 50000; // unit: msec

BluetoothAdapter::BluetoothAdapter(nsPIDOMWindow* aOwner, const BluetoothValue& aValue)
    : BluetoothPropertyContainer(BluetoothObjectType::TYPE_ADAPTER)
    , mEnabled(false)
    , mDiscoverable(false)
    , mDiscovering(false)
    , mPairable(false)
    , mPowered(false)
    , mJsUuids(nullptr)
    , mJsDeviceAddresses(nullptr)
    , mIsRooted(false)
{
  BindToOwner(aOwner);
  const InfallibleTArray<BluetoothNamedValue>& values =
    aValue.get_ArrayOfBluetoothNamedValue();
  for (uint32_t i = 0; i < values.Length(); ++i) {
    SetPropertyByValue(values[i]);
  }
}

BluetoothAdapter::~BluetoothAdapter()
{
  BluetoothService* bs = BluetoothService::Get();
  // We can be null on shutdown, where this might happen
  if (bs) {
    if (NS_FAILED(bs->UnregisterBluetoothSignalHandler(mPath, this))) {
      NS_WARNING("Failed to unregister object with observer!");
    }
  }
  Unroot();
}

void
BluetoothAdapter::Unroot()
{
  if (!mIsRooted) {
    return;
  }
  NS_DROP_JS_OBJECTS(this, BluetoothAdapter);
  mIsRooted = false;
}

void
BluetoothAdapter::Root()
{
  if (mIsRooted) {
    return;
  }
  NS_HOLD_JS_OBJECTS(this, BluetoothAdapter);
  mIsRooted = true;
}

void
BluetoothAdapter::SetPropertyByValue(const BluetoothNamedValue& aValue)
{
  const nsString& name = aValue.name();
  const BluetoothValue& value = aValue.value();
  if (name.EqualsLiteral("Name")) {
    mName = value.get_nsString();
  } else if (name.EqualsLiteral("Address")) {
    mAddress = value.get_nsString();
  } else if (name.EqualsLiteral("Path")) {
    mPath = value.get_nsString();
  } else if (name.EqualsLiteral("Enabled")) {
    mEnabled = value.get_bool();
  } else if (name.EqualsLiteral("Discoverable")) {
    mDiscoverable = value.get_bool();
  } else if (name.EqualsLiteral("Discovering")) {
    mDiscovering = value.get_bool();
  } else if (name.EqualsLiteral("Pairable")) {
    mPairable = value.get_bool();
  } else if (name.EqualsLiteral("Powered")) {
    mPowered = value.get_bool();
  } else if (name.EqualsLiteral("PairableTimeout")) {
    mPairableTimeout = value.get_uint32_t();
  } else if (name.EqualsLiteral("DiscoverableTimeout")) {
    mDiscoverableTimeout = value.get_uint32_t();
  } else if (name.EqualsLiteral("Class")) {
    mClass = value.get_uint32_t();
  } else if (name.EqualsLiteral("UUIDs")) {
    mUuids = value.get_ArrayOfnsString();
    nsresult rv;
    nsIScriptContext* sc = GetContextForEventHandlers(&rv);
    if (sc) {
      rv =
        StringArrayToJSArray(sc->GetNativeContext(),
                             sc->GetNativeGlobal(), mUuids, &mJsUuids);
      if (NS_FAILED(rv)) {
        NS_WARNING("Cannot set JS UUIDs object!");
        return;
      }
      Root();
    } else {
      NS_WARNING("Could not get context!");
    }
  } else if (name.EqualsLiteral("Devices")) {
    mDeviceAddresses = value.get_ArrayOfnsString();
    nsresult rv;
    nsIScriptContext* sc = GetContextForEventHandlers(&rv);
    if (sc) {
      rv =
        StringArrayToJSArray(sc->GetNativeContext(),
                             sc->GetNativeGlobal(), mDeviceAddresses, &mJsDeviceAddresses);
      if (NS_FAILED(rv)) {
        NS_WARNING("Cannot set JS Device Addresses object!");
        return;
      }
      Root();
    } else {
      NS_WARNING("Could not get context!");
    }
  } else {
#ifdef DEBUG
    nsCString warningMsg;
    warningMsg.AssignLiteral("Not handling adapter property: ");
    warningMsg.Append(NS_ConvertUTF16toUTF8(name));
    NS_WARNING(warningMsg.get());
#endif
  }
}

// static
already_AddRefed<BluetoothAdapter>
BluetoothAdapter::Create(nsPIDOMWindow* aOwner, const BluetoothValue& aValue)
{
  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return nullptr;
  }

  nsRefPtr<BluetoothAdapter> adapter = new BluetoothAdapter(aOwner, aValue);
  if (NS_FAILED(bs->RegisterBluetoothSignalHandler(adapter->GetPath(), adapter))) {
    NS_WARNING("Failed to register object with observer!");
    return nullptr;
  }

  if (NS_FAILED(bs->RegisterBluetoothSignalHandler(NS_LITERAL_STRING(LOCAL_AGENT_PATH), adapter))) {
    NS_WARNING("Failed to register local agent object with observer!");
    return nullptr;
  }

  if (NS_FAILED(bs->RegisterBluetoothSignalHandler(NS_LITERAL_STRING(REMOTE_AGENT_PATH), adapter))) {
    NS_WARNING("Failed to register remote agent object with observer!");
    return nullptr;
  }

  return adapter.forget();
}

void
BluetoothAdapter::Notify(const BluetoothSignal& aData)
{
  InfallibleTArray<BluetoothNamedValue> arr;

  if (aData.name().EqualsLiteral("DeviceFound")) {
    nsRefPtr<BluetoothDevice> device = BluetoothDevice::Create(GetOwner(), mPath, aData.value());
    nsCOMPtr<nsIDOMEvent> event;
    NS_NewDOMBluetoothDeviceEvent(getter_AddRefs(event), nullptr, nullptr);

    nsCOMPtr<nsIDOMBluetoothDeviceEvent> e = do_QueryInterface(event);
    e->InitBluetoothDeviceEvent(NS_LITERAL_STRING("devicefound"),
                                false, false, device);
    e->SetTrusted(true);
    bool dummy;
    DispatchEvent(event, &dummy);
  } else if (aData.name().EqualsLiteral("DeviceDisappeared")) {
		const nsAString& deviceAddress = aData.value().get_nsString();

    nsCOMPtr<nsIDOMEvent> event;
    NS_NewDOMBluetoothDeviceAddressEvent(getter_AddRefs(event), nullptr, nullptr);

    nsCOMPtr<nsIDOMBluetoothDeviceAddressEvent> e = do_QueryInterface(event);
    e->InitBluetoothDeviceAddressEvent(NS_LITERAL_STRING("devicedisappeared"),
                                       false, false, deviceAddress);
    e->SetTrusted(true);
    bool dummy;
    DispatchEvent(event, &dummy);
  } else if (aData.name().EqualsLiteral("PropertyChanged")) {
    // Get BluetoothNamedValue, make sure array length is 1
    arr = aData.value().get_ArrayOfBluetoothNamedValue();

    NS_ASSERTION(arr.Length() == 1, "Got more than one property in a change message!");
    NS_ASSERTION(arr[0].value().type() == BluetoothValue::TArrayOfBluetoothNamedValue,
                 "PropertyChanged: Invalid value type");

    BluetoothNamedValue v = arr[0];
    SetPropertyByValue(v);
    nsRefPtr<BluetoothPropertyEvent> e = BluetoothPropertyEvent::Create(v.name());
    e->Dispatch(ToIDOMEventTarget(), NS_LITERAL_STRING("propertychanged"));
  } else if (aData.name().EqualsLiteral("RequestConfirmation")) {
    arr = aData.value().get_ArrayOfBluetoothNamedValue();

    NS_ASSERTION(arr.Length() == 2, "RequestConfirmation: Wrong length of parameters");
    NS_ASSERTION(arr[0].value().type() == BluetoothValue::TnsString,
                 "RequestConfirmation: Invalid value type");
    NS_ASSERTION(arr[1].value().type() == BluetoothValue::Tuint32_t,
                 "RequestConfirmation: Invalid value type");

    nsCOMPtr<nsIDOMEvent> event;
    NS_NewDOMBluetoothPairingEvent(getter_AddRefs(event), nullptr, nullptr);

    nsCOMPtr<nsIDOMBluetoothPairingEvent> e = do_QueryInterface(event);
    e->InitBluetoothPairingEvent(NS_LITERAL_STRING("requestconfirmation"),
                                 false,
                                 false,
                                 arr[0].value().get_nsString(),
                                 arr[1].value().get_uint32_t());
    e->SetTrusted(true);
    bool dummy;
    DispatchEvent(event, &dummy);
  } else if (aData.name().EqualsLiteral("RequestPinCode")) {
    arr = aData.value().get_ArrayOfBluetoothNamedValue();

    NS_ASSERTION(arr.Length() == 1, "RequestPinCode: Wrong length of parameters");
    NS_ASSERTION(arr[0].value().type() == BluetoothValue::TnsString,
                 "RequestPinCode: Invalid value type");

    nsCOMPtr<nsIDOMEvent> event;
    NS_NewDOMBluetoothDeviceAddressEvent(getter_AddRefs(event), nullptr, nullptr);

    nsCOMPtr<nsIDOMBluetoothDeviceAddressEvent> e = do_QueryInterface(event);
    e->InitBluetoothDeviceAddressEvent(NS_LITERAL_STRING("requestpincode"),
                                       false, false, arr[0].value().get_nsString());
    e->SetTrusted(true);
    bool dummy;
    DispatchEvent(event, &dummy);
  } else if (aData.name().EqualsLiteral("RequestPasskey")) {
    arr = aData.value().get_ArrayOfBluetoothNamedValue();

    NS_ASSERTION(arr.Length() == 1, "RequestPasskey: Wrong length of parameters");
    NS_ASSERTION(arr[0].value().type() == BluetoothValue::TnsString,
                 "RequestPasskey: Invalid value type");

    nsCOMPtr<nsIDOMEvent> event;
    NS_NewDOMBluetoothDeviceAddressEvent(getter_AddRefs(event), nullptr, nullptr);

    nsCOMPtr<nsIDOMBluetoothDeviceAddressEvent> e = do_QueryInterface(event);
    e->InitBluetoothDeviceAddressEvent(NS_LITERAL_STRING("requestpasskey"),
                                       false, false, arr[0].value().get_nsString());
    e->SetTrusted(true);
    bool dummy;
    DispatchEvent(event, &dummy);
  } else if (aData.name().EqualsLiteral("Authorize")) {
    arr = aData.value().get_ArrayOfBluetoothNamedValue();

    NS_ASSERTION(arr.Length() == 2, "Authorize: Wrong length of parameters");
    NS_ASSERTION(arr[0].value().type() == BluetoothValue::TnsString,
                 "Authorize: Invalid value type");
    NS_ASSERTION(arr[1].value().type() == BluetoothValue::TnsString,
                 "Authorize: Invalid value type");

    nsCOMPtr<nsIDOMEvent> event;
    NS_NewDOMBluetoothAuthorizeEvent(getter_AddRefs(event), nullptr, nullptr);

    nsCOMPtr<nsIDOMBluetoothAuthorizeEvent> e = do_QueryInterface(event);
    e->InitBluetoothAuthorizeEvent(NS_LITERAL_STRING("authorize"),
                                   false,
                                   false,
                                   arr[0].value().get_nsString(),
                                   arr[1].value().get_nsString());
    e->SetTrusted(true);
    bool dummy;
    DispatchEvent(event, &dummy);
  } else if (aData.name().EqualsLiteral("Cancel")) {
    nsCOMPtr<nsIDOMEvent> event;
    NS_NewDOMBluetoothDeviceAddressEvent(getter_AddRefs(event), nullptr, nullptr);

    nsCOMPtr<nsIDOMBluetoothDeviceAddressEvent> e = do_QueryInterface(event);
    // Just send a null nsString, won't be used
    e->InitBluetoothDeviceAddressEvent(NS_LITERAL_STRING("cancel"),
                                       false, false, EmptyString());
    e->SetTrusted(true);
    bool dummy;
    DispatchEvent(event, &dummy);
  } else {
#ifdef DEBUG
    nsCString warningMsg;
    warningMsg.AssignLiteral("Not handling manager signal: ");
    warningMsg.Append(NS_ConvertUTF16toUTF8(aData.name()));
    NS_WARNING(warningMsg.get());
#endif
  }
}

nsresult
BluetoothAdapter::StartStopDiscovery(bool aStart, nsIDOMDOMRequest** aRequest)
{
  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMRequestService> rs = do_GetService("@mozilla.org/dom/dom-request-service;1");    
  if (!rs) {
    NS_WARNING("No DOMRequest Service!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMDOMRequest> req;
  nsresult rv = rs->CreateRequest(GetOwner(), getter_AddRefs(req));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create DOMRequest!");
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<BluetoothVoidReplyRunnable> results = new BluetoothVoidReplyRunnable(req);

  if (aStart) {
    rv = bs->StartDiscoveryInternal(mPath, results);
  } else {
    rv = bs->StopDiscoveryInternal(mPath, results);
  }
  if (NS_FAILED(rv)) {
    NS_WARNING("Starting discovery failed!");
    return NS_ERROR_FAILURE;
  }

  req.forget(aRequest);
  
  // mDiscovering is not set here, we'll get a Property update from our external
  // protocol to tell us that it's been set.
  
  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::StartDiscovery(nsIDOMDOMRequest** aRequest)
{
  return StartStopDiscovery(true, aRequest);
}

NS_IMETHODIMP
BluetoothAdapter::StopDiscovery(nsIDOMDOMRequest** aRequest)
{
  return StartStopDiscovery(false, aRequest);
}

NS_IMETHODIMP
BluetoothAdapter::GetEnabled(bool* aEnabled)
{
  *aEnabled = mEnabled;
  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::GetAddress(nsAString& aAddress)
{
  aAddress = mAddress;
  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::GetAdapterClass(PRUint32* aClass)
{
  *aClass = mClass;
  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::GetDiscovering(bool* aDiscovering)
{
  *aDiscovering = mDiscovering;
  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::GetName(nsAString& aName)
{
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::GetDiscoverable(bool* aDiscoverable)
{
  *aDiscoverable = mDiscoverable;
  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::GetDiscoverableTimeout(PRUint32* aDiscoverableTimeout)
{
  *aDiscoverableTimeout = mDiscoverableTimeout;
  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::GetDevices(JSContext* aCx, jsval* aDevices)
{
  if (mJsDeviceAddresses) {
    aDevices->setObject(*mJsDeviceAddresses);
  }
  else {
    NS_WARNING("Devices not yet set!\n");
    return NS_ERROR_FAILURE;
  }    
  return NS_OK;
}

nsresult
BluetoothAdapter::GetUuids(JSContext* aCx, jsval* aValue)
{
  if (mJsUuids) {
    aValue->setObject(*mJsUuids);
  }
  else {
    NS_WARNING("UUIDs not yet set!\n");
    return NS_ERROR_FAILURE;
  }    
  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::SetName(const nsAString& aName,
                          nsIDOMDOMRequest** aRequest)
{
  if (mName.Equals(aName)) {
    return FirePropertyAlreadySet(GetOwner(), aRequest);
  }
  nsString name(aName);
  BluetoothValue value(name);
  BluetoothNamedValue property(NS_LITERAL_STRING("Name"), value);
  return SetProperty(GetOwner(), property, aRequest);
}
 
NS_IMETHODIMP
BluetoothAdapter::SetDiscoverable(const bool aDiscoverable,
                                  nsIDOMDOMRequest** aRequest)
{
  if (aDiscoverable == mDiscoverable) {
    return FirePropertyAlreadySet(GetOwner(), aRequest);
  }
  BluetoothValue value(aDiscoverable);
  BluetoothNamedValue property(NS_LITERAL_STRING("Discoverable"), value);
  return SetProperty(GetOwner(), property, aRequest);
}
 
NS_IMETHODIMP
BluetoothAdapter::SetDiscoverableTimeout(const PRUint32 aDiscoverableTimeout,
                                         nsIDOMDOMRequest** aRequest)
{
  if (aDiscoverableTimeout == mDiscoverableTimeout) {
    return FirePropertyAlreadySet(GetOwner(), aRequest);
  }
  BluetoothValue value(aDiscoverableTimeout);
  BluetoothNamedValue property(NS_LITERAL_STRING("DiscoverableTimeout"), value);
  return SetProperty(GetOwner(), property, aRequest);
}

NS_IMETHODIMP
BluetoothAdapter::GetPairedDevices(nsIDOMDOMRequest** aRequest)
{
  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMRequestService> rs = do_GetService("@mozilla.org/dom/dom-request-service;1");
  if (!rs) {
    NS_WARNING("No DOMRequest Service!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMDOMRequest> request;
  nsresult rv = rs->CreateRequest(GetOwner(), getter_AddRefs(request));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create DOMRequest!");
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<BluetoothReplyRunnable> results = new GetPairedDevicesTask(this, request);
  if (NS_FAILED(bs->GetPairedDevicePropertiesInternal(mDeviceAddresses, results))) {
    return NS_ERROR_FAILURE;
  }
  request.forget(aRequest);
  return NS_OK;
}

nsresult
BluetoothAdapter::PairUnpair(bool aPair,
                             nsIDOMBluetoothDevice* aDevice,
                             nsIDOMDOMRequest** aRequest)
{
  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMRequestService> rs = do_GetService("@mozilla.org/dom/dom-request-service;1");
  if (!rs) {
    NS_WARNING("No DOMRequest Service!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMDOMRequest> req;
  nsresult rv = rs->CreateRequest(GetOwner(), getter_AddRefs(req));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create DOMRequest!");
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<BluetoothVoidReplyRunnable> results = new BluetoothVoidReplyRunnable(req);

  nsString addr;
  aDevice->GetAddress(addr);

  if (aPair) {
    rv = bs->CreatePairedDeviceInternal(mPath,
                                        addr,
                                        kCreatePairedDeviceTimeout,
                                        results);
  } else {
    rv = bs->RemoveDeviceInternal(mPath, addr, results);
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("Pair/Unpair failed!");
    return NS_ERROR_FAILURE;
  }

  req.forget(aRequest);

  return NS_OK;
}

nsresult
BluetoothAdapter::Pair(nsIDOMBluetoothDevice* aDevice, nsIDOMDOMRequest** aRequest)
{
  return PairUnpair(true, aDevice, aRequest);
}

nsresult
BluetoothAdapter::Unpair(nsIDOMBluetoothDevice* aDevice, nsIDOMDOMRequest** aRequest)
{
  return PairUnpair(false, aDevice, aRequest);
}

nsresult
BluetoothAdapter::SetPinCode(const nsAString& aDeviceAddress, const nsAString& aPinCode)
{
  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return NS_ERROR_FAILURE;
  }

  bool result = bs->SetPinCodeInternal(aDeviceAddress, aPinCode);

  return result ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
BluetoothAdapter::SetPasskey(const nsAString& aDeviceAddress, PRUint32 aPasskey)
{
  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return NS_ERROR_FAILURE;
  }

  bool result = bs->SetPasskeyInternal(aDeviceAddress, aPasskey);

  return result ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
BluetoothAdapter::SetPairingConfirmation(const nsAString& aDeviceAddress, bool aConfirmation)
{
  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return NS_ERROR_FAILURE;
  }

  bool result = bs->SetPairingConfirmationInternal(aDeviceAddress, aConfirmation);

  return result ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
BluetoothAdapter::SetAuthorization(const nsAString& aDeviceAddress, bool aAllow)
{
  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return NS_ERROR_FAILURE;
  }

  bool result = bs->SetAuthorizationInternal(aDeviceAddress, aAllow);

  return result ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMPL_EVENT_HANDLER(BluetoothAdapter, propertychanged)
NS_IMPL_EVENT_HANDLER(BluetoothAdapter, devicefound)
NS_IMPL_EVENT_HANDLER(BluetoothAdapter, devicedisappeared)
NS_IMPL_EVENT_HANDLER(BluetoothAdapter, requestconfirmation)
NS_IMPL_EVENT_HANDLER(BluetoothAdapter, requestpincode)
NS_IMPL_EVENT_HANDLER(BluetoothAdapter, requestpasskey)
NS_IMPL_EVENT_HANDLER(BluetoothAdapter, authorize)
NS_IMPL_EVENT_HANDLER(BluetoothAdapter, cancel)
