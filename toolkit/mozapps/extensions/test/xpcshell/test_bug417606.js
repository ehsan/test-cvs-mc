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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 *      Dave Townsend <dtownsend@oxymoronical.com>.
 *
 * Portions created by the Initial Developer are Copyright (C) 2007
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
 * ***** END LICENSE BLOCK *****
 */

Components.utils.import("resource://gre/modules/AddonRepository.jsm");

const PREF_GETADDONS_BROWSEADDONS        = "extensions.getAddons.browseAddons";
const PREF_GETADDONS_BROWSERECOMMENDED   = "extensions.getAddons.recommended.browseURL";
const PREF_GETADDONS_GETRECOMMENDED      = "extensions.getAddons.recommended.url";
const PREF_GETADDONS_BROWSESEARCHRESULTS = "extensions.getAddons.search.browseURL";
const PREF_GETADDONS_GETSEARCHRESULTS    = "extensions.getAddons.search.url";

const BROWSE      = "http://localhost:4444/";
const RECOMMENDED = "http://localhost:4444/recommended.html";
const SEARCH      = "http://localhost:4444/search/";

var BROWSE_SEARCH_URLS = [
  ["test",                              SEARCH + "test" ],
  ["SEARCH",                            SEARCH + "SEARCH" ],
  ["test search",                       SEARCH + "test%20search" ],
  ["odd=search:with&weird\"characters", SEARCH + "odd%3Dsearch%3Awith%26weird%22characters" ]
];

do_load_httpd_js();
var server;

var RESULTS = [
{
  id:               "test5@tests.mozilla.org",
  name:             "PASS",
  version:          "1.0",
  description:      "This should work fine",
  fullDescription:  "Test description",
  rating:           -1,
  iconURL:          null,
  screenshots:      [],
  homepageURL:      "https://addons.mozilla.org/addon/5992",
  type:             "extension",
  sourceURI:        "http://localhost:4444/test.xpi",
  size:             null
},
{
  id:               "test6@tests.mozilla.org",
  name:             "PASS",
  version:          "1.0",
  description:      "Specific OS should work fine",
  fullDescription:  null,
  rating:           4,
  iconURL:          "http://localhost:4444/test_bug404024/icon.png",
  screenshots:      ["http://localhost:4444/test_bug404024/thumbnail.png"],
  homepageURL:      null,
  type:             "theme",
  sourceURI:        "http://localhost:4444/XPCShell.xpi",
  size:             456,
}
];

function checkResults(addons) {
  do_check_eq(addons.length, RESULTS.length);
  
  for (var i = 0; i < addons.length; i++) {
    if (addons[i].name == "FAIL")
      do_throw(addons[i].id + " - " + addons[i].summary);
    if (addons[i].name != "PASS")
      do_throw(addons[i].id + " - " + "invalid add-on name " + addons[i].name);
  }

  for (var i = 0; i < addons.length; i++) {
    do_check_neq(addons[i]["install"], null);

    for (var p in RESULTS[i]) {
      if (p == "screenshots") {
        do_check_eq(addons[i][p].length, RESULTS[i][p].length);
        for (var j = 0; j < addons[i][p].length; j++)
          do_check_eq(addons[i][p][j], addons[i][p][j]);

        continue;
      }

      if (p == "sourceURI") {
        do_check_eq(addons[i]["install"][p].spec, RESULTS[i][p]);
        continue;
      }

      if (addons[i][p] != RESULTS[i][p])
        do_throw("Failed on property " + p + " on add-on " + addons[i].id +
                 addons[i][p] + " == " + RESULTS[i][p]);
    }
  }
}

var RecommendedCallback = {
  searchSucceeded: function(addons, length, total) {
    // Search is complete
    do_check_false(AddonRepository.isSearching);
    checkResults(addons);

    // "search" for the same results
    AddonRepository.searchAddons("bug417606", 10, SearchCallback);
    // Should be searching now and any attempt to retrieve again should be ignored
    do_check_true(AddonRepository.isSearching);
    AddonRepository.searchAddons("test search", 10, FailCallback);
  },

  searchFailed: function() {
    server.stop(do_test_finished);
    do_throw("Recommended results failed");
  }
};

var SearchCallback = {
  searchSucceeded: function(addons, length, total) {
    do_check_false(AddonRepository.isSearching);
    do_check_eq(total, 100);
    checkResults(addons);

    server.stop(do_test_finished);
  },

  searchFailed: function() {
    server.stop(do_test_finished);
    do_throw("Search results failed");
  }
};

var FailCallback = {
  searchSucceeded: function(addons, length, total) {
    server.stop(do_test_finished);
    do_throw("Should not be called");
  },

  searchFailed: function() {
    server.stop(do_test_finished);
    do_throw("Should not be called");
  }
};

function run_test()
{
  // Setup for test
  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  // Install an add-on so we can check the same add-on isn't returns in the results
  startupManager();
  installAllFiles([do_get_addon("test_bug397778")], function() {
    restartManager();
  
    server = new nsHttpServer();
    server.registerDirectory("/", do_get_file("data"));
    server.start(4444);

    // Point the addons repository to the test server
    Services.prefs.setCharPref(PREF_GETADDONS_BROWSEADDONS, BROWSE);
    Services.prefs.setCharPref(PREF_GETADDONS_BROWSERECOMMENDED, RECOMMENDED);
    Services.prefs.setCharPref(PREF_GETADDONS_GETRECOMMENDED, "http://localhost:4444/test_bug417606.xml");
    Services.prefs.setCharPref(PREF_GETADDONS_BROWSESEARCHRESULTS, SEARCH + "%TERMS%");
    Services.prefs.setCharPref(PREF_GETADDONS_GETSEARCHRESULTS, "http://localhost:4444/test_%TERMS%.xml");

    do_check_neq(AddonRepository, null);
    // Check the homepage and recommended urls
    do_check_eq(AddonRepository.homepageURL, BROWSE);
    do_check_eq(AddonRepository.getRecommendedURL(), RECOMMENDED);

    // Check that search urls are correct
    for (var i = 0; i < BROWSE_SEARCH_URLS.length; i++) {
      var url = AddonRepository.getSearchURL(BROWSE_SEARCH_URLS[i][0]);
      if (url != BROWSE_SEARCH_URLS[i][1])
        do_throw("BROWSE_SEARCH_URL[" + i + "] returned " + url);
    }

    // This should fail because we cancel it immediately.
    AddonRepository.retrieveRecommendedAddons(10, FailCallback);
    AddonRepository.cancelSearch();
    // Pull some results.
    AddonRepository.retrieveRecommendedAddons(10, RecommendedCallback);
    // Should be searching now and any attempt to retrieve again should be ignored
    do_check_true(AddonRepository.isSearching);
    AddonRepository.retrieveRecommendedAddons(10, FailCallback);
  });
}

