/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-common/async.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://testing-common/services/sync/fakeservices.js");

let btoa;
let atob;

let provider = {
  getFile: function(prop, persistent) {
    persistent.value = true;
    switch (prop) {
      case "ExtPrefDL":
        return [Services.dirsvc.get("CurProcD", Ci.nsIFile)];
      default:
        throw Cr.NS_ERROR_FAILURE;
    }
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider])
};
Services.dirsvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);

let timer;
function waitForZeroTimer(callback) {
  // First wait >100ms (nsITimers can take up to that much time to fire, so
  // we can account for the timer in delayedAutoconnect) and then two event
  // loop ticks (to account for the Utils.nextTick() in autoConnect).
  let ticks = 2;
  function wait() {
    if (ticks) {
      ticks -= 1;
      Utils.nextTick(wait);
      return;
    }
    callback();
  }
  timer = Utils.namedTimer(wait, 150, {}, "timer");
}

btoa = Cu.import("resource://services-common/log4moz.js").btoa;
atob = Cu.import("resource://services-common/log4moz.js").atob;

// This is needed for loadAddonTestFunctions().
let gGlobalScope = this;

function ExtensionsTestPath(path) {
  if (path[0] != "/") {
    throw Error("Path must begin with '/': " + path);
  }

  return "../../../../toolkit/mozapps/extensions/test/xpcshell" + path;
}

/**
 * Loads the AddonManager test functions by importing its test file.
 *
 * This should be called in the global scope of any test file needing to
 * interface with the AddonManager. It should only be called once, or the
 * universe will end.
 */
function loadAddonTestFunctions() {
  const path = ExtensionsTestPath("/head_addons.js");
  let file = do_get_file(path);
  let uri = Services.io.newFileURI(file);
  Services.scriptloader.loadSubScript(uri.spec, gGlobalScope);
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");
}

function getAddonInstall(name) {
  let f = do_get_file(ExtensionsTestPath("/addons/" + name + ".xpi"));
  let cb = Async.makeSyncCallback();
  AddonManager.getInstallForFile(f, cb);

  return Async.waitForSyncCallback(cb);
}

/**
 * Obtains an addon from the add-on manager by id.
 *
 * This is merely a synchronous wrapper.
 *
 * @param  id
 *         ID of add-on to fetch
 * @return addon object on success or undefined or null on failure
 */
function getAddonFromAddonManagerByID(id) {
   let cb = Async.makeSyncCallback();
   AddonManager.getAddonByID(id, cb);
   return Async.waitForSyncCallback(cb);
}

/**
 * Installs an add-on synchronously from an addonInstall
 *
 * @param  install addonInstall instance to install
 */
function installAddonFromInstall(install) {
  let cb = Async.makeSyncCallback();
  let listener = {onInstallEnded: cb};
  AddonManager.addInstallListener(listener);
  install.install();
  Async.waitForSyncCallback(cb);
  AddonManager.removeAddonListener(listener);

  do_check_neq(null, install.addon);
  do_check_neq(null, install.addon.syncGUID);

  return install.addon;
}

/**
 * Convenience function to install an add-on from the extensions unit tests.
 *
 * @param  name
 *         String name of add-on to install. e.g. test_install1
 * @return addon object that was installed
 */
function installAddon(name) {
  let install = getAddonInstall(name);
  do_check_neq(null, install);
  return installAddonFromInstall(install);
}

/**
 * Convenience function to uninstall an add-on synchronously.
 *
 * @param addon
 *        Addon instance to uninstall
 */
function uninstallAddon(addon) {
  let cb = Async.makeSyncCallback();
  let listener = {onUninstalled: function(uninstalled) {
    if (uninstalled.id == addon.id) {
      AddonManager.removeAddonListener(listener);
      cb(uninstalled);
    }
  }};

  AddonManager.addAddonListener(listener);
  addon.uninstall();
  Async.waitForSyncCallback(cb);
}

function setBasicCredentials(username, password, syncKey) {
  let ns = {};
  Cu.import("resource://services-sync/service.js", ns);

  let auth = ns.Service.identity;
  auth.username = username;
  auth.basicPassword = password;
  auth.syncKey = syncKey;
}

function SyncTestingInfrastructure(username, password, syncKey) {
  let ns = {};
  Cu.import("resource://services-sync/service.js", ns);

  let auth = ns.Service.identity;
  auth.account = username || "foo";
  auth.basicPassword = password || "password";
  auth.syncKey = syncKey || "foo";

  ns.Service.serverURL = TEST_SERVER_URL;
  ns.Service.clusterURL = TEST_CLUSTER_URL;

  this.logStats = initTestLogging();
  this.fakeFilesystem = new FakeFilesystemService({});
  this.fakeGUIDService = new FakeGUIDService();
  this.fakeCryptoService = new FakeCryptoService();
}

_("Setting the identity for passphrase");
Cu.import("resource://services-sync/identity.js");

/*
 * Test setup helpers.
 */

// Turn WBO cleartext into fake "encrypted" payload as it goes over the wire.
function encryptPayload(cleartext) {
  if (typeof cleartext == "object") {
    cleartext = JSON.stringify(cleartext);
  }

  return {ciphertext: cleartext, // ciphertext == cleartext with fake crypto
          IV: "irrelevant",
          hmac: fakeSHA256HMAC(cleartext, Utils.makeHMACKey(""))};
}

function generateNewKeys(collectionKeys, collections=null) {
  let wbo = collectionKeys.generateNewKeysWBO(collections);
  let modified = new_timestamp();
  collectionKeys.setContents(wbo.cleartext, modified);
}

