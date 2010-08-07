/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
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
   File Name:          11.5.1.js
   ECMA Section:       11.5.1 Applying the * operator
   Description:

   11.5.1 Applying the * operator

   The * operator performs multiplication, producing the product of its
   operands. Multiplication is commutative. Multiplication is not always
   associative in ECMAScript, because of finite precision.

   The result of a floating-point multiplication is governed by the rules
   of IEEE 754 double-precision arithmetic:

   If either operand is NaN, the result is NaN.
   The sign of the result is positive if both operands have the same sign,
   negative if the operands have different signs.
   Multiplication of an infinity by a zero results in NaN.
   Multiplication of an infinity by an infinity results in an infinity.
   The sign is determined by the rule already stated above.
   Multiplication of an infinity by a finite non-zero value results in a
   signed infinity. The sign is determined by the rule already stated above.
   In the remaining cases, where neither an infinity or NaN is involved, the
   product is computed and rounded to the nearest representable value using IEEE
   754 round-to-nearest mode. If the magnitude is too large to represent,
   the result is then an infinity of appropriate sign. If the magnitude is
   oo small to represent, the result is then a zero
   of appropriate sign. The ECMAScript language requires support of gradual
   underflow as defined by IEEE 754.

   Author:             christine@netscape.com
   Date:               12 november 1997
*/
var SECTION = "11.5.1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Applying the * operator");

new TestCase( SECTION,    "Number.NaN * Number.NaN",    Number.NaN,     Number.NaN * Number.NaN );
new TestCase( SECTION,    "Number.NaN * 1",             Number.NaN,     Number.NaN * 1 );
new TestCase( SECTION,    "1 * Number.NaN",             Number.NaN,     1 * Number.NaN );

new TestCase( SECTION,    "Number.POSITIVE_INFINITY * 0",   Number.NaN, Number.POSITIVE_INFINITY * 0 );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * 0",   Number.NaN, Number.NEGATIVE_INFINITY * 0 );
new TestCase( SECTION,    "0 * Number.POSITIVE_INFINITY",   Number.NaN, 0 * Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "0 * Number.NEGATIVE_INFINITY",   Number.NaN, 0 * Number.NEGATIVE_INFINITY );

new TestCase( SECTION,    "-0 * Number.POSITIVE_INFINITY",  Number.NaN,   -0 * Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "-0 * Number.NEGATIVE_INFINITY",  Number.NaN,   -0 * Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY * -0",  Number.NaN,   Number.POSITIVE_INFINITY * -0 );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * -0",  Number.NaN,   Number.NEGATIVE_INFINITY * -0 );

new TestCase( SECTION,    "0 * -0",                         -0,         0 * -0 );
new TestCase( SECTION,    "-0 * 0",                         -0,         -0 * 0 );
new TestCase( SECTION,    "-0 * -0",                        0,          -0 * -0 );
new TestCase( SECTION,    "0 * 0",                          0,          0 * 0 );

new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * Number.NEGATIVE_INFINITY",    Number.POSITIVE_INFINITY,   Number.NEGATIVE_INFINITY * Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY * Number.NEGATIVE_INFINITY",    Number.NEGATIVE_INFINITY,   Number.POSITIVE_INFINITY * Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * Number.POSITIVE_INFINITY",    Number.NEGATIVE_INFINITY,   Number.NEGATIVE_INFINITY * Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY * Number.POSITIVE_INFINITY",    Number.POSITIVE_INFINITY,   Number.POSITIVE_INFINITY * Number.POSITIVE_INFINITY );

new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * 1 ",                          Number.NEGATIVE_INFINITY,   Number.NEGATIVE_INFINITY * 1 );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * -1 ",                         Number.POSITIVE_INFINITY,   Number.NEGATIVE_INFINITY * -1 );
new TestCase( SECTION,    "1 * Number.NEGATIVE_INFINITY",                           Number.NEGATIVE_INFINITY,   1 * Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "-1 * Number.NEGATIVE_INFINITY",                          Number.POSITIVE_INFINITY,   -1 * Number.NEGATIVE_INFINITY );

new TestCase( SECTION,    "Number.POSITIVE_INFINITY * 1 ",                          Number.POSITIVE_INFINITY,   Number.POSITIVE_INFINITY * 1 );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY * -1 ",                         Number.NEGATIVE_INFINITY,   Number.POSITIVE_INFINITY * -1 );
new TestCase( SECTION,    "1 * Number.POSITIVE_INFINITY",                           Number.POSITIVE_INFINITY,   1 * Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "-1 * Number.POSITIVE_INFINITY",                          Number.NEGATIVE_INFINITY,   -1 * Number.POSITIVE_INFINITY );

test();

