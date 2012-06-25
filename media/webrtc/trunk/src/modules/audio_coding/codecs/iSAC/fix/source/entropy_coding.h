/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 * entropy_coding.h
 *
 * This header file contains all of the functions used to arithmetically
 * encode the iSAC bistream
 *
 */

#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_ENTROPY_CODING_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_ENTROPY_CODING_H_

#include "structs.h"

/* decode complex spectrum (return number of bytes in stream) */
WebRtc_Word16 WebRtcIsacfix_DecodeSpec(Bitstr_dec  *streamdata,
                                       WebRtc_Word16 *frQ7,
                                       WebRtc_Word16 *fiQ7,
                                       WebRtc_Word16 AvgPitchGain_Q12);

/* encode complex spectrum */
int WebRtcIsacfix_EncodeSpec(const WebRtc_Word16 *fr,
                             const WebRtc_Word16 *fi,
                             Bitstr_enc *streamdata,
                             WebRtc_Word16 AvgPitchGain_Q12);


/* decode & dequantize LPC Coef */
int WebRtcIsacfix_DecodeLpcCoef(Bitstr_dec  *streamdata,
                                WebRtc_Word32 *LPCCoefQ17,
                                WebRtc_Word32 *gain_lo_hiQ17,
                                WebRtc_Word16 *outmodel);

int WebRtcIsacfix_DecodeLpc(WebRtc_Word32 *gain_lo_hiQ17,
                            WebRtc_Word16 *LPCCoef_loQ15,
                            WebRtc_Word16 *LPCCoef_hiQ15,
                            Bitstr_dec  *streamdata,
                            WebRtc_Word16 *outmodel);

/* quantize & code LPC Coef */
int WebRtcIsacfix_EncodeLpc(WebRtc_Word32 *gain_lo_hiQ17,
                            WebRtc_Word16 *LPCCoef_loQ15,
                            WebRtc_Word16 *LPCCoef_hiQ15,
                            WebRtc_Word16 *model,
                            WebRtc_Word32 *sizeQ11,
                            Bitstr_enc *streamdata,
                            ISAC_SaveEncData_t* encData,
                            transcode_obj *transcodeParam);

int WebRtcIsacfix_EstCodeLpcGain(WebRtc_Word32 *gain_lo_hiQ17,
                                 Bitstr_enc *streamdata,
                                 ISAC_SaveEncData_t* encData);
/* decode & dequantize RC */
int WebRtcIsacfix_DecodeRcCoef(Bitstr_dec *streamdata,
                               WebRtc_Word16 *RCQ15);

/* quantize & code RC */
int WebRtcIsacfix_EncodeRcCoef(WebRtc_Word16 *RCQ15,
                               Bitstr_enc *streamdata);

/* decode & dequantize squared Gain */
int WebRtcIsacfix_DecodeGain2(Bitstr_dec *streamdata,
                              WebRtc_Word32 *Gain2);

/* quantize & code squared Gain (input is squared gain) */
int WebRtcIsacfix_EncodeGain2(WebRtc_Word32 *gain2,
                              Bitstr_enc *streamdata);

int WebRtcIsacfix_EncodePitchGain(WebRtc_Word16 *PitchGains_Q12,
                                  Bitstr_enc *streamdata,
                                  ISAC_SaveEncData_t* encData);

int WebRtcIsacfix_EncodePitchLag(WebRtc_Word16 *PitchLagQ7,
                                 WebRtc_Word16 *PitchGain_Q12,
                                 Bitstr_enc *streamdata,
                                 ISAC_SaveEncData_t* encData);

int WebRtcIsacfix_DecodePitchGain(Bitstr_dec *streamdata,
                                  WebRtc_Word16 *PitchGain_Q12);

int WebRtcIsacfix_DecodePitchLag(Bitstr_dec *streamdata,
                                 WebRtc_Word16 *PitchGain_Q12,
                                 WebRtc_Word16 *PitchLagQ7);

int WebRtcIsacfix_DecodeFrameLen(Bitstr_dec *streamdata,
                                 WebRtc_Word16 *framelength);


int WebRtcIsacfix_EncodeFrameLen(WebRtc_Word16 framelength,
                                 Bitstr_enc *streamdata);

int WebRtcIsacfix_DecodeSendBandwidth(Bitstr_dec *streamdata,
                                      WebRtc_Word16 *BWno);


int WebRtcIsacfix_EncodeReceiveBandwidth(WebRtc_Word16 *BWno,
                                         Bitstr_enc *streamdata);

void WebRtcIsacfix_TranscodeLpcCoef(WebRtc_Word32 *tmpcoeffs_gQ6,
                                    WebRtc_Word16 *index_gQQ);

#endif /* WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_ENTROPY_CODING_H_ */
