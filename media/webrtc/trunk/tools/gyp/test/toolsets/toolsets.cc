/* Copyright (c) 2009 Google Inc. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

const char *GetToolset() {
#ifdef TARGET
  return "Target";
#else
  return "Host";
#endif
}
