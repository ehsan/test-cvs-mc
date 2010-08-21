/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mounir Lamouri <mounir.lamouri@mozilla.com> (original author)
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

#include "nsDOMValidityState.h"

#include "nsDOMClassInfo.h"
#include "nsConstraintValidation.h"


DOMCI_DATA(ValidityState, nsDOMValidityState)

NS_IMPL_ADDREF(nsDOMValidityState)
NS_IMPL_RELEASE(nsDOMValidityState)

NS_INTERFACE_MAP_BEGIN(nsDOMValidityState)
  NS_INTERFACE_MAP_ENTRY(nsIDOMValidityState)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMValidityState)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ValidityState)
NS_INTERFACE_MAP_END

nsDOMValidityState::nsDOMValidityState(nsConstraintValidation* aConstraintValidation)
  : mConstraintValidation(aConstraintValidation)
{
}

NS_IMETHODIMP
nsDOMValidityState::GetValueMissing(PRBool* aValueMissing)
{
  *aValueMissing = mConstraintValidation && mConstraintValidation->IsValueMissing();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetTypeMismatch(PRBool* aTypeMismatch)
{
  *aTypeMismatch = mConstraintValidation && mConstraintValidation->HasTypeMismatch();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetPatternMismatch(PRBool* aPatternMismatch)
{
  *aPatternMismatch = mConstraintValidation && mConstraintValidation->HasPatternMismatch();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetTooLong(PRBool* aTooLong)
{
  *aTooLong = mConstraintValidation && mConstraintValidation->IsTooLong();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetRangeUnderflow(PRBool* aRangeUnderflow)
{
  *aRangeUnderflow = mConstraintValidation && mConstraintValidation->HasRangeUnderflow();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetRangeOverflow(PRBool* aRangeOverflow)
{
  *aRangeOverflow = mConstraintValidation && mConstraintValidation->HasRangeOverflow();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetStepMismatch(PRBool* aStepMismatch)
{
  *aStepMismatch = mConstraintValidation && mConstraintValidation->HasStepMismatch();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetCustomError(PRBool* aCustomError)
{
  *aCustomError = mConstraintValidation && mConstraintValidation->HasCustomError();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMValidityState::GetValid(PRBool* aValid)
{
  *aValid = !mConstraintValidation || mConstraintValidation->IsValid();
  return NS_OK;
}

