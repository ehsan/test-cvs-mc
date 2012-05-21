/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Main header first:
#include "nsSVGClipPathFrame.h"

// Keep others in (case-insensitive) order:
#include "gfxContext.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGClipPathElement.h"
#include "nsRenderingContext.h"
#include "nsSVGClipPathElement.h"
#include "nsSVGEffects.h"
#include "nsSVGUtils.h"

//----------------------------------------------------------------------
// Implementation

nsIFrame*
NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGClipPathFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGClipPathFrame)

nsresult
nsSVGClipPathFrame::ClipPaint(nsRenderingContext* aContext,
                              nsIFrame* aParent,
                              const gfxMatrix &aMatrix)
{
  // If the flag is set when we get here, it means this clipPath frame
  // has already been used painting the current clip, and the document
  // has a clip reference loop.
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return NS_OK;
  }
  AutoClipPathReferencer clipRef(this);

  mClipParent = aParent;
  if (mClipParentMatrix) {
    *mClipParentMatrix = aMatrix;
  } else {
    mClipParentMatrix = new gfxMatrix(aMatrix);
  }

  bool isTrivial = IsTrivial();

  SVGAutoRenderState mode(aContext,
                          isTrivial ? SVGAutoRenderState::CLIP
                                    : SVGAutoRenderState::CLIP_MASK);

  gfxContext *gfx = aContext->ThebesContext();

  nsSVGClipPathFrame *clipPathFrame =
    nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(nsnull);
  bool referencedClipIsTrivial;
  if (clipPathFrame) {
    referencedClipIsTrivial = clipPathFrame->IsTrivial();
    gfx->Save();
    if (referencedClipIsTrivial) {
      clipPathFrame->ClipPaint(aContext, aParent, aMatrix);
    } else {
      gfx->PushGroup(gfxASurface::CONTENT_ALPHA);
    }
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      // The CTM of each frame referencing us can be different.
      SVGFrame->NotifySVGChanged(
                          nsISVGChildFrame::DO_NOT_NOTIFY_RENDERING_OBSERVERS | 
                          nsISVGChildFrame::TRANSFORM_CHANGED);

      bool isOK = true;
      nsSVGClipPathFrame *clipPathFrame =
        nsSVGEffects::GetEffectProperties(kid).GetClipPathFrame(&isOK);
      if (!isOK) {
        continue;
      }

      bool isTrivial;

      if (clipPathFrame) {
        isTrivial = clipPathFrame->IsTrivial();
        gfx->Save();
        if (isTrivial) {
          clipPathFrame->ClipPaint(aContext, aParent, aMatrix);
        } else {
          gfx->PushGroup(gfxASurface::CONTENT_ALPHA);
        }
      }

      SVGFrame->PaintSVG(aContext, nsnull);

      if (clipPathFrame) {
        if (!isTrivial) {
          gfx->PopGroupToSource();

          nsRefPtr<gfxPattern> clipMaskSurface;
          gfx->PushGroup(gfxASurface::CONTENT_ALPHA);

          clipPathFrame->ClipPaint(aContext, aParent, aMatrix);
          clipMaskSurface = gfx->PopGroup();

          if (clipMaskSurface) {
            gfx->Mask(clipMaskSurface);
          }
        }
        gfx->Restore();
      }
    }
  }

  if (clipPathFrame) {
    if (!referencedClipIsTrivial) {
      gfx->PopGroupToSource();

      nsRefPtr<gfxPattern> clipMaskSurface;
      gfx->PushGroup(gfxASurface::CONTENT_ALPHA);

      clipPathFrame->ClipPaint(aContext, aParent, aMatrix);
      clipMaskSurface = gfx->PopGroup();

      if (clipMaskSurface) {
        gfx->Mask(clipMaskSurface);
      }
    }
    gfx->Restore();
  }

  if (isTrivial) {
    gfx->Clip();
    gfx->NewPath();
  }

  return NS_OK;
}

bool
nsSVGClipPathFrame::ClipHitTest(nsIFrame* aParent,
                                const gfxMatrix &aMatrix,
                                const nsPoint &aPoint)
{
  // If the flag is set when we get here, it means this clipPath frame
  // has already been used in hit testing against the current clip,
  // and the document has a clip reference loop.
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return false;
  }
  AutoClipPathReferencer clipRef(this);

  mClipParent = aParent;
  if (mClipParentMatrix) {
    *mClipParentMatrix = aMatrix;
  } else {
    mClipParentMatrix = new gfxMatrix(aMatrix);
  }

  nsSVGClipPathFrame *clipPathFrame =
    nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(nsnull);
  if (clipPathFrame && !clipPathFrame->ClipHitTest(aParent, aMatrix, aPoint))
    return false;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      // Notify the child frame that we may be working with a
      // different transform, so it can update its covered region
      // (used to shortcut hit testing).
      SVGFrame->NotifySVGChanged(nsISVGChildFrame::TRANSFORM_CHANGED);

      if (SVGFrame->GetFrameForPoint(aPoint))
        return true;
    }
  }
  return false;
}

bool
nsSVGClipPathFrame::IsTrivial()
{
  // If the clip path is clipped then it's non-trivial
  if (nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(nsnull))
    return false;

  bool foundChild = false;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame *svgChild = do_QueryFrame(kid);
    if (svgChild) {
      // We consider a non-trivial clipPath to be one containing
      // either more than one svg child and/or a svg container
      if (foundChild || svgChild->IsDisplayContainer())
        return false;

      // or where the child is itself clipped
      if (nsSVGEffects::GetEffectProperties(kid).GetClipPathFrame(nsnull))
        return false;

      foundChild = true;
    }
  }
  return true;
}

bool
nsSVGClipPathFrame::IsValid()
{
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return false;
  }
  AutoClipPathReferencer clipRef(this);

  bool isOK = true;
  nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(&isOK);
  if (!isOK) {
    return false;
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {

    nsIAtom *type = kid->GetType();

    if (type == nsGkAtoms::svgUseFrame) {
      for (nsIFrame* grandKid = kid->GetFirstPrincipalChild(); grandKid;
           grandKid = grandKid->GetNextSibling()) {

        nsIAtom *type = grandKid->GetType();

        if (type != nsGkAtoms::svgPathGeometryFrame &&
            type != nsGkAtoms::svgTextFrame) {
          return false;
        }
      }
      continue;
    }
    if (type != nsGkAtoms::svgPathGeometryFrame &&
        type != nsGkAtoms::svgTextFrame) {
      return false;
    }
  }
  return true;
}

NS_IMETHODIMP
nsSVGClipPathFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::transform) {
      nsSVGUtils::NotifyChildrenOfSVGChange(this,
                                            nsISVGChildFrame::TRANSFORM_CHANGED);
    }
    if (aAttribute == nsGkAtoms::clipPathUnits) {
      nsSVGEffects::InvalidateRenderingObservers(this);
    }
  }

  return nsSVGClipPathFrameBase::AttributeChanged(aNameSpaceID,
                                                  aAttribute, aModType);
}

NS_IMETHODIMP
nsSVGClipPathFrame::Init(nsIContent* aContent,
                         nsIFrame* aParent,
                         nsIFrame* aPrevInFlow)
{
#ifdef DEBUG
  nsCOMPtr<nsIDOMSVGClipPathElement> clipPath = do_QueryInterface(aContent);
  NS_ASSERTION(clipPath, "Content is not an SVG clipPath!");
#endif

  AddStateBits(NS_STATE_SVG_CLIPPATH_CHILD);
  return nsSVGClipPathFrameBase::Init(aContent, aParent, aPrevInFlow);
}

nsIAtom *
nsSVGClipPathFrame::GetType() const
{
  return nsGkAtoms::svgClipPathFrame;
}

gfxMatrix
nsSVGClipPathFrame::GetCanvasTM()
{
  nsSVGClipPathElement *content = static_cast<nsSVGClipPathElement*>(mContent);

  gfxMatrix tm =
    content->PrependLocalTransformsTo(mClipParentMatrix ?
                                      *mClipParentMatrix : gfxMatrix());

  return nsSVGUtils::AdjustMatrixForUnits(tm,
                                          &content->mEnumAttributes[nsSVGClipPathElement::CLIPPATHUNITS],
                                          mClipParent);
}
