/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=2 sts=2 expandtab
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Shawn Wilsher <me@shawnwilsher.com> (Original Author)
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

Components.utils.import("resource://gre/modules/PlacesBackground.jsm");

const TEST_URI = "http://test.com/";

function run_test()
{
  // Now add the visit in batch mode
  let bh = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
  let id = null;
  bh.runInBatchMode({
    runBatched: function(aUserData)
    {
      let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
               getService(Ci.nsINavHistoryService);

      id = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                       hs.TRANSITION_TYPED, false, 0);
      PlacesBackground.dispatch(new_test_visit_uri_event(id, TEST_URI, false),
                                Ci.nsIEventTarget.DISPATCH_SYNC);
    }
  }, null);
  do_check_neq(id, null);
  PlacesBackground.dispatch(new_test_visit_uri_event(id, TEST_URI, true, true),
                            Ci.nsIEventTarget.DISPATCH_NORMAL);
  do_test_pending();
}
