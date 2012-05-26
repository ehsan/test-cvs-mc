/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

function test() {
  /** Test for Bug 506482 **/

  // test setup
  waitForExplicitFinish();

  // read the sessionstore.js mtime (picked from browser_248970_a.js)
  let profilePath = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties).
                    get("ProfD", Ci.nsIFile);
  function getSessionstoreFile() {
    let sessionStoreJS = profilePath.clone();
    sessionStoreJS.append("sessionstore.js");
    return sessionStoreJS;
  }
  function getSessionstorejsModificationTime() {
    let file = getSessionstoreFile();
    if (file.exists())
      return file.lastModifiedTime;
    else
      return -1;
  }

  // delete existing sessionstore.js, to make sure we're not reading
  // the mtime of an old one initialy
  let (sessionStoreJS = getSessionstoreFile()) {
    if (sessionStoreJS.exists())
      sessionStoreJS.remove(false);
  }

  // test content URL
  const TEST_URL = "data:text/html,"
    + "<body style='width: 100000px; height: 100000px;'><p>top</p></body>"

  // preferences that we use
  const PREF_INTERVAL = "browser.sessionstore.interval";

  // make sure sessionstore.js is saved ASAP on all events
  gPrefService.setIntPref(PREF_INTERVAL, 0);

  // get the initial sessionstore.js mtime (-1 if it doesn't exist yet)
  let mtime0 = getSessionstorejsModificationTime();

  // create and select a first tab
  let tab = gBrowser.addTab(TEST_URL);
  tab.linkedBrowser.addEventListener("load", function loadListener(e) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    // step1: the above has triggered some saveStateDelayed(), sleep until
    // it's done, and get the initial sessionstore.js mtime
    setTimeout(function step1(e) {
      let mtime1 = getSessionstorejsModificationTime();
      isnot(mtime1, mtime0, "initial sessionstore.js update");

      // step2: test sessionstore.js is not updated on tab selection
      // or content scrolling
      gBrowser.selectedTab = tab;
      tab.linkedBrowser.contentWindow.scrollTo(1100, 1200);
      setTimeout(function step2(e) {
        let mtime2 = getSessionstorejsModificationTime();
        is(mtime2, mtime1,
           "tab selection and scrolling: sessionstore.js not updated");

        // ok, done, cleanup and finish
        if (gPrefService.prefHasUserValue(PREF_INTERVAL))
          gPrefService.clearUserPref(PREF_INTERVAL);
        gBrowser.removeTab(tab);
        finish();
      }, 3500); // end of sleep after tab selection and scrolling
    }, 3500); // end of sleep after initial saveStateDelayed()
  }, true);
}
