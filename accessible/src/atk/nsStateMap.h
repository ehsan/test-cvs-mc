/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:expandtab:shiftwidth=2:tabstop=2: */
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
 * The Initial Developer of the Original Code is IBM Corporation
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Aaron Leventhal (aleventh@us.ibm.com)
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

#include <atk/atk.h>
#include "nsAccessibleWrap.h"

/******************************************************************************
The following nsIAccessible states aren't translated, just ignored:
  STATE_READONLY:        Supported indirectly via EXT_STATE_EDITABLE
  STATE_HOTTRACKED:      No ATK equivalent.  No known use case.
                         The nsIAccessible state is not currently supported.
  STATE_FLOATING:        No ATK equivalent.  No known use case.
                         The nsIAccessible state is not currently supported.
  STATE_MOVEABLE:        No ATK equivalent.  No known use case.
                         The nsIAccessible state is not currently supported.
  STATE_SELFVOICING:     No ATK equivalent -- the object has self-TTS.
                         The nsIAccessible state is not currently supported.
  STATE_LINKED:          The object is formatted as a hyperlink. Supported via ATK_ROLE_LINK.
  STATE_EXTSELECTABLE:   Indicates that an object extends its selection.
                         This is supported via STATE_MULTISELECTABLE.
  STATE_PROTECTED:       The object is a password-protected edit control.
                         Supported via ATK_ROLE_PASSWORD_TEXT
  STATE_HASPOPUP:        Object displays a pop-up menu or window when invoked.
                         No ATK equivalent.  The nsIAccessible state is not currently supported.

The following ATK states are not supported:
  ATK_STATE_ARMED:       No clear use case, used briefly when button is activated
  ATK_STATE_HAS_TOOLTIP: No clear use case, no IA2 equivalent
  ATK_STATE_ICONIFIED:   Mozilla does not have elements which are collapsable into icons
  ATK_STATE_TRUNCATED:   No clear use case. Indicates that an object's onscreen content is truncated, 
                         e.g. a text value in a spreadsheet cell. No IA2 state.
******************************************************************************/

enum EStateMapEntryType {
  kMapDirectly,
  kMapOpposite,   // For example, UNAVAILABLE is the opposite of ENABLED
  kNoStateChange, // Don't fire state change event
  kNoSuchState
};

const AtkStateType kNone = ATK_STATE_INVALID;

struct AtkStateMap {
  AtkStateType atkState;
  EStateMapEntryType stateMapEntryType;

  static PRInt32 GetStateIndexFor(PRUint64 aState)
  {
    PRInt32 stateIndex = -1;
    while (aState > 0) {
      ++ stateIndex;
      aState >>= 1;
    }
    return stateIndex;  // Returns -1 if not mapped
  }
};


// Map array from cross platform roles to  ATK roles
static const AtkStateMap gAtkStateMap[] = {                     // Cross Platform States
  { kNone,                                    kMapOpposite },   // states::UNAVAILABLE             = 1 << 0
  { ATK_STATE_SELECTED,                       kMapDirectly },   // states::SELECTED                = 1 << 1
  { ATK_STATE_FOCUSED,                        kMapDirectly },   // states::FOCUSED                 = 1 << 2
  { ATK_STATE_PRESSED,                        kMapDirectly },   // states::PRESSED                 = 1 << 3
  { ATK_STATE_CHECKED,                        kMapDirectly },   // states::CHECKED                 = 1 << 4
  { ATK_STATE_INDETERMINATE,                  kMapDirectly },   // states::MIXED                   = 1 << 5
  { kNone,                                    kMapDirectly },   // states::READONLY                = 1 << 6
  { kNone,                                    kMapDirectly },   // states::HOTTRACKED              = 1 << 7
  { ATK_STATE_DEFAULT,                        kMapDirectly },   // states::DEFAULT                 = 1 << 8
  { ATK_STATE_EXPANDED,                       kMapDirectly },   // states::EXPANDED                = 1 << 9
  { kNone,                                    kNoStateChange }, // states::COLLAPSED               = 1 << 10
  { ATK_STATE_BUSY,                           kMapDirectly },   // states::BUSY                    = 1 << 11
  { kNone,                                    kMapDirectly },   // states::FLOATING                = 1 << 12
  { kNone,                                    kMapDirectly },   // states::CHECKABLE               = 1 << 13
  { ATK_STATE_ANIMATED,                       kMapDirectly },   // states::ANIMATED                = 1 << 14
  { ATK_STATE_VISIBLE,                        kMapOpposite },   // states::INVISIBLE               = 1 << 15
  { ATK_STATE_SHOWING,                        kMapOpposite },   // states::OFFSCREEN               = 1 << 16
  { ATK_STATE_RESIZABLE,                      kMapDirectly },   // states::SIZEABLE                = 1 << 17
  { kNone,                                    kMapDirectly },   // states::MOVEABLE                = 1 << 18
  { kNone,                                    kMapDirectly },   // states::SELFVOICING             = 1 << 19
  { ATK_STATE_FOCUSABLE,                      kMapDirectly },   // states::FOCUSABLE               = 1 << 20
  { ATK_STATE_SELECTABLE,                     kMapDirectly },   // states::SELECTABLE              = 1 << 21
  { kNone,                                    kMapDirectly },   // states::LINKED                  = 1 << 22
  { ATK_STATE_VISITED,                        kMapDirectly },   // states::TRAVERSED               = 1 << 23
  { ATK_STATE_MULTISELECTABLE,                kMapDirectly },   // states::MULTISELECTABLE         = 1 << 24
  { kNone,                                    kMapDirectly },   // states::EXTSELECTABLE           = 1 << 25
  { ATK_STATE_REQUIRED,                       kMapDirectly },   // states::STATE_REQUIRED          = 1 << 26
  { kNone,                                    kMapDirectly },   // states::ALERT_MEDIUM            = 1 << 27
  { ATK_STATE_INVALID_ENTRY,                  kMapDirectly },   // states::INVALID                 = 1 << 28
  { kNone,                                    kMapDirectly },   // states::PROTECTED               = 1 << 29
  { kNone,                                    kMapDirectly },   // states::HASPOPUP                = 1 << 30
  { ATK_STATE_SUPPORTS_AUTOCOMPLETION,        kMapDirectly },   // states::SUPPORTS_AUTOCOMPLETION = 1 << 31
  { ATK_STATE_DEFUNCT,                        kMapDirectly },   // states::DEFUNCT                 = 1 << 32
  { ATK_STATE_SELECTABLE_TEXT,                kMapDirectly },   // states::SELECTABLE_TEXT         = 1 << 33
  { ATK_STATE_EDITABLE,                       kMapDirectly },   // states::EDITABLE                = 1 << 34
  { ATK_STATE_ACTIVE,                         kMapDirectly },   // states::ACTIVE                  = 1 << 35
  { ATK_STATE_MODAL,                          kMapDirectly },   // states::MODAL                   = 1 << 36
  { ATK_STATE_MULTI_LINE,                     kMapDirectly },   // states::MULTI_LINE              = 1 << 37
  { ATK_STATE_HORIZONTAL,                     kMapDirectly },   // states::HORIZONTAL              = 1 << 38
  { ATK_STATE_OPAQUE,                         kMapDirectly },   // states::OPAQUE                  = 1 << 39
  { ATK_STATE_SINGLE_LINE,                    kMapDirectly },   // states::SINGLE_LINE             = 1 << 40
  { ATK_STATE_TRANSIENT,                      kMapDirectly },   // states::TRANSIENT               = 1 << 41
  { ATK_STATE_VERTICAL,                       kMapDirectly },   // states::VERTICAL                = 1 << 42
  { ATK_STATE_STALE,                          kMapDirectly },   // states::STALE                   = 1 << 43
  { ATK_STATE_ENABLED,                        kMapDirectly },   // states::ENABLED                 = 1 << 44
  { ATK_STATE_SENSITIVE,                      kMapDirectly },   // states::SENSITIVE               = 1 << 45
  { ATK_STATE_EXPANDABLE,                     kMapDirectly },   // states::EXPANDABLE              = 1 << 46
  { kNone,                                    kNoSuchState },   //                                 = 1 << 47
};
