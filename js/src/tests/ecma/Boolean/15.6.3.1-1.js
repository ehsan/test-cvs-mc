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
   File Name:          15.6.3.1-1.js
   ECMA Section:       15.6.3 Boolean.prototype

   Description:        The initial value of Boolean.prototype is the built-in
   Boolean prototype object (15.6.4).

   The property shall have the attributes [DontEnum,
   DontDelete, ReadOnly ].

   This tests the DontEnum property of Boolean.prototype

   Author:             christine@netscape.com
   Date:               june 27, 1997

*/
var SECTION = "15.6.3.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);


var array = new Array();
var item = 0;

new TestCase( SECTION,
	      "var str='';for ( p in Boolean ) { str += p } str;",
	      "",
	      eval("var str='';for ( p in Boolean ) { str += p } str;") );
test();
