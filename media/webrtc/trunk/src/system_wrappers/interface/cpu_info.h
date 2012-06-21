/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_INFO_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_INFO_H_

#include "typedefs.h"

namespace webrtc {
class CpuInfo
{
public:
    static WebRtc_UWord32 DetectNumberOfCores();

private:
    CpuInfo() {}
    static WebRtc_UWord32 _numberOfCores;
};
} // namespace webrtc
#endif // WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_INFO_H_
