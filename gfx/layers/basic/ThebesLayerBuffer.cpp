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
 * The Original Code is Mozilla Corporation code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Robert O'Callahan <robert@ocallahan.org>
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

#include "ThebesLayerBuffer.h"
#include "Layers.h"
#include "gfxContext.h"
#include "gfxPlatform.h"

namespace mozilla {
namespace layers {

static void
ClipToRegion(gfxContext* aContext, const nsIntRegion& aRegion)
{
  aContext->NewPath();
  nsIntRegionRectIterator iter(aRegion);
  const nsIntRect* r;
  while ((r = iter.Next()) != nsnull) {
    aContext->Rectangle(gfxRect(r->x, r->y, r->width, r->height));
  }
  aContext->Clip();
}

nsIntRect
ThebesLayerBuffer::GetQuadrantRectangle(XSide aXSide, YSide aYSide)
{
  // quadrantTranslation is the amount we translate the top-left
  // of the quadrant by to get coordinates relative to the layer
  nsIntPoint quadrantTranslation = -mBufferRotation;
  quadrantTranslation.x += aXSide == LEFT ? mBufferRect.width : 0;
  quadrantTranslation.y += aYSide == TOP ? mBufferRect.height : 0;
  return mBufferRect + quadrantTranslation;
}

/**
 * @param aXSide LEFT means we draw from the left side of the buffer (which
 * is drawn on the right side of mBufferRect). RIGHT means we draw from
 * the right side of the buffer (which is drawn on the left side of
 * mBufferRect).
 * @param aYSide TOP means we draw from the top side of the buffer (which
 * is drawn on the bottom side of mBufferRect). BOTTOM means we draw from
 * the bottom side of the buffer (which is drawn on the top side of
 * mBufferRect).
 */
void
ThebesLayerBuffer::DrawBufferQuadrant(gfxContext* aTarget,
                                      XSide aXSide, YSide aYSide, float aOpacity)
{
  // The rectangle that we're going to fill. Basically we're going to
  // render the buffer at mBufferRect + quadrantTranslation to get the
  // pixels in the right place, but we're only going to paint within
  // mBufferRect
  nsIntRect quadrantRect = GetQuadrantRectangle(aXSide, aYSide);
  nsIntRect fillRect;
  if (!fillRect.IntersectRect(mBufferRect, quadrantRect))
    return;

  aTarget->NewPath();
  aTarget->Rectangle(gfxRect(fillRect.x, fillRect.y, fillRect.width, fillRect.height),
                     PR_TRUE);
  aTarget->SetSource(mBuffer, gfxPoint(quadrantRect.x, quadrantRect.y));
  if (aOpacity != 1.0) {
    aTarget->Save();
    aTarget->Clip();
    aTarget->Paint(aOpacity);
    aTarget->Restore();
  } else {
    aTarget->Fill();
  }
}

void
ThebesLayerBuffer::DrawBufferWithRotation(gfxContext* aTarget, float aOpacity)
{
  // Draw four quadrants. We could use REPEAT_, but it's probably better
  // not to, to be performance-safe.
  DrawBufferQuadrant(aTarget, LEFT, TOP, aOpacity);
  DrawBufferQuadrant(aTarget, RIGHT, TOP, aOpacity);
  DrawBufferQuadrant(aTarget, LEFT, BOTTOM, aOpacity);
  DrawBufferQuadrant(aTarget, RIGHT, BOTTOM, aOpacity);
}

static void
WrapRotationAxis(PRInt32* aRotationPoint, PRInt32 aSize)
{
  if (*aRotationPoint < 0) {
    *aRotationPoint += aSize;
  } else if (*aRotationPoint >= aSize) {
    *aRotationPoint -= aSize;
  }
}

static already_AddRefed<gfxASurface>
CreateBuffer(gfxASurface* aTargetSurface, gfxASurface::gfxContentType aType,
             const nsIntSize& aSize)
{
  return aTargetSurface->CreateSimilarSurface(aType, gfxIntSize(aSize.width, aSize.height));
}

ThebesLayerBuffer::PaintState
ThebesLayerBuffer::BeginPaint(ThebesLayer* aLayer, gfxContext* aTarget,
                              PRUint32 aFlags)
{
  PaintState result;

  gfxASurface::gfxContentType desiredContentType = gfxASurface::CONTENT_COLOR_ALPHA;
  nsRefPtr<gfxASurface> targetSurface = aTarget->CurrentSurface();
  if (targetSurface->AreSimilarSurfacesSensitiveToContentType()) {
    if (aFlags & OPAQUE_CONTENT) {
      desiredContentType = gfxASurface::CONTENT_COLOR;
    }
    if (mBuffer && desiredContentType != mBuffer->GetContentType()) {
      result.mRegionToInvalidate = aLayer->GetValidRegion();
      Clear();
    }
  }

  result.mRegionToDraw.Sub(aLayer->GetVisibleRegion(), aLayer->GetValidRegion());
  if (result.mRegionToDraw.IsEmpty())
    return result;
  nsIntRect drawBounds = result.mRegionToDraw.GetBounds();

  nsIntRect visibleBounds = aLayer->GetVisibleRegion().GetBounds();
  nsRefPtr<gfxASurface> destBuffer;
  nsIntRect destBufferRect;

  if (mBufferRect.width >= visibleBounds.width &&
      mBufferRect.height >= visibleBounds.height) {
    // The current buffer is big enough to hold the visible area.
    if (mBufferRect.Contains(visibleBounds)) {
      // We don't need to adjust mBufferRect.
      destBufferRect = mBufferRect;
    } else {
      // The buffer's big enough but doesn't contain everything that's
      // going to be visible. We'll move it.
      destBufferRect = nsIntRect(visibleBounds.TopLeft(), mBufferRect.Size());
    }
    nsIntRect keepArea;
    if (keepArea.IntersectRect(destBufferRect, mBufferRect)) {
      // Set mBufferRotation so that the pixels currently in mBuffer
      // will still be rendered in the right place when mBufferRect
      // changes to destBufferRect.
      nsIntPoint newRotation = mBufferRotation +
        (destBufferRect.TopLeft() - mBufferRect.TopLeft());
      WrapRotationAxis(&newRotation.x, mBufferRect.width);
      WrapRotationAxis(&newRotation.y, mBufferRect.height);
      NS_ASSERTION(nsIntRect(nsIntPoint(0,0), mBufferRect.Size()).Contains(newRotation),
                   "newRotation out of bounds");
      PRInt32 xBoundary = destBufferRect.XMost() - newRotation.x;
      PRInt32 yBoundary = destBufferRect.YMost() - newRotation.y;
      if ((drawBounds.x < xBoundary && xBoundary < drawBounds.XMost()) ||
          (drawBounds.y < yBoundary && yBoundary < drawBounds.YMost())) {
        // The stuff we need to redraw will wrap around an edge of the
        // buffer, so we will need to do a self-copy
        if (mBufferRotation == nsIntPoint(0,0)) {
          destBuffer = mBuffer;
        } else {
          // We can't do a real self-copy because the buffer is rotated.
          // So allocate a new buffer for the destination.
          destBufferRect = visibleBounds;
          destBuffer = CreateBuffer(targetSurface, desiredContentType, destBufferRect.Size());
          if (!destBuffer)
            return result;
        }
      } else {
        mBufferRect = destBufferRect;
        mBufferRotation = newRotation;
      }
    } else {
      // No pixels are going to be kept. The whole visible region
      // will be redrawn, so we don't need to copy anything, so we don't
      // set destBuffer.
      mBufferRect = destBufferRect;
      mBufferRotation = nsIntPoint(0,0);
    }
  } else {
    // The buffer's not big enough, so allocate a new one
    destBufferRect = visibleBounds;
    destBuffer = CreateBuffer(targetSurface, desiredContentType, destBufferRect.Size());
    if (!destBuffer)
      return result;
  }

  if (destBuffer) {
    if (mBuffer) {
      // Copy the bits
      nsRefPtr<gfxContext> tmpCtx = new gfxContext(destBuffer);
      nsIntPoint offset = -destBufferRect.TopLeft();
      tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
      tmpCtx->Translate(gfxPoint(offset.x, offset.y));
      DrawBufferWithRotation(tmpCtx, 1.0);
    }

    mBuffer = destBuffer.forget();
    mBufferRect = destBufferRect;
    mBufferRotation = nsIntPoint(0,0);
  }

  nsIntRegion invalidate;
  invalidate.Sub(aLayer->GetValidRegion(), destBufferRect);
  result.mRegionToInvalidate.Or(result.mRegionToInvalidate, invalidate);

  result.mContext = new gfxContext(mBuffer);

  // Figure out which quadrant to draw in
  PRInt32 xBoundary = mBufferRect.XMost() - mBufferRotation.x;
  PRInt32 yBoundary = mBufferRect.YMost() - mBufferRotation.y;
  XSide sideX = drawBounds.XMost() <= xBoundary ? RIGHT : LEFT;
  YSide sideY = drawBounds.YMost() <= yBoundary ? BOTTOM : TOP;
  nsIntRect quadrantRect = GetQuadrantRectangle(sideX, sideY);
  NS_ASSERTION(quadrantRect.Contains(drawBounds), "Messed up quadrants");
  result.mContext->Translate(-gfxPoint(quadrantRect.x, quadrantRect.y));

  ClipToRegion(result.mContext, result.mRegionToDraw);
  if (desiredContentType == gfxASurface::CONTENT_COLOR_ALPHA) {
    result.mContext->SetOperator(gfxContext::OPERATOR_CLEAR);
    result.mContext->Paint();
    result.mContext->SetOperator(gfxContext::OPERATOR_OVER);
  }
  return result;
}

void
ThebesLayerBuffer::DrawTo(ThebesLayer* aLayer, PRUint32 aFlags, gfxContext* aTarget, float aOpacity)
{
  aTarget->Save();
  ClipToRegion(aTarget, aLayer->GetVisibleRegion());
  if (aFlags & OPAQUE_CONTENT) {
    aTarget->SetOperator(gfxContext::OPERATOR_SOURCE);
  }
  DrawBufferWithRotation(aTarget, aOpacity);
  aTarget->Restore();
}

}
}

