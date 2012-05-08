/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla Corporation code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Bas Schouten <bschouten@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "ImageScaling.h"
#include "2D.h"

#include <math.h>
#include <algorithm>

using namespace std;

namespace mozilla {
namespace gfx {

inline uint32_t Avg2x2(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
  // Prepare half-adder work
  uint32_t sum = a ^ b ^ c;
  uint32_t carry = (a & b) | (a & c) | (b & c);

  // Before shifting, mask lower order bits of each byte to avoid underflow.
  uint32_t mask = 0xfefefefe;

  // Add d to sum and divide by 2.
  sum = (((sum ^ d) & mask) >> 1) + (sum & d);

  // Sum is now shifted into place relative to carry, add them together.
  return (((sum ^ carry) & mask) >> 1) + (sum & carry);
}

inline uint32_t Avg2(uint32_t a, uint32_t b)
{
  // Prepare half-adder work
  uint32_t sum = a ^ b;
  uint32_t carry = (a & b);

  // Before shifting, mask lower order bits of each byte to avoid underflow.
  uint32_t mask = 0xfefefefe;

  // Add d to sum and divide by 2.
  return ((sum & mask) >> 1) + carry;
}

void
ImageHalfScaler::ScaleForSize(const IntSize &aSize)
{
  uint32_t horizontalDownscales = 0;
  uint32_t verticalDownscales = 0;

  IntSize scaleSize = mOrigSize;
  while ((scaleSize.height / 2) > aSize.height) {
    verticalDownscales++;
    scaleSize.height /= 2;
  }

  while ((scaleSize.width / 2) > aSize.width) {
    horizontalDownscales++;
    scaleSize.width /= 2;
  }

  if (scaleSize == mOrigSize) {
    return;
  }

  IntSize internalSurfSize;

  internalSurfSize.width = max(scaleSize.width, mOrigSize.width / 2);
  internalSurfSize.height = max(scaleSize.height, mOrigSize.height / 2);

  mStride = internalSurfSize.width * 4;
  if (mStride % 16) {
    mStride += 16 - (mStride % 16);
  }

  delete [] mDataStorage;
  // Allocate 15 bytes extra to make sure we can get 16 byte alignment. We
  // should add tools for this, see bug 751696.
  mDataStorage = new uint8_t[internalSurfSize.height * mStride + 15];

  if (uintptr_t(mDataStorage) % 16) {
    // Our storage does not start at a 16-byte boundary. Make sure mData does!
    mData = (uint8_t*)(uintptr_t(mDataStorage) +
      (16 - (uintptr_t(mDataStorage) % 16)));
  } else {
    mData = mDataStorage;
  }

  mSize = scaleSize;

  /* The surface we sample from might not be even sized, if it's not we will
   * ignore the last row/column. This means we lose some data but it keeps the
   * code very simple. There's also no perfect answer that provides a better
   * solution.
   */
  IntSize currentSampledSize = mOrigSize;
  uint32_t currentSampledStride = mOrigStride;
  uint8_t *currentSampledData = mOrigData;
  
  while (verticalDownscales && horizontalDownscales) {
    if (currentSampledSize.width % 2) {
      currentSampledSize.width -= 1;
    }
    if (currentSampledSize.height % 2) {
      currentSampledSize.height -= 1;
    }

    HalfImage2D(currentSampledData, currentSampledStride, currentSampledSize,
                mData, mStride);

    verticalDownscales--;
    horizontalDownscales--;
    currentSampledSize.width /= 2;
    currentSampledSize.height /= 2;
    currentSampledData = mData;
    currentSampledStride = mStride;
  }

  while (verticalDownscales) {
    if (currentSampledSize.height % 2) {
      currentSampledSize.height -= 1;
    }

    HalfImageVertical(currentSampledData, currentSampledStride, currentSampledSize,
                      mData, mStride);

    verticalDownscales--;
    currentSampledSize.height /= 2;
    currentSampledData = mData;
    currentSampledStride = mStride;
  }


  while (horizontalDownscales) {
    if (currentSampledSize.width % 2) {
      currentSampledSize.width -= 1;
    }

    HalfImageHorizontal(currentSampledData, currentSampledStride, currentSampledSize,
                        mData, mStride);

    horizontalDownscales--;
    currentSampledSize.width /= 2;
    currentSampledData = mData;
    currentSampledStride = mStride;
  }
}

void
ImageHalfScaler::HalfImage2D(uint8_t *aSource, int32_t aSourceStride,
                             const IntSize &aSourceSize, uint8_t *aDest,
                             uint32_t aDestStride)
{
#ifdef USE_SSE2
  if (Factory::HasSSE2()) {
    HalfImage2D_SSE2(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  } else
#endif
  {
    HalfImage2D_C(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  }
}

void
ImageHalfScaler::HalfImageVertical(uint8_t *aSource, int32_t aSourceStride,
                                   const IntSize &aSourceSize, uint8_t *aDest,
                                   uint32_t aDestStride)
{
#ifdef USE_SSE2
  if (Factory::HasSSE2()) {
    HalfImageVertical_SSE2(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  } else
#endif
  {
    HalfImageVertical_C(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  }
}

void
ImageHalfScaler::HalfImageHorizontal(uint8_t *aSource, int32_t aSourceStride,
                                     const IntSize &aSourceSize, uint8_t *aDest,
                                     uint32_t aDestStride)
{
#ifdef USE_SSE2
  if (Factory::HasSSE2()) {
    HalfImageHorizontal_SSE2(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  } else
#endif
  {
    HalfImageHorizontal_C(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  }
}

void
ImageHalfScaler::HalfImage2D_C(uint8_t *aSource, int32_t aSourceStride,
                               const IntSize &aSourceSize, uint8_t *aDest,
                               uint32_t aDestStride)
{
  for (int y = 0; y < aSourceSize.height; y += 2) {
    uint32_t *storage = (uint32_t*)(aDest + (y / 2) * aDestStride);
    for (int x = 0; x < aSourceSize.width; x += 2) {
      uint8_t *upperRow = aSource + (y * aSourceStride + x * 4);
      uint8_t *lowerRow = aSource + ((y + 1) * aSourceStride + x * 4);

      *storage++ = Avg2x2(*(uint32_t*)upperRow, *((uint32_t*)upperRow + 1),
                          *(uint32_t*)lowerRow, *((uint32_t*)lowerRow + 1));
    }
  }
}

void
ImageHalfScaler::HalfImageVertical_C(uint8_t *aSource, int32_t aSourceStride,
                                     const IntSize &aSourceSize, uint8_t *aDest,
                                     uint32_t aDestStride)
{
  for (int y = 0; y < aSourceSize.height; y += 2) {
    uint32_t *storage = (uint32_t*)(aDest + (y / 2) * aDestStride);
    for (int x = 0; x < aSourceSize.width; x++) {
      uint32_t *upperRow = (uint32_t*)(aSource + (y * aSourceStride + x * 4));
      uint32_t *lowerRow = (uint32_t*)(aSource + ((y + 1) * aSourceStride + x * 4));

      *storage++ = Avg2(*upperRow, *lowerRow);
    }
  }
}

void
ImageHalfScaler::HalfImageHorizontal_C(uint8_t *aSource, int32_t aSourceStride,
                                       const IntSize &aSourceSize, uint8_t *aDest,
                                       uint32_t aDestStride)
{
  for (int y = 0; y < aSourceSize.height; y++) {
    uint32_t *storage = (uint32_t*)(aDest + y * aDestStride);
    for (int x = 0; x < aSourceSize.width;  x+= 2) {
      uint32_t *pixels = (uint32_t*)(aSource + (y * aSourceStride + x * 4));

      *storage++ = Avg2(*pixels, *(pixels + 1));
    }
  }
}

}
}
