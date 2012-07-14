/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __NS_SVGGRADIENTFRAME_H__
#define __NS_SVGGRADIENTFRAME_H__

#include "gfxMatrix.h"
#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsLiteralString.h"
#include "nsSVGPaintServerFrame.h"

class gfxPattern;
class nsIAtom;
class nsIContent;
class nsIFrame;
class nsIPresShell;
class nsStyleContext;
class nsSVGLinearGradientElement;
class nsSVGRadialGradientElement;

struct gfxRect;

namespace mozilla {
class SVGAnimatedTransformList;
}

typedef nsSVGPaintServerFrame nsSVGGradientFrameBase;

/**
 * Gradients can refer to other gradients. We create an nsSVGPaintingProperty
 * with property type nsGkAtoms::href to track the referenced gradient.
 */
class nsSVGGradientFrame : public nsSVGGradientFrameBase
{
protected:
  nsSVGGradientFrame(nsStyleContext* aContext);

public:
  NS_DECL_FRAMEARENA_HELPERS

  // nsSVGPaintServerFrame methods:
  virtual already_AddRefed<gfxPattern>
    GetPaintServerPattern(nsIFrame *aSource,
                          const gfxMatrix& aContextMatrix,
                          nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                          float aGraphicOpacity,
                          const gfxRect *aOverrideBounds);

  // nsIFrame interface:
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGradient"), aResult);
  }
#endif // DEBUG

private:

  // Parse our xlink:href and set up our nsSVGPaintingProperty if we
  // reference another gradient and we don't have a property. Return
  // the referenced gradient's frame if available, null otherwise.
  nsSVGGradientFrame* GetReferencedGradient();

  // Optionally get a stop frame (returns stop index/count)
  PRInt32 GetStopFrame(PRInt32 aIndex, nsIFrame * *aStopFrame);

  PRUint32 GetStopCount();
  void GetStopInformation(PRInt32 aIndex,
                          float *aOffset, nscolor *aColor, float *aStopOpacity);

  const mozilla::SVGAnimatedTransformList* GetGradientTransformList(
    nsIContent* aDefault);
  // Will be singular for gradientUnits="objectBoundingBox" with an empty bbox.
  gfxMatrix GetGradientTransform(nsIFrame *aSource,
                                 const gfxRect *aOverrideBounds);

protected:
  virtual already_AddRefed<gfxPattern> CreateGradient() = 0;

  // Internal methods for handling referenced gradients
  class AutoGradientReferencer;
  nsSVGGradientFrame* GetReferencedGradientIfNotInUse();

  // Accessors to lookup gradient attributes
  PRUint16 GetEnumValue(PRUint32 aIndex, nsIContent *aDefault);
  PRUint16 GetEnumValue(PRUint32 aIndex)
  {
    return GetEnumValue(aIndex, mContent);
  }
  PRUint16 GetGradientUnits();
  PRUint16 GetSpreadMethod();

  // Gradient-type-specific lookups since the length values differ between
  // linear and radial gradients
  virtual nsSVGLinearGradientElement * GetLinearGradientWithLength(
    PRUint32 aIndex, nsSVGLinearGradientElement* aDefault);
  virtual nsSVGRadialGradientElement * GetRadialGradientWithLength(
    PRUint32 aIndex, nsSVGRadialGradientElement* aDefault);

  // The frame our gradient is (currently) being applied to
  nsIFrame*                              mSource;

private:
  // Flag to mark this frame as "in use" during recursive calls along our
  // gradient's reference chain so we can detect reference loops. See:
  // http://www.w3.org/TR/SVG11/pservers.html#LinearGradientElementHrefAttribute
  bool                                   mLoopFlag;
  // Gradients often don't reference other gradients, so here we cache
  // the fact that that isn't happening.
  bool                                   mNoHRefURI;
};


// -------------------------------------------------------------------------
// Linear Gradients
// -------------------------------------------------------------------------

typedef nsSVGGradientFrame nsSVGLinearGradientFrameBase;

class nsSVGLinearGradientFrame : public nsSVGLinearGradientFrameBase
{
  friend nsIFrame* NS_NewSVGLinearGradientFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext);
protected:
  nsSVGLinearGradientFrame(nsStyleContext* aContext) :
    nsSVGLinearGradientFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  // nsIFrame interface:
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  virtual nsIAtom* GetType() const;  // frame type: nsGkAtoms::svgLinearGradientFrame

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGLinearGradient"), aResult);
  }
#endif // DEBUG

protected:
  float GetLengthValue(PRUint32 aIndex);
  virtual nsSVGLinearGradientElement * GetLinearGradientWithLength(
    PRUint32 aIndex, nsSVGLinearGradientElement* aDefault);
  virtual already_AddRefed<gfxPattern> CreateGradient();
};

// -------------------------------------------------------------------------
// Radial Gradients
// -------------------------------------------------------------------------

typedef nsSVGGradientFrame nsSVGRadialGradientFrameBase;

class nsSVGRadialGradientFrame : public nsSVGRadialGradientFrameBase
{
  friend nsIFrame* NS_NewSVGRadialGradientFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext);
protected:
  nsSVGRadialGradientFrame(nsStyleContext* aContext) :
    nsSVGRadialGradientFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  // nsIFrame interface:
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  virtual nsIAtom* GetType() const;  // frame type: nsGkAtoms::svgRadialGradientFrame

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGRadialGradient"), aResult);
  }
#endif // DEBUG

protected:
  float GetLengthValue(PRUint32 aIndex);
  float GetLengthValue(PRUint32 aIndex, float aDefaultValue);
  float GetLengthValueFromElement(PRUint32 aIndex,
                                  nsSVGRadialGradientElement& aElement);
  virtual nsSVGRadialGradientElement * GetRadialGradientWithLength(
    PRUint32 aIndex, nsSVGRadialGradientElement* aDefault);
  virtual already_AddRefed<gfxPattern> CreateGradient();
};

#endif // __NS_SVGGRADIENTFRAME_H__

