/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "jsutil.h"

/*
 * Check that we can use js_bitscan_clz32 to implement JS_FLOOR_LOG2 and
 * JS_FLOOR_LOG2W and js_bitscan_clz64 to implement JS_FLOOR_LOG2W on 64-bit
 * systems.
 */
#ifdef JS_HAS_BUILTIN_BITSCAN32
JS_STATIC_ASSERT(sizeof(unsigned int) == sizeof(uint32_t));
JS_STATIC_ASSERT_IF(JS_BYTES_PER_WORD == 4,
                    sizeof(unsigned int) == sizeof(JSUword));
#endif
#ifdef JS_HAS_BUILTIN_BITSCAN64
JS_STATIC_ASSERT_IF(JS_BYTES_PER_WORD == 8,
                    sizeof(unsigned long long) == sizeof(JSUword));
#endif

#if !defined(JS_HAS_BUILTIN_BITSCAN32) && JS_BYTES_PER_WORD == 4

size_t
js_FloorLog2wImpl(size_t n)
{
    size_t log2;

    JS_FLOOR_LOG2(log2, n);
    return log2;
}
#endif
/*
 * js_FloorLog2wImpl has to be defined only for 64-bit non-GCC case.
 */
#if !defined(JS_HAS_BUILTIN_BITSCAN64) && JS_BYTES_PER_WORD == 8

size_t
js_FloorLog2wImpl(size_t n)
{
    size_t log2, m;

    JS_ASSERT(n != 0);

    log2 = 0;
    m = n >> 32;
    if (m != 0) { n = m; log2 = 32; }
    m = n >> 16;
    if (m != 0) { n = m; log2 |= 16; }
    m = n >> 8;
    if (m != 0) { n = m; log2 |= 8; }
    m = n >> 4;
    if (m != 0) { n = m; log2 |= 4; }
    m = n >> 2;
    if (m != 0) { n = m; log2 |= 2; }
    log2 |= (n >> 1);

    return log2;
}

#endif
