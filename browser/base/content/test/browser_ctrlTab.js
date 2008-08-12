function test() {
  gBrowser.addTab();
  gBrowser.addTab();
  gBrowser.addTab();

  assertTabs(4);

  ctrlTabTest([2]      , 1, 0);
  ctrlTabTest([2, 3, 1], 2, 2);
  ctrlTabTest([]       , 4, 2);

  {
    let selectedIndex = gBrowser.tabContainer.selectedIndex;
    pressCtrlTab();
    pressCtrlTab(true);
    releaseCtrl();
    is(gBrowser.tabContainer.selectedIndex, selectedIndex,
       "Ctrl+Tab -> Ctrl+Shift+Tab keeps the selected tab");
  }

  { // test for bug 445369
    let tabs = gBrowser.mTabs.length;
    pressCtrlTab();
    EventUtils.synthesizeKey("w", { ctrlKey: true });
    is(gBrowser.mTabs.length, tabs - 1, "Ctrl+Tab -> Ctrl+W removes one tab");
    releaseCtrl();
  }
  assertTabs(3);

  ctrlTabTest([2, 1, 0], 7, 1);

  { // test for bug 445369
    selectTabs([1, 2, 0]);

    let selectedTab = gBrowser.selectedTab;
    let tabToRemove = gBrowser.mTabs[1];

    pressCtrlTab();
    pressCtrlTab();
    EventUtils.synthesizeKey("w", { ctrlKey: true });
    ok(!tabToRemove.parentNode,
       "Ctrl+Tab*2 -> Ctrl+W removes the second most recently selected tab");

    pressCtrlTab(true);
    pressCtrlTab(true);
    releaseCtrl();
    ok(gBrowser.selectedTab == selectedTab,
       "Ctrl+Tab*2 -> Ctrl+W -> Ctrl+Shift+Tab*2 keeps the selected tab");
  }
  assertTabs(2);

  ctrlTabTest([1]      , 1, 0);

  gBrowser.removeTab(gBrowser.tabContainer.lastChild);

  assertTabs(1);

  { // test for bug 445768
    let focusedWindow = document.commandDispatcher.focusedWindow;
    let eventConsumed = true;
    let detectKeyEvent = function (event) {
      eventConsumed = event.getPreventDefault();
    };
    document.addEventListener("keypress", detectKeyEvent, false);
    pressCtrlTab();
    document.removeEventListener("keypress", detectKeyEvent, false);
    ok(eventConsumed, "Ctrl+Tab consumed by the tabbed browser if one tab is open");
    is(focusedWindow.location, document.commandDispatcher.focusedWindow.location,
       "Ctrl+Tab doesn't change focus if one tab is open");
  }

  /* private utitily functions */

  function pressCtrlTab(aShiftKey)
    EventUtils.synthesizeKey("VK_TAB", { ctrlKey: true, shiftKey: !!aShiftKey });

  function releaseCtrl()
    EventUtils.synthesizeKey("VK_CONTROL", { type: "keyup" });

  function assertTabs(aTabs) {
    var tabs = gBrowser.mTabs.length;
    if (tabs != aTabs) {
      while (gBrowser.mTabs.length > 1)
        gBrowser.removeCurrentTab();
      throw "expected " + aTabs + " open tabs, got " + tabs;
    }
  }

  function selectTabs(tabs) {
    tabs.forEach(function (index) {
      gBrowser.selectedTab = gBrowser.mTabs[index];
    });
  }

  function ctrlTabTest(tabsToSelect, tabTimes, expectedIndex) {
    selectTabs(tabsToSelect);

    var indexStart = gBrowser.tabContainer.selectedIndex;
    var tabCount = gBrowser.mTabs.length;
    var normalized = tabTimes % tabCount;
    var where = normalized == 1 ? "back to the previously selected tab" :
                normalized + " tabs back in most-recently-selected order";

    for (let i = 0; i < tabTimes; i++) {
      pressCtrlTab();

      if (tabCount > 2)
       is(gBrowser.tabContainer.selectedIndex, indexStart,
         "Selected tab doesn't change while tabbing");
    }

    if (tabCount > 2) {
      ok(ctrlTab.panel.state == "showing" || ctrlTab.panel.state == "open",
         "With " + tabCount + " tabs open, Ctrl+Tab opens the preview panel");

      is(ctrlTab.label.value, gBrowser.mTabs[expectedIndex].label,
         "Preview panel displays label of expected tab");

      releaseCtrl();

      ok(ctrlTab.panel.state == "hiding" || ctrlTab.panel.state == "closed",
         "Releasing Ctrl closes the preview panel");
    } else {
      ok(ctrlTab.panel.state == "hiding" || ctrlTab.panel.state == "closed",
         "With " + tabCount + " tabs open, Ctrl+Tab doesn't open the preview panel");
    }

    is(gBrowser.tabContainer.selectedIndex, expectedIndex,
       "With "+ tabCount +" tabs open and tab " + indexStart
       + " selected, Ctrl+Tab*" + tabTimes + " goes " + where);
  }
}
