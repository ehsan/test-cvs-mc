/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

function test() {
  /** Test for Bug 485563 **/
  
  waitForExplicitFinish();
  
  let uniqueValue = Math.random() + "\u2028Second line\u2029Second paragraph\u2027";
  
  let tab = gBrowser.addTab();
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
    ss.setTabValue(tab, "bug485563", uniqueValue);
    let tabState = JSON.parse(ss.getTabState(tab));
    is(tabState.extData["bug485563"], uniqueValue,
       "unicode line separator wasn't over-encoded");
    ss.deleteTabValue(tab, "bug485563");
    ss.setTabState(tab, JSON.stringify(tabState));
    is(ss.getTabValue(tab, "bug485563"), uniqueValue,
       "unicode line separator was correctly preserved");
    
    gBrowser.removeTab(tab);
    finish();
  }, true);
}
