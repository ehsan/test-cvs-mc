/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sw=2 sts=2 et: */
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
 * The Original Code is Mozilla SMIL Test Code.
 *
 * The Initial Developer of the Original Code is the Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Daniel Holbert <dholbert@mozilla.com> (original author)
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

/* testcase data for simple "from-by" animations of CSS properties */

// NOTE: This js file requires db_smilCSSPropertyList.js

// Lists of testcases for re-use across multiple properties of the same type
var _fromByTestLists =
{
  color: [
    new AnimTestcaseFromBy("rgb(10, 20, 30)", "currentColor",
                           { midComp: "rgb(35, 45, 55)",
                             toComp:  "rgb(60, 70, 80)"}),
    new AnimTestcaseFromBy("currentColor", "rgb(30, 20, 10)",
                           { fromComp: "rgb(50, 50, 50)",
                             midComp:  "rgb(65, 60, 55)",
                             toComp:   "rgb(80, 70, 60)"}),
  ],
  lengthPx: [
    new AnimTestcaseFromBy("1px", "10px", { midComp: "6px", toComp: "11px"}),
  ],
  opacity: [
    new AnimTestcaseFromBy("1", "-1", { midComp: "0.5", toComp: "0"}),
    new AnimTestcaseFromBy("0.4", "-0.6", { midComp: "0.1", toComp: "0"}),
    new AnimTestcaseFromBy("0.8", "-1.4", { midComp: "0.1", toComp: "0"},
                           "opacities with abs val >1 get clamped too early"),
    new AnimTestcaseFromBy("1.2", "-0.6", { midComp: "0.9", toComp: "0.6"},
                           "opacities with abs val >1 get clamped too early"),
  ],
};

// List of attribute/testcase-list bundles to be tested
var gFromByBundles =
[
  new TestcaseBundle(gPropList.fill,           _fromByTestLists.color),
  new TestcaseBundle(gPropList.font_size,      _fromByTestLists.lengthPx),
  new TestcaseBundle(gPropList.lighting_color, _fromByTestLists.color),
  new TestcaseBundle(gPropList.opacity,        _fromByTestLists.opacity),
  new TestcaseBundle(gPropList.stroke_miterlimit, [
    new AnimTestcaseFromBy("1", "1", { midComp: "1.5", toComp: "2" }),
    new AnimTestcaseFromBy("20.1", "-10", { midComp: "15.1", toComp: "10.1" }),
  ]),
  new TestcaseBundle(gPropList.stroke_width,   _fromByTestLists.lengthPx),
];
