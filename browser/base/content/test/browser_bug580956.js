function numClosedTabs()
  Cc["@mozilla.org/browser/sessionstore;1"].
    getService(Ci.nsISessionStore).
    getClosedTabCount(window);

function isUndoCloseEnabled() {
  document.popupNode = gBrowser.tabs[0];
  TabContextMenu.updateContextMenu(document.getElementById("tabContextMenu"));
  TabContextMenu.contextTab = null;
  return !document.getElementById("context_undoCloseTab").disabled;
}

function test() {
  waitForExplicitFinish();

  gPrefService.setIntPref("browser.sessionstore.max_tabs_undo", 0);
  gPrefService.clearUserPref("browser.sessionstore.max_tabs_undo");
  is(numClosedTabs(), 0, "There should be 0 closed tabs.");
  ok(!isUndoCloseEnabled(), "Undo Close Tab should be disabled.");

  var tab = gBrowser.addTab("http://mochi.test:8888/");
  var browser = gBrowser.getBrowserForTab(tab);
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    gBrowser.removeTab(tab);
    ok(isUndoCloseEnabled(), "Undo Close Tab should be enabled.");

    finish();
  }, true);
}
