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

 WebRtcIlbcfix_Lsf2Lsp.c

******************************************************************/

#include "defines.h"
#include "constants.h"

/*----------------------------------------------------------------*
 *  conversion from lsf to lsp coefficients
 *---------------------------------------------------------------*/

void WebRtcIlbcfix_Lsf2Lsp(
    WebRtc_Word16 *lsf, /* (i) lsf in Q13 values between 0 and pi */
    WebRtc_Word16 *lsp, /* (o) lsp in Q15 values between -1 and 1 */
    WebRtc_Word16 m  /* (i) number of coefficients */
                           ) {
  WebRtc_Word16 i, k;
  WebRtc_Word16 diff; /* difference, which is used for the
                           linear approximation (Q8) */
  WebRtc_Word16 freq; /* normalized frequency in Q15 (0..1) */
  WebRtc_Word32 tmpW32;

  for(i=0; i<m; i++)
  {
    freq = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(lsf[i], 20861, 15);
    /* 20861: 1.0/(2.0*PI) in Q17 */
    /*
       Upper 8 bits give the index k and
       Lower 8 bits give the difference, which needs
       to be approximated linearly
    */
    k = WEBRTC_SPL_RSHIFT_W16(freq, 8);
    diff = (freq&0x00ff);

    /* Guard against getting outside table */

    if (k>63) {
      k = 63;
    }

    /* Calculate linear approximation */
    tmpW32 = WEBRTC_SPL_MUL_16_16(WebRtcIlbcfix_kCosDerivative[k], diff);
    lsp[i] = WebRtcIlbcfix_kCos[k]+(WebRtc_Word16)(WEBRTC_SPL_RSHIFT_W32(tmpW32, 12));
  }

  return;
}
