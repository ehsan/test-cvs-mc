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
 * A wrapper for resampling a numerous amount of sampling combinations.
 */

#include <stdlib.h>
#include <string.h>

#include "signal_processing_library.h"
#include "resampler.h"


namespace webrtc
{

Resampler::Resampler()
{
    state1_ = NULL;
    state2_ = NULL;
    state3_ = NULL;
    in_buffer_ = NULL;
    out_buffer_ = NULL;
    in_buffer_size_ = 0;
    out_buffer_size_ = 0;
    in_buffer_size_max_ = 0;
    out_buffer_size_max_ = 0;
    // we need a reset before we will work
    my_in_frequency_khz_ = 0;
    my_out_frequency_khz_ = 0;
    my_mode_ = kResamplerMode1To1;
    my_type_ = kResamplerInvalid;
    slave_left_ = NULL;
    slave_right_ = NULL;
}

Resampler::Resampler(int inFreq, int outFreq, ResamplerType type)
{
    state1_ = NULL;
    state2_ = NULL;
    state3_ = NULL;
    in_buffer_ = NULL;
    out_buffer_ = NULL;
    in_buffer_size_ = 0;
    out_buffer_size_ = 0;
    in_buffer_size_max_ = 0;
    out_buffer_size_max_ = 0;
    // we need a reset before we will work
    my_in_frequency_khz_ = 0;
    my_out_frequency_khz_ = 0;
    my_mode_ = kResamplerMode1To1;
    my_type_ = kResamplerInvalid;
    slave_left_ = NULL;
    slave_right_ = NULL;

    Reset(inFreq, outFreq, type);
}

Resampler::~Resampler()
{
    if (state1_)
    {
        free(state1_);
    }
    if (state2_)
    {
        free(state2_);
    }
    if (state3_)
    {
        free(state3_);
    }
    if (in_buffer_)
    {
        free(in_buffer_);
    }
    if (out_buffer_)
    {
        free(out_buffer_);
    }
    if (slave_left_)
    {
        delete slave_left_;
    }
    if (slave_right_)
    {
        delete slave_right_;
    }
}

int Resampler::ResetIfNeeded(int inFreq, int outFreq, ResamplerType type)
{
    int tmpInFreq_kHz = inFreq / 1000;
    int tmpOutFreq_kHz = outFreq / 1000;

    if ((tmpInFreq_kHz != my_in_frequency_khz_) || (tmpOutFreq_kHz != my_out_frequency_khz_)
            || (type != my_type_))
    {
        return Reset(inFreq, outFreq, type);
    } else
    {
        return 0;
    }
}

int Resampler::Reset(int inFreq, int outFreq, ResamplerType type)
{

    if (state1_)
    {
        free(state1_);
        state1_ = NULL;
    }
    if (state2_)
    {
        free(state2_);
        state2_ = NULL;
    }
    if (state3_)
    {
        free(state3_);
        state3_ = NULL;
    }
    if (in_buffer_)
    {
        free(in_buffer_);
        in_buffer_ = NULL;
    }
    if (out_buffer_)
    {
        free(out_buffer_);
        out_buffer_ = NULL;
    }
    if (slave_left_)
    {
        delete slave_left_;
        slave_left_ = NULL;
    }
    if (slave_right_)
    {
        delete slave_right_;
        slave_right_ = NULL;
    }

    in_buffer_size_ = 0;
    out_buffer_size_ = 0;
    in_buffer_size_max_ = 0;
    out_buffer_size_max_ = 0;

    // This might be overridden if parameters are not accepted.
    my_type_ = type;

    // Start with a math exercise, Euclid's algorithm to find the gcd:

    int a = inFreq;
    int b = outFreq;
    int c = a % b;
    while (c != 0)
    {
        a = b;
        b = c;
        c = a % b;
    }
    // b is now the gcd;

    // We need to track what domain we're in.
    my_in_frequency_khz_ = inFreq / 1000;
    my_out_frequency_khz_ = outFreq / 1000;

    // Scale with GCD
    inFreq = inFreq / b;
    outFreq = outFreq / b;

    // Do we need stereo?
    if ((my_type_ & 0xf0) == 0x20)
    {
        // Change type to mono
        type = static_cast<ResamplerType>(
            ((static_cast<int>(type) & 0x0f) + 0x10));
        slave_left_ = new Resampler(inFreq, outFreq, type);
        slave_right_ = new Resampler(inFreq, outFreq, type);
    }

    if (inFreq == outFreq)
    {
        my_mode_ = kResamplerMode1To1;
    } else if (inFreq == 1)
    {
        switch (outFreq)
        {
            case 2:
                my_mode_ = kResamplerMode1To2;
                break;
            case 3:
                my_mode_ = kResamplerMode1To3;
                break;
            case 4:
                my_mode_ = kResamplerMode1To4;
                break;
            case 6:
                my_mode_ = kResamplerMode1To6;
                break;
            case 12:
                my_mode_ = kResamplerMode1To12;
                break;
            default:
                my_type_ = kResamplerInvalid;
                return -1;
        }
    } else if (outFreq == 1)
    {
        switch (inFreq)
        {
            case 2:
                my_mode_ = kResamplerMode2To1;
                break;
            case 3:
                my_mode_ = kResamplerMode3To1;
                break;
            case 4:
                my_mode_ = kResamplerMode4To1;
                break;
            case 6:
                my_mode_ = kResamplerMode6To1;
                break;
            case 12:
                my_mode_ = kResamplerMode12To1;
                break;
            default:
                my_type_ = kResamplerInvalid;
                return -1;
        }
    } else if ((inFreq == 2) && (outFreq == 3))
    {
        my_mode_ = kResamplerMode2To3;
    } else if ((inFreq == 2) && (outFreq == 11))
    {
        my_mode_ = kResamplerMode2To11;
    } else if ((inFreq == 4) && (outFreq == 11))
    {
        my_mode_ = kResamplerMode4To11;
    } else if ((inFreq == 8) && (outFreq == 11))
    {
        my_mode_ = kResamplerMode8To11;
    } else if ((inFreq == 3) && (outFreq == 2))
    {
        my_mode_ = kResamplerMode3To2;
    } else if ((inFreq == 11) && (outFreq == 2))
    {
        my_mode_ = kResamplerMode11To2;
    } else if ((inFreq == 11) && (outFreq == 4))
    {
        my_mode_ = kResamplerMode11To4;
    } else if ((inFreq == 11) && (outFreq == 16))
    {
        my_mode_ = kResamplerMode11To16;
    } else if ((inFreq == 11) && (outFreq == 32))
    {
        my_mode_ = kResamplerMode11To32;
    } else if ((inFreq == 11) && (outFreq == 8))
    {
        my_mode_ = kResamplerMode11To8;
    } else
    {
        my_type_ = kResamplerInvalid;
        return -1;
    }

    // Now create the states we need
    switch (my_mode_)
    {
        case kResamplerMode1To1:
            // No state needed;
            break;
        case kResamplerMode1To2:
            state1_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state1_, 0, 8 * sizeof(WebRtc_Word32));
            break;
        case kResamplerMode1To3:
            state1_ = malloc(sizeof(WebRtcSpl_State16khzTo48khz));
            WebRtcSpl_ResetResample16khzTo48khz((WebRtcSpl_State16khzTo48khz *)state1_);
            break;
        case kResamplerMode1To4:
            // 1:2
            state1_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state1_, 0, 8 * sizeof(WebRtc_Word32));
            // 2:4
            state2_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state2_, 0, 8 * sizeof(WebRtc_Word32));
            break;
        case kResamplerMode1To6:
            // 1:2
            state1_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state1_, 0, 8 * sizeof(WebRtc_Word32));
            // 2:6
            state2_ = malloc(sizeof(WebRtcSpl_State16khzTo48khz));
            WebRtcSpl_ResetResample16khzTo48khz((WebRtcSpl_State16khzTo48khz *)state2_);
            break;
        case kResamplerMode1To12:
            // 1:2
            state1_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state1_, 0, 8 * sizeof(WebRtc_Word32));
            // 2:4
            state2_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state2_, 0, 8 * sizeof(WebRtc_Word32));
            // 4:12
            state3_ = malloc(sizeof(WebRtcSpl_State16khzTo48khz));
            WebRtcSpl_ResetResample16khzTo48khz(
                (WebRtcSpl_State16khzTo48khz*) state3_);
            break;
        case kResamplerMode2To3:
            // 2:6
            state1_ = malloc(sizeof(WebRtcSpl_State16khzTo48khz));
            WebRtcSpl_ResetResample16khzTo48khz((WebRtcSpl_State16khzTo48khz *)state1_);
            // 6:3
            state2_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state2_, 0, 8 * sizeof(WebRtc_Word32));
            break;
        case kResamplerMode2To11:
            state1_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state1_, 0, 8 * sizeof(WebRtc_Word32));

            state2_ = malloc(sizeof(WebRtcSpl_State8khzTo22khz));
            WebRtcSpl_ResetResample8khzTo22khz((WebRtcSpl_State8khzTo22khz *)state2_);
            break;
        case kResamplerMode4To11:
            state1_ = malloc(sizeof(WebRtcSpl_State8khzTo22khz));
            WebRtcSpl_ResetResample8khzTo22khz((WebRtcSpl_State8khzTo22khz *)state1_);
            break;
        case kResamplerMode8To11:
            state1_ = malloc(sizeof(WebRtcSpl_State16khzTo22khz));
            WebRtcSpl_ResetResample16khzTo22khz((WebRtcSpl_State16khzTo22khz *)state1_);
            break;
        case kResamplerMode11To16:
            state1_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state1_, 0, 8 * sizeof(WebRtc_Word32));

            state2_ = malloc(sizeof(WebRtcSpl_State22khzTo16khz));
            WebRtcSpl_ResetResample22khzTo16khz((WebRtcSpl_State22khzTo16khz *)state2_);
            break;
        case kResamplerMode11To32:
            // 11 -> 22
            state1_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state1_, 0, 8 * sizeof(WebRtc_Word32));

            // 22 -> 16
            state2_ = malloc(sizeof(WebRtcSpl_State22khzTo16khz));
            WebRtcSpl_ResetResample22khzTo16khz((WebRtcSpl_State22khzTo16khz *)state2_);

            // 16 -> 32
            state3_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state3_, 0, 8 * sizeof(WebRtc_Word32));

            break;
        case kResamplerMode2To1:
            state1_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state1_, 0, 8 * sizeof(WebRtc_Word32));
            break;
        case kResamplerMode3To1:
            state1_ = malloc(sizeof(WebRtcSpl_State48khzTo16khz));
            WebRtcSpl_ResetResample48khzTo16khz((WebRtcSpl_State48khzTo16khz *)state1_);
            break;
        case kResamplerMode4To1:
            // 4:2
            state1_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state1_, 0, 8 * sizeof(WebRtc_Word32));
            // 2:1
            state2_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state2_, 0, 8 * sizeof(WebRtc_Word32));
            break;
        case kResamplerMode6To1:
            // 6:2
            state1_ = malloc(sizeof(WebRtcSpl_State48khzTo16khz));
            WebRtcSpl_ResetResample48khzTo16khz((WebRtcSpl_State48khzTo16khz *)state1_);
            // 2:1
            state2_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state2_, 0, 8 * sizeof(WebRtc_Word32));
            break;
        case kResamplerMode12To1:
            // 12:4
            state1_ = malloc(sizeof(WebRtcSpl_State48khzTo16khz));
            WebRtcSpl_ResetResample48khzTo16khz(
                (WebRtcSpl_State48khzTo16khz*) state1_);
            // 4:2
            state2_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state2_, 0, 8 * sizeof(WebRtc_Word32));
            // 2:1
            state3_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state3_, 0, 8 * sizeof(WebRtc_Word32));
            break;
        case kResamplerMode3To2:
            // 3:6
            state1_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state1_, 0, 8 * sizeof(WebRtc_Word32));
            // 6:2
            state2_ = malloc(sizeof(WebRtcSpl_State48khzTo16khz));
            WebRtcSpl_ResetResample48khzTo16khz((WebRtcSpl_State48khzTo16khz *)state2_);
            break;
        case kResamplerMode11To2:
            state1_ = malloc(sizeof(WebRtcSpl_State22khzTo8khz));
            WebRtcSpl_ResetResample22khzTo8khz((WebRtcSpl_State22khzTo8khz *)state1_);

            state2_ = malloc(8 * sizeof(WebRtc_Word32));
            memset(state2_, 0, 8 * sizeof(WebRtc_Word32));

            break;
        case kResamplerMode11To4:
            state1_ = malloc(sizeof(WebRtcSpl_State22khzTo8khz));
            WebRtcSpl_ResetResample22khzTo8khz((WebRtcSpl_State22khzTo8khz *)state1_);
            break;
        case kResamplerMode11To8:
            state1_ = malloc(sizeof(WebRtcSpl_State22khzTo16khz));
            WebRtcSpl_ResetResample22khzTo16khz((WebRtcSpl_State22khzTo16khz *)state1_);
            break;

    }

    return 0;
}

// Synchronous resampling, all output samples are written to samplesOut
int Resampler::Push(const WebRtc_Word16 * samplesIn, int lengthIn, WebRtc_Word16* samplesOut,
                    int maxLen, int &outLen)
{
    // Check that the resampler is not in asynchronous mode
    if (my_type_ & 0x0f)
    {
        return -1;
    }

    // Do we have a stereo signal?
    if ((my_type_ & 0xf0) == 0x20)
    {

        // Split up the signal and call the slave object for each channel

        WebRtc_Word16* left = (WebRtc_Word16*)malloc(lengthIn * sizeof(WebRtc_Word16) / 2);
        WebRtc_Word16* right = (WebRtc_Word16*)malloc(lengthIn * sizeof(WebRtc_Word16) / 2);
        WebRtc_Word16* out_left = (WebRtc_Word16*)malloc(maxLen / 2 * sizeof(WebRtc_Word16));
        WebRtc_Word16* out_right =
                (WebRtc_Word16*)malloc(maxLen / 2 * sizeof(WebRtc_Word16));
        int res = 0;
        for (int i = 0; i < lengthIn; i += 2)
        {
            left[i >> 1] = samplesIn[i];
            right[i >> 1] = samplesIn[i + 1];
        }

        // It's OK to overwrite the local parameter, since it's just a copy
        lengthIn = lengthIn / 2;

        int actualOutLen_left = 0;
        int actualOutLen_right = 0;
        // Do resampling for right channel
        res |= slave_left_->Push(left, lengthIn, out_left, maxLen / 2, actualOutLen_left);
        res |= slave_right_->Push(right, lengthIn, out_right, maxLen / 2, actualOutLen_right);
        if (res || (actualOutLen_left != actualOutLen_right))
        {
            free(left);
            free(right);
            free(out_left);
            free(out_right);
            return -1;
        }

        // Reassemble the signal
        for (int i = 0; i < actualOutLen_left; i++)
        {
            samplesOut[i * 2] = out_left[i];
            samplesOut[i * 2 + 1] = out_right[i];
        }
        outLen = 2 * actualOutLen_left;

        free(left);
        free(right);
        free(out_left);
        free(out_right);

        return 0;
    }

    // Containers for temp samples
    WebRtc_Word16* tmp;
    WebRtc_Word16* tmp_2;
    // tmp data for resampling routines
    WebRtc_Word32* tmp_mem;

    switch (my_mode_)
    {
        case kResamplerMode1To1:
            memcpy(samplesOut, samplesIn, lengthIn * sizeof(WebRtc_Word16));
            outLen = lengthIn;
            break;
        case kResamplerMode1To2:
            if (maxLen < (lengthIn * 2))
            {
                return -1;
            }
            WebRtcSpl_UpsampleBy2(samplesIn, lengthIn, samplesOut, (WebRtc_Word32*)state1_);
            outLen = lengthIn * 2;
            return 0;
        case kResamplerMode1To3:

            // We can only handle blocks of 160 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 160) != 0)
            {
                return -1;
            }
            if (maxLen < (lengthIn * 3))
            {
                return -1;
            }
            tmp_mem = (WebRtc_Word32*)malloc(336 * sizeof(WebRtc_Word32));

            for (int i = 0; i < lengthIn; i += 160)
            {
                WebRtcSpl_Resample16khzTo48khz(samplesIn + i, samplesOut + i * 3,
                                               (WebRtcSpl_State16khzTo48khz *)state1_,
                                               tmp_mem);
            }
            outLen = lengthIn * 3;
            free(tmp_mem);
            return 0;
        case kResamplerMode1To4:
            if (maxLen < (lengthIn * 4))
            {
                return -1;
            }

            tmp = (WebRtc_Word16*)malloc(sizeof(WebRtc_Word16) * 2 * lengthIn);
            // 1:2
            WebRtcSpl_UpsampleBy2(samplesIn, lengthIn, tmp, (WebRtc_Word32*)state1_);
            // 2:4
            WebRtcSpl_UpsampleBy2(tmp, lengthIn * 2, samplesOut, (WebRtc_Word32*)state2_);
            outLen = lengthIn * 4;
            free(tmp);
            return 0;
        case kResamplerMode1To6:
            // We can only handle blocks of 80 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 80) != 0)
            {
                return -1;
            }
            if (maxLen < (lengthIn * 6))
            {
                return -1;
            }

            //1:2

            tmp_mem = (WebRtc_Word32*)malloc(336 * sizeof(WebRtc_Word32));
            tmp = (WebRtc_Word16*)malloc(sizeof(WebRtc_Word16) * 2 * lengthIn);

            WebRtcSpl_UpsampleBy2(samplesIn, lengthIn, tmp, (WebRtc_Word32*)state1_);
            outLen = lengthIn * 2;

            for (int i = 0; i < outLen; i += 160)
            {
                WebRtcSpl_Resample16khzTo48khz(tmp + i, samplesOut + i * 3,
                                               (WebRtcSpl_State16khzTo48khz *)state2_,
                                               tmp_mem);
            }
            outLen = outLen * 3;
            free(tmp_mem);
            free(tmp);

            return 0;
        case kResamplerMode1To12:
            // We can only handle blocks of 40 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 40) != 0) {
              return -1;
            }
            if (maxLen < (lengthIn * 12)) {
              return -1;
            }

            tmp_mem = (WebRtc_Word32*) malloc(336 * sizeof(WebRtc_Word32));
            tmp = (WebRtc_Word16*) malloc(sizeof(WebRtc_Word16) * 4 * lengthIn);
            //1:2
            WebRtcSpl_UpsampleBy2(samplesIn, lengthIn, samplesOut,
                                  (WebRtc_Word32*) state1_);
            outLen = lengthIn * 2;
            //2:4
            WebRtcSpl_UpsampleBy2(samplesOut, outLen, tmp, (WebRtc_Word32*) state2_);
            outLen = outLen * 2;
            // 4:12
            for (int i = 0; i < outLen; i += 160) {
              // WebRtcSpl_Resample16khzTo48khz() takes a block of 160 samples
              // as input and outputs a resampled block of 480 samples. The
              // data is now actually in 32 kHz sampling rate, despite the
              // function name, and with a resampling factor of three becomes
              // 96 kHz.
              WebRtcSpl_Resample16khzTo48khz(tmp + i, samplesOut + i * 3,
                                             (WebRtcSpl_State16khzTo48khz*) state3_,
                                             tmp_mem);
            }
            outLen = outLen * 3;
            free(tmp_mem);
            free(tmp);

            return 0;
        case kResamplerMode2To3:
            if (maxLen < (lengthIn * 3 / 2))
            {
                return -1;
            }
            // 2:6
            // We can only handle blocks of 160 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 160) != 0)
            {
                return -1;
            }
            tmp = static_cast<WebRtc_Word16*> (malloc(sizeof(WebRtc_Word16) * lengthIn * 3));
            tmp_mem = (WebRtc_Word32*)malloc(336 * sizeof(WebRtc_Word32));
            for (int i = 0; i < lengthIn; i += 160)
            {
                WebRtcSpl_Resample16khzTo48khz(samplesIn + i, tmp + i * 3,
                                               (WebRtcSpl_State16khzTo48khz *)state1_,
                                               tmp_mem);
            }
            lengthIn = lengthIn * 3;
            // 6:3
            WebRtcSpl_DownsampleBy2(tmp, lengthIn, samplesOut, (WebRtc_Word32*)state2_);
            outLen = lengthIn / 2;
            free(tmp);
            free(tmp_mem);
            return 0;
        case kResamplerMode2To11:

            // We can only handle blocks of 80 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 80) != 0)
            {
                return -1;
            }
            if (maxLen < ((lengthIn * 11) / 2))
            {
                return -1;
            }
            tmp = (WebRtc_Word16*)malloc(sizeof(WebRtc_Word16) * 2 * lengthIn);
            // 1:2
            WebRtcSpl_UpsampleBy2(samplesIn, lengthIn, tmp, (WebRtc_Word32*)state1_);
            lengthIn *= 2;

            tmp_mem = (WebRtc_Word32*)malloc(98 * sizeof(WebRtc_Word32));

            for (int i = 0; i < lengthIn; i += 80)
            {
                WebRtcSpl_Resample8khzTo22khz(tmp + i, samplesOut + (i * 11) / 4,
                                              (WebRtcSpl_State8khzTo22khz *)state2_,
                                              tmp_mem);
            }
            outLen = (lengthIn * 11) / 4;
            free(tmp_mem);
            free(tmp);
            return 0;
        case kResamplerMode4To11:

            // We can only handle blocks of 80 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 80) != 0)
            {
                return -1;
            }
            if (maxLen < ((lengthIn * 11) / 4))
            {
                return -1;
            }
            tmp_mem = (WebRtc_Word32*)malloc(98 * sizeof(WebRtc_Word32));

            for (int i = 0; i < lengthIn; i += 80)
            {
                WebRtcSpl_Resample8khzTo22khz(samplesIn + i, samplesOut + (i * 11) / 4,
                                              (WebRtcSpl_State8khzTo22khz *)state1_,
                                              tmp_mem);
            }
            outLen = (lengthIn * 11) / 4;
            free(tmp_mem);
            return 0;
        case kResamplerMode8To11:
            // We can only handle blocks of 160 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 160) != 0)
            {
                return -1;
            }
            if (maxLen < ((lengthIn * 11) / 8))
            {
                return -1;
            }
            tmp_mem = (WebRtc_Word32*)malloc(88 * sizeof(WebRtc_Word32));

            for (int i = 0; i < lengthIn; i += 160)
            {
                WebRtcSpl_Resample16khzTo22khz(samplesIn + i, samplesOut + (i * 11) / 8,
                                               (WebRtcSpl_State16khzTo22khz *)state1_,
                                               tmp_mem);
            }
            outLen = (lengthIn * 11) / 8;
            free(tmp_mem);
            return 0;

        case kResamplerMode11To16:
            // We can only handle blocks of 110 samples
            if ((lengthIn % 110) != 0)
            {
                return -1;
            }
            if (maxLen < ((lengthIn * 16) / 11))
            {
                return -1;
            }

            tmp_mem = (WebRtc_Word32*)malloc(104 * sizeof(WebRtc_Word32));
            tmp = (WebRtc_Word16*)malloc((sizeof(WebRtc_Word16) * lengthIn * 2));

            WebRtcSpl_UpsampleBy2(samplesIn, lengthIn, tmp, (WebRtc_Word32*)state1_);

            for (int i = 0; i < (lengthIn * 2); i += 220)
            {
                WebRtcSpl_Resample22khzTo16khz(tmp + i, samplesOut + (i / 220) * 160,
                                               (WebRtcSpl_State22khzTo16khz *)state2_,
                                               tmp_mem);
            }

            outLen = (lengthIn * 16) / 11;

            free(tmp_mem);
            free(tmp);
            return 0;

        case kResamplerMode11To32:

            // We can only handle blocks of 110 samples
            if ((lengthIn % 110) != 0)
            {
                return -1;
            }
            if (maxLen < ((lengthIn * 32) / 11))
            {
                return -1;
            }

            tmp_mem = (WebRtc_Word32*)malloc(104 * sizeof(WebRtc_Word32));
            tmp = (WebRtc_Word16*)malloc((sizeof(WebRtc_Word16) * lengthIn * 2));

            // 11 -> 22 kHz in samplesOut
            WebRtcSpl_UpsampleBy2(samplesIn, lengthIn, samplesOut, (WebRtc_Word32*)state1_);

            // 22 -> 16 in tmp
            for (int i = 0; i < (lengthIn * 2); i += 220)
            {
                WebRtcSpl_Resample22khzTo16khz(samplesOut + i, tmp + (i / 220) * 160,
                                               (WebRtcSpl_State22khzTo16khz *)state2_,
                                               tmp_mem);
            }

            // 16 -> 32 in samplesOut
            WebRtcSpl_UpsampleBy2(tmp, (lengthIn * 16) / 11, samplesOut,
                                  (WebRtc_Word32*)state3_);

            outLen = (lengthIn * 32) / 11;

            free(tmp_mem);
            free(tmp);
            return 0;

        case kResamplerMode2To1:
            if (maxLen < (lengthIn / 2))
            {
                return -1;
            }
            WebRtcSpl_DownsampleBy2(samplesIn, lengthIn, samplesOut, (WebRtc_Word32*)state1_);
            outLen = lengthIn / 2;
            return 0;
        case kResamplerMode3To1:
            // We can only handle blocks of 480 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 480) != 0)
            {
                return -1;
            }
            if (maxLen < (lengthIn / 3))
            {
                return -1;
            }
            tmp_mem = (WebRtc_Word32*)malloc(496 * sizeof(WebRtc_Word32));

            for (int i = 0; i < lengthIn; i += 480)
            {
                WebRtcSpl_Resample48khzTo16khz(samplesIn + i, samplesOut + i / 3,
                                               (WebRtcSpl_State48khzTo16khz *)state1_,
                                               tmp_mem);
            }
            outLen = lengthIn / 3;
            free(tmp_mem);
            return 0;
        case kResamplerMode4To1:
            if (maxLen < (lengthIn / 4))
            {
                return -1;
            }
            tmp = (WebRtc_Word16*)malloc(sizeof(WebRtc_Word16) * lengthIn / 2);
            // 4:2
            WebRtcSpl_DownsampleBy2(samplesIn, lengthIn, tmp, (WebRtc_Word32*)state1_);
            // 2:1
            WebRtcSpl_DownsampleBy2(tmp, lengthIn / 2, samplesOut, (WebRtc_Word32*)state2_);
            outLen = lengthIn / 4;
            free(tmp);
            return 0;

        case kResamplerMode6To1:
            // We can only handle blocks of 480 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 480) != 0)
            {
                return -1;
            }
            if (maxLen < (lengthIn / 6))
            {
                return -1;
            }

            tmp_mem = (WebRtc_Word32*)malloc(496 * sizeof(WebRtc_Word32));
            tmp = (WebRtc_Word16*)malloc((sizeof(WebRtc_Word16) * lengthIn) / 3);

            for (int i = 0; i < lengthIn; i += 480)
            {
                WebRtcSpl_Resample48khzTo16khz(samplesIn + i, tmp + i / 3,
                                               (WebRtcSpl_State48khzTo16khz *)state1_,
                                               tmp_mem);
            }
            outLen = lengthIn / 3;
            free(tmp_mem);
            WebRtcSpl_DownsampleBy2(tmp, outLen, samplesOut, (WebRtc_Word32*)state2_);
            free(tmp);
            outLen = outLen / 2;
            return 0;
        case kResamplerMode12To1:
            // We can only handle blocks of 480 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 480) != 0) {
              return -1;
            }
            if (maxLen < (lengthIn / 12)) {
              return -1;
            }

            tmp_mem = (WebRtc_Word32*) malloc(496 * sizeof(WebRtc_Word32));
            tmp = (WebRtc_Word16*) malloc((sizeof(WebRtc_Word16) * lengthIn) / 3);
            tmp_2 = (WebRtc_Word16*) malloc((sizeof(WebRtc_Word16) * lengthIn) / 6);
            // 12:4
            for (int i = 0; i < lengthIn; i += 480) {
              // WebRtcSpl_Resample48khzTo16khz() takes a block of 480 samples
              // as input and outputs a resampled block of 160 samples. The
              // data is now actually in 96 kHz sampling rate, despite the
              // function name, and with a resampling factor of 1/3 becomes
              // 32 kHz.
              WebRtcSpl_Resample48khzTo16khz(samplesIn + i, tmp + i / 3,
                                             (WebRtcSpl_State48khzTo16khz*) state1_,
                                             tmp_mem);
            }
            outLen = lengthIn / 3;
            free(tmp_mem);
            // 4:2
            WebRtcSpl_DownsampleBy2(tmp, outLen, tmp_2,
                                    (WebRtc_Word32*) state2_);
            outLen = outLen / 2;
            free(tmp);
            // 2:1
            WebRtcSpl_DownsampleBy2(tmp_2, outLen, samplesOut,
                                    (WebRtc_Word32*) state3_);
            free(tmp_2);
            outLen = outLen / 2;
            return 0;
        case kResamplerMode3To2:
            if (maxLen < (lengthIn * 2 / 3))
            {
                return -1;
            }
            // 3:6
            tmp = static_cast<WebRtc_Word16*> (malloc(sizeof(WebRtc_Word16) * lengthIn * 2));
            WebRtcSpl_UpsampleBy2(samplesIn, lengthIn, tmp, (WebRtc_Word32*)state1_);
            lengthIn *= 2;
            // 6:2
            // We can only handle blocks of 480 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 480) != 0)
            {
                free(tmp);
                return -1;
            }
            tmp_mem = (WebRtc_Word32*)malloc(496 * sizeof(WebRtc_Word32));
            for (int i = 0; i < lengthIn; i += 480)
            {
                WebRtcSpl_Resample48khzTo16khz(tmp + i, samplesOut + i / 3,
                                               (WebRtcSpl_State48khzTo16khz *)state2_,
                                               tmp_mem);
            }
            outLen = lengthIn / 3;
            free(tmp);
            free(tmp_mem);
            return 0;
        case kResamplerMode11To2:
            // We can only handle blocks of 220 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 220) != 0)
            {
                return -1;
            }
            if (maxLen < ((lengthIn * 2) / 11))
            {
                return -1;
            }
            tmp_mem = (WebRtc_Word32*)malloc(126 * sizeof(WebRtc_Word32));
            tmp = (WebRtc_Word16*)malloc((lengthIn * 4) / 11 * sizeof(WebRtc_Word16));

            for (int i = 0; i < lengthIn; i += 220)
            {
                WebRtcSpl_Resample22khzTo8khz(samplesIn + i, tmp + (i * 4) / 11,
                                              (WebRtcSpl_State22khzTo8khz *)state1_,
                                              tmp_mem);
            }
            lengthIn = (lengthIn * 4) / 11;

            WebRtcSpl_DownsampleBy2(tmp, lengthIn, samplesOut, (WebRtc_Word32*)state2_);
            outLen = lengthIn / 2;

            free(tmp_mem);
            free(tmp);
            return 0;
        case kResamplerMode11To4:
            // We can only handle blocks of 220 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 220) != 0)
            {
                return -1;
            }
            if (maxLen < ((lengthIn * 4) / 11))
            {
                return -1;
            }
            tmp_mem = (WebRtc_Word32*)malloc(126 * sizeof(WebRtc_Word32));

            for (int i = 0; i < lengthIn; i += 220)
            {
                WebRtcSpl_Resample22khzTo8khz(samplesIn + i, samplesOut + (i * 4) / 11,
                                              (WebRtcSpl_State22khzTo8khz *)state1_,
                                              tmp_mem);
            }
            outLen = (lengthIn * 4) / 11;
            free(tmp_mem);
            return 0;
        case kResamplerMode11To8:
            // We can only handle blocks of 160 samples
            // Can be fixed, but I don't think it's needed
            if ((lengthIn % 220) != 0)
            {
                return -1;
            }
            if (maxLen < ((lengthIn * 8) / 11))
            {
                return -1;
            }
            tmp_mem = (WebRtc_Word32*)malloc(104 * sizeof(WebRtc_Word32));

            for (int i = 0; i < lengthIn; i += 220)
            {
                WebRtcSpl_Resample22khzTo16khz(samplesIn + i, samplesOut + (i * 8) / 11,
                                               (WebRtcSpl_State22khzTo16khz *)state1_,
                                               tmp_mem);
            }
            outLen = (lengthIn * 8) / 11;
            free(tmp_mem);
            return 0;
            break;

    }
    return 0;
}

// Asynchronous resampling, input
int Resampler::Insert(WebRtc_Word16 * samplesIn, int lengthIn)
{
    if (my_type_ != kResamplerAsynchronous)
    {
        return -1;
    }
    int sizeNeeded, tenMsblock;

    // Determine need for size of outBuffer
    sizeNeeded = out_buffer_size_ + ((lengthIn + in_buffer_size_) * my_out_frequency_khz_)
            / my_in_frequency_khz_;
    if (sizeNeeded > out_buffer_size_max_)
    {
        // Round the value upwards to complete 10 ms blocks
        tenMsblock = my_out_frequency_khz_ * 10;
        sizeNeeded = (sizeNeeded / tenMsblock + 1) * tenMsblock;
        out_buffer_ = (WebRtc_Word16*)realloc(out_buffer_, sizeNeeded * sizeof(WebRtc_Word16));
        out_buffer_size_max_ = sizeNeeded;
    }

    // If we need to use inBuffer, make sure all input data fits there.

    tenMsblock = my_in_frequency_khz_ * 10;
    if (in_buffer_size_ || (lengthIn % tenMsblock))
    {
        // Check if input buffer size is enough
        if ((in_buffer_size_ + lengthIn) > in_buffer_size_max_)
        {
            // Round the value upwards to complete 10 ms blocks
            sizeNeeded = ((in_buffer_size_ + lengthIn) / tenMsblock + 1) * tenMsblock;
            in_buffer_ = (WebRtc_Word16*)realloc(in_buffer_,
                                                 sizeNeeded * sizeof(WebRtc_Word16));
            in_buffer_size_max_ = sizeNeeded;
        }
        // Copy in data to input buffer
        memcpy(in_buffer_ + in_buffer_size_, samplesIn, lengthIn * sizeof(WebRtc_Word16));

        // Resample all available 10 ms blocks
        int lenOut;
        int dataLenToResample = (in_buffer_size_ / tenMsblock) * tenMsblock;
        Push(in_buffer_, dataLenToResample, out_buffer_ + out_buffer_size_,
             out_buffer_size_max_ - out_buffer_size_, lenOut);
        out_buffer_size_ += lenOut;

        // Save the rest
        memmove(in_buffer_, in_buffer_ + dataLenToResample,
                (in_buffer_size_ - dataLenToResample) * sizeof(WebRtc_Word16));
        in_buffer_size_ -= dataLenToResample;
    } else
    {
        // Just resample
        int lenOut;
        Push(in_buffer_, lengthIn, out_buffer_ + out_buffer_size_,
             out_buffer_size_max_ - out_buffer_size_, lenOut);
        out_buffer_size_ += lenOut;
    }

    return 0;
}

// Asynchronous resampling output, remaining samples are buffered
int Resampler::Pull(WebRtc_Word16* samplesOut, int desiredLen, int &outLen)
{
    if (my_type_ != kResamplerAsynchronous)
    {
        return -1;
    }

    // Check that we have enough data
    if (desiredLen <= out_buffer_size_)
    {
        // Give out the date
        memcpy(samplesOut, out_buffer_, desiredLen * sizeof(WebRtc_Word32));

        // Shuffle down remaining
        memmove(out_buffer_, out_buffer_ + desiredLen,
                (out_buffer_size_ - desiredLen) * sizeof(WebRtc_Word16));

        // Update remaining size
        out_buffer_size_ -= desiredLen;

        return 0;
    } else
    {
        return -1;
    }
}

} // namespace webrtc
