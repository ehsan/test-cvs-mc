/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:expandtab:shiftwidth=2:tabstop=2:
 */
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
 *   Aaron Leventhal <aleventh@us.ibm.com>
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

#include "nsARIAMap.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleStates.h"

/**
 *  This list of WAI-defined roles are currently hardcoded.
 *  Eventually we will most likely be loading an RDF resource that contains this information
 *  Using RDF will also allow for role extensibility. See bug 280138.
 *
 *  Definition of nsRoleMapEntry and nsStateMapEntry contains comments explaining this table.
 *
 *  When no nsIAccessibleRole enum mapping exists for an ARIA role, the
 *  role will be exposed via the object attribute "xml-roles".
 *  In addition, in MSAA, the unmapped role will also be exposed as a BSTR string role.
 *
 *  There are no nsIAccessibleRole enums for the following landmark roles:
 *    banner, contentinfo, main, navigation, note, search, secondary, seealso, breadcrumbs
 */ 

static const nsStateMapEntry kEndEntry = {nsnull, 0, 0};  // To fill in array of state mappings

nsRoleMapEntry nsARIAMap::gWAIRoleMap[] = 
{
  {
    "alert",
    nsIAccessibleRole::ROLE_ALERT,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "alertdialog",
    nsIAccessibleRole::ROLE_DIALOG,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "application",
    nsIAccessibleRole::ROLE_APPLICATION,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "article",
    nsIAccessibleRole::ROLE_DOCUMENT,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "button",
    nsIAccessibleRole::ROLE_PUSHBUTTON,
    eNoValue,
    eClickAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_pressed, kBoolState, nsIAccessibleStates::STATE_PRESSED},
    {&nsAccessibilityAtoms::aria_pressed, "mixed", nsIAccessibleStates::STATE_MIXED},
    kEndEntry
  },
  {
    "checkbox",
    nsIAccessibleRole::ROLE_CHECKBUTTON,
    eNoValue,
    eCheckUncheckAction,
    nsIAccessibleStates::STATE_CHECKABLE,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "columnheader",
    nsIAccessibleRole::ROLE_COLUMNHEADER,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "combobox",
    nsIAccessibleRole::ROLE_COMBOBOX,
    eHasValueMinMax,
    eOpenCloseAction,
    nsIAccessibleStates::STATE_COLLAPSED | nsIAccessibleStates::STATE_HASPOPUP,
    // Manually map EXT_STATE_SUPPORTS_AUTOCOMPLETION aria-autocomplete
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "dialog",
    nsIAccessibleRole::ROLE_DIALOG,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "document",
    nsIAccessibleRole::ROLE_DOCUMENT,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "grid",
    nsIAccessibleRole::ROLE_TABLE,
    eNoValue,
    eNoAction,
    nsIAccessibleStates::STATE_FOCUSABLE,
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "gridcell",
    nsIAccessibleRole::ROLE_GRID_CELL,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "group",
    nsIAccessibleRole::ROLE_GROUPING,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "heading",
    nsIAccessibleRole::ROLE_HEADING,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "img",
    nsIAccessibleRole::ROLE_GRAPHIC,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "label",
    nsIAccessibleRole::ROLE_LABEL,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "link",
    nsIAccessibleRole::ROLE_LINK,
    eNoValue,
    eJumpAction,
    nsIAccessibleStates::STATE_LINKED,
    kEndEntry
  },
  {
    "list",
    nsIAccessibleRole::ROLE_LIST,
    eNoValue,
    eNoAction,
    nsIAccessibleStates::STATE_READONLY,
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "listbox",
    nsIAccessibleRole::ROLE_LISTBOX,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "listitem",
    nsIAccessibleRole::ROLE_LISTITEM,
    eNoValue,
    eNoAction, // XXX: should depend on state, parent accessible
    nsIAccessibleStates::STATE_READONLY,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
    kEndEntry
  },
  {
    "math",
    nsIAccessibleRole::ROLE_FLAT_EQUATION,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "menu",
    nsIAccessibleRole::ROLE_MENUPOPUP,
    eNoValue,
    eNoAction, // XXX: technically accessibles of menupopup role haven't
               // any action, but menu can be open or close.
    kNoReqStates,
    kEndEntry
  },
  {
    "menubar",
    nsIAccessibleRole::ROLE_MENUBAR,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "menuitem",
    nsIAccessibleRole::ROLE_MENUITEM,
    eNoValue,
    eClickAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
    kEndEntry
  },
  {
    "menuitemcheckbox",
    nsIAccessibleRole::ROLE_CHECK_MENU_ITEM,
    eNoValue,
    eClickAction,
    nsIAccessibleStates::STATE_CHECKABLE,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED },
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED},
    kEndEntry
  },
  {
    "menuitemradio",
    nsIAccessibleRole::ROLE_RADIO_MENU_ITEM,
    eNoValue,
    eClickAction,
    nsIAccessibleStates::STATE_CHECKABLE,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED },
    kEndEntry
  },
  {
    "option",
    nsIAccessibleRole::ROLE_OPTION,
    eNoValue,
    eSelectAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
    kEndEntry
  },
  {
    "presentation",
    nsIAccessibleRole::ROLE_NOTHING,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "progressbar",
    nsIAccessibleRole::ROLE_PROGRESSBAR,
    eHasValueMinMax,
    eNoAction,
    nsIAccessibleStates::STATE_READONLY,
    kEndEntry
  },
  {
    "radio",
    nsIAccessibleRole::ROLE_RADIOBUTTON,
    eNoValue,
    eSelectAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED},
    kEndEntry
  },
  {
    "radiogroup",
    nsIAccessibleRole::ROLE_GROUPING,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "region",
    nsIAccessibleRole::ROLE_PANE,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "row",
    nsIAccessibleRole::ROLE_ROW,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    kEndEntry
  },
  {
    "rowheader",
    nsIAccessibleRole::ROLE_ROWHEADER,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "section",
    nsIAccessibleRole::ROLE_SECTION,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "separator",
    nsIAccessibleRole::ROLE_SEPARATOR,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "slider",
    nsIAccessibleRole::ROLE_SLIDER,
    eHasValueMinMax,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "spinbutton",
    nsIAccessibleRole::ROLE_SPINBUTTON,
    eHasValueMinMax,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "status",
    nsIAccessibleRole::ROLE_STATUSBAR,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "tab",
    nsIAccessibleRole::ROLE_PAGETAB,
    eNoValue,
    eSwitchAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "tablist",
    nsIAccessibleRole::ROLE_PAGETABLIST,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "tabpanel",
    nsIAccessibleRole::ROLE_PROPERTYPAGE,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "textbox",
    nsIAccessibleRole::ROLE_ENTRY,
    eNoValue,
    eActivateAction,
    kNoReqStates,
    // Manually map EXT_STATE_SINGLE_LINE and EXT_STATE_MULTI_LINE FROM aria-multiline
    // Manually map EXT_STATE_SUPPORTS_AUTOCOMPLETION aria-autocomplete
    {&nsAccessibilityAtoms::aria_autocomplete, "list", nsIAccessibleStates::STATE_HASPOPUP},
    {&nsAccessibilityAtoms::aria_autocomplete, "both", nsIAccessibleStates::STATE_HASPOPUP},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "toolbar",
    nsIAccessibleRole::ROLE_TOOLBAR,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "tooltip",
    nsIAccessibleRole::ROLE_TOOLTIP,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "tree",
    nsIAccessibleRole::ROLE_OUTLINE,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "treegrid",
    nsIAccessibleRole::ROLE_TREE_TABLE,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "treeitem",
    nsIAccessibleRole::ROLE_OUTLINEITEM,
    eNoValue,
    eActivateAction, // XXX: should expose second 'expand/collapse' action based
                     // on states
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
  },
};

PRUint32 nsARIAMap::gWAIRoleMapLength = NS_ARRAY_LENGTH(nsARIAMap::gWAIRoleMap);

nsRoleMapEntry nsARIAMap::gLandmarkRoleMap = {
  "",
  nsIAccessibleRole::ROLE_NOTHING,
  eNoValue,
  eNoAction,
  kNoReqStates,
  kEndEntry
};

nsRoleMapEntry nsARIAMap::gEmptyRoleMap = {
  "",
  nsIAccessibleRole::ROLE_NOTHING,
  eNoValue,
  eNoAction,
  kNoReqStates,
  kEndEntry
};

/**
 * Universal states:
 * The following state rules are applied to any accessible element,
 * whether there is an ARIA role or not:
 */
nsStateMapEntry nsARIAMap::gWAIUnivStateMap[] = {
  {&nsAccessibilityAtoms::aria_required, kBoolState, nsIAccessibleStates::STATE_REQUIRED},
  {&nsAccessibilityAtoms::aria_invalid,  kBoolState, nsIAccessibleStates::STATE_INVALID},
  {&nsAccessibilityAtoms::aria_haspopup, kBoolState, nsIAccessibleStates::STATE_HASPOPUP},
  {&nsAccessibilityAtoms::aria_busy,     "true",     nsIAccessibleStates::STATE_BUSY},
  {&nsAccessibilityAtoms::aria_busy,     "error",    nsIAccessibleStates::STATE_INVALID},
  {&nsAccessibilityAtoms::aria_disabled, kBoolState, nsIAccessibleStates::STATE_UNAVAILABLE},
  {&nsAccessibilityAtoms::aria_expanded, kBoolState, nsIAccessibleStates::STATE_EXPANDED},
  {&nsAccessibilityAtoms::aria_expanded, "false", nsIAccessibleStates::STATE_COLLAPSED},
  kEndEntry
};

