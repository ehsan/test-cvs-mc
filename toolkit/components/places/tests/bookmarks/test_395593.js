/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et: */
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
 * The Original Code is bug 395593 unit test code.
 *
 * The Initial Developer of the Original Code is Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Asaf Romano <mano@mozilla.com> (Original Author)
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


var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);

function check_queries_results(aQueries, aOptions, aExpectedItemIds) {
  var result = hs.executeQueries(aQueries, aQueries.length, aOptions);
  var root = result.root;
  root.containerOpen = true;

  // Dump found nodes.
  for (let i = 0; i < root.childCount; i++) {
    dump("nodes[" + i + "]: " + root.getChild(0).title + "\n");
  }

  do_check_eq(root.childCount, aExpectedItemIds.length);
  for (let i = 0; i < root.childCount; i++) {
    do_check_eq(root.getChild(i).itemId, aExpectedItemIds[i]);
  }

  root.containerOpen = false;
}

// main
function run_test() {
  var id1 = bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://foo.tld"),
                              bs.DEFAULT_INDEX, "123 0");
  var id2 = bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://foo.tld"),
                              bs.DEFAULT_INDEX, "456");
  var id3 = bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://foo.tld"),
                              bs.DEFAULT_INDEX, "123 456");
  var id4 = bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://foo.tld"),
                              bs.DEFAULT_INDEX, "789 456");

  /**
   * All of the query objects are ORed together. Within a query, all the terms
   * are ANDed together. See nsINavHistory.idl.
   */
  var queries = [];
  queries.push(hs.getNewQuery());
  queries.push(hs.getNewQuery());
  var options = hs.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;

  // Test 1
  dump("Test searching for 123 OR 789\n");
  queries[0].searchTerms = "123";
  queries[1].searchTerms = "789";
  check_queries_results(queries, options, [id1, id3, id4]);

  // Test 2
  dump("Test searching for 123 OR 456\n");
  queries[0].searchTerms = "123";
  queries[1].searchTerms = "456";
  check_queries_results(queries, options, [id1, id2, id3, id4]);

  // Test 3
  dump("Test searching for 00 OR 789\n");
  queries[0].searchTerms = "00";
  queries[1].searchTerms = "789";
  check_queries_results(queries, options, [id4]);
}
