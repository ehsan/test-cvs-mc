Components.utils.import("resource://gre/modules/Services.jsm");

function install(data, reason) {
  Services.prefs.setIntPref("bootstraptest.installed_version", 3);
}

function startup(data, reason) {
  Services.prefs.setIntPref("bootstraptest.active_version", 3);
  Services.prefs.setIntPref("bootstraptest.startup_reason", reason);
}

function shutdown(data, reason) {
  Services.prefs.setIntPref("bootstraptest.active_version", 0);
  Services.prefs.setIntPref("bootstraptest.shutdown_reason", reason);
}

function uninstall(data, reason) {
  Services.prefs.setIntPref("bootstraptest.installed_version", 0);
}
