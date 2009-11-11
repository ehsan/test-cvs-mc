/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/licenses/publicdomain/
 * Contributor:
 *   Jason Orendorff
 *   Jeff Walden <jwalden+code@mit.edu>
 */

var gTestfile = "template.js";
//-----------------------------------------------------------------------------
var BUGNUMBER = 523846;
var summary =
  "Assignments to a property that has a getter but not a setter should not " +
  "throw a TypeError per ES5 (at least not until strict mode is supported)";
var actual = "Early failure";
var expect = "No errors";


printBugNumber(BUGNUMBER);
printStatus(summary);

var o = { get p() { return "a"; } };

function test1()
{
  o.p = "b";
  assertEq(o.p, "a");
}

function test2()
{
  function T() {}
  T.prototype = o;
  y = new T();
  y.p = "b";
  assertEq(y.p, "a");
}

function strictTest1()
{
  "use strict";

  o.p = "b"; // strict-mode violation here
  assertEq(o.p, "a");
}

function strictTest2()
{
  "use strict";

  function T() {}
  T.prototype = o;
  y = new T;
  y.p = "b";  // strict-mode violation here
  assertEq(y.p, "a");
}

// Feel free to tweak this as necessary to preserve the effectiveness of not
// running the strict-mode correctness tests, as long as we don't support
// strict mode and as long as we haven't updated our get/set code to throw when
// setting a property that only has a getter in strict mode.
var strictModeSupported = (function() { "use strict"; return !this; })();

var errors = [];
try
{
  try
  {
    test1();
  }
  catch (e)
  {
    errors.push(e);
  }

  try
  {
    test2();
  }
  catch (e)
  {
    errors.push(e);
  }

  if (strictModeSupported)
  {
    try
    {
      strictTest1();
      errors.push("strictTest1 didn't fail");
    }
    catch (e)
    {
      if (!(e instanceof TypeError))
        errors.push("strictTest1 didn't fail with a TypeError: " + e);
    }

    try
    {
      strictTest2();
      errors.push("strictTest2 didn't fail");
    }
    catch (e)
    {
      if (!(e instanceof TypeError))
        errors.push("strictTest2 didn't fail with a TypeError: " + e);
    }
  }
}
catch (e)
{
  errors.push("Unexpected error: " + e);
}
finally
{
  actual = errors.length > 0 ? errors.join(", ") : "No errors";
}

reportCompare(expect, actual, summary);
