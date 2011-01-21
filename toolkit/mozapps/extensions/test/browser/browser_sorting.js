/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

// Tests that sorting of add-ons works correctly
// (this test uses the list view, even though it no longer has sort buttons - see bug 623207)

var gManagerWindow;
var gProvider;

function test() {
  waitForExplicitFinish();

  gProvider = new MockProvider();
  gProvider.createAddons([{
    // Enabled extensions
    id: "test1@tests.mozilla.org",
    name: "Test add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 02, 00, 00, 00),
    size: 1
  }, {
    id: "test2@tests.mozilla.org",
    name: "a first add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 01, 23, 59, 59),
    size: 0265
  }, {
    id: "test3@tests.mozilla.org",
    name: "\u010Cesk\u00FD slovn\u00EDk", // Český slovník
    description: "foo",
    updateDate: new Date(2010, 04, 02, 00, 00, 01),
    size: 12
  }, {
    id: "test4@tests.mozilla.org",
    name: "canadian dictionary",
    updateDate: new Date(1970, 0, 01, 00, 00, 00),
    description: "foo",
  }, {
    id: "test5@tests.mozilla.org",
    name: "croatian dictionary",
    description: "foo",
    updateDate: new Date(2012, 12, 12, 00, 00, 00),
    size: 5
  }, {
    // Incompatible, disabled extensions
    id: "test6@tests.mozilla.org",
    name: "orange Add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 02, 00, 00, 00),
    size: 142,
    isCompatible: false,
    isActive: false,
  }, {
    id: "test7@tests.mozilla.org",
    name: "Blue Add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 01, 23, 59, 59),
    size: 65,
    isCompatible: false,
    isActive: false,
  }, {
    id: "test8@tests.mozilla.org",
    name: "Green Add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 03, 00, 00, 01),
    size: 125,
    isCompatible: false,
    isActive: false,
  }, {
    id: "test9@tests.mozilla.org",
    name: "red Add-on",
    updateDate: new Date(2011, 04, 01, 00, 00, 00),
    description: "foo",
    isCompatible: false,
    isActive: false,
  }, {
    id: "test10@tests.mozilla.org",
    name: "Purple Add-on",
    description: "foo",
    updateDate: new Date(2012, 12, 12, 00, 00, 00),
    size: 56,
    isCompatible: false,
    isActive: false,
  }, {
    // Disabled, compatible extensions
    id: "test11@tests.mozilla.org",
    name: "amber Add-on",
    description: "foo",
    updateDate: new Date(1978, 04, 02, 00, 00, 00),
    size: 142,
    isActive: false,
  }, {
    id: "test12@tests.mozilla.org",
    name: "Salmon Add-on",
    description: "foo",
    updateDate: new Date(2054, 04, 01, 23, 59, 59),
    size: 65,
    isActive: false,
  }, {
    id: "test13@tests.mozilla.org",
    name: "rose Add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 02, 00, 00, 01),
    size: 125,
    isActive: false,
  }, {
    id: "test14@tests.mozilla.org",
    name: "Violet Add-on",
    updateDate: new Date(2010, 05, 01, 00, 00, 00),
    description: "foo",
    isActive: false,
  }, {
    id: "test15@tests.mozilla.org",
    name: "white Add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 12, 00, 00, 00),
    size: 56,
    isActive: false,
  }, {
    // Blocked extensions
    id: "test16@tests.mozilla.org",
    name: "grimsby Add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 01, 00, 00, 00),
    size: 142,
    isActive: false,
    blocklistState: Ci.nsIBlocklistService.STATE_SOFTBLOCKED,
  }, {
    id: "test17@tests.mozilla.org",
    name: "beamsville Add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 8, 23, 59, 59),
    size: 65,
    isActive: false,
    blocklistState: Ci.nsIBlocklistService.STATE_SOFTBLOCKED,
  }, {
    id: "test18@tests.mozilla.org",
    name: "smithville Add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 03, 00, 00, 01),
    size: 125,
    isActive: false,
    blocklistState: Ci.nsIBlocklistService.STATE_OUTDATED,
  }, {
    id: "test19@tests.mozilla.org",
    name: "dunnville Add-on",
    updateDate: new Date(2010, 04, 02, 00, 00, 00),
    description: "foo",
    isActive: false,
    blocklistState: Ci.nsIBlocklistService.STATE_SOFTBLOCKED,
  }, {
    id: "test20@tests.mozilla.org",
    name: "silverdale Add-on",
    description: "foo",
    updateDate: new Date(2010, 04, 12, 00, 00, 00),
    size: 56,
    isActive: false,
    blocklistState: Ci.nsIBlocklistService.STATE_BLOCKED,
  }]);

  open_manager("addons://list/extension", function(aWindow) {
    gManagerWindow = aWindow;
    run_next_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, function() {
    finish();
  });
}

function set_order(aSortBy, aAscending) {
  var list = gManagerWindow.document.getElementById("addon-list");
  var elements = [];
  var node = list.firstChild;
  while (node) {
    elements.push(node);
    node = node.nextSibling;
  }
  gManagerWindow.sortElements(elements, ["uiState", aSortBy], aAscending);
  elements.forEach(function(aElement) {
    list.appendChild(aElement);
  });
}

function check_order(aExpectedOrder) {
  var order = [];
  var list = gManagerWindow.document.getElementById("addon-list");
  var node = list.firstChild;
  while (node) {
    var id = node.getAttribute("value");
    if (id && id.substring(id.length - 18) == "@tests.mozilla.org")
      order.push(node.getAttribute("value"));
    node = node.nextSibling;
  }

  is(order.toSource(), aExpectedOrder.toSource(), "Should have seen the right order");
}

// Tests that ascending name ordering was the default
add_test(function() {

  check_order([
    "test2@tests.mozilla.org",
    "test4@tests.mozilla.org",
    "test3@tests.mozilla.org",
    "test5@tests.mozilla.org",
    "test1@tests.mozilla.org",
    "test7@tests.mozilla.org",
    "test8@tests.mozilla.org",
    "test6@tests.mozilla.org",
    "test10@tests.mozilla.org",
    "test9@tests.mozilla.org",
    "test11@tests.mozilla.org",
    "test13@tests.mozilla.org",
    "test12@tests.mozilla.org",
    "test14@tests.mozilla.org",
    "test15@tests.mozilla.org",
    "test17@tests.mozilla.org",
    "test19@tests.mozilla.org",
    "test16@tests.mozilla.org",
    "test20@tests.mozilla.org",
    "test18@tests.mozilla.org",
  ]);

  run_next_test();
});

// Tests that switching to date ordering works
add_test(function() {
  set_order("updateDate", false);

  // When we're ascending with updateDate, it's from newest
  // to oldest.

  check_order([
    "test5@tests.mozilla.org",
    "test3@tests.mozilla.org",
    "test1@tests.mozilla.org",
    "test2@tests.mozilla.org",
    "test4@tests.mozilla.org",
    "test10@tests.mozilla.org",
    "test9@tests.mozilla.org",
    "test8@tests.mozilla.org",
    "test6@tests.mozilla.org",
    "test7@tests.mozilla.org",
    "test12@tests.mozilla.org",
    "test14@tests.mozilla.org",
    "test15@tests.mozilla.org",
    "test13@tests.mozilla.org",
    "test11@tests.mozilla.org",
    "test20@tests.mozilla.org",
    "test17@tests.mozilla.org",
    "test18@tests.mozilla.org",
    "test19@tests.mozilla.org",
    "test16@tests.mozilla.org",
  ]);

  set_order("updateDate", true);

  check_order([
    "test4@tests.mozilla.org",
    "test2@tests.mozilla.org",
    "test1@tests.mozilla.org",
    "test3@tests.mozilla.org",
    "test5@tests.mozilla.org",
    "test7@tests.mozilla.org",
    "test6@tests.mozilla.org",
    "test8@tests.mozilla.org",
    "test9@tests.mozilla.org",
    "test10@tests.mozilla.org",
    "test11@tests.mozilla.org",
    "test13@tests.mozilla.org",
    "test15@tests.mozilla.org",
    "test14@tests.mozilla.org",
    "test12@tests.mozilla.org",
    "test16@tests.mozilla.org",
    "test19@tests.mozilla.org",
    "test18@tests.mozilla.org",
    "test17@tests.mozilla.org",
    "test20@tests.mozilla.org",
  ]);

  run_next_test();
});

// Tests that switching to name ordering works
add_test(function() {
  set_order("name", true);

  check_order([
    "test2@tests.mozilla.org",
    "test4@tests.mozilla.org",
    "test3@tests.mozilla.org",
    "test5@tests.mozilla.org",
    "test1@tests.mozilla.org",
    "test7@tests.mozilla.org",
    "test8@tests.mozilla.org",
    "test6@tests.mozilla.org",
    "test10@tests.mozilla.org",
    "test9@tests.mozilla.org",
    "test11@tests.mozilla.org",
    "test13@tests.mozilla.org",
    "test12@tests.mozilla.org",
    "test14@tests.mozilla.org",
    "test15@tests.mozilla.org",
    "test17@tests.mozilla.org",
    "test19@tests.mozilla.org",
    "test16@tests.mozilla.org",
    "test20@tests.mozilla.org",
    "test18@tests.mozilla.org",
  ]);

  set_order("name", false);

  check_order([
    "test1@tests.mozilla.org",
    "test5@tests.mozilla.org",
    "test3@tests.mozilla.org",
    "test4@tests.mozilla.org",
    "test2@tests.mozilla.org",
    "test9@tests.mozilla.org",
    "test10@tests.mozilla.org",
    "test6@tests.mozilla.org",
    "test8@tests.mozilla.org",
    "test7@tests.mozilla.org",
    "test15@tests.mozilla.org",
    "test14@tests.mozilla.org",
    "test12@tests.mozilla.org",
    "test13@tests.mozilla.org",
    "test11@tests.mozilla.org",
    "test18@tests.mozilla.org",
    "test20@tests.mozilla.org",
    "test16@tests.mozilla.org",
    "test19@tests.mozilla.org",
    "test17@tests.mozilla.org",
  ]);

  run_next_test();
});
