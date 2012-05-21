/* -*- Mode: java; tab-width:8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


var summary = 'Do not assert: JSVAL_IS_STRING(v)';
var BUGNUMBER = 477053;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

try
{
    function f() { eval("with(arguments)throw <x/>;"); }
    f();
}
catch(ex)
{
}

TEST(1, expect, actual);

END();
