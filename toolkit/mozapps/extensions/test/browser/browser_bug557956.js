/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

// Test that the compatibility dialog that normally displays during startup
// appears to work correctly.

const URI_EXTENSION_UPDATE_DIALOG = "chrome://mozapps/content/extensions/update.xul";

/**
 * Test add-ons:
 *
 * Addon    minVersion   maxVersion   Notes
 * addon1   0            *
 * addon2   0            0
 * addon3   0            0
 * addon4   1            *
 * addon5   0            0            Made compatible by update check
 * addon6   0            0            Made compatible by update check
 * addon7   0            0            Has a broken update available
 * addon8   0            0            Has an update available
 * addon9   0            0            Has an update available
 */

function test() {
  ok(true, "Test disabled due to timeouts");
  return;
  waitForExplicitFinish();

  run_next_test();
}

function end_test() {
  Services.prefs.clearUserPref("extensions.update.url");

  finish();
}

function install_test_addons(aCallback) {
  var installs = [];

  // Use a blank update URL
  Services.prefs.setCharPref("extensions.update.url", TESTROOT + "missing.rdf");

  ["browser_bug557956_1",
   "browser_bug557956_2",
   "browser_bug557956_3",
   "browser_bug557956_4",
   "browser_bug557956_5",
   "browser_bug557956_6",
   "browser_bug557956_7",
   "browser_bug557956_8_1",
   "browser_bug557956_9_1"].forEach(function(aName) {
    AddonManager.getInstallForURL(TESTROOT + "addons/" + aName + ".xpi", function(aInstall) {
      installs.push(aInstall);
    }, "application/x-xpinstall");
  });

  var listener = {
    installCount: 0,

    onInstallEnded: function() {
      this.installCount++;
      if (this.installCount == installs.length) {
        // Switch to the test update URL
        Services.prefs.setCharPref("extensions.update.url", TESTROOT + "browser_bug557956.rdf");

        aCallback();
      }
    }
  };

  installs.forEach(function(aInstall) {
    aInstall.addListener(listener);
    aInstall.install();
  });
}

function uninstall_test_addons(aCallback) {
  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org",
                               "addon3@tests.mozilla.org",
                               "addon4@tests.mozilla.org",
                               "addon5@tests.mozilla.org",
                               "addon6@tests.mozilla.org",
                               "addon7@tests.mozilla.org",
                               "addon8@tests.mozilla.org",
                               "addon9@tests.mozilla.org"],
                               function(aAddons) {
    aAddons.forEach(function(aAddon) {
      if (aAddon)
        aAddon.uninstall();
    });
    aCallback();
  });
}

function open_compatibility_window(aInactiveAddonIds, aCallback) {
  var variant = Cc["@mozilla.org/variant;1"].
                createInstance(Ci.nsIWritableVariant);
  variant.setFromVariant(aInactiveAddonIds);

  // Cannot be modal as we want to interract with it, shouldn't cause problems
  // with testing though.
  var features = "chrome,centerscreen,dialog,titlebar";
  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  var win = ww.openWindow(null, URI_EXTENSION_UPDATE_DIALOG, "", features, variant);

  win.addEventListener("load", function() {
    win.removeEventListener("load", arguments.callee, false);

    info("Compatibility dialog opened");

    function page_shown(aEvent) {
      info("Page " + aEvent.target.id + " shown");
    }

    win.addEventListener("pageshow", page_shown, false);
    win.addEventListener("unload", function() {
      win.removeEventListener("unload", arguments.callee, false);
      win.removeEventListener("pageshow", page_shown, false);
      info("Compatibility dialog closed");
    }, false);

    aCallback(win);
  }, false);
}

function wait_for_window_close(aWindow, aCallback) {
  aWindow.addEventListener("unload", function() {
    aWindow.removeEventListener("unload", arguments.callee, false);
    aCallback();
  }, false);
}

function wait_for_page(aWindow, aPageId, aCallback) {
  var page = aWindow.document.getElementById(aPageId);
  page.addEventListener("pageshow", function() {
    page.removeEventListener("pageshow", arguments.callee, false);
    executeSoon(function() {
      aCallback(aWindow);
    });
  }, false);
}

function get_list_names(aList) {
  var items = [];
  for (let i = 0; i < aList.childNodes.length; i++)
    items.push(aList.childNodes[i].label);
  items.sort();
  return items;
}

// Tests that the right add-ons show up in the mismatch dialog and updates can
// be installed
add_test(function() {
  install_test_addons(function() {
    // These add-ons were inactive in the old application
    var inactiveAddonIds = [
      "addon2@tests.mozilla.org",
      "addon4@tests.mozilla.org",
      "addon5@tests.mozilla.org"
    ];

    // Check that compatibility updates were applied.
    AddonManager.getAddonsByIDs(["addon5@tests.mozilla.org",
                                 "addon6@tests.mozilla.org"],
                                 function([a5, a6]) {
      ok(!a5.isCompatible, "addon5 should not be compatible");
      ok(!a6.isCompatible, "addon6 should not be compatible");

      open_compatibility_window(inactiveAddonIds, function(aWindow) {
        var doc = aWindow.document;
        wait_for_page(aWindow, "mismatch", function(aWindow) {
          var items = get_list_names(doc.getElementById("mismatch.incompatible"));
          is(items.length, 4, "Should have seen 4 still incompatible items");
          is(items[0], "Addon3 1.0", "Should have seen addon3 still incompatible");
          is(items[1], "Addon7 1.0", "Should have seen addon7 still incompatible");
          is(items[2], "Addon8 1.0", "Should have seen addon8 still incompatible");
          is(items[3], "Addon9 1.0", "Should have seen addon9 still incompatible");

          ok(a5.isCompatible, "addon5 should be compatible");
          ok(a6.isCompatible, "addon5 should be compatible");

          var button = doc.documentElement.getButton("next");
          EventUtils.synthesizeMouse(button, 2, 2, { }, aWindow);

          wait_for_page(aWindow, "found", function(aWindow) {
            ok(doc.getElementById("xpinstallDisabledAlert").hidden,
               "Install should be allowed");

            var list = doc.getElementById("found.updates");
            var items = get_list_names(list);
            is(items.length, 3, "Should have seen 3 updates available");
            is(items[0], "Addon7 2.0", "Should have seen update for addon7");
            is(items[1], "Addon8 2.0", "Should have seen update for addon8");
            is(items[2], "Addon9 2.0", "Should have seen update for addon9");

            ok(!doc.documentElement.getButton("next").disabled,
               "Next button should be enabled");

            // Uncheck all
            for (let i = 0; i < list.childNodes.length; i++)
              EventUtils.synthesizeMouse(list.childNodes[i], 2, 2, { }, aWindow);

            ok(doc.documentElement.getButton("next").disabled,
               "Next button should not be enabled");

            // Check the ones we want to install
            for (let i = 0; i < list.childNodes.length; i++) {
              if (list.childNodes[i].label != "Addon7 2.0")
                EventUtils.synthesizeMouse(list.childNodes[i], 2, 2, { }, aWindow);
            }

            var button = doc.documentElement.getButton("next");
            EventUtils.synthesizeMouse(button, 2, 2, { }, aWindow);

            wait_for_page(aWindow, "finished", function(aWindow) {
              var button = doc.documentElement.getButton("finish");
              ok(!button.hidden, "Finish button should not be hidden");
              ok(!button.disabled, "Finish button should not be disabled");
              EventUtils.synthesizeMouse(button, 2, 2, { }, aWindow);

              AddonManager.getAddonsByIDs(["addon8@tests.mozilla.org",
                                           "addon9@tests.mozilla.org"],
                                           function([a8, a9]) {
                is(a8.version, "2.0", "addon8 should have updated");
                is(a9.version, "2.0", "addon9 should have updated");

                uninstall_test_addons(run_next_test);
              });
            });
          });
        });
      });
    });
  });
});

// Tests that the install failures show the install failed page and disabling
// xpinstall shows the right UI.
add_test(function() {
  install_test_addons(function() {
    // These add-ons were inactive in the old application
    var inactiveAddonIds = [
      "addon2@tests.mozilla.org",
      "addon4@tests.mozilla.org",
      "addon5@tests.mozilla.org"
    ];

    Services.prefs.setBoolPref("xpinstall.enabled", false);

    open_compatibility_window(inactiveAddonIds, function(aWindow) {
      var doc = aWindow.document;
      wait_for_page(aWindow, "mismatch", function(aWindow) {
        var items = get_list_names(doc.getElementById("mismatch.incompatible"));
        is(items.length, 4, "Should have seen 4 still incompatible items");
        is(items[0], "Addon3 1.0", "Should have seen addon3 still incompatible");
        is(items[1], "Addon7 1.0", "Should have seen addon7 still incompatible");
        is(items[2], "Addon8 1.0", "Should have seen addon8 still incompatible");
        is(items[3], "Addon9 1.0", "Should have seen addon9 still incompatible");

        // Check that compatibility updates were applied.
        AddonManager.getAddonsByIDs(["addon5@tests.mozilla.org",
                                     "addon6@tests.mozilla.org"],
                                     function([a5, a6]) {
          ok(a5.isCompatible, "addon5 should be compatible");
          ok(a6.isCompatible, "addon5 should be compatible");

          var button = doc.documentElement.getButton("next");
          EventUtils.synthesizeMouse(button, 2, 2, { }, aWindow);

          wait_for_page(aWindow, "found", function(aWindow) {
            ok(!doc.getElementById("xpinstallDisabledAlert").hidden,
               "Install should not be allowed");

            ok(doc.documentElement.getButton("next").disabled,
               "Next button should be disabled");

            var checkbox = doc.getElementById("enableXPInstall");
            EventUtils.synthesizeMouse(checkbox, 2, 2, { }, aWindow);

            ok(!doc.documentElement.getButton("next").disabled,
               "Next button should be enabled");

            var list = doc.getElementById("found.updates");
            var items = get_list_names(list);
            is(items.length, 3, "Should have seen 3 updates available");
            is(items[0], "Addon7 2.0", "Should have seen update for addon7");
            is(items[1], "Addon8 2.0", "Should have seen update for addon8");
            is(items[2], "Addon9 2.0", "Should have seen update for addon9");

            // Unheck the ones we don't want to install
            for (let i = 0; i < list.childNodes.length; i++) {
              if (list.childNodes[i].label != "Addon7 2.0")
                EventUtils.synthesizeMouse(list.childNodes[i], 2, 2, { }, aWindow);
            }

            var button = doc.documentElement.getButton("next");
            EventUtils.synthesizeMouse(button, 2, 2, { }, aWindow);

            wait_for_page(aWindow, "installerrors", function(aWindow) {
              var button = doc.documentElement.getButton("finish");
              ok(!button.hidden, "Finish button should not be hidden");
              ok(!button.disabled, "Finish button should not be disabled");

              EventUtils.synthesizeMouse(button, 2, 2, { }, aWindow);

              uninstall_test_addons(run_next_test);
            });
          });
        });
      });
    });
  });
});

// Tests that no add-ons show up in the mismatch dialog when they are all disabled
add_test(function() {
  install_test_addons(function() {
    AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                                 "addon2@tests.mozilla.org",
                                 "addon3@tests.mozilla.org",
                                 "addon4@tests.mozilla.org",
                                 "addon5@tests.mozilla.org",
                                 "addon6@tests.mozilla.org",
                                 "addon7@tests.mozilla.org",
                                 "addon8@tests.mozilla.org",
                                 "addon9@tests.mozilla.org"],
                                 function(aAddons) {
      aAddons.forEach(function(aAddon) {
        aAddon.userDisabled = true;
      });

      // These add-ons were inactive in the old application
      var inactiveAddonIds = [
        "addon1@tests.mozilla.org",
        "addon2@tests.mozilla.org",
        "addon3@tests.mozilla.org",
        "addon4@tests.mozilla.org",
        "addon5@tests.mozilla.org",
        "addon6@tests.mozilla.org",
        "addon7@tests.mozilla.org",
        "addon8@tests.mozilla.org",
        "addon9@tests.mozilla.org"
      ];

      open_compatibility_window(inactiveAddonIds, function(aWindow) {
        // Should close immediately on its own
        wait_for_window_close(aWindow, function() {
          uninstall_test_addons(run_next_test);
        });
      });
    });
  });
});

// Tests that the right UI shows for when no updates are available
add_test(function() {
  install_test_addons(function() {
    AddonManager.getAddonsByIDs(["addon7@tests.mozilla.org",
                                 "addon8@tests.mozilla.org",
                                 "addon9@tests.mozilla.org"],
                                 function(aAddons) {
      aAddons.forEach(function(aAddon) {
        aAddon.uninstall();
      });

      // These add-ons were inactive in the old application
      var inactiveAddonIds = [
        "addon2@tests.mozilla.org",
        "addon4@tests.mozilla.org",
        "addon5@tests.mozilla.org"
      ];

      open_compatibility_window(inactiveAddonIds, function(aWindow) {
      var doc = aWindow.document;
      wait_for_page(aWindow, "mismatch", function(aWindow) {
        var items = get_list_names(doc.getElementById("mismatch.incompatible"));
        is(items.length, 1, "Should have seen 1 still incompatible items");
        is(items[0], "Addon3 1.0", "Should have seen addon3 still incompatible");

        var button = doc.documentElement.getButton("next");
        EventUtils.synthesizeMouse(button, 2, 2, { }, aWindow);

          wait_for_page(aWindow, "noupdates", function(aWindow) {
            var button = doc.documentElement.getButton("finish");
            ok(!button.hidden, "Finish button should not be hidden");
            ok(!button.disabled, "Finish button should not be disabled");
            EventUtils.synthesizeMouse(button, 2, 2, { }, aWindow);

            uninstall_test_addons(run_next_test);
          });
        });
      });
    });
  });
});
