/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=8 et :
 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Code.
 *
 * The Initial Developer of the Original Code is
 *   The Mozilla Foundation
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Chris Jones <jones.chris.g@gmail.com>
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

#include "nsBaseScreen.h"

NS_IMPL_ISUPPORTS1(nsBaseScreen, nsIScreen)

nsBaseScreen::nsBaseScreen()
{
  for (PRUint32 i = 0; i < nsIScreen::BRIGHTNESS_LEVELS; i++)
    mBrightnessLocks[i] = 0;
}

nsBaseScreen::~nsBaseScreen() { }

NS_IMETHODIMP
nsBaseScreen::LockMinimumBrightness(PRUint32 aBrightness)
{
  NS_ABORT_IF_FALSE(
    aBrightness < nsIScreen::BRIGHTNESS_LEVELS,
    "Invalid brightness level to lock");
  mBrightnessLocks[aBrightness]++;
  NS_ABORT_IF_FALSE(mBrightnessLocks[aBrightness] > 0,
    "Overflow after locking brightness level");

  CheckMinimumBrightness();

  return NS_OK;
}

NS_IMETHODIMP
nsBaseScreen::UnlockMinimumBrightness(PRUint32 aBrightness)
{
  NS_ABORT_IF_FALSE(
    aBrightness < nsIScreen::BRIGHTNESS_LEVELS,
    "Invalid brightness level to lock");
  NS_ABORT_IF_FALSE(mBrightnessLocks[aBrightness] > 0,
    "Unlocking a brightness level with no corresponding lock");
  mBrightnessLocks[aBrightness]--;

  CheckMinimumBrightness();

  return NS_OK;
}

void
nsBaseScreen::CheckMinimumBrightness()
{
  PRUint32 brightness = nsIScreen::BRIGHTNESS_LEVELS;
  for (PRUint32 i = 0; i < nsIScreen::BRIGHTNESS_LEVELS; i++)
    if (mBrightnessLocks[i] > 0)
      brightness = i;

  ApplyMinimumBrightness(brightness);
}
