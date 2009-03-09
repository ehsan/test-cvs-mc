/* ***** BEGIN LICENSE BLOCK *****
 *   Version: MPL 1.1/GPL 2.0/LGPL 2.1
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
 * The Original Code is Plural Form l10n Test Code.
 *
 * The Initial Developer of the Original Code is
 * Edward Lee <edward.lee@engineering.uiuc.edu>.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

/**
 * Make sure each of the plural forms have the correct number of forms and
 * match up in functionality.
 */

Components.utils.import("resource://gre/modules/PluralForm.jsm");

function run_test()
{
  let allExpect = [[
    // 0: Chinese 0-9, 10-19, ..., 90-99
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    // 100-109, 110-119, ..., 190-199
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    // 200-209, 210-219, ..., 290-299
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
  ], [
    // 1: English 0-9, 10-19, ..., 90-99
    2,1,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    // 100-109, 110-119, ..., 190-199
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    // 200-209, 210-219, ..., 290-299
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
  ], [
    // 2: French 0-9, 10-19, ..., 90-99
    1,1,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    // 100-109, 110-119, ..., 190-199
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    // 200-209, 210-219, ..., 290-299
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
  ], [
    // 3: Latvian 0-9, 10-19, ..., 90-99
    1,2,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    // 100-109, 110-119, ..., 190-199
    3,2,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    // 200-209, 210-219, ..., 290-299
    3,2,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
    3,2,3,3,3,3,3,3,3,3,
  ], [
    // 4: Scottish Gaelic 0-9, 10-19, ..., 90-99
    3,1,2,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    // 100-109, 110-119, ..., 190-199
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    // 200-209, 210-219, ..., 290-299
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
  ], [
    // 5: Romanian 0-9, 10-19, ..., 90-99
    2,1,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    // 100-109, 110-119, ..., 190-199
    3,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    // 200-209, 210-219, ..., 290-299
    3,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
  ], [
    // 6: Lithuanian 0-9, 10-19, ..., 90-99
    2,1,3,3,3,3,3,3,3,3,
    2,2,2,2,2,2,2,2,2,2,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    // 100-109, 110-119, ..., 190-199
    2,1,3,3,3,3,3,3,3,3,
    2,2,2,2,2,2,2,2,2,2,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    // 200-209, 210-219, ..., 290-299
    2,1,3,3,3,3,3,3,3,3,
    2,2,2,2,2,2,2,2,2,2,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
    2,1,3,3,3,3,3,3,3,3,
  ], [
    // 7: Russian 0-9, 10-19, ..., 90-99
    3,1,2,2,2,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    // 100-109, 110-119, ..., 190-199
    3,1,2,2,2,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    // 200-209, 210-219, ..., 290-299
    3,1,2,2,2,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
    3,1,2,2,2,3,3,3,3,3,
  ], [
    // 8: Slovak 0-9, 10-19, ..., 90-99
    3,1,2,2,2,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    // 100-109, 110-119, ..., 190-199
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    // 200-209, 210-219, ..., 290-299
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
  ], [
    // 9: Polish 0-9, 10-19, ..., 90-99
    3,1,2,2,2,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    // 100-109, 110-119, ..., 190-199
    3,3,2,2,2,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    // 200-209, 210-219, ..., 290-299
    3,3,2,2,2,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
    3,3,2,2,2,3,3,3,3,3,
  ], [
    // 10: Slovenian 0-9, 10-19, ..., 90-99
    4,1,2,3,3,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    // 100-109, 110-119, ..., 190-199
    4,1,2,3,3,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    // 200-209, 210-219, ..., 290-299
    4,1,2,3,3,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
  ], [
    // 11: Irish Gaeilge 0-9, 10-19, ..., 90-99
    5,1,2,3,3,3,3,4,4,4,
    4,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    // 100-109, 110-119, ..., 190-199
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    // 200-209, 210-219, ..., 290-299
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,
  ], [
    // 12: Arabic 0-9, 10-19, ..., 90-99
    6,1,2,3,3,3,3,3,3,3,
    3,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    // 100-109, 110-119, ..., 190-199
    5,5,5,3,3,3,3,3,3,3,
    3,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    // 200-209, 210-219, ..., 290-299
    5,5,5,3,3,3,3,3,3,3,
    3,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
  ], [
    // 13: Maltese 0-9, 10-19, ..., 90-99
    2,1,2,2,2,2,2,2,2,2,
    2,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    // 100-109, 110-119, ..., 190-199
    4,2,2,2,2,2,2,2,2,2,
    2,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    // 200-209, 210-219, ..., 290-299
    4,2,2,2,2,2,2,2,2,2,
    2,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,
  ], [
    // 14: Macedonian 0-9, 10-19, ..., 90-99
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    // 100-109, 110-119, ..., 190-199
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    // 200-209, 210-219, ..., 290-299
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
    3,1,2,3,3,3,3,3,3,3,
  ]];

  for (let [rule, expect] in Iterator(allExpect)) {
    print("\nTesting rule #" + rule);

    let [get, numForms] = PluralForm.makeGetter(rule);

    // Make sure the largest value expected matches the number of plural forms
    let maxExpect = Math.max.apply(this, expect);
    do_check_eq(maxExpect, numForms());

    // Make a string of numbers, e.g., 1;2;3;4;5
    let words = [];
    for (let i = 1; i <= maxExpect; i++)
      words.push(i);
    words = words.join(";");

    // Make sure we get the expected number
    for (let [index, number] in Iterator(expect)) {
      print(["Plural form of ", index, " should be ", number, " (", words, ")"].join(""));
      do_check_eq(get(index, words), number);
    }
  }
}
