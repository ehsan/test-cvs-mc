/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZILLA_GFX_BASESIZE_H_
#define MOZILLA_GFX_BASESIZE_H_

namespace mozilla {
namespace gfx {

/**
 * Do not use this class directly. Subclass it, pass that subclass as the
 * Sub parameter, and only use that subclass. This allows methods to safely
 * cast 'this' to 'Sub*'.
 */
template <class T, class Sub>
struct BaseSize {
  T width, height;

  // Constructors
  BaseSize() : width(0), height(0) {}
  BaseSize(T aWidth, T aHeight) : width(aWidth), height(aHeight) {}

  void SizeTo(T aWidth, T aHeight) { width = aWidth; height = aHeight; }

  // Note that '=' isn't defined so we'll get the
  // compiler generated default assignment operator

  bool operator==(const Sub& aSize) const {
    return width == aSize.width && height == aSize.height;
  }
  bool operator!=(const Sub& aSize) const {
    return width != aSize.width || height != aSize.height;
  }
  bool operator<=(const Sub& aSize) const {
    return width <= aSize.width && height <= aSize.height;
  }
  bool operator<(const Sub& aSize) const {
    return *this <= aSize && *this != aSize;
  }

  Sub operator+(const Sub& aSize) const {
    return Sub(width + aSize.width, height + aSize.height);
  }
  Sub operator-(const Sub& aSize) const {
    return Sub(width - aSize.width, height - aSize.height);
  }
  Sub& operator+=(const Sub& aSize) {
    width += aSize.width;
    height += aSize.height;
    return *static_cast<Sub*>(this);
  }
  Sub& operator-=(const Sub& aSize) {
    width -= aSize.width;
    height -= aSize.height;
    return *static_cast<Sub*>(this);
  }

  Sub operator*(T aScale) const {
    return Sub(width * aScale, height * aScale);
  }
  Sub operator/(T aScale) const {
    return Sub(width / aScale, height / aScale);
  }
};

}
}

#endif /* MOZILLA_GFX_BASESIZE_H_ */
