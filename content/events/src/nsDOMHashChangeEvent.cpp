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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
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

#include "nsDOMHashChangeEvent.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF_INHERITED(nsDOMHashChangeEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMHashChangeEvent, nsDOMEvent)

DOMCI_DATA(HashChangeEvent, nsDOMHashChangeEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMHashChangeEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHashChangeEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HashChangeEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

nsDOMHashChangeEvent::~nsDOMHashChangeEvent()
{
}

NS_IMETHODIMP
nsDOMHashChangeEvent::GetOldURL(nsAString &aURL)
{
  aURL.Assign(mOldURL);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMHashChangeEvent::GetNewURL(nsAString &aURL)
{
  aURL.Assign(mNewURL);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMHashChangeEvent::InitHashChangeEvent(const nsAString &aTypeArg,
                                          bool aCanBubbleArg,
                                          bool aCancelableArg,
                                          const nsAString &aOldURL,
                                          const nsAString &aNewURL)
{
  nsresult rv = nsDOMEvent::InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mOldURL.Assign(aOldURL);
  mNewURL.Assign(aNewURL);
  return NS_OK;
}

nsresult NS_NewDOMHashChangeEvent(nsIDOMEvent** aInstancePtrResult,
                                nsPresContext* aPresContext,
                                nsEvent* aEvent)
{
  nsDOMHashChangeEvent* event =
    new nsDOMHashChangeEvent(aPresContext, aEvent);

  return CallQueryInterface(event, aInstancePtrResult);
}
