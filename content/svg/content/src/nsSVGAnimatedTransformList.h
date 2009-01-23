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

#ifndef __NS_SVGANIMATEDTRANSFORMLIST_H__
#define __NS_SVGANIMATEDTRANSFORMLIST_H__

#include "nsIDOMSVGAnimTransformList.h"
#include "nsIDOMSVGTransformList.h"
#include "nsSVGValue.h"

////////////////////////////////////////////////////////////////////////
// nsSVGAnimatedTransformList

class nsSVGTransformSMILAttr;

class nsSVGAnimatedTransformList : public nsIDOMSVGAnimatedTransformList,
                                   public nsSVGValue,
                                   public nsISVGValueObserver
{  
protected:
  friend nsresult
  NS_NewSVGAnimatedTransformList(nsIDOMSVGAnimatedTransformList** result,
                                 nsIDOMSVGTransformList* baseVal);

  ~nsSVGAnimatedTransformList();
  void Init(nsIDOMSVGTransformList* baseVal);

public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsIDOMSVGAnimatedTransformList interface:
  NS_DECL_NSIDOMSVGANIMATEDTRANSFORMLIST

  // remainder of nsISVGValue interface:
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  // nsISVGValueObserver
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     modificationType aModType);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     modificationType aModType);

  // nsISupportsWeakReference
  // implementation inherited from nsSupportsWeakReference

protected:
  friend class nsSVGTransformSMILAttr;

  nsCOMPtr<nsIDOMSVGTransformList> mBaseVal;
  // XXX This should be read-only, i.e. its setters should throw
  nsCOMPtr<nsIDOMSVGTransformList> mAnimVal;
};

nsresult
NS_NewSVGAnimatedTransformList(nsIDOMSVGAnimatedTransformList** result,
                               nsIDOMSVGTransformList* baseVal);

#endif //__NS_SVGANIMATEDTRANSFORMLIST_H__
