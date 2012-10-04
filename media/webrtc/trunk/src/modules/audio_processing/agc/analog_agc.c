/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/* analog_agc.c
 *
 * Using a feedback system, determines an appropriate analog volume level
 * given an input signal and current volume level. Targets a conservative
 * signal level and is intended for use with a digital AGC to apply
 * additional gain.
 *
 */

#include <assert.h>
#include <stdlib.h>
#ifdef AGC_DEBUG //test log
#include <stdio.h>
#endif
#include "analog_agc.h"

/* The slope of in Q13*/
static const WebRtc_Word16 kSlope1[8] = {21793, 12517, 7189, 4129, 2372, 1362, 472, 78};

/* The offset in Q14 */
static const WebRtc_Word16 kOffset1[8] = {25395, 23911, 22206, 20737, 19612, 18805, 17951,
        17367};

/* The slope of in Q13*/
static const WebRtc_Word16 kSlope2[8] = {2063, 1731, 1452, 1218, 1021, 857, 597, 337};

/* The offset in Q14 */
static const WebRtc_Word16 kOffset2[8] = {18432, 18379, 18290, 18177, 18052, 17920, 17670,
        17286};

static const WebRtc_Word16 kMuteGuardTimeMs = 8000;
static const WebRtc_Word16 kInitCheck = 42;

/* Default settings if config is not used */
#define AGC_DEFAULT_TARGET_LEVEL 3
#define AGC_DEFAULT_COMP_GAIN 9
/* This is the target level for the analog part in ENV scale. To convert to RMS scale you
 * have to add OFFSET_ENV_TO_RMS.
 */
#define ANALOG_TARGET_LEVEL 11
#define ANALOG_TARGET_LEVEL_2 5 // ANALOG_TARGET_LEVEL / 2
/* Offset between RMS scale (analog part) and ENV scale (digital part). This value actually
 * varies with the FIXED_ANALOG_TARGET_LEVEL, hence we should in the future replace it with
 * a table.
 */
#define OFFSET_ENV_TO_RMS 9
/* The reference input level at which the digital part gives an output of targetLevelDbfs
 * (desired level) if we have no compression gain. This level should be set high enough not
 * to compress the peaks due to the dynamics.
 */
#define DIGITAL_REF_AT_0_COMP_GAIN 4
/* Speed of reference level decrease.
 */
#define DIFF_REF_TO_ANALOG 5

#ifdef MIC_LEVEL_FEEDBACK
#define NUM_BLOCKS_IN_SAT_BEFORE_CHANGE_TARGET 7
#endif
/* Size of analog gain table */
#define GAIN_TBL_LEN 32
/* Matlab code:
 * fprintf(1, '\t%i, %i, %i, %i,\n', round(10.^(linspace(0,10,32)/20) * 2^12));
 */
/* Q12 */
static const WebRtc_UWord16 kGainTableAnalog[GAIN_TBL_LEN] = {4096, 4251, 4412, 4579, 4752,
        4932, 5118, 5312, 5513, 5722, 5938, 6163, 6396, 6638, 6889, 7150, 7420, 7701, 7992,
        8295, 8609, 8934, 9273, 9623, 9987, 10365, 10758, 11165, 11587, 12025, 12480, 12953};

/* Gain/Suppression tables for virtual Mic (in Q10) */
static const WebRtc_UWord16 kGainTableVirtualMic[128] = {1052, 1081, 1110, 1141, 1172, 1204,
        1237, 1271, 1305, 1341, 1378, 1416, 1454, 1494, 1535, 1577, 1620, 1664, 1710, 1757,
        1805, 1854, 1905, 1957, 2010, 2065, 2122, 2180, 2239, 2301, 2364, 2428, 2495, 2563,
        2633, 2705, 2779, 2855, 2933, 3013, 3096, 3180, 3267, 3357, 3449, 3543, 3640, 3739,
        3842, 3947, 4055, 4166, 4280, 4397, 4517, 4640, 4767, 4898, 5032, 5169, 5311, 5456,
        5605, 5758, 5916, 6078, 6244, 6415, 6590, 6770, 6956, 7146, 7341, 7542, 7748, 7960,
        8178, 8402, 8631, 8867, 9110, 9359, 9615, 9878, 10148, 10426, 10711, 11004, 11305,
        11614, 11932, 12258, 12593, 12938, 13292, 13655, 14029, 14412, 14807, 15212, 15628,
        16055, 16494, 16945, 17409, 17885, 18374, 18877, 19393, 19923, 20468, 21028, 21603,
        22194, 22801, 23425, 24065, 24724, 25400, 26095, 26808, 27541, 28295, 29069, 29864,
        30681, 31520, 32382};
static const WebRtc_UWord16 kSuppressionTableVirtualMic[128] = {1024, 1006, 988, 970, 952,
        935, 918, 902, 886, 870, 854, 839, 824, 809, 794, 780, 766, 752, 739, 726, 713, 700,
        687, 675, 663, 651, 639, 628, 616, 605, 594, 584, 573, 563, 553, 543, 533, 524, 514,
        505, 496, 487, 478, 470, 461, 453, 445, 437, 429, 421, 414, 406, 399, 392, 385, 378,
        371, 364, 358, 351, 345, 339, 333, 327, 321, 315, 309, 304, 298, 293, 288, 283, 278,
        273, 268, 263, 258, 254, 249, 244, 240, 236, 232, 227, 223, 219, 215, 211, 208, 204,
        200, 197, 193, 190, 186, 183, 180, 176, 173, 170, 167, 164, 161, 158, 155, 153, 150,
        147, 145, 142, 139, 137, 134, 132, 130, 127, 125, 123, 121, 118, 116, 114, 112, 110,
        108, 106, 104, 102};

/* Table for target energy levels. Values in Q(-7)
 * Matlab code
 * targetLevelTable = fprintf('%d,\t%d,\t%d,\t%d,\n', round((32767*10.^(-(0:63)'/20)).^2*16/2^7) */

static const WebRtc_Word32 kTargetLevelTable[64] = {134209536, 106606424, 84680493, 67264106,
        53429779, 42440782, 33711911, 26778323, 21270778, 16895980, 13420954, 10660642,
        8468049, 6726411, 5342978, 4244078, 3371191, 2677832, 2127078, 1689598, 1342095,
        1066064, 846805, 672641, 534298, 424408, 337119, 267783, 212708, 168960, 134210,
        106606, 84680, 67264, 53430, 42441, 33712, 26778, 21271, 16896, 13421, 10661, 8468,
        6726, 5343, 4244, 3371, 2678, 2127, 1690, 1342, 1066, 847, 673, 534, 424, 337, 268,
        213, 169, 134, 107, 85, 67};

int WebRtcAgc_AddMic(void *state, WebRtc_Word16 *in_mic, WebRtc_Word16 *in_mic_H,
                     WebRtc_Word16 samples)
{
    WebRtc_Word32 nrg, max_nrg, sample, tmp32;
    WebRtc_Word32 *ptr;
    WebRtc_UWord16 targetGainIdx, gain;
    WebRtc_Word16 i, n, L, M, subFrames, tmp16, tmp_speech[16];
    Agc_t *stt;
    stt = (Agc_t *)state;

    //default/initial values corresponding to 10ms for wb and swb
    M = 10;
    L = 16;
    subFrames = 160;

    if (stt->fs == 8000)
    {
        if (samples == 80)
        {
            subFrames = 80;
            M = 10;
            L = 8;
        } else if (samples == 160)
        {
            subFrames = 80;
            M = 20;
            L = 8;
        } else
        {
#ifdef AGC_DEBUG //test log
            fprintf(stt->fpt,
                    "AGC->add_mic, frame %d: Invalid number of samples\n\n",
                    (stt->fcount + 1));
#endif
            return -1;
        }
    } else if (stt->fs == 16000)
    {
        if (samples == 160)
        {
            subFrames = 160;
            M = 10;
            L = 16;
        } else if (samples == 320)
        {
            subFrames = 160;
            M = 20;
            L = 16;
        } else
        {
#ifdef AGC_DEBUG //test log
            fprintf(stt->fpt,
                    "AGC->add_mic, frame %d: Invalid number of samples\n\n",
                    (stt->fcount + 1));
#endif
            return -1;
        }
    } else if (stt->fs == 32000)
    {
        /* SWB is processed as 160 sample for L and H bands */
        if (samples == 160)
        {
            subFrames = 160;
            M = 10;
            L = 16;
        } else
        {
#ifdef AGC_DEBUG
            fprintf(stt->fpt,
                    "AGC->add_mic, frame %d: Invalid sample rate\n\n",
                    (stt->fcount + 1));
#endif
            return -1;
        }
    }

    /* Check for valid pointers based on sampling rate */
    if ((stt->fs == 32000) && (in_mic_H == NULL))
    {
        return -1;
    }
    /* Check for valid pointer for low band */
    if (in_mic == NULL)
    {
        return -1;
    }

    /* apply slowly varying digital gain */
    if (stt->micVol > stt->maxAnalog)
    {
        /* |maxLevel| is strictly >= |micVol|, so this condition should be
         * satisfied here, ensuring there is no divide-by-zero. */
        assert(stt->maxLevel > stt->maxAnalog);

        /* Q1 */
        tmp16 = (WebRtc_Word16)(stt->micVol - stt->maxAnalog);
        tmp32 = WEBRTC_SPL_MUL_16_16(GAIN_TBL_LEN - 1, tmp16);
        tmp16 = (WebRtc_Word16)(stt->maxLevel - stt->maxAnalog);
        targetGainIdx = (WebRtc_UWord16)WEBRTC_SPL_DIV(tmp32, tmp16);
        assert(targetGainIdx < GAIN_TBL_LEN);

        /* Increment through the table towards the target gain.
         * If micVol drops below maxAnalog, we allow the gain
         * to be dropped immediately. */
        if (stt->gainTableIdx < targetGainIdx)
        {
            stt->gainTableIdx++;
        } else if (stt->gainTableIdx > targetGainIdx)
        {
            stt->gainTableIdx--;
        }

        /* Q12 */
        gain = kGainTableAnalog[stt->gainTableIdx];

        for (i = 0; i < samples; i++)
        {
            // For lower band
            tmp32 = WEBRTC_SPL_MUL_16_U16(in_mic[i], gain);
            sample = WEBRTC_SPL_RSHIFT_W32(tmp32, 12);
            if (sample > 32767)
            {
                in_mic[i] = 32767;
            } else if (sample < -32768)
            {
                in_mic[i] = -32768;
            } else
            {
                in_mic[i] = (WebRtc_Word16)sample;
            }

            // For higher band
            if (stt->fs == 32000)
            {
                tmp32 = WEBRTC_SPL_MUL_16_U16(in_mic_H[i], gain);
                sample = WEBRTC_SPL_RSHIFT_W32(tmp32, 12);
                if (sample > 32767)
                {
                    in_mic_H[i] = 32767;
                } else if (sample < -32768)
                {
                    in_mic_H[i] = -32768;
                } else
                {
                    in_mic_H[i] = (WebRtc_Word16)sample;
                }
            }
        }
    } else
    {
        stt->gainTableIdx = 0;
    }

    /* compute envelope */
    if ((M == 10) && (stt->inQueue > 0))
    {
        ptr = stt->env[1];
    } else
    {
        ptr = stt->env[0];
    }

    for (i = 0; i < M; i++)
    {
        /* iterate over samples */
        max_nrg = 0;
        for (n = 0; n < L; n++)
        {
            nrg = WEBRTC_SPL_MUL_16_16(in_mic[i * L + n], in_mic[i * L + n]);
            if (nrg > max_nrg)
            {
                max_nrg = nrg;
            }
        }
        ptr[i] = max_nrg;
    }

    /* compute energy */
    if ((M == 10) && (stt->inQueue > 0))
    {
        ptr = stt->Rxx16w32_array[1];
    } else
    {
        ptr = stt->Rxx16w32_array[0];
    }

    for (i = 0; i < WEBRTC_SPL_RSHIFT_W16(M, 1); i++)
    {
        if (stt->fs == 16000)
        {
            WebRtcSpl_DownsampleBy2(&in_mic[i * 32], 32, tmp_speech, stt->filterState);
        } else
        {
            memcpy(tmp_speech, &in_mic[i * 16], 16 * sizeof(short));
        }
        /* Compute energy in blocks of 16 samples */
        ptr[i] = WebRtcSpl_DotProductWithScale(tmp_speech, tmp_speech, 16, 4);
    }

    /* update queue information */
    if ((stt->inQueue == 0) && (M == 10))
    {
        stt->inQueue = 1;
    } else
    {
        stt->inQueue = 2;
    }

    /* call VAD (use low band only) */
    for (i = 0; i < samples; i += subFrames)
    {
        WebRtcAgc_ProcessVad(&stt->vadMic, &in_mic[i], subFrames);
    }

    return 0;
}

int WebRtcAgc_AddFarend(void *state, const WebRtc_Word16 *in_far, WebRtc_Word16 samples)
{
    WebRtc_Word32 errHandle = 0;
    WebRtc_Word16 i, subFrames;
    Agc_t *stt;
    stt = (Agc_t *)state;

    if (stt == NULL)
    {
        return -1;
    }

    if (stt->fs == 8000)
    {
        if ((samples != 80) && (samples != 160))
        {
#ifdef AGC_DEBUG //test log
            fprintf(stt->fpt,
                    "AGC->add_far_end, frame %d: Invalid number of samples\n\n",
                    stt->fcount);
#endif
            return -1;
        }
        subFrames = 80;
    } else if (stt->fs == 16000)
    {
        if ((samples != 160) && (samples != 320))
        {
#ifdef AGC_DEBUG //test log
            fprintf(stt->fpt,
                    "AGC->add_far_end, frame %d: Invalid number of samples\n\n",
                    stt->fcount);
#endif
            return -1;
        }
        subFrames = 160;
    } else if (stt->fs == 32000)
    {
        if ((samples != 160) && (samples != 320))
        {
#ifdef AGC_DEBUG //test log
            fprintf(stt->fpt,
                    "AGC->add_far_end, frame %d: Invalid number of samples\n\n",
                    stt->fcount);
#endif
            return -1;
        }
        subFrames = 160;
    } else
    {
#ifdef AGC_DEBUG //test log
        fprintf(stt->fpt,
                "AGC->add_far_end, frame %d: Invalid sample rate\n\n",
                stt->fcount + 1);
#endif
        return -1;
    }

    for (i = 0; i < samples; i += subFrames)
    {
        errHandle += WebRtcAgc_AddFarendToDigital(&stt->digitalAgc, &in_far[i], subFrames);
    }

    return errHandle;
}

int WebRtcAgc_VirtualMic(void *agcInst, WebRtc_Word16 *in_near, WebRtc_Word16 *in_near_H,
                         WebRtc_Word16 samples, WebRtc_Word32 micLevelIn,
                         WebRtc_Word32 *micLevelOut)
{
    WebRtc_Word32 tmpFlt, micLevelTmp, gainIdx;
    WebRtc_UWord16 gain;
    WebRtc_Word16 ii;
    Agc_t *stt;

    WebRtc_UWord32 nrg;
    WebRtc_Word16 sampleCntr;
    WebRtc_UWord32 frameNrg = 0;
    WebRtc_UWord32 frameNrgLimit = 5500;
    WebRtc_Word16 numZeroCrossing = 0;
    const WebRtc_Word16 kZeroCrossingLowLim = 15;
    const WebRtc_Word16 kZeroCrossingHighLim = 20;

    stt = (Agc_t *)agcInst;

    /*
     *  Before applying gain decide if this is a low-level signal.
     *  The idea is that digital AGC will not adapt to low-level
     *  signals.
     */
    if (stt->fs != 8000)
    {
        frameNrgLimit = frameNrgLimit << 1;
    }

    frameNrg = WEBRTC_SPL_MUL_16_16(in_near[0], in_near[0]);
    for (sampleCntr = 1; sampleCntr < samples; sampleCntr++)
    {

        // increment frame energy if it is less than the limit
        // the correct value of the energy is not important
        if (frameNrg < frameNrgLimit)
        {
            nrg = WEBRTC_SPL_MUL_16_16(in_near[sampleCntr], in_near[sampleCntr]);
            frameNrg += nrg;
        }

        // Count the zero crossings
        numZeroCrossing += ((in_near[sampleCntr] ^ in_near[sampleCntr - 1]) < 0);
    }

    if ((frameNrg < 500) || (numZeroCrossing <= 5))
    {
        stt->lowLevelSignal = 1;
    } else if (numZeroCrossing <= kZeroCrossingLowLim)
    {
        stt->lowLevelSignal = 0;
    } else if (frameNrg <= frameNrgLimit)
    {
        stt->lowLevelSignal = 1;
    } else if (numZeroCrossing >= kZeroCrossingHighLim)
    {
        stt->lowLevelSignal = 1;
    } else
    {
        stt->lowLevelSignal = 0;
    }

    micLevelTmp = WEBRTC_SPL_LSHIFT_W32(micLevelIn, stt->scale);
    /* Set desired level */
    gainIdx = stt->micVol;
    if (stt->micVol > stt->maxAnalog)
    {
        gainIdx = stt->maxAnalog;
    }
    if (micLevelTmp != stt->micRef)
    {
        /* Something has happened with the physical level, restart. */
        stt->micRef = micLevelTmp;
        stt->micVol = 127;
        *micLevelOut = 127;
        stt->micGainIdx = 127;
        gainIdx = 127;
    }
    /* Pre-process the signal to emulate the microphone level. */
    /* Take one step at a time in the gain table. */
    if (gainIdx > 127)
    {
        gain = kGainTableVirtualMic[gainIdx - 128];
    } else
    {
        gain = kSuppressionTableVirtualMic[127 - gainIdx];
    }
    for (ii = 0; ii < samples; ii++)
    {
        tmpFlt = WEBRTC_SPL_RSHIFT_W32(WEBRTC_SPL_MUL_16_U16(in_near[ii], gain), 10);
        if (tmpFlt > 32767)
        {
            tmpFlt = 32767;
            gainIdx--;
            if (gainIdx >= 127)
            {
                gain = kGainTableVirtualMic[gainIdx - 127];
            } else
            {
                gain = kSuppressionTableVirtualMic[127 - gainIdx];
            }
        }
        if (tmpFlt < -32768)
        {
            tmpFlt = -32768;
            gainIdx--;
            if (gainIdx >= 127)
            {
                gain = kGainTableVirtualMic[gainIdx - 127];
            } else
            {
                gain = kSuppressionTableVirtualMic[127 - gainIdx];
            }
        }
        in_near[ii] = (WebRtc_Word16)tmpFlt;
        if (stt->fs == 32000)
        {
            tmpFlt = WEBRTC_SPL_MUL_16_U16(in_near_H[ii], gain);
            tmpFlt = WEBRTC_SPL_RSHIFT_W32(tmpFlt, 10);
            if (tmpFlt > 32767)
            {
                tmpFlt = 32767;
            }
            if (tmpFlt < -32768)
            {
                tmpFlt = -32768;
            }
            in_near_H[ii] = (WebRtc_Word16)tmpFlt;
        }
    }
    /* Set the level we (finally) used */
    stt->micGainIdx = gainIdx;
//    *micLevelOut = stt->micGainIdx;
    *micLevelOut = WEBRTC_SPL_RSHIFT_W32(stt->micGainIdx, stt->scale);
    /* Add to Mic as if it was the output from a true microphone */
    if (WebRtcAgc_AddMic(agcInst, in_near, in_near_H, samples) != 0)
    {
        return -1;
    }
    return 0;
}

void WebRtcAgc_UpdateAgcThresholds(Agc_t *stt)
{

    WebRtc_Word16 tmp16;
#ifdef MIC_LEVEL_FEEDBACK
    int zeros;

    if (stt->micLvlSat)
    {
        /* Lower the analog target level since we have reached its maximum */
        zeros = WebRtcSpl_NormW32(stt->Rxx160_LPw32);
        stt->targetIdxOffset = WEBRTC_SPL_RSHIFT_W16((3 * zeros) - stt->targetIdx - 2, 2);
    }
#endif

    /* Set analog target level in envelope dBOv scale */
    tmp16 = (DIFF_REF_TO_ANALOG * stt->compressionGaindB) + ANALOG_TARGET_LEVEL_2;
    tmp16 = WebRtcSpl_DivW32W16ResW16((WebRtc_Word32)tmp16, ANALOG_TARGET_LEVEL);
    stt->analogTarget = DIGITAL_REF_AT_0_COMP_GAIN + tmp16;
    if (stt->analogTarget < DIGITAL_REF_AT_0_COMP_GAIN)
    {
        stt->analogTarget = DIGITAL_REF_AT_0_COMP_GAIN;
    }
    if (stt->agcMode == kAgcModeFixedDigital)
    {
        /* Adjust for different parameter interpretation in FixedDigital mode */
        stt->analogTarget = stt->compressionGaindB;
    }
#ifdef MIC_LEVEL_FEEDBACK
    stt->analogTarget += stt->targetIdxOffset;
#endif
    /* Since the offset between RMS and ENV is not constant, we should make this into a
     * table, but for now, we'll stick with a constant, tuned for the chosen analog
     * target level.
     */
    stt->targetIdx = ANALOG_TARGET_LEVEL + OFFSET_ENV_TO_RMS;
#ifdef MIC_LEVEL_FEEDBACK
    stt->targetIdx += stt->targetIdxOffset;
#endif
    /* Analog adaptation limits */
    /* analogTargetLevel = round((32767*10^(-targetIdx/20))^2*16/2^7) */
    stt->analogTargetLevel = RXX_BUFFER_LEN * kTargetLevelTable[stt->targetIdx]; /* ex. -20 dBov */
    stt->startUpperLimit = RXX_BUFFER_LEN * kTargetLevelTable[stt->targetIdx - 1];/* -19 dBov */
    stt->startLowerLimit = RXX_BUFFER_LEN * kTargetLevelTable[stt->targetIdx + 1];/* -21 dBov */
    stt->upperPrimaryLimit = RXX_BUFFER_LEN * kTargetLevelTable[stt->targetIdx - 2];/* -18 dBov */
    stt->lowerPrimaryLimit = RXX_BUFFER_LEN * kTargetLevelTable[stt->targetIdx + 2];/* -22 dBov */
    stt->upperSecondaryLimit = RXX_BUFFER_LEN * kTargetLevelTable[stt->targetIdx - 5];/* -15 dBov */
    stt->lowerSecondaryLimit = RXX_BUFFER_LEN * kTargetLevelTable[stt->targetIdx + 5];/* -25 dBov */
    stt->upperLimit = stt->startUpperLimit;
    stt->lowerLimit = stt->startLowerLimit;
}

void WebRtcAgc_SaturationCtrl(Agc_t *stt, WebRtc_UWord8 *saturated, WebRtc_Word32 *env)
{
    WebRtc_Word16 i, tmpW16;

    /* Check if the signal is saturated */
    for (i = 0; i < 10; i++)
    {
        tmpW16 = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(env[i], 20);
        if (tmpW16 > 875)
        {
            stt->envSum += tmpW16;
        }
    }

    if (stt->envSum > 25000)
    {
        *saturated = 1;
        stt->envSum = 0;
    }

    /* stt->envSum *= 0.99; */
    stt->envSum = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(stt->envSum,
            (WebRtc_Word16)32440, 15);
}

void WebRtcAgc_ZeroCtrl(Agc_t *stt, WebRtc_Word32 *inMicLevel, WebRtc_Word32 *env)
{
    WebRtc_Word16 i;
    WebRtc_Word32 tmp32 = 0;
    WebRtc_Word32 midVal;

    /* Is the input signal zero? */
    for (i = 0; i < 10; i++)
    {
        tmp32 += env[i];
    }

    /* Each block is allowed to have a few non-zero
     * samples.
     */
    if (tmp32 < 500)
    {
        stt->msZero += 10;
    } else
    {
        stt->msZero = 0;
    }

    if (stt->muteGuardMs > 0)
    {
        stt->muteGuardMs -= 10;
    }

    if (stt->msZero > 500)
    {
        stt->msZero = 0;

        /* Increase microphone level only if it's less than 50% */
        midVal = WEBRTC_SPL_RSHIFT_W32(stt->maxAnalog + stt->minLevel + 1, 1);
        if (*inMicLevel < midVal)
        {
            /* *inMicLevel *= 1.1; */
            tmp32 = WEBRTC_SPL_MUL(1126, *inMicLevel);
            *inMicLevel = WEBRTC_SPL_RSHIFT_W32(tmp32, 10);
            /* Reduces risk of a muted mic repeatedly triggering excessive levels due
             * to zero signal detection. */
            *inMicLevel = WEBRTC_SPL_MIN(*inMicLevel, stt->zeroCtrlMax);
            stt->micVol = *inMicLevel;
        }

#ifdef AGC_DEBUG //test log
        fprintf(stt->fpt,
                "\t\tAGC->zeroCntrl, frame %d: 500 ms under threshold, micVol:\n",
                stt->fcount, stt->micVol);
#endif

        stt->activeSpeech = 0;
        stt->Rxx16_LPw32Max = 0;

        /* The AGC has a tendency (due to problems with the VAD parameters), to
         * vastly increase the volume after a muting event. This timer prevents
         * upwards adaptation for a short period. */
        stt->muteGuardMs = kMuteGuardTimeMs;
    }
}

void WebRtcAgc_SpeakerInactiveCtrl(Agc_t *stt)
{
    /* Check if the near end speaker is inactive.
     * If that is the case the VAD threshold is
     * increased since the VAD speech model gets
     * more sensitive to any sound after a long
     * silence.
     */

    WebRtc_Word32 tmp32;
    WebRtc_Word16 vadThresh;

    if (stt->vadMic.stdLongTerm < 2500)
    {
        stt->vadThreshold = 1500;
    } else
    {
        vadThresh = kNormalVadThreshold;
        if (stt->vadMic.stdLongTerm < 4500)
        {
            /* Scale between min and max threshold */
            vadThresh += WEBRTC_SPL_RSHIFT_W16(4500 - stt->vadMic.stdLongTerm, 1);
        }

        /* stt->vadThreshold = (31 * stt->vadThreshold + vadThresh) / 32; */
        tmp32 = (WebRtc_Word32)vadThresh;
        tmp32 += WEBRTC_SPL_MUL_16_16((WebRtc_Word16)31, stt->vadThreshold);
        stt->vadThreshold = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(tmp32, 5);
    }
}

void WebRtcAgc_ExpCurve(WebRtc_Word16 volume, WebRtc_Word16 *index)
{
    // volume in Q14
    // index in [0-7]
    /* 8 different curves */
    if (volume > 5243)
    {
        if (volume > 7864)
        {
            if (volume > 12124)
            {
                *index = 7;
            } else
            {
                *index = 6;
            }
        } else
        {
            if (volume > 6554)
            {
                *index = 5;
            } else
            {
                *index = 4;
            }
        }
    } else
    {
        if (volume > 2621)
        {
            if (volume > 3932)
            {
                *index = 3;
            } else
            {
                *index = 2;
            }
        } else
        {
            if (volume > 1311)
            {
                *index = 1;
            } else
            {
                *index = 0;
            }
        }
    }
}

WebRtc_Word32 WebRtcAgc_ProcessAnalog(void *state, WebRtc_Word32 inMicLevel,
                                        WebRtc_Word32 *outMicLevel,
                                        WebRtc_Word16 vadLogRatio,
                                        WebRtc_Word16 echo, WebRtc_UWord8 *saturationWarning)
{
    WebRtc_UWord32 tmpU32;
    WebRtc_Word32 Rxx16w32, tmp32;
    WebRtc_Word32 inMicLevelTmp, lastMicVol;
    WebRtc_Word16 i;
    WebRtc_UWord8 saturated = 0;
    Agc_t *stt;

    stt = (Agc_t *)state;
    inMicLevelTmp = WEBRTC_SPL_LSHIFT_W32(inMicLevel, stt->scale);

    if (inMicLevelTmp > stt->maxAnalog)
    {
#ifdef AGC_DEBUG //test log
        fprintf(stt->fpt, "\tAGC->ProcessAnalog, frame %d: micLvl > maxAnalog\n", stt->fcount);
#endif
        return -1;
    } else if (inMicLevelTmp < stt->minLevel)
    {
#ifdef AGC_DEBUG //test log
        fprintf(stt->fpt, "\tAGC->ProcessAnalog, frame %d: micLvl < minLevel\n", stt->fcount);
#endif
        return -1;
    }

    if (stt->firstCall == 0)
    {
        WebRtc_Word32 tmpVol;
        stt->firstCall = 1;
        tmp32 = WEBRTC_SPL_RSHIFT_W32((stt->maxLevel - stt->minLevel) * (WebRtc_Word32)51, 9);
        tmpVol = (stt->minLevel + tmp32);

        /* If the mic level is very low at start, increase it! */
        if ((inMicLevelTmp < tmpVol) && (stt->agcMode == kAgcModeAdaptiveAnalog))
        {
            inMicLevelTmp = tmpVol;
        }
        stt->micVol = inMicLevelTmp;
    }

    /* Set the mic level to the previous output value if there is digital input gain */
    if ((inMicLevelTmp == stt->maxAnalog) && (stt->micVol > stt->maxAnalog))
    {
        inMicLevelTmp = stt->micVol;
    }

    /* If the mic level was manually changed to a very low value raise it! */
    if ((inMicLevelTmp != stt->micVol) && (inMicLevelTmp < stt->minOutput))
    {
        tmp32 = WEBRTC_SPL_RSHIFT_W32((stt->maxLevel - stt->minLevel) * (WebRtc_Word32)51, 9);
        inMicLevelTmp = (stt->minLevel + tmp32);
        stt->micVol = inMicLevelTmp;
#ifdef MIC_LEVEL_FEEDBACK
        //stt->numBlocksMicLvlSat = 0;
#endif
#ifdef AGC_DEBUG //test log
        fprintf(stt->fpt,
                "\tAGC->ProcessAnalog, frame %d: micLvl < minLevel by manual decrease, raise vol\n",
                stt->fcount);
#endif
    }

    if (inMicLevelTmp != stt->micVol)
    {
        // Incoming level mismatch; update our level.
        // This could be the case if the volume is changed manually, or if the
        // sound device has a low volume resolution.
        stt->micVol = inMicLevelTmp;
    }

    if (inMicLevelTmp > stt->maxLevel)
    {
        // Always allow the user to raise the volume above the maxLevel.
        stt->maxLevel = inMicLevelTmp;
    }

    // Store last value here, after we've taken care of manual updates etc.
    lastMicVol = stt->micVol;

    /* Checks if the signal is saturated. Also a check if individual samples
     * are larger than 12000 is done. If they are the counter for increasing
     * the volume level is set to -100ms
     */
    WebRtcAgc_SaturationCtrl(stt, &saturated, stt->env[0]);

    /* The AGC is always allowed to lower the level if the signal is saturated */
    if (saturated == 1)
    {
        /* Lower the recording level
         * Rxx160_LP is adjusted down because it is so slow it could
         * cause the AGC to make wrong decisions. */
        /* stt->Rxx160_LPw32 *= 0.875; */
        stt->Rxx160_LPw32 = WEBRTC_SPL_MUL(WEBRTC_SPL_RSHIFT_W32(stt->Rxx160_LPw32, 3), 7);

        stt->zeroCtrlMax = stt->micVol;

        /* stt->micVol *= 0.903; */
        tmp32 = inMicLevelTmp - stt->minLevel;
        tmpU32 = WEBRTC_SPL_UMUL(29591, (WebRtc_UWord32)(tmp32));
        stt->micVol = (WebRtc_Word32)WEBRTC_SPL_RSHIFT_U32(tmpU32, 15) + stt->minLevel;
        if (stt->micVol > lastMicVol - 2)
        {
            stt->micVol = lastMicVol - 2;
        }
        inMicLevelTmp = stt->micVol;

#ifdef AGC_DEBUG //test log
        fprintf(stt->fpt,
                "\tAGC->ProcessAnalog, frame %d: saturated, micVol = %d\n",
                stt->fcount, stt->micVol);
#endif

        if (stt->micVol < stt->minOutput)
        {
            *saturationWarning = 1;
        }

        /* Reset counter for decrease of volume level to avoid
         * decreasing too much. The saturation control can still
         * lower the level if needed. */
        stt->msTooHigh = -100;

        /* Enable the control mechanism to ensure that our measure,
         * Rxx160_LP, is in the correct range. This must be done since
         * the measure is very slow. */
        stt->activeSpeech = 0;
        stt->Rxx16_LPw32Max = 0;

        /* Reset to initial values */
        stt->msecSpeechInnerChange = kMsecSpeechInner;
        stt->msecSpeechOuterChange = kMsecSpeechOuter;
        stt->changeToSlowMode = 0;

        stt->muteGuardMs = 0;

        stt->upperLimit = stt->startUpperLimit;
        stt->lowerLimit = stt->startLowerLimit;
#ifdef MIC_LEVEL_FEEDBACK
        //stt->numBlocksMicLvlSat = 0;
#endif
    }

    /* Check if the input speech is zero. If so the mic volume
     * is increased. On some computers the input is zero up as high
     * level as 17% */
    WebRtcAgc_ZeroCtrl(stt, &inMicLevelTmp, stt->env[0]);

    /* Check if the near end speaker is inactive.
     * If that is the case the VAD threshold is
     * increased since the VAD speech model gets
     * more sensitive to any sound after a long
     * silence.
     */
    WebRtcAgc_SpeakerInactiveCtrl(stt);

    for (i = 0; i < 5; i++)
    {
        /* Computed on blocks of 16 samples */

        Rxx16w32 = stt->Rxx16w32_array[0][i];

        /* Rxx160w32 in Q(-7) */
        tmp32 = WEBRTC_SPL_RSHIFT_W32(Rxx16w32 - stt->Rxx16_vectorw32[stt->Rxx16pos], 3);
        stt->Rxx160w32 = stt->Rxx160w32 + tmp32;
        stt->Rxx16_vectorw32[stt->Rxx16pos] = Rxx16w32;

        /* Circular buffer */
        stt->Rxx16pos++;
        if (stt->Rxx16pos == RXX_BUFFER_LEN)
        {
            stt->Rxx16pos = 0;
        }

        /* Rxx16_LPw32 in Q(-4) */
        tmp32 = WEBRTC_SPL_RSHIFT_W32(Rxx16w32 - stt->Rxx16_LPw32, kAlphaShortTerm);
        stt->Rxx16_LPw32 = (stt->Rxx16_LPw32) + tmp32;

        if (vadLogRatio > stt->vadThreshold)
        {
            /* Speech detected! */

            /* Check if Rxx160_LP is in the correct range. If
             * it is too high/low then we set it to the maximum of
             * Rxx16_LPw32 during the first 200ms of speech.
             */
            if (stt->activeSpeech < 250)
            {
                stt->activeSpeech += 2;

                if (stt->Rxx16_LPw32 > stt->Rxx16_LPw32Max)
                {
                    stt->Rxx16_LPw32Max = stt->Rxx16_LPw32;
                }
            } else if (stt->activeSpeech == 250)
            {
                stt->activeSpeech += 2;
                tmp32 = WEBRTC_SPL_RSHIFT_W32(stt->Rxx16_LPw32Max, 3);
                stt->Rxx160_LPw32 = WEBRTC_SPL_MUL(tmp32, RXX_BUFFER_LEN);
            }

            tmp32 = WEBRTC_SPL_RSHIFT_W32(stt->Rxx160w32 - stt->Rxx160_LPw32, kAlphaLongTerm);
            stt->Rxx160_LPw32 = stt->Rxx160_LPw32 + tmp32;

            if (stt->Rxx160_LPw32 > stt->upperSecondaryLimit)
            {
                stt->msTooHigh += 2;
                stt->msTooLow = 0;
                stt->changeToSlowMode = 0;

                if (stt->msTooHigh > stt->msecSpeechOuterChange)
                {
                    stt->msTooHigh = 0;

                    /* Lower the recording level */
                    /* Multiply by 0.828125 which corresponds to decreasing ~0.8dB */
                    tmp32 = WEBRTC_SPL_RSHIFT_W32(stt->Rxx160_LPw32, 6);
                    stt->Rxx160_LPw32 = WEBRTC_SPL_MUL(tmp32, 53);

                    /* Reduce the max gain to avoid excessive oscillation
                     * (but never drop below the maximum analog level).
                     * stt->maxLevel = (15 * stt->maxLevel + stt->micVol) / 16;
                     */
                    tmp32 = (15 * stt->maxLevel) + stt->micVol;
                    stt->maxLevel = WEBRTC_SPL_RSHIFT_W32(tmp32, 4);
                    stt->maxLevel = WEBRTC_SPL_MAX(stt->maxLevel, stt->maxAnalog);

                    stt->zeroCtrlMax = stt->micVol;

                    /* 0.95 in Q15 */
                    tmp32 = inMicLevelTmp - stt->minLevel;
                    tmpU32 = WEBRTC_SPL_UMUL(31130, (WebRtc_UWord32)(tmp32));
                    stt->micVol = (WebRtc_Word32)WEBRTC_SPL_RSHIFT_U32(tmpU32, 15) + stt->minLevel;
                    if (stt->micVol > lastMicVol - 1)
                    {
                        stt->micVol = lastMicVol - 1;
                    }
                    inMicLevelTmp = stt->micVol;

                    /* Enable the control mechanism to ensure that our measure,
                     * Rxx160_LP, is in the correct range.
                     */
                    stt->activeSpeech = 0;
                    stt->Rxx16_LPw32Max = 0;
#ifdef MIC_LEVEL_FEEDBACK
                    //stt->numBlocksMicLvlSat = 0;
#endif
#ifdef AGC_DEBUG //test log
                    fprintf(stt->fpt,
                            "\tAGC->ProcessAnalog, frame %d: measure > 2ndUpperLim, micVol = %d, maxLevel = %d\n",
                            stt->fcount, stt->micVol, stt->maxLevel);
#endif
                }
            } else if (stt->Rxx160_LPw32 > stt->upperLimit)
            {
                stt->msTooHigh += 2;
                stt->msTooLow = 0;
                stt->changeToSlowMode = 0;

                if (stt->msTooHigh > stt->msecSpeechInnerChange)
                {
                    /* Lower the recording level */
                    stt->msTooHigh = 0;
                    /* Multiply by 0.828125 which corresponds to decreasing ~0.8dB */
                    tmp32 = WEBRTC_SPL_RSHIFT_W32(stt->Rxx160_LPw32, 6);
                    stt->Rxx160_LPw32 = WEBRTC_SPL_MUL(tmp32, 53);

                    /* Reduce the max gain to avoid excessive oscillation
                     * (but never drop below the maximum analog level).
                     * stt->maxLevel = (15 * stt->maxLevel + stt->micVol) / 16;
                     */
                    tmp32 = (15 * stt->maxLevel) + stt->micVol;
                    stt->maxLevel = WEBRTC_SPL_RSHIFT_W32(tmp32, 4);
                    stt->maxLevel = WEBRTC_SPL_MAX(stt->maxLevel, stt->maxAnalog);

                    stt->zeroCtrlMax = stt->micVol;

                    /* 0.965 in Q15 */
                    tmp32 = inMicLevelTmp - stt->minLevel;
                    tmpU32 = WEBRTC_SPL_UMUL(31621, (WebRtc_UWord32)(inMicLevelTmp - stt->minLevel));
                    stt->micVol = (WebRtc_Word32)WEBRTC_SPL_RSHIFT_U32(tmpU32, 15) + stt->minLevel;
                    if (stt->micVol > lastMicVol - 1)
                    {
                        stt->micVol = lastMicVol - 1;
                    }
                    inMicLevelTmp = stt->micVol;

#ifdef MIC_LEVEL_FEEDBACK
                    //stt->numBlocksMicLvlSat = 0;
#endif
#ifdef AGC_DEBUG //test log
                    fprintf(stt->fpt,
                            "\tAGC->ProcessAnalog, frame %d: measure > UpperLim, micVol = %d, maxLevel = %d\n",
                            stt->fcount, stt->micVol, stt->maxLevel);
#endif
                }
            } else if (stt->Rxx160_LPw32 < stt->lowerSecondaryLimit)
            {
                stt->msTooHigh = 0;
                stt->changeToSlowMode = 0;
                stt->msTooLow += 2;

                if (stt->msTooLow > stt->msecSpeechOuterChange)
                {
                    /* Raise the recording level */
                    WebRtc_Word16 index, weightFIX;
                    WebRtc_Word16 volNormFIX = 16384; // =1 in Q14.

                    stt->msTooLow = 0;

                    /* Normalize the volume level */
                    tmp32 = WEBRTC_SPL_LSHIFT_W32(inMicLevelTmp - stt->minLevel, 14);
                    if (stt->maxInit != stt->minLevel)
                    {
                        volNormFIX = (WebRtc_Word16)WEBRTC_SPL_DIV(tmp32,
                                                              (stt->maxInit - stt->minLevel));
                    }

                    /* Find correct curve */
                    WebRtcAgc_ExpCurve(volNormFIX, &index);

                    /* Compute weighting factor for the volume increase, 32^(-2*X)/2+1.05 */
                    weightFIX = kOffset1[index]
                              - (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(kSlope1[index],
                                                                         volNormFIX, 13);

                    /* stt->Rxx160_LPw32 *= 1.047 [~0.2 dB]; */
                    tmp32 = WEBRTC_SPL_RSHIFT_W32(stt->Rxx160_LPw32, 6);
                    stt->Rxx160_LPw32 = WEBRTC_SPL_MUL(tmp32, 67);

                    tmp32 = inMicLevelTmp - stt->minLevel;
                    tmpU32 = ((WebRtc_UWord32)weightFIX * (WebRtc_UWord32)(inMicLevelTmp - stt->minLevel));
                    stt->micVol = (WebRtc_Word32)WEBRTC_SPL_RSHIFT_U32(tmpU32, 14) + stt->minLevel;
                    if (stt->micVol < lastMicVol + 2)
                    {
                        stt->micVol = lastMicVol + 2;
                    }

                    inMicLevelTmp = stt->micVol;

#ifdef MIC_LEVEL_FEEDBACK
                    /* Count ms in level saturation */
                    //if (stt->micVol > stt->maxAnalog) {
                    if (stt->micVol > 150)
                    {
                        /* mic level is saturated */
                        stt->numBlocksMicLvlSat++;
                        fprintf(stderr, "Sat mic Level: %d\n", stt->numBlocksMicLvlSat);
                    }
#endif
#ifdef AGC_DEBUG //test log
                    fprintf(stt->fpt,
                            "\tAGC->ProcessAnalog, frame %d: measure < 2ndLowerLim, micVol = %d\n",
                            stt->fcount, stt->micVol);
#endif
                }
            } else if (stt->Rxx160_LPw32 < stt->lowerLimit)
            {
                stt->msTooHigh = 0;
                stt->changeToSlowMode = 0;
                stt->msTooLow += 2;

                if (stt->msTooLow > stt->msecSpeechInnerChange)
                {
                    /* Raise the recording level */
                    WebRtc_Word16 index, weightFIX;
                    WebRtc_Word16 volNormFIX = 16384; // =1 in Q14.

                    stt->msTooLow = 0;

                    /* Normalize the volume level */
                    tmp32 = WEBRTC_SPL_LSHIFT_W32(inMicLevelTmp - stt->minLevel, 14);
                    if (stt->maxInit != stt->minLevel)
                    {
                        volNormFIX = (WebRtc_Word16)WEBRTC_SPL_DIV(tmp32,
                                                              (stt->maxInit - stt->minLevel));
                    }

                    /* Find correct curve */
                    WebRtcAgc_ExpCurve(volNormFIX, &index);

                    /* Compute weighting factor for the volume increase, (3.^(-2.*X))/8+1 */
                    weightFIX = kOffset2[index]
                              - (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(kSlope2[index],
                                                                         volNormFIX, 13);

                    /* stt->Rxx160_LPw32 *= 1.047 [~0.2 dB]; */
                    tmp32 = WEBRTC_SPL_RSHIFT_W32(stt->Rxx160_LPw32, 6);
                    stt->Rxx160_LPw32 = WEBRTC_SPL_MUL(tmp32, 67);

                    tmp32 = inMicLevelTmp - stt->minLevel;
                    tmpU32 = ((WebRtc_UWord32)weightFIX * (WebRtc_UWord32)(inMicLevelTmp - stt->minLevel));
                    stt->micVol = (WebRtc_Word32)WEBRTC_SPL_RSHIFT_U32(tmpU32, 14) + stt->minLevel;
                    if (stt->micVol < lastMicVol + 1)
                    {
                        stt->micVol = lastMicVol + 1;
                    }

                    inMicLevelTmp = stt->micVol;

#ifdef MIC_LEVEL_FEEDBACK
                    /* Count ms in level saturation */
                    //if (stt->micVol > stt->maxAnalog) {
                    if (stt->micVol > 150)
                    {
                        /* mic level is saturated */
                        stt->numBlocksMicLvlSat++;
                        fprintf(stderr, "Sat mic Level: %d\n", stt->numBlocksMicLvlSat);
                    }
#endif
#ifdef AGC_DEBUG //test log
                    fprintf(stt->fpt,
                            "\tAGC->ProcessAnalog, frame %d: measure < LowerLim, micVol = %d\n",
                            stt->fcount, stt->micVol);
#endif

                }
            } else
            {
                /* The signal is inside the desired range which is:
                 * lowerLimit < Rxx160_LP/640 < upperLimit
                 */
                if (stt->changeToSlowMode > 4000)
                {
                    stt->msecSpeechInnerChange = 1000;
                    stt->msecSpeechOuterChange = 500;
                    stt->upperLimit = stt->upperPrimaryLimit;
                    stt->lowerLimit = stt->lowerPrimaryLimit;
                } else
                {
                    stt->changeToSlowMode += 2; // in milliseconds
                }
                stt->msTooLow = 0;
                stt->msTooHigh = 0;

                stt->micVol = inMicLevelTmp;

            }
#ifdef MIC_LEVEL_FEEDBACK
            if (stt->numBlocksMicLvlSat > NUM_BLOCKS_IN_SAT_BEFORE_CHANGE_TARGET)
            {
                stt->micLvlSat = 1;
                fprintf(stderr, "target before = %d (%d)\n", stt->analogTargetLevel, stt->targetIdx);
                WebRtcAgc_UpdateAgcThresholds(stt);
                WebRtcAgc_CalculateGainTable(&(stt->digitalAgc.gainTable[0]),
                        stt->compressionGaindB, stt->targetLevelDbfs, stt->limiterEnable,
                        stt->analogTarget);
                stt->numBlocksMicLvlSat = 0;
                stt->micLvlSat = 0;
                fprintf(stderr, "target offset = %d\n", stt->targetIdxOffset);
                fprintf(stderr, "target after  = %d (%d)\n", stt->analogTargetLevel, stt->targetIdx);
            }
#endif
        }
    }

    /* Ensure gain is not increased in presence of echo or after a mute event
     * (but allow the zeroCtrl() increase on the frame of a mute detection).
     */
    if (echo == 1 || (stt->muteGuardMs > 0 && stt->muteGuardMs < kMuteGuardTimeMs))
    {
        if (stt->micVol > lastMicVol)
        {
            stt->micVol = lastMicVol;
        }
    }

    /* limit the gain */
    if (stt->micVol > stt->maxLevel)
    {
        stt->micVol = stt->maxLevel;
    } else if (stt->micVol < stt->minOutput)
    {
        stt->micVol = stt->minOutput;
    }

    *outMicLevel = WEBRTC_SPL_RSHIFT_W32(stt->micVol, stt->scale);
    if (*outMicLevel > WEBRTC_SPL_RSHIFT_W32(stt->maxAnalog, stt->scale))
    {
        *outMicLevel = WEBRTC_SPL_RSHIFT_W32(stt->maxAnalog, stt->scale);
    }

    return 0;
}

int WebRtcAgc_Process(void *agcInst, const WebRtc_Word16 *in_near,
                      const WebRtc_Word16 *in_near_H, WebRtc_Word16 samples,
                      WebRtc_Word16 *out, WebRtc_Word16 *out_H, WebRtc_Word32 inMicLevel,
                      WebRtc_Word32 *outMicLevel, WebRtc_Word16 echo,
                      WebRtc_UWord8 *saturationWarning)
{
    Agc_t *stt;
    WebRtc_Word32 inMicLevelTmp;
    WebRtc_Word16 subFrames, i;
    WebRtc_UWord8 satWarningTmp = 0;

    stt = (Agc_t *)agcInst;

    //
    if (stt == NULL)
    {
        return -1;
    }
    //


    if (stt->fs == 8000)
    {
        if ((samples != 80) && (samples != 160))
        {
#ifdef AGC_DEBUG //test log
            fprintf(stt->fpt,
                    "AGC->Process, frame %d: Invalid number of samples\n\n", stt->fcount);
#endif
            return -1;
        }
        subFrames = 80;
    } else if (stt->fs == 16000)
    {
        if ((samples != 160) && (samples != 320))
        {
#ifdef AGC_DEBUG //test log
            fprintf(stt->fpt,
                    "AGC->Process, frame %d: Invalid number of samples\n\n", stt->fcount);
#endif
            return -1;
        }
        subFrames = 160;
    } else if (stt->fs == 32000)
    {
        if ((samples != 160) && (samples != 320))
        {
#ifdef AGC_DEBUG //test log
            fprintf(stt->fpt,
                    "AGC->Process, frame %d: Invalid number of samples\n\n", stt->fcount);
#endif
            return -1;
        }
        subFrames = 160;
    } else
    {
#ifdef AGC_DEBUG// test log
        fprintf(stt->fpt,
                "AGC->Process, frame %d: Invalid sample rate\n\n", stt->fcount);
#endif
        return -1;
    }

    /* Check for valid pointers based on sampling rate */
    if (stt->fs == 32000 && in_near_H == NULL)
    {
        return -1;
    }
    /* Check for valid pointers for low band */
    if (in_near == NULL)
    {
        return -1;
    }

    *saturationWarning = 0;
    //TODO: PUT IN RANGE CHECKING FOR INPUT LEVELS
    *outMicLevel = inMicLevel;
    inMicLevelTmp = inMicLevel;

    // TODO(andrew): clearly we don't need input and output pointers...
    //   Change the interface to take a shared input/output.
    if (in_near != out)
    {
        // Only needed if they don't already point to the same place.
        memcpy(out, in_near, samples * sizeof(WebRtc_Word16));
    }
    if (stt->fs == 32000)
    {
        if (in_near_H != out_H)
        {
            memcpy(out_H, in_near_H, samples * sizeof(WebRtc_Word16));
        }
    }

#ifdef AGC_DEBUG//test log
    stt->fcount++;
#endif

    for (i = 0; i < samples; i += subFrames)
    {
        if (WebRtcAgc_ProcessDigital(&stt->digitalAgc, &in_near[i], &in_near_H[i], &out[i], &out_H[i],
                           stt->fs, stt->lowLevelSignal) == -1)
        {
#ifdef AGC_DEBUG//test log
            fprintf(stt->fpt, "AGC->Process, frame %d: Error from DigAGC\n\n", stt->fcount);
#endif
            return -1;
        }
        if ((stt->agcMode < kAgcModeFixedDigital) && ((stt->lowLevelSignal == 0)
                || (stt->agcMode != kAgcModeAdaptiveDigital)))
        {
            if (WebRtcAgc_ProcessAnalog(agcInst, inMicLevelTmp, outMicLevel,
                                          stt->vadMic.logRatio, echo, saturationWarning) == -1)
            {
                return -1;
            }
        }
#ifdef AGC_DEBUG//test log
        fprintf(stt->agcLog, "%5d\t%d\t%d\t%d\n", stt->fcount, inMicLevelTmp, *outMicLevel, stt->maxLevel, stt->micVol);
#endif

        /* update queue */
        if (stt->inQueue > 1)
        {
            memcpy(stt->env[0], stt->env[1], 10 * sizeof(WebRtc_Word32));
            memcpy(stt->Rxx16w32_array[0], stt->Rxx16w32_array[1], 5 * sizeof(WebRtc_Word32));
        }

        if (stt->inQueue > 0)
        {
            stt->inQueue--;
        }

        /* If 20ms frames are used the input mic level must be updated so that
         * the analog AGC does not think that there has been a manual volume
         * change. */
        inMicLevelTmp = *outMicLevel;

        /* Store a positive saturation warning. */
        if (*saturationWarning == 1)
        {
            satWarningTmp = 1;
        }
    }

    /* Trigger the saturation warning if displayed by any of the frames. */
    *saturationWarning = satWarningTmp;

    return 0;
}

int WebRtcAgc_set_config(void *agcInst, WebRtcAgc_config_t agcConfig)
{
    Agc_t *stt;
    stt = (Agc_t *)agcInst;

    if (stt == NULL)
    {
        return -1;
    }

    if (stt->initFlag != kInitCheck)
    {
        stt->lastError = AGC_UNINITIALIZED_ERROR;
        return -1;
    }

    if (agcConfig.limiterEnable != kAgcFalse && agcConfig.limiterEnable != kAgcTrue)
    {
        stt->lastError = AGC_BAD_PARAMETER_ERROR;
        return -1;
    }
    stt->limiterEnable = agcConfig.limiterEnable;
    stt->compressionGaindB = agcConfig.compressionGaindB;
    if ((agcConfig.targetLevelDbfs < 0) || (agcConfig.targetLevelDbfs > 31))
    {
        stt->lastError = AGC_BAD_PARAMETER_ERROR;
        return -1;
    }
    stt->targetLevelDbfs = agcConfig.targetLevelDbfs;

    if (stt->agcMode == kAgcModeFixedDigital)
    {
        /* Adjust for different parameter interpretation in FixedDigital mode */
        stt->compressionGaindB += agcConfig.targetLevelDbfs;
    }

    /* Update threshold levels for analog adaptation */
    WebRtcAgc_UpdateAgcThresholds(stt);

    /* Recalculate gain table */
    if (WebRtcAgc_CalculateGainTable(&(stt->digitalAgc.gainTable[0]), stt->compressionGaindB,
                           stt->targetLevelDbfs, stt->limiterEnable, stt->analogTarget) == -1)
    {
#ifdef AGC_DEBUG//test log
        fprintf(stt->fpt, "AGC->set_config, frame %d: Error from calcGainTable\n\n", stt->fcount);
#endif
        return -1;
    }
    /* Store the config in a WebRtcAgc_config_t */
    stt->usedConfig.compressionGaindB = agcConfig.compressionGaindB;
    stt->usedConfig.limiterEnable = agcConfig.limiterEnable;
    stt->usedConfig.targetLevelDbfs = agcConfig.targetLevelDbfs;

    return 0;
}

int WebRtcAgc_get_config(void *agcInst, WebRtcAgc_config_t *config)
{
    Agc_t *stt;
    stt = (Agc_t *)agcInst;

    if (stt == NULL)
    {
        return -1;
    }

    if (config == NULL)
    {
        stt->lastError = AGC_NULL_POINTER_ERROR;
        return -1;
    }

    if (stt->initFlag != kInitCheck)
    {
        stt->lastError = AGC_UNINITIALIZED_ERROR;
        return -1;
    }

    config->limiterEnable = stt->usedConfig.limiterEnable;
    config->targetLevelDbfs = stt->usedConfig.targetLevelDbfs;
    config->compressionGaindB = stt->usedConfig.compressionGaindB;

    return 0;
}

int WebRtcAgc_Create(void **agcInst)
{
    Agc_t *stt;
    if (agcInst == NULL)
    {
        return -1;
    }
    stt = (Agc_t *)malloc(sizeof(Agc_t));

    *agcInst = stt;
    if (stt == NULL)
    {
        return -1;
    }

#ifdef AGC_DEBUG
    stt->fpt = fopen("./agc_test_log.txt", "wt");
    stt->agcLog = fopen("./agc_debug_log.txt", "wt");
    stt->digitalAgc.logFile = fopen("./agc_log.txt", "wt");
#endif

    stt->initFlag = 0;
    stt->lastError = 0;

    return 0;
}

int WebRtcAgc_Free(void *state)
{
    Agc_t *stt;

    stt = (Agc_t *)state;
#ifdef AGC_DEBUG
    fclose(stt->fpt);
    fclose(stt->agcLog);
    fclose(stt->digitalAgc.logFile);
#endif
    free(stt);

    return 0;
}

/* minLevel     - Minimum volume level
 * maxLevel     - Maximum volume level
 */
int WebRtcAgc_Init(void *agcInst, WebRtc_Word32 minLevel, WebRtc_Word32 maxLevel,
                             WebRtc_Word16 agcMode, WebRtc_UWord32 fs)
{
    WebRtc_Word32 max_add, tmp32;
    WebRtc_Word16 i;
    int tmpNorm;
    Agc_t *stt;

    /* typecast state pointer */
    stt = (Agc_t *)agcInst;

    if (WebRtcAgc_InitDigital(&stt->digitalAgc, agcMode) != 0)
    {
        stt->lastError = AGC_UNINITIALIZED_ERROR;
        return -1;
    }

    /* Analog AGC variables */
    stt->envSum = 0;

    /* mode     = 0 - Only saturation protection
     *            1 - Analog Automatic Gain Control [-targetLevelDbfs (default -3 dBOv)]
     *            2 - Digital Automatic Gain Control [-targetLevelDbfs (default -3 dBOv)]
     *            3 - Fixed Digital Gain [compressionGaindB (default 8 dB)]
     */
#ifdef AGC_DEBUG//test log
    stt->fcount = 0;
    fprintf(stt->fpt, "AGC->Init\n");
#endif
    if (agcMode < kAgcModeUnchanged || agcMode > kAgcModeFixedDigital)
    {
#ifdef AGC_DEBUG//test log
        fprintf(stt->fpt, "AGC->Init: error, incorrect mode\n\n");
#endif
        return -1;
    }
    stt->agcMode = agcMode;
    stt->fs = fs;

    /* initialize input VAD */
    WebRtcAgc_InitVad(&stt->vadMic);

    /* If the volume range is smaller than 0-256 then
     * the levels are shifted up to Q8-domain */
    tmpNorm = WebRtcSpl_NormU32((WebRtc_UWord32)maxLevel);
    stt->scale = tmpNorm - 23;
    if (stt->scale < 0)
    {
        stt->scale = 0;
    }
    // TODO(bjornv): Investigate if we really need to scale up a small range now when we have
    // a guard against zero-increments. For now, we do not support scale up (scale = 0).
    stt->scale = 0;
    maxLevel = WEBRTC_SPL_LSHIFT_W32(maxLevel, stt->scale);
    minLevel = WEBRTC_SPL_LSHIFT_W32(minLevel, stt->scale);

    /* Make minLevel and maxLevel static in AdaptiveDigital */
    if (stt->agcMode == kAgcModeAdaptiveDigital)
    {
        minLevel = 0;
        maxLevel = 255;
        stt->scale = 0;
    }
    /* The maximum supplemental volume range is based on a vague idea
     * of how much lower the gain will be than the real analog gain. */
    max_add = WEBRTC_SPL_RSHIFT_W32(maxLevel - minLevel, 2);

    /* Minimum/maximum volume level that can be set */
    stt->minLevel = minLevel;
    stt->maxAnalog = maxLevel;
    stt->maxLevel = maxLevel + max_add;
    stt->maxInit = stt->maxLevel;

    stt->zeroCtrlMax = stt->maxAnalog;

    /* Initialize micVol parameter */
    stt->micVol = stt->maxAnalog;
    if (stt->agcMode == kAgcModeAdaptiveDigital)
    {
        stt->micVol = 127; /* Mid-point of mic level */
    }
    stt->micRef = stt->micVol;
    stt->micGainIdx = 127;
#ifdef MIC_LEVEL_FEEDBACK
    stt->numBlocksMicLvlSat = 0;
    stt->micLvlSat = 0;
#endif
#ifdef AGC_DEBUG//test log
    fprintf(stt->fpt,
            "AGC->Init: minLevel = %d, maxAnalog = %d, maxLevel = %d\n",
            stt->minLevel, stt->maxAnalog, stt->maxLevel);
#endif

    /* Minimum output volume is 4% higher than the available lowest volume level */
    tmp32 = WEBRTC_SPL_RSHIFT_W32((stt->maxLevel - stt->minLevel) * (WebRtc_Word32)10, 8);
    stt->minOutput = (stt->minLevel + tmp32);

    stt->msTooLow = 0;
    stt->msTooHigh = 0;
    stt->changeToSlowMode = 0;
    stt->firstCall = 0;
    stt->msZero = 0;
    stt->muteGuardMs = 0;
    stt->gainTableIdx = 0;

    stt->msecSpeechInnerChange = kMsecSpeechInner;
    stt->msecSpeechOuterChange = kMsecSpeechOuter;

    stt->activeSpeech = 0;
    stt->Rxx16_LPw32Max = 0;

    stt->vadThreshold = kNormalVadThreshold;
    stt->inActive = 0;

    for (i = 0; i < RXX_BUFFER_LEN; i++)
    {
        stt->Rxx16_vectorw32[i] = (WebRtc_Word32)1000; /* -54dBm0 */
    }
    stt->Rxx160w32 = 125 * RXX_BUFFER_LEN; /* (stt->Rxx16_vectorw32[0]>>3) = 125 */

    stt->Rxx16pos = 0;
    stt->Rxx16_LPw32 = (WebRtc_Word32)16284; /* Q(-4) */

    for (i = 0; i < 5; i++)
    {
        stt->Rxx16w32_array[0][i] = 0;
    }
    for (i = 0; i < 10; i++)
    {
        stt->env[0][i] = 0;
        stt->env[1][i] = 0;
    }
    stt->inQueue = 0;

#ifdef MIC_LEVEL_FEEDBACK
    stt->targetIdxOffset = 0;
#endif

    WebRtcSpl_MemSetW32(stt->filterState, 0, 8);

    stt->initFlag = kInitCheck;
    // Default config settings.
    stt->defaultConfig.limiterEnable = kAgcTrue;
    stt->defaultConfig.targetLevelDbfs = AGC_DEFAULT_TARGET_LEVEL;
    stt->defaultConfig.compressionGaindB = AGC_DEFAULT_COMP_GAIN;

    if (WebRtcAgc_set_config(stt, stt->defaultConfig) == -1)
    {
        stt->lastError = AGC_UNSPECIFIED_ERROR;
        return -1;
    }
    stt->Rxx160_LPw32 = stt->analogTargetLevel; // Initialize rms value

    stt->lowLevelSignal = 0;

    /* Only positive values are allowed that are not too large */
    if ((minLevel >= maxLevel) || (maxLevel & 0xFC000000))
    {
#ifdef AGC_DEBUG//test log
        fprintf(stt->fpt, "minLevel, maxLevel value(s) are invalid\n\n");
#endif
        return -1;
    } else
    {
#ifdef AGC_DEBUG//test log
        fprintf(stt->fpt, "\n");
#endif
        return 0;
    }
}
