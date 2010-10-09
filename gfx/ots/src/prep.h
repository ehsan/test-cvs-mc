// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OTS_PREP_H_
#define OTS_PREP_H_

#include "ots.h"

namespace ots {

struct OpenTypePREP {
  const uint8_t *data;
  uint32_t length;
};

}  // namespace ots

#endif  // OTS_PREP_H_
