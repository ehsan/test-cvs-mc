/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

//-----------------------------------------------------------------------------
var BUGNUMBER = 355339;
var summary = 'Do not assert: sprop->setter != js_watch_set';
var actual = '';
var expect = '';


//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------

function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = actual = 'No Crash';
  o = {};
  o.watch("j", function(a,b,c) { print("*",a,b,c) });
  o.unwatch("j");
  o.watch("j", function(a,b,c) { print("*",a,b,c) });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
