var Cc = Components.classes;
var Ci = Components.interfaces;

var runtime = Cc["@mozilla.org/xre/runtime;1"]
                .getService(Ci.nsIXULRuntime);
var started = document.getElementById("started");
started.textContent = runtime.startupTimestamp
