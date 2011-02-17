/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

// Tests that the Find bar goes away on tab switch or location change.

let newTab, iframe;

function test() {
  waitForExplicitFinish();
  newTab = gBrowser.addTab("about:blank");
  gBrowser.addEventListener("DOMContentLoaded", testFindBarCloseOnTabSwitch,
                            false);
}

function testFindBarCloseOnTabSwitch() {
  gBrowser.removeEventListener("DOMContentLoaded", testFindBarCloseOnTabSwitch,
                               false);

  gFindBar.open();
  gBrowser.selectedTab = newTab;
  ok(gFindBar.hidden, "the Find bar is hidden after switching tabs");

  gFindBar.open();
  newTab.linkedBrowser.addEventListener("DOMContentLoaded",
                                        testFindBarCloseOnLocationChange,
                                        false);
  newTab.linkedBrowser.contentWindow.location = "http://example.com/";
}

function testFindBarCloseOnLocationChange() {
  newTab.linkedBrowser.removeEventListener("DOMContentLoaded",
                                           testFindBarCloseOnLocationChange,
                                           false);

  ok(gFindBar.hidden, "the Find bar is hidden after the location changes");

  newTab.linkedBrowser.addEventListener("DOMContentLoaded",
    prepareTestFindBarStaysOpenOnSubdocumentLocationChange, false);
  newTab.linkedBrowser.contentWindow.location = "http://example.com/browser/" +
    "browser/base/content/test/test_bug628179.html";
}

function prepareTestFindBarStaysOpenOnSubdocumentLocationChange() {
  newTab.linkedBrowser.removeEventListener("DOMContentLoaded",
    prepareTestFindBarStaysOpenOnSubdocumentLocationChange, false);

  gFindBar.open();

  iframe = newTab.linkedBrowser.contentDocument.getElementById("iframe");
  iframe.addEventListener("load",
    testFindBarStaysOpenOnSubdocumentLocationChange, false);
  iframe.src = "http://example.org/";
}

function testFindBarStaysOpenOnSubdocumentLocationChange() {
  iframe.removeEventListener("load",
    testFindBarStaysOpenOnSubdocumentLocationChange, false);

  ok(!gFindBar.hidden, "the Find bar isn't hidden after the location of a " +
     "subdocument changes");

  gFindBar.close();
  gBrowser.removeTab(newTab);
  finish();
}

