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

 WebRtcIlbcfix_EnhUpsample.h

******************************************************************/

#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENH_UPSAMPLE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ENH_UPSAMPLE_H_

#include "defines.h"

/*----------------------------------------------------------------*
 * upsample finite array assuming zeros outside bounds
 *---------------------------------------------------------------*/

void WebRtcIlbcfix_EnhUpsample(
    WebRtc_Word32 *useq1, /* (o) upsampled output sequence */
    WebRtc_Word16 *seq1 /* (i) unupsampled sequence */
                                );

#endif
