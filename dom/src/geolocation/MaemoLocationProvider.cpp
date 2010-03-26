/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Geolocation.
 *
 * The Initial Developer of the Original Code is Mozilla Corporation
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Oleg Romashin <romaxa@gmail.com>  (Original Author)
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

#include <stdio.h>
#include <math.h>
#include "MaemoLocationProvider.h"
#include "nsIClassInfo.h"
#include "nsDOMClassInfoID.h"
#include "nsIDOMClassInfo.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsServiceManagerUtils.h"

////////////////////////////////////////////////////
// nsGeoPositionCoords
////////////////////////////////////////////////////

class nsGeoPositionCoords : public nsIDOMGeoPositionCoords
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITIONCOORDS

  nsGeoPositionCoords(double aLat, double aLong, double aAlt, double aHError,
                      double aVError, double aHeading, double aSpeed) :
    mLat(aLat), mLong(aLong), mAlt(aAlt), mHError(aHError),
    mVError(aVError), mHeading(aHeading), mSpeed(aSpeed) { };
private:
  ~nsGeoPositionCoords() { }
  double mLat, mLong, mAlt, mHError, mVError, mHeading, mSpeed;
};

NS_INTERFACE_MAP_BEGIN(nsGeoPositionCoords)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoPositionCoords)
NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPositionCoords)
NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GeoPositionCoords)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsGeoPositionCoords)
NS_IMPL_THREADSAFE_RELEASE(nsGeoPositionCoords)

NS_IMETHODIMP
nsGeoPositionCoords::GetLatitude(double *aLatitude)
{
  *aLatitude = mLat;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetLongitude(double *aLongitude)
{
  *aLongitude = mLong;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetAltitude(double *aAltitude)
{
  *aAltitude = mAlt;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetAccuracy(double *aAccuracy)
{
  *aAccuracy = mHError;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetAltitudeAccuracy(double *aAltitudeAccuracy)
{
  *aAltitudeAccuracy = mVError;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetHeading(double *aHeading)
{
  *aHeading = mHeading;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetSpeed(double *aSpeed)
{
  *aSpeed = mSpeed;
  return NS_OK;
}

////////////////////////////////////////////////////
// nsGeoPosition
////////////////////////////////////////////////////

class nsGeoPosition : public nsIDOMGeoPosition
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITION

  nsGeoPosition(double aLat, double aLong, double aAlt, double aHError,
                double aVError, double aHeading, double aSpeed,
                long long aTimestamp): mTimestamp(aTimestamp)
  {
    mCoords = new nsGeoPositionCoords(aLat, aLong, aAlt, aHError,
                                      aVError, aHeading, aSpeed);
    NS_ASSERTION(mCoords, "null mCoords in nsGeoPosition");
  };

private:
  ~nsGeoPosition() {}
  long long mTimestamp;
  nsRefPtr<nsGeoPositionCoords> mCoords;
};

NS_INTERFACE_MAP_BEGIN(nsGeoPosition)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoPosition)
NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPosition)
NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GeoPosition)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsGeoPosition)
NS_IMPL_THREADSAFE_RELEASE(nsGeoPosition)

NS_IMETHODIMP
nsGeoPosition::GetTimestamp(DOMTimeStamp* aTimestamp)
{
  *aTimestamp = mTimestamp;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetCoords(nsIDOMGeoPositionCoords * *aCoords)
{
  NS_IF_ADDREF(*aCoords = mCoords);
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetAddress(nsIDOMGeoPositionAddress** aAddress)
{
  *aAddress = nsnull;
  return NS_OK;
}

NS_IMPL_ISUPPORTS2(MaemoLocationProvider, nsIGeolocationProvider, nsITimerCallback)

MaemoLocationProvider::MaemoLocationProvider() :
  mLocationChanged(0),
  mControlError(0),
  mDeviceDisconnected(0),
  mControlStopped(0),
  mHasSeenLocation(PR_FALSE),
  mHasGPS(PR_TRUE),
  mGPSControl(nsnull),
  mGPSDevice(nsnull),
  mIgnoreMinorChanges(PR_FALSE),
  mPrevLat(0.0),
  mPrevLong(0.0),
  mInterval(LOCATION_INTERVAL_5S),
  mIgnoreBigHErr(PR_TRUE),
  mMaxHErr(1000),
  mIgnoreBigVErr(PR_TRUE),
  mMaxVErr(100)
{
}

MaemoLocationProvider::~MaemoLocationProvider()
{
}

void MaemoLocationProvider::DeviceDisconnected(LocationGPSDevice* device, void* self)
{
}

void MaemoLocationProvider::ControlStopped(LocationGPSDControl* device, void* self)
{
  MaemoLocationProvider* provider = static_cast<MaemoLocationProvider*>(self);
  provider->StartControl();
}

void MaemoLocationProvider::ControlError(LocationGPSDControl* control, void* self)
{
}

void MaemoLocationProvider::LocationChanged(LocationGPSDevice* device, void* self)
{
  if (!device || !device->fix)
    return;

  guint32 &fields = device->fix->fields;
  if (!(fields & LOCATION_GPS_DEVICE_LATLONG_SET))
    return;

  if (!(device->fix->eph && !isnan(device->fix->eph)))
    return;

  MaemoLocationProvider* provider = static_cast<MaemoLocationProvider*>(self);
  NS_ENSURE_TRUE(provider, );
  provider->LocationUpdate(device);
}

nsresult
MaemoLocationProvider::LocationUpdate(LocationGPSDevice* aDev)
{
  double hErr = aDev->fix->eph/100;
  if (mIgnoreBigHErr && hErr > (double)mMaxHErr)
    hErr = (double)mMaxHErr;

  double vErr = aDev->fix->epv/2;
  if (mIgnoreBigVErr && vErr > (double)mMaxVErr)
    vErr = (double)mMaxVErr;

  double altitude = 0, speed = 0, track = 0;
  if (aDev->fix->epv && !isnan(aDev->fix->epv))
    altitude = aDev->fix->altitude;
  if (aDev->fix->eps && !isnan(aDev->fix->eps))
    speed = aDev->fix->speed;
  if (aDev->fix->epd && !isnan(aDev->fix->epd))
    track = aDev->fix->track;

#ifdef DEBUG
  double dist = location_distance_between(mPrevLat, mPrevLong, aDev->fix->latitude, aDev->fix->longitude)*1000;
  fprintf(stderr, "dist:%.9f, Lat: %.6f, Long:%.6f, HErr:%g, Alt:%.6f, VErr:%g, dir:%g[%g], sp:%g[%g]\n",
           dist, aDev->fix->latitude, aDev->fix->longitude,
           hErr, altitude,
           aDev->fix->epv/2, track, aDev->fix->epd,
           speed, aDev->fix->eps);
  mPrevLat = aDev->fix->latitude;
  mPrevLong = aDev->fix->longitude;
#endif

  nsRefPtr<nsGeoPosition> somewhere = new nsGeoPosition(aDev->fix->latitude,
                                                        aDev->fix->longitude,
                                                        altitude,
                                                        hErr,
                                                        vErr,
                                                        track,
                                                        speed,
                                                        PR_Now());
  Update(somewhere);

  return NS_OK;
}

NS_IMETHODIMP
MaemoLocationProvider::Notify(nsITimer* aTimer)
{
  LocationChanged(mGPSDevice, this);
  return NS_OK;
}

nsresult
MaemoLocationProvider::StartControl()
{
  if (mGPSControl)
    return NS_OK;

  mGPSControl = location_gpsd_control_get_default();
  NS_ENSURE_TRUE(mGPSControl, NS_ERROR_FAILURE);

  g_object_set(G_OBJECT(mGPSControl),
               "preferred-interval", mInterval,
               "preferred-method", LOCATION_METHOD_USER_SELECTED,
                NULL);

  mControlError = g_signal_connect(mGPSControl, "error",
                                   G_CALLBACK(ControlError), this);

  mControlStopped = g_signal_connect(mGPSControl, "gpsd_stopped",
                                     G_CALLBACK(ControlStopped), this);

  location_gpsd_control_start(mGPSControl);
  return NS_OK;
}

nsresult
MaemoLocationProvider::StartDevice()
{
  if (mGPSDevice)
    return NS_OK;

  mGPSDevice = (LocationGPSDevice*)g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);
  NS_ENSURE_TRUE(mGPSDevice, NS_ERROR_FAILURE);

  mLocationChanged    = g_signal_connect(mGPSDevice, "changed",
                                         G_CALLBACK(LocationChanged), this);

  mDeviceDisconnected = g_signal_connect(mGPSDevice, "disconnected",
                                         G_CALLBACK(DeviceDisconnected), this);
  return NS_OK;
}

NS_IMETHODIMP MaemoLocationProvider::Startup()
{
  nsresult rv(NS_OK);

  PRInt32 freqVal = 5;
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    rv = prefs->GetIntPref("geo.location_update_freq", &freqVal);
    switch (freqVal) {
      case 0:
        mInterval = LOCATION_INTERVAL_DEFAULT;
        break;
      case 1:
        mInterval = LOCATION_INTERVAL_1S;
        break;
      case 2:
        mInterval = LOCATION_INTERVAL_2S;
        break;
      case 5:
        mInterval = LOCATION_INTERVAL_5S;
        break;
      case 10:
        mInterval = LOCATION_INTERVAL_10S;
        break;
      case 20:
        mInterval = LOCATION_INTERVAL_20S;
        break;
      case 30:
        mInterval = LOCATION_INTERVAL_30S;
        break;
      case 60:
        mInterval = LOCATION_INTERVAL_60S;
        break;
      case 120:
        mInterval = LOCATION_INTERVAL_120S;
        break;
      default:
        mInterval = LOCATION_INTERVAL_DEFAULT;
        break;
    }
  }

  rv = StartControl();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StartDevice();
  NS_ENSURE_SUCCESS(rv, rv);

  if (prefs)
    prefs->GetBoolPref("geo.herror.ignore.big", &mIgnoreBigHErr);

  if (mIgnoreBigHErr)
    prefs->GetIntPref("geo.herror.max.value", &mMaxHErr);

  if (prefs)
    prefs->GetBoolPref("geo.verror.ignore.big", &mIgnoreBigVErr);

  if (mIgnoreBigVErr)
    prefs->GetIntPref("geo.verror.max.value", &mMaxVErr);

  if (mUpdateTimer)
    return NS_OK;

  PRInt32 update = 0; //0 second no timer created
  if (prefs)
    prefs->GetIntPref("geo.default.update", &update);

  if (!update)
    return NS_OK;

  mUpdateTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);

  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  if (update)
    mUpdateTimer->InitWithCallback(this, update, nsITimer::TYPE_REPEATING_SLACK);

  return NS_OK;
}

NS_IMETHODIMP MaemoLocationProvider::Watch(nsIGeolocationUpdate *callback)
{
  if (mCallback)
    return NS_OK;

  mCallback = callback;
  return NS_OK;
}

NS_IMETHODIMP MaemoLocationProvider::Shutdown()
{
  if (mUpdateTimer)
    mUpdateTimer->Cancel();

  g_signal_handler_disconnect(mGPSDevice, mLocationChanged);
  g_signal_handler_disconnect(mGPSDevice, mDeviceDisconnected);

  g_signal_handler_disconnect(mGPSDevice, mControlError);
  g_signal_handler_disconnect(mGPSDevice, mControlStopped);

  mHasSeenLocation = PR_FALSE;
  mCallback = nsnull;

  if (mGPSControl) {
    location_gpsd_control_stop(mGPSControl);
    g_object_unref(mGPSControl);
    mGPSControl = nsnull;
  }
  if (mGPSDevice) {
    g_object_unref(mGPSDevice);
    mGPSDevice = nsnull;
  }

  return NS_OK;
}

void MaemoLocationProvider::Update(nsIDOMGeoPosition* aPosition)
{
  mHasSeenLocation = PR_TRUE;
  if (mCallback)
    mCallback->Update(aPosition);
}

