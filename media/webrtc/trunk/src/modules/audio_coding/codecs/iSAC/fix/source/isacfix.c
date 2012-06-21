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
 * isacfix.c
 *
 * This C file contains the functions for the ISAC API
 *
 */

#include <stdlib.h>
#include <string.h>

#include "isacfix.h"
#include "bandwidth_estimator.h"
#include "codec.h"
#include "entropy_coding.h"
#include "structs.h"


/**************************************************************************
 * WebRtcIsacfix_AssignSize(...)
 *
 * Functions used when malloc is not allowed
 * Returns number of bytes needed to allocate for iSAC struct.
 *
 */

WebRtc_Word16 WebRtcIsacfix_AssignSize(int *sizeinbytes) {
  *sizeinbytes=sizeof(ISACFIX_SubStruct)*2/sizeof(WebRtc_Word16);
  return(0);
}

/***************************************************************************
 * WebRtcIsacfix_Assign(...)
 *
 * Functions used when malloc is not allowed
 * Place struct at given address
 *
 * If successful, Return 0, else Return -1
 */

WebRtc_Word16 WebRtcIsacfix_Assign(ISACFIX_MainStruct **inst, void *ISACFIX_inst_Addr) {
  if (ISACFIX_inst_Addr!=NULL) {
    *inst = (ISACFIX_MainStruct*)ISACFIX_inst_Addr;
    (*(ISACFIX_SubStruct**)inst)->errorcode = 0;
    (*(ISACFIX_SubStruct**)inst)->initflag = 0;
    (*(ISACFIX_SubStruct**)inst)->ISACenc_obj.SaveEnc_ptr = NULL;
    return(0);
  } else {
    return(-1);
  }
}


#ifndef ISACFIX_NO_DYNAMIC_MEM

/****************************************************************************
 * WebRtcIsacfix_Create(...)
 *
 * This function creates a ISAC instance, which will contain the state
 * information for one coding/decoding channel.
 *
 * Input:
 *      - *ISAC_main_inst   : a pointer to the coder instance.
 *
 * Return value             :  0 - Ok
 *                            -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_Create(ISACFIX_MainStruct **ISAC_main_inst)
{
  ISACFIX_SubStruct *tempo;
  tempo = malloc(1 * sizeof(ISACFIX_SubStruct));
  *ISAC_main_inst = (ISACFIX_MainStruct *)tempo;
  if (*ISAC_main_inst!=NULL) {
    (*(ISACFIX_SubStruct**)ISAC_main_inst)->errorcode = 0;
    (*(ISACFIX_SubStruct**)ISAC_main_inst)->initflag = 0;
    (*(ISACFIX_SubStruct**)ISAC_main_inst)->ISACenc_obj.SaveEnc_ptr = NULL;
    return(0);
  } else {
    return(-1);
  }
}


/****************************************************************************
 * WebRtcIsacfix_CreateInternal(...)
 *
 * This function creates the memory that is used to store data in the encoder
 *
 * Input:
 *      - *ISAC_main_inst   : a pointer to the coder instance.
 *
 * Return value             :  0 - Ok
 *                            -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_CreateInternal(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst;

  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* Allocate memory for storing encoder data */
  ISAC_inst->ISACenc_obj.SaveEnc_ptr = malloc(1 * sizeof(ISAC_SaveEncData_t));

  if (ISAC_inst->ISACenc_obj.SaveEnc_ptr!=NULL) {
    return(0);
  } else {
    return(-1);
  }
}


#endif



/****************************************************************************
 * WebRtcIsacfix_Free(...)
 *
 * This function frees the ISAC instance created at the beginning.
 *
 * Input:
 *      - ISAC_main_inst    : a ISAC instance.
 *
 * Return value             :  0 - Ok
 *                            -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_Free(ISACFIX_MainStruct *ISAC_main_inst)
{
  free(ISAC_main_inst);
  return(0);
}

/****************************************************************************
 * WebRtcIsacfix_FreeInternal(...)
 *
 * This function frees the internal memory for storing encoder data.
 *
 * Input:
 *       - ISAC_main_inst    : a ISAC instance.
 *
 * Return value              :  0 - Ok
 *                             -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_FreeInternal(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst;

  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* Release memory */
  free(ISAC_inst->ISACenc_obj.SaveEnc_ptr);

  return(0);
}

/****************************************************************************
 * WebRtcIsacfix_EncoderInit(...)
 *
 * This function initializes a ISAC instance prior to the encoder calls.
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - CodingMode        : 0 -> Bit rate and frame length are automatically
 *                                 adjusted to available bandwidth on
 *                                 transmission channel.
 *                            1 -> User sets a frame length and a target bit
 *                                 rate which is taken as the maximum short-term
 *                                 average bit rate.
 *
 * Return value             :  0 - Ok
 *                            -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_EncoderInit(ISACFIX_MainStruct *ISAC_main_inst,
                                        WebRtc_Word16  CodingMode)
{
  int k;
  WebRtc_Word16 statusInit;
  ISACFIX_SubStruct *ISAC_inst;

  statusInit = 0;
  /* typecast pointer to rela structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* flag encoder init */
  ISAC_inst->initflag |= 2;

  if (CodingMode == 0)
    /* Adaptive mode */
    ISAC_inst->ISACenc_obj.new_framelength  = INITIAL_FRAMESAMPLES;
  else if (CodingMode == 1)
    /* Instantaneous mode */
    ISAC_inst->ISACenc_obj.new_framelength = 480;    /* default for I-mode */
  else {
    ISAC_inst->errorcode = ISAC_DISALLOWED_CODING_MODE;
    statusInit = -1;
  }

  ISAC_inst->CodingMode = CodingMode;

  WebRtcIsacfix_InitMaskingEnc(&ISAC_inst->ISACenc_obj.maskfiltstr_obj);
  WebRtcIsacfix_InitPreFilterbank(&ISAC_inst->ISACenc_obj.prefiltbankstr_obj);
  WebRtcIsacfix_InitPitchFilter(&ISAC_inst->ISACenc_obj.pitchfiltstr_obj);
  WebRtcIsacfix_InitPitchAnalysis(&ISAC_inst->ISACenc_obj.pitchanalysisstr_obj);


  WebRtcIsacfix_InitBandwidthEstimator(&ISAC_inst->bwestimator_obj);
  WebRtcIsacfix_InitRateModel(&ISAC_inst->ISACenc_obj.rate_data_obj);


  ISAC_inst->ISACenc_obj.buffer_index   = 0;
  ISAC_inst->ISACenc_obj.frame_nb    = 0;
  ISAC_inst->ISACenc_obj.BottleNeck      = 32000; /* default for I-mode */
  ISAC_inst->ISACenc_obj.MaxDelay    = 10;    /* default for I-mode */
  ISAC_inst->ISACenc_obj.current_framesamples = 0;
  ISAC_inst->ISACenc_obj.s2nr     = 0;
  ISAC_inst->ISACenc_obj.MaxBits    = 0;
  ISAC_inst->ISACenc_obj.bitstr_seed   = 4447;
  ISAC_inst->ISACenc_obj.payloadLimitBytes30  = STREAM_MAXW16_30MS << 1;
  ISAC_inst->ISACenc_obj.payloadLimitBytes60  = STREAM_MAXW16_60MS << 1;
  ISAC_inst->ISACenc_obj.maxPayloadBytes      = STREAM_MAXW16_60MS << 1;
  ISAC_inst->ISACenc_obj.maxRateInBytes       = STREAM_MAXW16_30MS << 1;
  ISAC_inst->ISACenc_obj.enforceFrameSize     = 0;

  /* Init the bistream data area to zero */
  for (k=0; k<STREAM_MAXW16_60MS; k++){
    ISAC_inst->ISACenc_obj.bitstr_obj.stream[k] = 0;
  }

#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  WebRtcIsacfix_InitPostFilterbank(&ISAC_inst->ISACenc_obj.interpolatorstr_obj);
#endif

  // Initiaze function pointers.
  WebRtcIsacfix_AutocorrFix = WebRtcIsacfix_AutocorrC;
  WebRtcIsacfix_FilterMaLoopFix = WebRtcIsacfix_FilterMaLoopC;

#ifdef WEBRTC_ARCH_ARM_NEON
  WebRtcIsacfix_AutocorrFix = WebRtcIsacfix_AutocorrNeon;
  WebRtcIsacfix_FilterMaLoopFix = WebRtcIsacfix_FilterMaLoopNeon;
#endif

  return statusInit;
}

/****************************************************************************
 * WebRtcIsacfix_Encode(...)
 *
 * This function encodes 10ms frame(s) and inserts it into a package.
 * Input speech length has to be 160 samples (10ms). The encoder buffers those
 * 10ms frames until it reaches the chosen Framesize (480 or 960 samples
 * corresponding to 30 or 60 ms frames), and then proceeds to the encoding.
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - speechIn          : input speech vector.
 *
 * Output:
 *      - encoded           : the encoded data vector
 *
 * Return value:
 *                          : >0 - Length (in bytes) of coded data
 *                          :  0 - The buffer didn't reach the chosen framesize
 *                            so it keeps buffering speech samples.
 *                          : -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_Encode(ISACFIX_MainStruct *ISAC_main_inst,
                                   const WebRtc_Word16    *speechIn,
                                   WebRtc_Word16          *encoded)
{
  ISACFIX_SubStruct *ISAC_inst;
  WebRtc_Word16 stream_len;
#ifndef WEBRTC_BIG_ENDIAN
  int k;
#endif

  /* typecast pointer to rela structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;


  /* check if encoder initiated */
  if ((ISAC_inst->initflag & 2) != 2) {
    ISAC_inst->errorcode = ISAC_ENCODER_NOT_INITIATED;
    return (-1);
  }

  stream_len = WebRtcIsacfix_EncodeImpl((WebRtc_Word16*)speechIn,
                                    &ISAC_inst->ISACenc_obj,
                                    &ISAC_inst->bwestimator_obj,
                                    ISAC_inst->CodingMode);
  if (stream_len<0) {
    ISAC_inst->errorcode = - stream_len;
    return -1;
  }


  /* convert from bytes to WebRtc_Word16 */
#ifndef WEBRTC_BIG_ENDIAN
  for (k=0;k<(stream_len+1)>>1;k++) {
    encoded[k] = (WebRtc_Word16)( ( (WebRtc_UWord16)(ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] >> 8 )
                                  | (((ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] & 0x00FF) << 8));
  }

#else
  WEBRTC_SPL_MEMCPY_W16(encoded, (ISAC_inst->ISACenc_obj.bitstr_obj).stream, (stream_len + 1)>>1);
#endif



  return stream_len;

}




/****************************************************************************
 * WebRtcIsacfix_EncodeNb(...)
 *
 * This function encodes 10ms narrow band (8 kHz sampling) frame(s) and inserts
 * it into a package. Input speech length has to be 80 samples (10ms). The encoder
 * interpolates into wide-band (16 kHz sampling) buffers those
 * 10ms frames until it reaches the chosen Framesize (480 or 960 wide-band samples
 * corresponding to 30 or 60 ms frames), and then proceeds to the encoding.
 *
 * The function is enabled if WEBRTC_ISAC_FIX_NB_CALLS_ENABLED is defined
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - speechIn          : input speech vector.
 *
 * Output:
 *      - encoded           : the encoded data vector
 *
 * Return value:
 *                          : >0 - Length (in bytes) of coded data
 *                          :  0 - The buffer didn't reach the chosen framesize
 *                            so it keeps buffering speech samples.
 *                          : -1 - Error
 */
#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
WebRtc_Word16 WebRtcIsacfix_EncodeNb(ISACFIX_MainStruct *ISAC_main_inst,
                                      const WebRtc_Word16    *speechIn,
                                      WebRtc_Word16          *encoded)
{
  ISACFIX_SubStruct *ISAC_inst;
  WebRtc_Word16 stream_len;
  WebRtc_Word16 speechInWB[FRAMESAMPLES_10ms];
  WebRtc_Word16 Vector_Word16_1[FRAMESAMPLES_10ms/2];
  WebRtc_Word16 Vector_Word16_2[FRAMESAMPLES_10ms/2];

  int k;


  /* typecast pointer to rela structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;


  /* check if encoder initiated */
  if ((ISAC_inst->initflag & 2) != 2) {
    ISAC_inst->errorcode = ISAC_ENCODER_NOT_INITIATED;
    return (-1);
  }


  /* Oversample to WB */

  /* Form polyphase signals, and compensate for DC offset */
  for (k=0;k<FRAMESAMPLES_10ms/2;k++) {
    Vector_Word16_1[k] = speechIn[k] + 1;
    Vector_Word16_2[k] = speechIn[k];
  }
  WebRtcIsacfix_FilterAndCombine2(Vector_Word16_1, Vector_Word16_2, speechInWB, &ISAC_inst->ISACenc_obj.interpolatorstr_obj, FRAMESAMPLES_10ms);


  /* Encode WB signal */
  stream_len = WebRtcIsacfix_EncodeImpl((WebRtc_Word16*)speechInWB,
                                    &ISAC_inst->ISACenc_obj,
                                    &ISAC_inst->bwestimator_obj,
                                    ISAC_inst->CodingMode);
  if (stream_len<0) {
    ISAC_inst->errorcode = - stream_len;
    return -1;
  }


  /* convert from bytes to WebRtc_Word16 */
#ifndef WEBRTC_BIG_ENDIAN
  for (k=0;k<(stream_len+1)>>1;k++) {
    encoded[k] = (WebRtc_Word16)(((WebRtc_UWord16)(ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] >> 8)
                                 | (((ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] & 0x00FF) << 8));
  }

#else
  WEBRTC_SPL_MEMCPY_W16(encoded, (ISAC_inst->ISACenc_obj.bitstr_obj).stream, (stream_len + 1)>>1);
#endif



  return stream_len;
}
#endif  /* WEBRTC_ISAC_FIX_NB_CALLS_ENABLED */


/****************************************************************************
 * WebRtcIsacfix_GetNewBitStream(...)
 *
 * This function returns encoded data, with the recieved bwe-index in the
 * stream. It should always return a complete packet, i.e. only called once
 * even for 60 msec frames
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - bweIndex          : index of bandwidth estimate to put in new bitstream
 *
 * Output:
 *      - encoded           : the encoded data vector
 *
 * Return value:
 *                          : >0 - Length (in bytes) of coded data
 *                          : -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_GetNewBitStream(ISACFIX_MainStruct *ISAC_main_inst,
                                            WebRtc_Word16      bweIndex,
                                            float              scale,
                                            WebRtc_Word16        *encoded)
{
  ISACFIX_SubStruct *ISAC_inst;
  WebRtc_Word16 stream_len;
#ifndef WEBRTC_BIG_ENDIAN
  int k;
#endif

  /* typecast pointer to rela structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;


  /* check if encoder initiated */
  if ((ISAC_inst->initflag & 2) != 2) {
    ISAC_inst->errorcode = ISAC_ENCODER_NOT_INITIATED;
    return (-1);
  }

  stream_len = WebRtcIsacfix_EncodeStoredData(&ISAC_inst->ISACenc_obj,
                                              bweIndex,
                                              scale);
  if (stream_len<0) {
    ISAC_inst->errorcode = - stream_len;
    return -1;
  }

#ifndef WEBRTC_BIG_ENDIAN
  for (k=0;k<(stream_len+1)>>1;k++) {
    encoded[k] = (WebRtc_Word16)( ( (WebRtc_UWord16)(ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] >> 8 )
                                  | (((ISAC_inst->ISACenc_obj.bitstr_obj).stream[k] & 0x00FF) << 8));
  }

#else
  WEBRTC_SPL_MEMCPY_W16(encoded, (ISAC_inst->ISACenc_obj.bitstr_obj).stream, (stream_len + 1)>>1);
#endif

  return stream_len;

}



/****************************************************************************
 * WebRtcIsacfix_DecoderInit(...)
 *
 * This function initializes a ISAC instance prior to the decoder calls.
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *
 * Return value
 *                          :  0 - Ok
 *                            -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_DecoderInit(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst;

  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* flag decoder init */
  ISAC_inst->initflag |= 1;


  WebRtcIsacfix_InitMaskingDec(&ISAC_inst->ISACdec_obj.maskfiltstr_obj);
  WebRtcIsacfix_InitPostFilterbank(&ISAC_inst->ISACdec_obj.postfiltbankstr_obj);
  WebRtcIsacfix_InitPitchFilter(&ISAC_inst->ISACdec_obj.pitchfiltstr_obj);

  /* TS */
  WebRtcIsacfix_InitPlc( &ISAC_inst->ISACdec_obj.plcstr_obj );


#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  WebRtcIsacfix_InitPreFilterbank(&ISAC_inst->ISACdec_obj.decimatorstr_obj);
#endif

  return 0;
}


/****************************************************************************
 * WebRtcIsacfix_UpdateBwEstimate1(...)
 *
 * This function updates the estimate of the bandwidth.
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - encoded           : encoded ISAC frame(s).
 *      - packet_size       : size of the packet.
 *      - rtp_seq_number    : the RTP number of the packet.
 *      - arr_ts            : the arrival time of the packet (from NetEq)
 *                            in samples.
 *
 * Return value             :  0 - Ok
 *                            -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_UpdateBwEstimate1(ISACFIX_MainStruct *ISAC_main_inst,
                                     const WebRtc_UWord16   *encoded,
                                     WebRtc_Word32          packet_size,
                                     WebRtc_UWord16         rtp_seq_number,
                                     WebRtc_UWord32         arr_ts)
{
  ISACFIX_SubStruct *ISAC_inst;
  Bitstr_dec streamdata;
  WebRtc_UWord16 partOfStream[5];
#ifndef WEBRTC_BIG_ENDIAN
  int k;
#endif
  WebRtc_Word16 err;

  /* Set stream pointer to point at partOfStream */
  streamdata.stream = (WebRtc_UWord16 *)partOfStream;

  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* Sanity check of packet length */
  if (packet_size <= 0) {
    /* return error code if the packet length is null or less */
    ISAC_inst->errorcode = ISAC_EMPTY_PACKET;
    return -1;
  } else if (packet_size > (STREAM_MAXW16<<1)) {
    /* return error code if length of stream is too long */
    ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
    return -1;
  }

  /* check if decoder initiated */
  if ((ISAC_inst->initflag & 1) != 1) {
    ISAC_inst->errorcode = ISAC_DECODER_NOT_INITIATED;
    return (-1);
  }

  streamdata.W_upper = 0xFFFFFFFF;
  streamdata.streamval = 0;
  streamdata.stream_index = 0;
  streamdata.full = 1;

#ifndef WEBRTC_BIG_ENDIAN
  for (k=0; k<5; k++) {
    streamdata.stream[k] = (WebRtc_UWord16) (((WebRtc_UWord16)encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
#else
  memcpy(streamdata.stream, encoded, 5);
#endif

  if (packet_size == 0)
  {
    /* return error code if the packet length is null */
    ISAC_inst->errorcode = ISAC_EMPTY_PACKET;
    return -1;
  }

  err = WebRtcIsacfix_EstimateBandwidth(&ISAC_inst->bwestimator_obj,
                                        &streamdata,
                                        packet_size,
                                        rtp_seq_number,
                                        0,
                                        arr_ts);


  if (err < 0)
  {
    /* return error code if something went wrong */
    ISAC_inst->errorcode = -err;
    return -1;
  }


  return 0;
}

/****************************************************************************
 * WebRtcIsacfix_UpdateBwEstimate(...)
 *
 * This function updates the estimate of the bandwidth.
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - encoded           : encoded ISAC frame(s).
 *      - packet_size       : size of the packet.
 *      - rtp_seq_number    : the RTP number of the packet.
 *      - send_ts           : Send Time Stamp from RTP header
 *      - arr_ts            : the arrival time of the packet (from NetEq)
 *                            in samples.
 *
 * Return value             :  0 - Ok
 *                            -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_UpdateBwEstimate(ISACFIX_MainStruct *ISAC_main_inst,
                                       const WebRtc_UWord16   *encoded,
                                       WebRtc_Word32          packet_size,
                                       WebRtc_UWord16         rtp_seq_number,
                                       WebRtc_UWord32         send_ts,
                                       WebRtc_UWord32         arr_ts)
{
  ISACFIX_SubStruct *ISAC_inst;
  Bitstr_dec streamdata;
  WebRtc_UWord16 partOfStream[5];
#ifndef WEBRTC_BIG_ENDIAN
  int k;
#endif
  WebRtc_Word16 err;

  /* Set stream pointer to point at partOfStream */
  streamdata.stream = (WebRtc_UWord16 *)partOfStream;

  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* Sanity check of packet length */
  if (packet_size <= 0) {
    /* return error code if the packet length is null  or less */
    ISAC_inst->errorcode = ISAC_EMPTY_PACKET;
    return -1;
  } else if (packet_size > (STREAM_MAXW16<<1)) {
    /* return error code if length of stream is too long */
    ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
    return -1;
  }

  /* check if decoder initiated */
  if ((ISAC_inst->initflag & 1) != 1) {
    ISAC_inst->errorcode = ISAC_DECODER_NOT_INITIATED;
    return (-1);
  }

  streamdata.W_upper = 0xFFFFFFFF;
  streamdata.streamval = 0;
  streamdata.stream_index = 0;
  streamdata.full = 1;

#ifndef WEBRTC_BIG_ENDIAN
  for (k=0; k<5; k++) {
    streamdata.stream[k] = (WebRtc_UWord16) ((encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
#else
  memcpy(streamdata.stream, encoded, 5);
#endif

  if (packet_size == 0)
  {
    /* return error code if the packet length is null */
    ISAC_inst->errorcode = ISAC_EMPTY_PACKET;
    return -1;
  }

  err = WebRtcIsacfix_EstimateBandwidth(&ISAC_inst->bwestimator_obj,
                                        &streamdata,
                                        packet_size,
                                        rtp_seq_number,
                                        send_ts,
                                        arr_ts);

  if (err < 0)
  {
    /* return error code if something went wrong */
    ISAC_inst->errorcode = -err;
    return -1;
  }


  return 0;
}

/****************************************************************************
 * WebRtcIsacfix_Decode(...)
 *
 * This function decodes a ISAC frame. Output speech length
 * will be a multiple of 480 samples: 480 or 960 samples,
 * depending on the framesize (30 or 60 ms).
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - encoded           : encoded ISAC frame(s)
 *      - len               : bytes in encoded vector
 *
 * Output:
 *      - decoded           : The decoded vector
 *
 * Return value             : >0 - number of samples in decoded vector
 *                            -1 - Error
 */


WebRtc_Word16 WebRtcIsacfix_Decode(ISACFIX_MainStruct *ISAC_main_inst,
                                     const WebRtc_UWord16   *encoded,
                                     WebRtc_Word16          len,
                                     WebRtc_Word16          *decoded,
                                     WebRtc_Word16     *speechType)
{
  ISACFIX_SubStruct *ISAC_inst;
  /* number of samples (480 or 960), output from decoder */
  /* that were actually used in the encoder/decoder (determined on the fly) */
  WebRtc_Word16     number_of_samples;
#ifndef WEBRTC_BIG_ENDIAN
  int k;
#endif
  WebRtc_Word16 declen = 0;

  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* check if decoder initiated */
  if ((ISAC_inst->initflag & 1) != 1) {
    ISAC_inst->errorcode = ISAC_DECODER_NOT_INITIATED;
    return (-1);
  }

  /* Sanity check of packet length */
  if (len <= 0) {
    /* return error code if the packet length is null  or less */
    ISAC_inst->errorcode = ISAC_EMPTY_PACKET;
    return -1;
  } else if (len > (STREAM_MAXW16<<1)) {
    /* return error code if length of stream is too long */
    ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
    return -1;
  }

  (ISAC_inst->ISACdec_obj.bitstr_obj).stream = (WebRtc_UWord16 *)encoded;

  /* convert bitstream from WebRtc_Word16 to bytes */
#ifndef WEBRTC_BIG_ENDIAN
  for (k=0; k<(len>>1); k++) {
    (ISAC_inst->ISACdec_obj.bitstr_obj).stream[k] = (WebRtc_UWord16) ((encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
  if (len & 0x0001)
    (ISAC_inst->ISACdec_obj.bitstr_obj).stream[k] = (WebRtc_UWord16) ((encoded[k] & 0xFF)<<8);
#endif

  /* added for NetEq purposes (VAD/DTX related) */
  *speechType=1;

  declen = WebRtcIsacfix_DecodeImpl(decoded,&ISAC_inst->ISACdec_obj, &number_of_samples);

  if (declen < 0) {
    /* Some error inside the decoder */
    ISAC_inst->errorcode = -declen;
    memset(decoded, 0, sizeof(WebRtc_Word16) * MAX_FRAMESAMPLES);
    return -1;
  }

  /* error check */

  if (declen & 0x0001) {
    if (len != declen && len != declen + (((ISAC_inst->ISACdec_obj.bitstr_obj).stream[declen>>1]) & 0x00FF) ) {
      ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
      memset(decoded, 0, sizeof(WebRtc_Word16) * number_of_samples);
      return -1;
    }
  } else {
    if (len != declen && len != declen + (((ISAC_inst->ISACdec_obj.bitstr_obj).stream[declen>>1]) >> 8) ) {
      ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
      memset(decoded, 0, sizeof(WebRtc_Word16) * number_of_samples);
      return -1;
    }
  }

  return number_of_samples;
}





/****************************************************************************
 * WebRtcIsacfix_DecodeNb(...)
 *
 * This function decodes a ISAC frame in narrow-band (8 kHz sampling).
 * Output speech length will be a multiple of 240 samples: 240 or 480 samples,
 * depending on the framesize (30 or 60 ms).
 *
 * The function is enabled if WEBRTC_ISAC_FIX_NB_CALLS_ENABLED is defined
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - encoded           : encoded ISAC frame(s)
 *      - len               : bytes in encoded vector
 *
 * Output:
 *      - decoded           : The decoded vector
 *
 * Return value             : >0 - number of samples in decoded vector
 *                            -1 - Error
 */

#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
WebRtc_Word16 WebRtcIsacfix_DecodeNb(ISACFIX_MainStruct *ISAC_main_inst,
                                        const WebRtc_UWord16   *encoded,
                                        WebRtc_Word16          len,
                                        WebRtc_Word16          *decoded,
                                        WebRtc_Word16    *speechType)
{
  ISACFIX_SubStruct *ISAC_inst;
  /* twice the number of samples (480 or 960), output from decoder */
  /* that were actually used in the encoder/decoder (determined on the fly) */
  WebRtc_Word16     number_of_samples;
#ifndef WEBRTC_BIG_ENDIAN
  int k;
#endif
  WebRtc_Word16 declen = 0;
  WebRtc_Word16 dummy[FRAMESAMPLES/2];


  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* check if decoder initiated */
  if ((ISAC_inst->initflag & 1) != 1) {
    ISAC_inst->errorcode = ISAC_DECODER_NOT_INITIATED;
    return (-1);
  }

  if (len == 0)
  {  /* return error code if the packet length is null */

    ISAC_inst->errorcode = ISAC_EMPTY_PACKET;
    return -1;
  }

  (ISAC_inst->ISACdec_obj.bitstr_obj).stream = (WebRtc_UWord16 *)encoded;

  /* convert bitstream from WebRtc_Word16 to bytes */
#ifndef WEBRTC_BIG_ENDIAN
  for (k=0; k<(len>>1); k++) {
    (ISAC_inst->ISACdec_obj.bitstr_obj).stream[k] = (WebRtc_UWord16) ((encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
  if (len & 0x0001)
    (ISAC_inst->ISACdec_obj.bitstr_obj).stream[k] = (WebRtc_UWord16) ((encoded[k] & 0xFF)<<8);
#endif

  /* added for NetEq purposes (VAD/DTX related) */
  *speechType=1;

  declen = WebRtcIsacfix_DecodeImpl(decoded,&ISAC_inst->ISACdec_obj, &number_of_samples);

  if (declen < 0) {
    /* Some error inside the decoder */
    ISAC_inst->errorcode = -declen;
    memset(decoded, 0, sizeof(WebRtc_Word16) * FRAMESAMPLES);
    return -1;
  }

  /* error check */

  if (declen & 0x0001) {
    if (len != declen && len != declen + (((ISAC_inst->ISACdec_obj.bitstr_obj).stream[declen>>1]) & 0x00FF) ) {
      ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
      memset(decoded, 0, sizeof(WebRtc_Word16) * number_of_samples);
      return -1;
    }
  } else {
    if (len != declen && len != declen + (((ISAC_inst->ISACdec_obj.bitstr_obj).stream[declen>>1]) >> 8) ) {
      ISAC_inst->errorcode = ISAC_LENGTH_MISMATCH;
      memset(decoded, 0, sizeof(WebRtc_Word16) * number_of_samples);
      return -1;
    }
  }

  WebRtcIsacfix_SplitAndFilter2(decoded, decoded, dummy, &ISAC_inst->ISACdec_obj.decimatorstr_obj);

  if (number_of_samples>FRAMESAMPLES) {
    WebRtcIsacfix_SplitAndFilter2(decoded + FRAMESAMPLES, decoded + FRAMESAMPLES/2,
                                  dummy, &ISAC_inst->ISACdec_obj.decimatorstr_obj);
  }

  return number_of_samples/2;
}
#endif /* WEBRTC_ISAC_FIX_NB_CALLS_ENABLED */


/****************************************************************************
 * WebRtcIsacfix_DecodePlcNb(...)
 *
 * This function conducts PLC for ISAC frame(s) in narrow-band (8kHz sampling).
 * Output speech length  will be "240*noOfLostFrames" samples
 * that is equevalent of "30*noOfLostFrames" millisecond.
 *
 * The function is enabled if WEBRTC_ISAC_FIX_NB_CALLS_ENABLED is defined
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - noOfLostFrames    : Number of PLC frames (240 sample=30ms) to produce
 *
 * Output:
 *      - decoded           : The decoded vector
 *
 * Return value             : >0 - number of samples in decoded PLC vector
 *                            -1 - Error
 */

#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
WebRtc_Word16 WebRtcIsacfix_DecodePlcNb(ISACFIX_MainStruct *ISAC_main_inst,
                                         WebRtc_Word16          *decoded,
                                         WebRtc_Word16 noOfLostFrames )
{
  WebRtc_Word16 no_of_samples, declen, k, ok;
  WebRtc_Word16 outframeNB[FRAMESAMPLES];
  WebRtc_Word16 outframeWB[FRAMESAMPLES];
  WebRtc_Word16 dummy[FRAMESAMPLES/2];


  ISACFIX_SubStruct *ISAC_inst;
  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* Limit number of frames to two = 60 msec. Otherwise we exceed data vectors */
  if (noOfLostFrames > 2){
    noOfLostFrames = 2;
  }

  k = 0;
  declen = 0;
  while( noOfLostFrames > 0 )
  {
    ok = WebRtcIsacfix_DecodePlcImpl( outframeWB, &ISAC_inst->ISACdec_obj, &no_of_samples );
    if(ok)
      return -1;

    WebRtcIsacfix_SplitAndFilter2(outframeWB, &(outframeNB[k*240]), dummy, &ISAC_inst->ISACdec_obj.decimatorstr_obj);

    declen += no_of_samples;
    noOfLostFrames--;
    k++;
  }

  declen>>=1;

  for (k=0;k<declen;k++) {
    decoded[k] = outframeNB[k];
  }

  return declen;
}
#endif /* WEBRTC_ISAC_FIX_NB_CALLS_ENABLED */




/****************************************************************************
 * WebRtcIsacfix_DecodePlc(...)
 *
 * This function conducts PLC for ISAC frame(s) in wide-band (16kHz sampling).
 * Output speech length  will be "480*noOfLostFrames" samples
 * that is equevalent of "30*noOfLostFrames" millisecond.
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - noOfLostFrames    : Number of PLC frames (480sample = 30ms)
 *                                to produce
 *
 * Output:
 *      - decoded           : The decoded vector
 *
 * Return value             : >0 - number of samples in decoded PLC vector
 *                            -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_DecodePlc(ISACFIX_MainStruct *ISAC_main_inst,
                                      WebRtc_Word16          *decoded,
                                      WebRtc_Word16 noOfLostFrames)
{

  WebRtc_Word16 no_of_samples, declen, k, ok;
  WebRtc_Word16 outframe16[MAX_FRAMESAMPLES];

  ISACFIX_SubStruct *ISAC_inst;
  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* Limit number of frames to two = 60 msec. Otherwise we exceed data vectors */
  if (noOfLostFrames > 2) {
    noOfLostFrames = 2;
  }
  k = 0;
  declen = 0;
  while( noOfLostFrames > 0 )
  {
    ok = WebRtcIsacfix_DecodePlcImpl( &(outframe16[k*480]), &ISAC_inst->ISACdec_obj, &no_of_samples );
    if(ok)
      return -1;
    declen += no_of_samples;
    noOfLostFrames--;
    k++;
  }

  for (k=0;k<declen;k++) {
    decoded[k] = outframe16[k];
  }

  return declen;
}


/****************************************************************************
 * WebRtcIsacfix_Control(...)
 *
 * This function sets the limit on the short-term average bit rate and the
 * frame length. Should be used only in Instantaneous mode.
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance.
 *      - rate              : limit on the short-term average bit rate,
 *                            in bits/second (between 10000 and 32000)
 *      - framesize         : number of milliseconds per frame (30 or 60)
 *
 * Return value             : 0  - ok
 *                            -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_Control(ISACFIX_MainStruct *ISAC_main_inst,
                                    WebRtc_Word16          rate,
                                    WebRtc_Word16          framesize)
{
  ISACFIX_SubStruct *ISAC_inst;
  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  if (ISAC_inst->CodingMode == 0)
  {
    /* in adaptive mode */
    ISAC_inst->errorcode = ISAC_MODE_MISMATCH;
    return -1;
  }


  if (rate >= 10000 && rate <= 32000)
    ISAC_inst->ISACenc_obj.BottleNeck = rate;
  else {
    ISAC_inst->errorcode = ISAC_DISALLOWED_BOTTLENECK;
    return -1;
  }



  if (framesize  == 30 || framesize == 60)
    ISAC_inst->ISACenc_obj.new_framelength = (FS/1000) * framesize;
  else {
    ISAC_inst->errorcode = ISAC_DISALLOWED_FRAME_LENGTH;
    return -1;
  }

  return 0;
}


/****************************************************************************
 * WebRtcIsacfix_ControlBwe(...)
 *
 * This function sets the initial values of bottleneck and frame-size if
 * iSAC is used in channel-adaptive mode. Through this API, users can
 * enforce a frame-size for all values of bottleneck. Then iSAC will not
 * automatically change the frame-size.
 *
 *
 * Input:
 *  - ISAC_main_inst : ISAC instance.
 *      - rateBPS           : initial value of bottleneck in bits/second
 *                            10000 <= rateBPS <= 32000 is accepted
 *                            For default bottleneck set rateBPS = 0
 *      - frameSizeMs       : number of milliseconds per frame (30 or 60)
 *      - enforceFrameSize  : 1 to enforce the given frame-size through out
 *                            the adaptation process, 0 to let iSAC change
 *                            the frame-size if required.
 *
 * Return value    : 0  - ok
 *         -1 - Error
 */

WebRtc_Word16 WebRtcIsacfix_ControlBwe(ISACFIX_MainStruct *ISAC_main_inst,
                                        WebRtc_Word16 rateBPS,
                                        WebRtc_Word16 frameSizeMs,
                                        WebRtc_Word16 enforceFrameSize)
{
  ISACFIX_SubStruct *ISAC_inst;
  /* Typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* check if encoder initiated */
  if ((ISAC_inst->initflag & 2) != 2) {
    ISAC_inst->errorcode = ISAC_ENCODER_NOT_INITIATED;
    return (-1);
  }

  /* Check that we are in channel-adaptive mode, otherwise, return -1 */
  if (ISAC_inst->CodingMode != 0) {
    ISAC_inst->errorcode = ISAC_MODE_MISMATCH;
    return (-1);
  }

  /* Set struct variable if enforceFrameSize is set. ISAC will then keep the */
  /* chosen frame size.                                                      */
  ISAC_inst->ISACenc_obj.enforceFrameSize = (enforceFrameSize != 0)? 1:0;

  /* Set initial rate, if value between 10000 and 32000,                */
  /* if rateBPS is 0, keep the default initial bottleneck value (15000) */
  if ((rateBPS >= 10000) && (rateBPS <= 32000)) {
    ISAC_inst->bwestimator_obj.sendBwAvg = (((WebRtc_UWord32)rateBPS) << 7);
  } else if (rateBPS != 0) {
    ISAC_inst->errorcode = ISAC_DISALLOWED_BOTTLENECK;
    return -1;
  }

  /* Set initial framesize. If enforceFrameSize is set the frame size will not change */
  if ((frameSizeMs  == 30) || (frameSizeMs == 60)) {
    ISAC_inst->ISACenc_obj.new_framelength = (FS/1000) * frameSizeMs;
  } else {
    ISAC_inst->errorcode = ISAC_DISALLOWED_FRAME_LENGTH;
    return -1;
  }

  return 0;
}





/****************************************************************************
 * WebRtcIsacfix_GetDownLinkBwIndex(...)
 *
 * This function returns index representing the Bandwidth estimate from
 * other side to this side.
 *
 * Input:
 *      - ISAC_main_inst: iSAC struct
 *
 * Output:
 *      - rateIndex     : Bandwidth estimate to transmit to other side.
 *
 */

WebRtc_Word16 WebRtcIsacfix_GetDownLinkBwIndex(ISACFIX_MainStruct* ISAC_main_inst,
                                       WebRtc_Word16*     rateIndex)
{
  ISACFIX_SubStruct *ISAC_inst;

  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* Call function to get Bandwidth Estimate */
  *rateIndex = WebRtcIsacfix_GetDownlinkBwIndexImpl(&ISAC_inst->bwestimator_obj);

  return 0;
}


/****************************************************************************
 * WebRtcIsacfix_UpdateUplinkBw(...)
 *
 * This function takes an index representing the Bandwidth estimate from
 * this side to other side and updates BWE.
 *
 * Input:
 *      - ISAC_main_inst: iSAC struct
 *      - rateIndex     : Bandwidth estimate from other side.
 *
 */

WebRtc_Word16 WebRtcIsacfix_UpdateUplinkBw(ISACFIX_MainStruct* ISAC_main_inst,
                                   WebRtc_Word16     rateIndex)
{
  WebRtc_Word16 err = 0;
  ISACFIX_SubStruct *ISAC_inst;

  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  /* Call function to update BWE with received Bandwidth Estimate */
  err = WebRtcIsacfix_UpdateUplinkBwRec(&ISAC_inst->bwestimator_obj, rateIndex);
  if (err < 0) {
    ISAC_inst->errorcode = -err;
    return (-1);
  }

  return 0;
}

/****************************************************************************
 * WebRtcIsacfix_ReadFrameLen(...)
 *
 * This function returns the length of the frame represented in the packet.
 *
 * Input:
 *      - encoded       : Encoded bitstream
 *
 * Output:
 *      - frameLength   : Length of frame in packet (in samples)
 *
 */

WebRtc_Word16 WebRtcIsacfix_ReadFrameLen(const WebRtc_Word16* encoded,
                                        WebRtc_Word16* frameLength)
{
  Bitstr_dec streamdata;
  WebRtc_UWord16 partOfStream[5];
#ifndef WEBRTC_BIG_ENDIAN
  int k;
#endif
  WebRtc_Word16 err;

  /* Set stream pointer to point at partOfStream */
  streamdata.stream = (WebRtc_UWord16 *)partOfStream;

  streamdata.W_upper = 0xFFFFFFFF;
  streamdata.streamval = 0;
  streamdata.stream_index = 0;
  streamdata.full = 1;

#ifndef WEBRTC_BIG_ENDIAN
  for (k=0; k<5; k++) {
    streamdata.stream[k] = (WebRtc_UWord16) (((WebRtc_UWord16)encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
#else
  memcpy(streamdata.stream, encoded, 5);
#endif

  /* decode frame length */
  err = WebRtcIsacfix_DecodeFrameLen(&streamdata, frameLength);
  if (err<0)  // error check
    return err;

  return 0;
}


/****************************************************************************
 * WebRtcIsacfix_ReadBwIndex(...)
 *
 * This function returns the index of the Bandwidth estimate from the bitstream.
 *
 * Input:
 *      - encoded       : Encoded bitstream
 *
 * Output:
 *      - frameLength   : Length of frame in packet (in samples)
 *      - rateIndex     : Bandwidth estimate in bitstream
 *
 */

WebRtc_Word16 WebRtcIsacfix_ReadBwIndex(const WebRtc_Word16* encoded,
                                   WebRtc_Word16* rateIndex)
{
  Bitstr_dec streamdata;
  WebRtc_UWord16 partOfStream[5];
#ifndef WEBRTC_BIG_ENDIAN
  int k;
#endif
  WebRtc_Word16 err;

  /* Set stream pointer to point at partOfStream */
  streamdata.stream = (WebRtc_UWord16 *)partOfStream;

  streamdata.W_upper = 0xFFFFFFFF;
  streamdata.streamval = 0;
  streamdata.stream_index = 0;
  streamdata.full = 1;

#ifndef WEBRTC_BIG_ENDIAN
  for (k=0; k<5; k++) {
    streamdata.stream[k] = (WebRtc_UWord16) (((WebRtc_UWord16)encoded[k] >> 8)|((encoded[k] & 0xFF)<<8));
  }
#else
  memcpy(streamdata.stream, encoded, 5);
#endif

  /* decode frame length, needed to get to the rateIndex in the bitstream */
  err = WebRtcIsacfix_DecodeFrameLen(&streamdata, rateIndex);
  if (err<0)  // error check
    return err;

  /* decode BW estimation */
  err = WebRtcIsacfix_DecodeSendBandwidth(&streamdata, rateIndex);
  if (err<0)  // error check
    return err;

  return 0;
}




/****************************************************************************
 * WebRtcIsacfix_GetErrorCode(...)
 *
 * This function can be used to check the error code of an iSAC instance. When
 * a function returns -1 a error code will be set for that instance. The
 * function below extract the code of the last error that occured in the
 * specified instance.
 *
 * Input:
 *      - ISAC_main_inst    : ISAC instance
 *
 * Return value             : Error code
 */

WebRtc_Word16 WebRtcIsacfix_GetErrorCode(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst;
  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  return ISAC_inst->errorcode;
}



/****************************************************************************
 * WebRtcIsacfix_GetUplinkBw(...)
 *
 * This function returns the inst quantized iSAC send bitrate
 *
 * Input:
 *      - ISAC_main_inst    : iSAC instance
 *
 * Return value             : bitrate
 */

WebRtc_Word32 WebRtcIsacfix_GetUplinkBw(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;
  BwEstimatorstr * bw = (BwEstimatorstr*)&(ISAC_inst->bwestimator_obj);

  return (WebRtc_Word32) WebRtcIsacfix_GetUplinkBandwidth(bw);
}

/****************************************************************************
 * WebRtcIsacfix_GetNewFrameLen(...)
 *
 * This function return the next frame length (in samples) of iSAC.
 *
 * Input:
 *      - ISAC_main_inst    : iSAC instance
 *
 * Return value             :  frame lenght in samples
 */

WebRtc_Word16 WebRtcIsacfix_GetNewFrameLen(ISACFIX_MainStruct *ISAC_main_inst)
{
  ISACFIX_SubStruct *ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;
  return ISAC_inst->ISACenc_obj.new_framelength;
}


/****************************************************************************
 * WebRtcIsacfix_SetMaxPayloadSize(...)
 *
 * This function sets a limit for the maximum payload size of iSAC. The same
 * value is used both for 30 and 60 msec packets.
 * The absolute max will be valid until next time the function is called.
 * NOTE! This function may override the function WebRtcIsacfix_SetMaxRate()
 *
 * Input:
 *      - ISAC_main_inst    : iSAC instance
 *      - maxPayloadBytes   : maximum size of the payload in bytes
 *                            valid values are between 100 and 400 bytes
 *
 *
 * Return value             : 0 if sucessful
 *                           -1 if error happens
 */

WebRtc_Word16 WebRtcIsacfix_SetMaxPayloadSize(ISACFIX_MainStruct *ISAC_main_inst,
                                              WebRtc_Word16 maxPayloadBytes)
{
  ISACFIX_SubStruct *ISAC_inst;

  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  if((maxPayloadBytes < 100) || (maxPayloadBytes > 400))
  {
    /* maxPayloadBytes is out of valid range */
    return -1;
  }
  else
  {
    /* Set new absolute max, which will not change unless this function
       is called again with a new value */
    ISAC_inst->ISACenc_obj.maxPayloadBytes = maxPayloadBytes;

    /* Set new maximum values for 30 and 60 msec packets */
    if (maxPayloadBytes < ISAC_inst->ISACenc_obj.maxRateInBytes) {
      ISAC_inst->ISACenc_obj.payloadLimitBytes30 = maxPayloadBytes;
    } else {
      ISAC_inst->ISACenc_obj.payloadLimitBytes30 = ISAC_inst->ISACenc_obj.maxRateInBytes;
    }

    if ( maxPayloadBytes < (ISAC_inst->ISACenc_obj.maxRateInBytes << 1)) {
      ISAC_inst->ISACenc_obj.payloadLimitBytes60 = maxPayloadBytes;
    } else {
      ISAC_inst->ISACenc_obj.payloadLimitBytes60 = (ISAC_inst->ISACenc_obj.maxRateInBytes << 1);
    }
  }
  return 0;
}


/****************************************************************************
 * WebRtcIsacfix_SetMaxRate(...)
 *
 * This function sets the maximum rate which the codec may not exceed for a
 * singel packet. The maximum rate is set in bits per second.
 * The codec has an absolute maximum rate of 53400 bits per second (200 bytes
 * per 30 msec).
 * It is possible to set a maximum rate between 32000 and 53400 bits per second.
 *
 * The rate limit is valid until next time the function is called.
 *
 * NOTE! Packet size will never go above the value set if calling
 * WebRtcIsacfix_SetMaxPayloadSize() (default max packet size is 400 bytes).
 *
 * Input:
 *      - ISAC_main_inst    : iSAC instance
 *      - maxRateInBytes    : maximum rate in bits per second,
 *                            valid values are 32000 to 53400 bits
 *
 * Return value             : 0 if sucessful
 *                           -1 if error happens
 */

WebRtc_Word16 WebRtcIsacfix_SetMaxRate(ISACFIX_MainStruct *ISAC_main_inst,
                                       WebRtc_Word32 maxRate)
{
  ISACFIX_SubStruct *ISAC_inst;
  WebRtc_Word16 maxRateInBytes;

  /* typecast pointer to real structure */
  ISAC_inst = (ISACFIX_SubStruct *)ISAC_main_inst;

  if((maxRate < 32000) || (maxRate > 53400))
  {
    /* maxRate is out of valid range */
    return -1;
  }
  else
  {
    /* Calculate maximum number of bytes per 30 msec packets for the given
       maximum rate. Multiply with 30/1000 to get number of bits per 30 msec,
       divide by 8 to get number of bytes per 30 msec:
       maxRateInBytes = floor((maxRate * 30/1000) / 8); */
    maxRateInBytes = (WebRtc_Word16)( WebRtcSpl_DivW32W16ResW16(WEBRTC_SPL_MUL(maxRate, 3), 800) );

    /* Store the value for usage in the WebRtcIsacfix_SetMaxPayloadSize-function */
    ISAC_inst->ISACenc_obj.maxRateInBytes = maxRateInBytes;

    /* For 30 msec packets: if the new limit is below the maximum
       payload size, set a new limit */
    if (maxRateInBytes < ISAC_inst->ISACenc_obj.maxPayloadBytes) {
      ISAC_inst->ISACenc_obj.payloadLimitBytes30 = maxRateInBytes;
    } else {
      ISAC_inst->ISACenc_obj.payloadLimitBytes30 = ISAC_inst->ISACenc_obj.maxPayloadBytes;
    }

    /* For 60 msec packets: if the new limit (times 2) is below the
       maximum payload size, set a new limit */
    if ( (maxRateInBytes << 1) < ISAC_inst->ISACenc_obj.maxPayloadBytes) {
      ISAC_inst->ISACenc_obj.payloadLimitBytes60 = (maxRateInBytes << 1);
    } else {
      ISAC_inst->ISACenc_obj.payloadLimitBytes60 = ISAC_inst->ISACenc_obj.maxPayloadBytes;
    }
  }

  return 0;
}



/****************************************************************************
 * WebRtcIsacfix_version(...)
 *
 * This function returns the version number.
 *
 * Output:
 *      - version  : Pointer to character string
 *
 */

void WebRtcIsacfix_version(char *version)
{
  strcpy(version, "3.6.0");
}
