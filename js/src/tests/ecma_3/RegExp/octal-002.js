/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 *
 * Date:    31 July 2002
 * SUMMARY: Testing regexps containing octal escape sequences
 * This is an elaboration of mozilla/js/tests/ecma_2/RegExp/octal-003.js
 *
 * See http://bugzilla.mozilla.org/show_bug.cgi?id=141078
 * for a reference on octal escape sequences in regexps.
 *
 * NOTE:
 * We will use the identities '\011' === '\u0009' === '\x09' === '\t'
 *
 * The first is an octal escape sequence (\(0-3)OO; O an octal digit).
 * See ECMA-262 Edition 2, Section 7.7.4 "String Literals". These were
 * dropped in Edition 3 but we support them for backward compatibility.
 *
 * The second is a Unicode escape sequence (\uHHHH; H a hex digit).
 * Since octal 11 = hex 9, the two escapes define the same character.
 *
 * The third is a hex escape sequence (\xHH; H a hex digit).
 * Since hex 09 = hex 0009, this defines the same character.
 *
 * The fourth is the familiar escape sequence for a horizontal tab,
 * defined in the ECMA spec as having Unicode value \u0009.
 */
//-----------------------------------------------------------------------------
var i = 0;
var BUGNUMBER = 141078;
var summary = 'Testing regexps containing octal escape sequences';
var status = '';
var statusmessages = new Array();
var pattern = '';
var patterns = new Array();
var string = '';
var strings = new Array();
var actualmatch = '';
var actualmatches = new Array();
var expectedmatch = '';
var expectedmatches = new Array();


/*
 * Test a string containing the null character '\0' followed by the string '11'
 *
 *               'a' + String.fromCharCode(0) + '11';
 *
 * Note we can't simply write 'a\011', because '\011' would be interpreted
 * as the octal escape sequence for the tab character (see above).
 *
 * We should get no match from the regexp /.\011/, because it should be
 * looking for the octal escape sequence \011, i.e. the tab character -
 *
 */
status = inSection(1);
pattern = /.\011/;
string = 'a' + String.fromCharCode(0) + '11';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();


/*
 * Try same thing with 'xx' in place of '11'.
 *
 * Should get a match now, because the octal escape sequence in the regexp
 * has been reduced from \011 to \0, and '\0' is present in the string -
 */
status = inSection(2);
pattern = /.\0xx/;
string = 'a' + String.fromCharCode(0) + 'xx';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();


/*
 * Same thing; don't use |String.fromCharCode(0)| this time.
 * There is no ambiguity in '\0xx': it is the null character
 * followed by two x's, no other interpretation is possible.
 */
status = inSection(3);
pattern = /.\0xx/;
string = 'a\0xx';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();


/*
 * This one should produce a match. The two-character string
 * 'a' + '\011' is duplicated in the pattern and test string:
 */
status = inSection(4);
pattern = /.\011/;
string = 'a\011';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();


/*
 * Same as above, only now, for the second character of the string,
 * use the Unicode escape '\u0009' instead of the octal escape '\011'
 */
status = inSection(5);
pattern = /.\011/;
string = 'a\u0009';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();


/*
 * Same as above, only now  for the second character of the string,
 * use the hex escape '\x09' instead of the octal escape '\011'
 */
status = inSection(6);
pattern = /.\011/;
string = 'a\x09';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();


/*
 * Same as above, only now  for the second character of the string,
 * use the escape '\t' instead of the octal escape '\011'
 */
status = inSection(7);
pattern = /.\011/;
string = 'a\t';
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();


/*
 * Return to the string from Section 1.
 *
 * Unlike Section 1, use the RegExp() function to create the
 * regexp pattern: null character followed by the string '11'.
 *
 * Since this is exactly what the string is, we should get a match -
 */
status = inSection(8);
string = 'a' + String.fromCharCode(0) + '11';
pattern = RegExp(string);
actualmatch = string.match(pattern);
expectedmatch = Array(string);
addThis();




//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------



function addThis()
{
  statusmessages[i] = status;
  patterns[i] = pattern;
  strings[i] = string;
  actualmatches[i] = actualmatch;
  expectedmatches[i] = expectedmatch;
  i++;
}


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
  testRegExp(statusmessages, patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
