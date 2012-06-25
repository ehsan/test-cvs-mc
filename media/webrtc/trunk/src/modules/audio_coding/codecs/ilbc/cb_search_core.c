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

 WebRtcIlbcfix_CbSearchCore.c

******************************************************************/

#include "defines.h"
#include "constants.h"

void WebRtcIlbcfix_CbSearchCore(
    WebRtc_Word32 *cDot,    /* (i) Cross Correlation */
    WebRtc_Word16 range,    /* (i) Search range */
    WebRtc_Word16 stage,    /* (i) Stage of this search */
    WebRtc_Word16 *inverseEnergy,  /* (i) Inversed energy */
    WebRtc_Word16 *inverseEnergyShift, /* (i) Shifts of inversed energy
                                           with the offset 2*16-29 */
    WebRtc_Word32 *Crit,    /* (o) The criteria */
    WebRtc_Word16 *bestIndex,   /* (o) Index that corresponds to
                                                   maximum criteria (in this
                                                   vector) */
    WebRtc_Word32 *bestCrit,   /* (o) Value of critera for the
                                                   chosen index */
    WebRtc_Word16 *bestCritSh)   /* (o) The domain of the chosen
                                                   criteria */
{
  WebRtc_Word32 maxW32, tmp32;
  WebRtc_Word16 max, sh, tmp16;
  int i;
  WebRtc_Word32 *cDotPtr;
  WebRtc_Word16 cDotSqW16;
  WebRtc_Word16 *inverseEnergyPtr;
  WebRtc_Word32 *critPtr;
  WebRtc_Word16 *inverseEnergyShiftPtr;

  /* Don't allow negative values for stage 0 */
  if (stage==0) {
    cDotPtr=cDot;
    for (i=0;i<range;i++) {
      *cDotPtr=WEBRTC_SPL_MAX(0, (*cDotPtr));
      cDotPtr++;
    }
  }

  /* Normalize cDot to WebRtc_Word16, calculate the square of cDot and store the upper WebRtc_Word16 */
  maxW32 = WebRtcSpl_MaxAbsValueW32(cDot, range);

  sh = (WebRtc_Word16)WebRtcSpl_NormW32(maxW32);
  cDotPtr = cDot;
  inverseEnergyPtr = inverseEnergy;
  critPtr = Crit;
  inverseEnergyShiftPtr=inverseEnergyShift;
  max=WEBRTC_SPL_WORD16_MIN;

  for (i=0;i<range;i++) {
    /* Calculate cDot*cDot and put the result in a WebRtc_Word16 */
    tmp32 = WEBRTC_SPL_LSHIFT_W32(*cDotPtr,sh);
    tmp16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp32,16);
    cDotSqW16 = (WebRtc_Word16)(((WebRtc_Word32)(tmp16)*(tmp16))>>16);

    /* Calculate the criteria (cDot*cDot/energy) */
    *critPtr=WEBRTC_SPL_MUL_16_16(cDotSqW16, (*inverseEnergyPtr));

    /* Extract the maximum shift value under the constraint
       that the criteria is not zero */
    if ((*critPtr)!=0) {
      max = WEBRTC_SPL_MAX((*inverseEnergyShiftPtr), max);
    }

    inverseEnergyPtr++;
    inverseEnergyShiftPtr++;
    critPtr++;
    cDotPtr++;
  }

  /* If no max shifts still at initialization value, set shift to zero */
  if (max==WEBRTC_SPL_WORD16_MIN) {
    max = 0;
  }

  /* Modify the criterias, so that all of them use the same Q domain */
  critPtr=Crit;
  inverseEnergyShiftPtr=inverseEnergyShift;
  for (i=0;i<range;i++) {
    /* Guarantee that the shift value is less than 16
       in order to simplify for DSP's (and guard against >31) */
    tmp16 = WEBRTC_SPL_MIN(16, max-(*inverseEnergyShiftPtr));

    (*critPtr)=WEBRTC_SPL_SHIFT_W32((*critPtr),-tmp16);
    critPtr++;
    inverseEnergyShiftPtr++;
  }

  /* Find the index of the best value */
  *bestIndex = WebRtcSpl_MaxIndexW32(Crit, range);
  *bestCrit = Crit[*bestIndex];

  /* Calculate total shifts of this criteria */
  *bestCritSh = 32 - 2*sh + max;

  return;
}
