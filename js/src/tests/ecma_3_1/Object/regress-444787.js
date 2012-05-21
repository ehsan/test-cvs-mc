/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

//-----------------------------------------------------------------------------
var BUGNUMBER = 444787;
var summary = 'Object.getPrototypeOf';
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
 
  var i;
  var type;
  var instance;
  var types = [
    Array,
    Boolean,
    Date,
    Error,
    Function,
    Math,
    Number,
    Object,
    RegExp,
    String,
    ];

  for (i = 0; i < types.length; i++)
  {
    type = types[i];

    if (typeof type.__proto__ != 'undefined')
    {
      expect = type.__proto__;
      actual = Object.getPrototypeOf(type);
      reportCompare(expect, actual, summary + ': ' + type.name);
    }

    try
    {
      eval('instance = new ' + type.name);
      expect = type.prototype;
      actual = Object.getPrototypeOf(instance);
      reportCompare(expect, actual, summary + ': new ' + type.name);
    }
    catch(ex if ex instanceof TypeError)
    {
      print('Ignore ' + ex);
    }
    catch(ex)
    {
      actual = ex + '';
      reportCompare(expect, actual, summary + ': new ' + type.name);
    }

  }

  types = [null, undefined];

  for (i = 0; i < types.length; i++)
  {
    type = types[i];
    expect = 'TypeError: Object.getPrototype is not a function';
    try
    {
      actual = Object.getPrototype(null);
    }
    catch(ex)
    {
      actual = ex + '';
    }
    reportCompare(expect, actual, summary + ': ' + type);
  }

  var objects = [
    {instance: [0], type: Array},
    {instance: (function () {}), type: Function},
    {instance: eval, type: Function},
    {instance: parseInt, type: Function},
    {instance: {a: ''}, type: Object},
    {instance: /foo/, type: RegExp}
    ];

  for (i = 0; i < objects.length; i++)
  {
    instance = objects[i].instance;
    type     = objects[i].type;
    expect   = type.prototype;
    actual   = Object.getPrototypeOf(instance);
    reportCompare(expect, actual, summary + ' instance: ' + instance + ', type: ' + type.name);
  }

  var non_objects = [ true, false, 1.0, Infinity, NaN, Math.PI, "bar" ];

  for (i = 0; i < non_objects.length; i++)
  {
    instance = non_objects[i];
    expect = 'TypeError: instance is not an object';
    try
    {
      actual = Object.getPrototypeOf(instance);
    }
    catch(ex)
    {
      actual = ex + '';
    }
    reportCompare(expect, actual, summary + ' non-object: ' + actual);
  }

  exitFunc ('test');
}
