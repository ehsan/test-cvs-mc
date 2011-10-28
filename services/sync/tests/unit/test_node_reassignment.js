/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

_("Test that node reassignment responses are respected on all kinds of " +
  "requests.");

// Don't sync any engines by default.
Svc.DefaultPrefs.set("registerEngines", "")

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/rest.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/log4moz.js");

function run_test() {
  Log4Moz.repository.getLogger("Sync.AsyncResource").level = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.ErrorHandler").level  = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.Resource").level      = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.RESTRequest").level   = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.Service").level       = Log4Moz.Level.Trace;
  Log4Moz.repository.getLogger("Sync.SyncScheduler").level = Log4Moz.Level.Trace;
  initTestLogging();

  Engines.register(RotaryEngine);

  // None of the failures in this file should result in a UI error.
  function onUIError() {
    do_throw("Errors should not be presented in the UI.");
  }
  Svc.Obs.add("weave:ui:login:error", onUIError);
  Svc.Obs.add("weave:ui:sync:error", onUIError);

  run_next_test();
}

/**
 * Emulate the following Zeus config:
 * $draining = data.get($prefix . $host . " draining");
 * if ($draining == "drain.") {
 *   log.warn($log_host_db_status . " migrating=1 (node-reassignment)" .
 *            $log_suffix);
 *   http.sendResponse("401 Node reassignment", $content_type,
 *                     '"server request: node reassignment"', "");
 * }
 */
const reassignBody = "\"server request: node reassignment\"";

// API-compatible with SyncServer handler. Bind `handler` to something to use
// as a ServerCollection handler.
// We keep this format because in a patch or two we're going to switch to using
// SyncServer.
function handleReassign(handler, req, resp) {
  resp.setStatusLine(req.httpVersion, 401, "Node reassignment");
  resp.setHeader("Content-Type", "application/json");
  resp.bodyOutputStream.write(reassignBody, reassignBody.length);
}

/**
 * A node assignment handler.
 */
const newNodeBody = "http://localhost:8080/";
function installNodeHandler(server, next) {
  function handleNodeRequest(req, resp) {
    _("Client made a request for a node reassignment.");
    resp.setStatusLine(req.httpVersion, 200, "OK");
    resp.setHeader("Content-Type", "text/plain");
    resp.bodyOutputStream.write(newNodeBody, newNodeBody.length);
    Utils.nextTick(next);
  }
  let nodePath = "/user/1.0/johndoe/node/weave";
  server.registerPathHandler(nodePath, handleNodeRequest);
  _("Registered node handler at " + nodePath);
}

/**
 * Optionally return a 401 for the provided handler, if the value of `name` in
 * the `reassignments` object is true.
 */
let reassignments = {
  "crypto": false,
  "info": false,
  "meta": false,
  "rotary": false
};
function maybeReassign(handler, name) {
  return function (request, response) {
    if (reassignments[name]) {
      return handleReassign(null, request, response);
    }
    return handler(request, response);
  };
}

function prepareServer() {
  Service.username   = "johndoe";
  Service.passphrase = "abcdeabcdeabcdeabcdeabcdea";
  Service.password   = "ilovejane";
  Service.serverURL  = "http://localhost:8080/";
  Service.clusterURL = "http://localhost:8080/";

  do_check_eq(Service.userAPI, "http://localhost:8080/user/1.0/");

  let collectionsHelper = track_collections_helper();
  let upd = collectionsHelper.with_updated_collection;
  let collections = collectionsHelper.collections;

  let engine  = Engines.get("rotary");
  let engines = {rotary: {version: engine.version,
                          syncID:  engine.syncID}};
  let global  = new ServerWBO("global", {engines: engines});
  let rotary  = new ServerCollection({}, true);
  let clients = new ServerCollection({}, true);
  let keys    = new ServerWBO("keys");

  let rotaryHandler = maybeReassign(upd("rotary", rotary.handler()), "rotary");
  let cryptoHandler = maybeReassign(upd("crypto", keys.handler()), "crypto");
  let metaHandler   = maybeReassign(upd("meta", global.handler()), "meta");
  let infoHandler   = maybeReassign(collectionsHelper.handler, "info");

  let server = httpd_setup({
    "/1.1/johndoe/storage/clients":     upd("clients", clients.handler()),
    "/1.1/johndoe/storage/crypto/keys": cryptoHandler,
    "/1.1/johndoe/storage/meta/global": metaHandler,
    "/1.1/johndoe/storage/rotary":      rotaryHandler,
    "/1.1/johndoe/info/collections":    infoHandler
  });

  return [server, global, rotary, collectionsHelper];
}

/**
 * Make a test request to `url`, then watch the result of two syncs
 * to ensure that a node request was made.
 * Runs `undo` between the two.
 */
function syncAndExpectNodeReassignment(server, firstNotification, undo,
                                       secondNotification, url) {
  function onwards() {
    let nodeFetched = false;
    function onFirstSync() {
      _("First sync completed.");
      Svc.Obs.remove(firstNotification, onFirstSync);
      Svc.Obs.add(secondNotification, onSecondSync);

      do_check_eq(Service.clusterURL, "");

      // Track whether we fetched node/weave. We want to wait for the second
      // sync to finish so that we're cleaned up for the next test, so don't
      // run_next_test in the node handler.
      nodeFetched = false;

      // Verify that the client requests a node reassignment.
      // Install a node handler to watch for these requests.
      installNodeHandler(server, function () {
        nodeFetched = true;
      });

      // Allow for tests to clean up error conditions.
      _("Undoing test changes.");
      undo();
    }
    function onSecondSync() {
      _("Second sync completed.");
      Svc.Obs.remove(secondNotification, onSecondSync);
      SyncScheduler.clearSyncTriggers();

      // Make absolutely sure that any event listeners are done with their work
      // before we proceed.
      waitForZeroTimer(function () {
        _("Second sync nextTick.");
        do_check_true(nodeFetched);
        Service.startOver();
        server.stop(run_next_test);
      });
    }

    Svc.Obs.add(firstNotification, onFirstSync);
    Service.sync();
  }

  // Make sure that it works!
  let request = new RESTRequest(url);
  request.get(function () {
    do_check_eq(request.response.status, 401);
    Utils.nextTick(onwards);
  });
}

add_test(function test_momentary_401_engine() {
  _("Test a failure for engine URLs that's resolved by reassignment.");
  let [server, global, rotary, collectionsHelper] = prepareServer();

  _("Enabling the Rotary engine.");
  let engine = Engines.get("rotary");
  engine.enabled = true;

  // We need the server to be correctly set up prior to experimenting. Do this
  // through a sync.
  let g = {syncID: Service.syncID,
           storageVersion: STORAGE_VERSION,
           rotary: {version: engine.version,
                    syncID:  engine.syncID}}

  global.payload = JSON.stringify(g);
  global.modified = new_timestamp();
  collectionsHelper.update_collection("meta", global.modified);

  _("First sync to prepare server contents.");
  Service.sync();

  _("Setting up Rotary collection to 401.");
  reassignments["rotary"] = true;

  // We want to verify that the clusterURL pref has been cleared after a 401
  // inside a sync. Flag the Rotary engine to need syncing.
  rotary.modified = new_timestamp() + 10;
  collectionsHelper.update_collection("rotary", rotary.modified);

  function undo() {
    reassignments["rotary"] = false;
  }

  syncAndExpectNodeReassignment(server,
                                "weave:service:sync:finish",
                                undo,
                                "weave:service:sync:finish",
                                Service.storageURL + "rotary");
});

// This test ends up being a failing fetch *after we're already logged in*.
add_test(function test_momentary_401_info_collections() {
  _("Test a failure for info/collections that's resolved by reassignment.");
  let [server, global, rotary] = prepareServer();

  _("First sync to prepare server contents.");
  Service.sync();

  // Return a 401 for info/collections requests.
  reassignments["info"] = true;

  function undo() {
    reassignments["info"] = false;
  }

  syncAndExpectNodeReassignment(server,
                                "weave:service:sync:error",
                                undo,
                                "weave:service:sync:finish",
                                Service.infoURL);
});

add_test(function test_momentary_401_storage() {
  _("Test a failure for any storage URL, not just engine parts. " +
    "Resolved by reassignment.");
  let [server, global, rotary] = prepareServer();

  // Return a 401 for all storage requests.
  reassignments["crypto"] = true;
  reassignments["meta"]   = true;
  reassignments["rotary"] = true;

  function undo() {
    reassignments["crypto"] = false;
    reassignments["meta"]   = false;
    reassignments["rotary"] = false;
  }

  syncAndExpectNodeReassignment(server,
                                "weave:service:login:error",
                                undo,
                                "weave:service:sync:finish",
                                Service.storageURL + "meta/global");
});

add_test(function test_loop_avoidance() {
  _("Test that a repeated failure doesn't result in a sync loop " +
    "if node reassignment cannot resolve the failure.");

  let [server, global, rotary] = prepareServer();

  // Return a 401 for all storage requests.
  reassignments["crypto"] = true;
  reassignments["meta"]   = true;
  reassignments["rotary"] = true;

  let firstNotification  = "weave:service:login:error";
  let secondNotification = "weave:service:login:error";
  let thirdNotification  = "weave:service:sync:finish";

  let nodeFetched = false;

  // Track the time. We want to make sure the duration between the first and
  // second sync is small, and then that the duration between second and third
  // is set to be large.
  let now;

  function getReassigned() {
    return Services.prefs.getBoolPref("services.sync.lastSyncReassigned");
  }

  function onFirstSync() {
    _("First sync completed.");
    Svc.Obs.remove(firstNotification, onFirstSync);
    Svc.Obs.add(secondNotification, onSecondSync);

    do_check_eq(Service.clusterURL, "");

    // We got a 401 mid-sync, and set the pref accordingly.
    do_check_true(Services.prefs.getBoolPref("services.sync.lastSyncReassigned"));

    // Track whether we fetched node/weave. We want to wait for the second
    // sync to finish so that we're cleaned up for the next test, so don't
    // run_next_test in the node handler.
    nodeFetched = false;

    // Verify that the client requests a node reassignment.
    // Install a node handler to watch for these requests.
    installNodeHandler(server, function () {
      nodeFetched = true;
    });

    // Update the timestamp.
    now = Date.now();
  }

  function onSecondSync() {
    _("Second sync completed.");
    Svc.Obs.remove(secondNotification, onSecondSync);
    Svc.Obs.add(thirdNotification, onThirdSync);

    // This sync occurred within the backoff interval.
    let elapsedTime = Date.now() - now;
    do_check_true(elapsedTime < MINIMUM_BACKOFF_INTERVAL);

    // This pref will be true until a sync completes successfully.
    do_check_true(getReassigned());

    // The timer will be set for some distant time.
    // We store nextSync in prefs, which offers us only limited resolution.
    // Include that logic here.
    let expectedNextSync = 1000 * Math.floor((now + MINIMUM_BACKOFF_INTERVAL) / 1000);
    _("Next sync scheduled for " + SyncScheduler.nextSync);
    _("Expected to be slightly greater than " + expectedNextSync);

    do_check_true(SyncScheduler.nextSync >= expectedNextSync);
    do_check_true(!!SyncScheduler.syncTimer);

    // Undo our evil scheme.
    reassignments["crypto"] = false;
    reassignments["meta"]   = false;
    reassignments["rotary"] = false;

    // Bring the timer forward to kick off a successful sync, so we can watch
    // the pref get cleared.
    SyncScheduler.scheduleNextSync(0);
  }
  function onThirdSync() {
    Svc.Obs.remove(thirdNotification, onThirdSync);

    // That'll do for now; no more syncs.
    SyncScheduler.clearSyncTriggers();

    // Make absolutely sure that any event listeners are done with their work
    // before we proceed.
    waitForZeroTimer(function () {
      _("Third sync nextTick.");

      // A missing pref throws.
      do_check_throws(getReassigned, Cr.NS_ERROR_UNEXPECTED);
      do_check_true(nodeFetched);
      Service.startOver();
      server.stop(run_next_test);
    });
  }

  Svc.Obs.add(firstNotification, onFirstSync);

  now = Date.now();
  Service.sync();
});
