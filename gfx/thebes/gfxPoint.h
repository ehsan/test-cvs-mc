/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Oracle Corporation code.
 *
 * The Initial Developer of the Original Code is Oracle Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Stuart Parmenter <pavlov@pavlov.net>
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

#ifndef GFX_POINT_H
#define GFX_POINT_H

#include "nsMathUtils.h"
#include "mozilla/gfx/BaseSize.h"
#include "mozilla/gfx/BasePoint.h"
#include "nsSize.h"
#include "nsPoint.h"

#include "gfxTypes.h"

typedef nsIntSize gfxIntSize;

struct THEBES_API gfxSize : public mozilla::gfx::BaseSize<gfxFloat, gfxSize> {
    typedef mozilla::gfx::BaseSize<gfxFloat, gfxSize> Super;

    gfxSize() : Super() {}
    gfxSize(gfxFloat aWidth, gfxFloat aHeight) : Super(aWidth, aHeight) {}
    gfxSize(const nsIntSize& aSize) : Super(aSize.width, aSize.height) {}
};

struct THEBES_API gfxPoint : public mozilla::gfx::BasePoint<gfxFloat, gfxPoint> {
    typedef mozilla::gfx::BasePoint<gfxFloat, gfxPoint> Super;

    gfxPoint() : Super() {}
    gfxPoint(gfxFloat aX, gfxFloat aY) : Super(aX, aY) {}
    gfxPoint(const nsIntPoint& aPoint) : Super(aPoint.x, aPoint.y) {}

    // Round() is *not* rounding to nearest integer if the values are negative.
    // They are always rounding as floor(n + 0.5).
    // See https://bugzilla.mozilla.org/show_bug.cgi?id=410748#c14
    // And if you need similar method which is using NS_round(), you should
    // create new |RoundAwayFromZero()| method.
    gfxPoint& Round() {
        x = floor(x + 0.5);
        y = floor(y + 0.5);
        return *this;
    }
};

#endif /* GFX_POINT_H */
