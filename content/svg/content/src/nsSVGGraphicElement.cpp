/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is
 * Crocodile Clips Ltd..
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *   Jonathan Watt <jonathan.watt@strath.ac.uk>
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

#include "mozilla/Util.h"

#include "nsSVGGraphicElement.h"
#include "nsSVGSVGElement.h"
#include "DOMSVGAnimatedTransformList.h"
#include "DOMSVGMatrix.h"
#include "nsGkAtoms.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMMutationEvent.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsIDOMSVGPoint.h"
#include "nsSVGUtils.h"
#include "nsDOMError.h"
#include "nsSVGRect.h"
#include "nsContentUtils.h"

using namespace mozilla;

//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_ADDREF_INHERITED(nsSVGGraphicElement, nsSVGGraphicElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGGraphicElement, nsSVGGraphicElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGGraphicElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLocatable)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransformable)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGraphicElementBase)

//----------------------------------------------------------------------
// Implementation

nsSVGGraphicElement::nsSVGGraphicElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGGraphicElementBase(aNodeInfo)
{
}

//----------------------------------------------------------------------
// nsIDOMSVGLocatable methods

/* readonly attribute nsIDOMSVGElement nearestViewportElement; */
NS_IMETHODIMP nsSVGGraphicElement::GetNearestViewportElement(nsIDOMSVGElement * *aNearestViewportElement)
{
  *aNearestViewportElement = nsSVGUtils::GetNearestViewportElement(this).get();
  return NS_OK;
}

/* readonly attribute nsIDOMSVGElement farthestViewportElement; */
NS_IMETHODIMP nsSVGGraphicElement::GetFarthestViewportElement(nsIDOMSVGElement * *aFarthestViewportElement)
{
  NS_IF_ADDREF(*aFarthestViewportElement = nsSVGUtils::GetOuterSVGElement(this));
  return NS_OK;
}

/* nsIDOMSVGRect getBBox (); */
NS_IMETHODIMP nsSVGGraphicElement::GetBBox(nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;

  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);

  if (!frame || (frame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD))
    return NS_ERROR_FAILURE;

  nsISVGChildFrame* svgframe = do_QueryFrame(frame);
  if (svgframe) {
    return NS_NewSVGRect(_retval, nsSVGUtils::GetBBox(frame));
  }
  return NS_ERROR_FAILURE;
}

/* nsIDOMSVGMatrix getCTM (); */
NS_IMETHODIMP nsSVGGraphicElement::GetCTM(nsIDOMSVGMatrix * *aCTM)
{
  gfxMatrix m = nsSVGUtils::GetCTM(this, false);
  *aCTM = m.IsSingular() ? nsnull : new DOMSVGMatrix(m);
  NS_IF_ADDREF(*aCTM);
  return NS_OK;
}

/* nsIDOMSVGMatrix getScreenCTM (); */
NS_IMETHODIMP nsSVGGraphicElement::GetScreenCTM(nsIDOMSVGMatrix * *aCTM)
{
  gfxMatrix m = nsSVGUtils::GetCTM(this, true);
  *aCTM = m.IsSingular() ? nsnull : new DOMSVGMatrix(m);
  NS_IF_ADDREF(*aCTM);
  return NS_OK;
}

/* nsIDOMSVGMatrix getTransformToElement (in nsIDOMSVGElement element); */
NS_IMETHODIMP nsSVGGraphicElement::GetTransformToElement(nsIDOMSVGElement *element, nsIDOMSVGMatrix **_retval)
{
  if (!element)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  nsresult rv;
  *_retval = nsnull;
  nsCOMPtr<nsIDOMSVGMatrix> ourScreenCTM;
  nsCOMPtr<nsIDOMSVGMatrix> targetScreenCTM;
  nsCOMPtr<nsIDOMSVGMatrix> tmp;
  nsCOMPtr<nsIDOMSVGLocatable> target = do_QueryInterface(element, &rv);
  if (NS_FAILED(rv)) return rv;

  // the easiest way to do this (if likely to increase rounding error):
  GetScreenCTM(getter_AddRefs(ourScreenCTM));
  if (!ourScreenCTM) return NS_ERROR_DOM_SVG_MATRIX_NOT_INVERTABLE;
  target->GetScreenCTM(getter_AddRefs(targetScreenCTM));
  if (!targetScreenCTM) return NS_ERROR_DOM_SVG_MATRIX_NOT_INVERTABLE;
  rv = targetScreenCTM->Inverse(getter_AddRefs(tmp));
  if (NS_FAILED(rv)) return rv;
  return tmp->Multiply(ourScreenCTM, _retval);  // addrefs, so we don't
}

//----------------------------------------------------------------------
// nsIDOMSVGTransformable methods
/* readonly attribute nsIDOMSVGAnimatedTransformList transform; */

NS_IMETHODIMP nsSVGGraphicElement::GetTransform(
    nsIDOMSVGAnimatedTransformList **aTransform)
{
  *aTransform =
    DOMSVGAnimatedTransformList::GetDOMWrapper(GetAnimatedTransformList(), this)
    .get();
  return NS_OK;
}

//----------------------------------------------------------------------
// nsIContent methods

NS_IMETHODIMP_(bool)
nsSVGGraphicElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sGraphicsMap
  };
  
  return FindAttributeDependence(name, map) ||
    nsSVGGraphicElementBase::IsAttributeMapped(name);
}

nsChangeHint
nsSVGGraphicElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                            PRInt32 aModType) const
{
  nsChangeHint retval =
    nsSVGGraphicElementBase::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::transform) {
    // We add nsChangeHint_UpdateOverflow so that nsFrame::UpdateOverflow()
    // will be called on us and our ancestors.
    nsIFrame* frame =
      const_cast<nsSVGGraphicElement*>(this)->GetPrimaryFrame();
    if (frame && frame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD) {
      // No need to do anything.
    } else if (aModType == nsIDOMMutationEvent::ADDITION ||
               aModType == nsIDOMMutationEvent::REMOVAL) {
      // In order to handle view creation/destruction and stacking context
      // changes, the code in nsStyleDisplay::CalcDifference uses
      // nsChangeHint_ReconstructFrame if the transform was added/removed.
      // XXXSDL Currently we don't need to reconstruct SVG frames when their
      // transform is set/unset since we don't currently create GFX layers for
      // SVG transforms, but we will after bug 614732 is fixed. Also change the
      // assertion in ApplyRenderingChangeToTree when we do that.
      NS_UpdateHint(retval, nsChangeHint_UpdateOverflow);
    } else {
      NS_ABORT_IF_FALSE(aModType == nsIDOMMutationEvent::MODIFICATION,
                        "Unknown modification type.");
      // We just assume the old and new transforms are different.
      // XXXSDL Once we use GFX layers for SVG transforms, we will need to pass
      // the nsChangeHint_UpdateTransformLayer hint too. Note that the
      // assertion in ApplyRenderingChangeToTree will fail if that hint is
      // passed on nsIDOMMutationEvent::REMOVAL though.
      NS_UpdateHint(retval, nsChangeHint_UpdateOverflow);
    }
  }
  return retval;
}

//----------------------------------------------------------------------
// nsSVGElement overrides

bool
nsSVGGraphicElement::IsEventName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}

gfxMatrix
nsSVGGraphicElement::PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                                              TransformTypes aWhich) const
{
  NS_ABORT_IF_FALSE(aWhich != eChildToUserSpace || aMatrix.IsIdentity(),
                    "Skipping eUserSpaceToParent transforms makes no sense");

  gfxMatrix result(aMatrix);

  if (aWhich == eChildToUserSpace) {
    // We don't have anything to prepend.
    // eChildToUserSpace is not the common case, which is why we return
    // 'result' to benefit from NRVO rather than returning aMatrix before
    // creating 'result'.
    return result;
  }

  NS_ABORT_IF_FALSE(aWhich == eAllTransforms || aWhich == eUserSpaceToParent,
                    "Unknown TransformTypes");

  // animateMotion's resulting transform is supposed to apply *on top of*
  // any transformations from the |transform| attribute. So since we're
  // PRE-multiplying, we need to apply the animateMotion transform *first*.
  if (mAnimateMotionTransform) {
    result.PreMultiply(*mAnimateMotionTransform);
  }

  if (mTransforms) {
    result.PreMultiply(mTransforms->GetAnimValue().GetConsolidationMatrix());
  }

  return result;
}

void
nsSVGGraphicElement::SetAnimateMotionTransform(const gfxMatrix* aMatrix)
{
  mAnimateMotionTransform = aMatrix ? new gfxMatrix(*aMatrix) : nsnull;
  DidAnimateTransformList();
}

SVGAnimatedTransformList*
nsSVGGraphicElement::GetAnimatedTransformList()
{
  if (!mTransforms) {
    mTransforms = new SVGAnimatedTransformList();
  }
  return mTransforms;
}
