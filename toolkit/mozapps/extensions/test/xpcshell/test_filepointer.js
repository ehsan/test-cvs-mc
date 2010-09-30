/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

// Tests that various operations with file pointers work and do not affect the
// source files

var addon1 = {
  id: "addon1@tests.mozilla.org",
  version: "1.0",
  name: "Test 1",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
};

var addon1_2 = {
  id: "addon1@tests.mozilla.org",
  version: "2.0",
  name: "Test 1",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
};

var addon2 = {
  id: "addon2@tests.mozilla.org",
  version: "1.0",
  name: "Test 2",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }]
};

const profileDir = gProfD.clone();
profileDir.append("extensions");
profileDir.create(AM_Ci.nsILocalFile.DIRECTORY_TYPE, 0755);

const sourceDir = gProfD.clone();
sourceDir.append("source");

do_load_httpd_js();
var testserver;

function writePointer(aId, aName) {
  let file = profileDir.clone();
  file.append(aName ? aName : aId);

  let target = sourceDir.clone();
  target.append(do_get_expected_addon_name(aId));

  var fos = AM_Cc["@mozilla.org/network/file-output-stream;1"].
            createInstance(AM_Ci.nsIFileOutputStream);
  fos.init(file,
           FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE | FileUtils.MODE_TRUNCATE,
           FileUtils.PERMS_FILE, 0);
  fos.write(target.path, target.path.length);
  fos.close();
}

function run_test() {
  // pointer files only work with unpacked directories
  if (Services.prefs.getBoolPref("extensions.alwaysUnpack") == false)
    return;

  do_test_pending();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1");

  // Create and configure the HTTP server.
  testserver = new nsHttpServer();
  testserver.registerDirectory("/data/", do_get_file("data"));
  testserver.registerDirectory("/addons/", do_get_file("addons"));
  testserver.start(4444);

  run_test_1();
}

function end_test() {
  testserver.stop(do_test_finished);
}

// Tests that installing a new add-on by pointer works
function run_test_1() {
  writeInstallRDFForExtension(addon1, sourceDir);
  writePointer(addon1.id);

  startupManager();

  AddonManager.getAddonByID(addon1.id, function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "1.0");

    let file = a1.getResourceURI().QueryInterface(AM_Ci.nsIFileURL).file;
    do_check_eq(file.parent.path, sourceDir.path);

    let rootUri = do_get_addon_root_uri(sourceDir, addon1.id);
    let uri = a1.getResourceURI("/");
    do_check_eq(uri.spec, rootUri);
    uri = a1.getResourceURI("install.rdf");
    do_check_eq(uri.spec, rootUri + "install.rdf");

    run_test_2();
  });
}

// Tests that installing an upgrade doesn't clobber the original sources
function run_test_2() {
  prepare_test({}, [
    "onNewInstall",
  ]);

  let url = "http://localhost:4444/addons/test_filepointer.xpi";
  AddonManager.getInstallForURL(url, function(install) {
    ensure_test_completed();

    prepare_test({
      "addon1@tests.mozilla.org": [
        "onInstalling"
      ]
    }, [
      "onDownloadStarted",
      "onDownloadEnded",
      "onInstallStarted",
      "onInstallEnded"
    ], check_test_2);

    install.install();
  }, "application/x-xpinstall");
}

function check_test_2() {
  restartManager();

  AddonManager.getAddonByID(addon1.id, function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "2.0");

    let file = a1.getResourceURI().QueryInterface(AM_Ci.nsIFileURL).file;
    do_check_eq(file.parent.path, profileDir.path);

    let rootUri = do_get_addon_root_uri(profileDir, addon1.id);
    let uri = a1.getResourceURI("/");
    do_check_eq(uri.spec, rootUri);
    uri = a1.getResourceURI("install.rdf");
    do_check_eq(uri.spec, rootUri + "install.rdf");

    let source = sourceDir.clone();
    source.append(addon1.id);
    do_check_true(source.exists());

    a1.uninstall();

    restartManager();

    run_test_3();
  });
}

// Tests that uninstalling doesn't clobber the original sources
function run_test_3() {
  writePointer(addon1.id);

  restartManager();

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "1.0");

    a1.uninstall();

    restartManager();

    let source = sourceDir.clone();
    source.append(addon1.id);
    do_check_true(source.exists());

    run_test_4();
  });
}

// Tests that misnaming a pointer doesn't clobber the sources
function run_test_4() {
  writePointer("addon2@tests.mozilla.org", addon1.id);

  restartManager();

  AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                               "addon2@tests.mozilla.org"], function([a1, a2]) {
    do_check_eq(a1, null);
    do_check_eq(a2, null);

    let source = sourceDir.clone();
    source.append(addon1.id);
    do_check_true(source.exists());

    let pointer = profileDir.clone();
    pointer.append("addon2@tests.mozilla.org");
    do_check_false(pointer.exists());

    run_test_5();
  });
}

// Tests that changing the ID of an existing add-on doesn't clobber the sources
function run_test_5() {
  writeInstallRDFForExtension(addon1, sourceDir);
  writePointer(addon1.id);

  restartManager();

  AddonManager.getAddonByID(addon1.id, function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "1.0");

    var dest = writeInstallRDFForExtension(addon2, sourceDir, addon1.id);
    // Make sure the modification time changes enough to be detected.
    dest.lastModifiedTime -= 5000;

    restartManager();

    AddonManager.getAddonsByIDs(["addon1@tests.mozilla.org",
                                 "addon2@tests.mozilla.org"], function([a1, a2]) {
      do_check_eq(a1, null);
      do_check_eq(a2, null);

      let source = sourceDir.clone();
      source.append(addon1.id);
      do_check_true(source.exists());

      let pointer = profileDir.clone();
      pointer.append(addon1.id);
      do_check_false(pointer.exists());

      run_test_6();
    });
  });
}

// Removing the pointer file should uninstall the add-on
function run_test_6() {
  writeInstallRDFForExtension(addon1, sourceDir);
  writePointer(addon1.id);

  restartManager();

  AddonManager.getAddonByID(addon1.id, function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "1.0");

    let pointer = profileDir.clone();
    pointer.append(addon1.id);
    pointer.remove(false);

    restartManager();

    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
      do_check_eq(a1, null);

      run_test_7();
    });
  });
}

// Removing the pointer file and replacing it with a directory should work
function run_test_7() {
  writePointer(addon1.id);

  restartManager();

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "1.0");

    let pointer = profileDir.clone();
    pointer.append(addon1.id);
    pointer.remove(false);

    writeInstallRDFForExtension(addon1_2, profileDir);

    restartManager();

    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
      do_check_neq(a1, null);
      do_check_eq(a1.version, "2.0");

      a1.uninstall();

      restartManager();

      run_test_8();
    });
  });
}

// Changes to the source files should be detected
function run_test_8() {
  writePointer(addon1.id);

  restartManager();

  AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
    do_check_neq(a1, null);
    do_check_eq(a1.version, "1.0");

    var dest = writeInstallRDFForExtension(addon1_2, sourceDir);
    // Make sure the modification time changes enough to be detected.
    dest.lastModifiedTime -= 5000;

    restartManager();

    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
      do_check_neq(a1, null);
      do_check_eq(a1.version, "2.0");

      a1.uninstall();

      restartManager();

      end_test();
    });
  });
}
