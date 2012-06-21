/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/******************************************************************

 iLBC Speech Coder ANSI-C Source Code

 WebRtcIlbcfix_EnergyInverse.c

******************************************************************/

/* Inverses the in vector in into Q29 domain */

#include "energy_inverse.h"

void WebRtcIlbcfix_EnergyInverse(
    WebRtc_Word16 *energy,    /* (i/o) Energy and inverse
                                                           energy (in Q29) */
    int noOfEnergies)  /* (i)   The length of the energy
                                   vector */
{
  WebRtc_Word32 Nom=(WebRtc_Word32)0x1FFFFFFF;
  WebRtc_Word16 *energyPtr;
  int i;

  /* Set the minimum energy value to 16384 to avoid overflow */
  energyPtr=energy;
  for (i=0; i<noOfEnergies; i++) {
    (*energyPtr)=WEBRTC_SPL_MAX((*energyPtr),16384);
    energyPtr++;
  }

  /* Calculate inverse energy in Q29 */
  energyPtr=energy;
  for (i=0; i<noOfEnergies; i++) {
    (*energyPtr) = (WebRtc_Word16)WebRtcSpl_DivW32W16(Nom, (*energyPtr));
    energyPtr++;
  }
}
