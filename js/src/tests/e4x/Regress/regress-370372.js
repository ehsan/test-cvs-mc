/* -*- Mode: java; tab-width:8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

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
 * The Original Code is JavaScript Engine testing utilities.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Igor Bukanov
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


var BUGNUMBER = 370372;
var summary = 'with (xmllist) function::name assignments';
var actual = 'No Exception';
var expect = 'No Exception';

printBugNumber(BUGNUMBER);
START(summary);

var tests = [{ v: <></>, expect: "" },
             { v: <><a/></>, expect: "" },
             { v: <><a/><b/></>, expect: "<a/>\n<b/>" }];

function do_one(x, expect)
{
    x.function::f = Math.sin;
    with (x) {
        function::toString = function::f = function() { return "test"; };
    }

    assertEq(String(x), expect, "didn't implement ToString(XMLList) correctly: " + x.toXMLString());

    if (x.f() !== "test")
        throw "Failed to set set function f";
}


for (var i  = 0; i != tests.length; ++i)
    do_one(tests[i].v, tests[i].expect);

TEST(1, expect, actual);
END();
