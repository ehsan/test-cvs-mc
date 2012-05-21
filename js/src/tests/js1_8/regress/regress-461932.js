/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

//-----------------------------------------------------------------------------
var BUGNUMBER = 461932;
var summary = 'TM: Do not crash in nanojit::LIns::isop';
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

  jit(true);

  function gen() { for (var j = 0; j < 4; ++j) { NaN; yield 3; } }
  for (let i in gen()) { }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
