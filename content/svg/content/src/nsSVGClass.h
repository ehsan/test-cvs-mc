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
 * The Initial Developer of the Original Code is Robert Longson.
 * Portions created by the Initial Developer are Copyright (C) 2011
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

#ifndef __NS_SVGCLASS_H__
#define __NS_SVGCLASS_H__

#include "nsIDOMSVGAnimatedString.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsString.h"
#include "nsISMILAttr.h"
#include "nsDOMError.h"

class nsSVGStylableElement;

class nsSVGClass
{

public:
  void Init() {
    mAnimVal = nsnull;
  }

  void SetBaseValue(const nsAString& aValue,
                    nsSVGStylableElement *aSVGElement,
                    bool aDoSetAttr);
  void GetBaseValue(nsAString& aValue, const nsSVGStylableElement *aSVGElement) const;

  void SetAnimValue(const nsAString& aValue, nsSVGStylableElement *aSVGElement);
  void GetAnimValue(nsAString& aValue, const nsSVGStylableElement *aSVGElement) const;
  bool IsAnimated() const
    { return !!mAnimVal; }

  nsresult ToDOMAnimatedString(nsIDOMSVGAnimatedString **aResult,
                               nsSVGStylableElement *aSVGElement);
#ifdef MOZ_SMIL
  // Returns a new nsISMILAttr object that the caller must delete
  nsISMILAttr* ToSMILAttr(nsSVGStylableElement *aSVGElement);
#endif // MOZ_SMIL

private:

  nsAutoPtr<nsString> mAnimVal;

public:
  struct DOMAnimatedString : public nsIDOMSVGAnimatedString
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedString)

    DOMAnimatedString(nsSVGClass *aVal, nsSVGStylableElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGClass* mVal; // kept alive because it belongs to content
    nsRefPtr<nsSVGStylableElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsAString& aResult)
      { mVal->GetBaseValue(aResult, mSVGElement); return NS_OK; }
    NS_IMETHOD SetBaseVal(const nsAString& aValue)
      { mVal->SetBaseValue(aValue, mSVGElement, true); return NS_OK; }

    NS_IMETHOD GetAnimVal(nsAString& aResult);
  };
#ifdef MOZ_SMIL
  struct SMILString : public nsISMILAttr
  {
  public:
    SMILString(nsSVGClass *aVal, nsSVGStylableElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    // These will stay alive because a nsISMILAttr only lives as long
    // as the Compositing step, and DOM elements don't get a chance to
    // die during that.
    nsSVGClass* mVal;
    nsSVGStylableElement* mSVGElement;

    // nsISMILAttr methods
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement *aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
#endif // MOZ_SMIL
};
#endif //__NS_SVGCLASS_H__
