/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ObjectWrapper.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsISyncMessageSender");

function debug(aMsg) {
  //dump("-- ActivityProxy " + Date.now() + " : " + aMsg + "\n");
}

/**
  * nsIActivityProxy implementation
  * We keep a reference to the C++ Activity object, and
  * communicate with the Message Manager to know when to
  * fire events on it.
  */
function ActivityProxy() {
  debug("ActivityProxy");
  this.activity = null;
  let inParent = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime)
                   .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
  debug("inParent: " + inParent);
  Cu.import(inParent ? "resource://gre/modules/Webapps.jsm"
                     : "resource://gre/modules/AppsServiceChild.jsm");
}

ActivityProxy.prototype = {
  startActivity: function actProxy_startActivity(aActivity, aOptions, aWindow) {
    debug("startActivity");

    this.window = aWindow;
    this.activity = aActivity;
    this.id = Cc["@mozilla.org/uuid-generator;1"]
                .getService(Ci.nsIUUIDGenerator)
                .generateUUID().toString();
    // Retrieve the app's manifest url from the principal, so that we can
    // later notify when the activity handler called postResult or postError
    let principal = aWindow.document.nodePrincipal;
    let appId = principal.appId;
    let manifestURL = (appId != Ci.nsIScriptSecurityManager.NO_APP_ID &&
                       appId != Ci.nsIScriptSecurityManager.UNKNOWN_APP_ID)
                        ? DOMApplicationRegistry.getManifestURLByLocalId(appId)
                        : null;
    cpmm.sendAsyncMessage("Activity:Start", { id: this.id,
                                              options: aOptions,
                                              manifestURL: manifestURL,
                                              pageURL: aWindow.document.location.href });

    cpmm.addMessageListener("Activity:FireSuccess", this);
    cpmm.addMessageListener("Activity:FireError", this);
  },

  receiveMessage: function actProxy_receiveMessage(aMessage) {
    debug("Got message: " + aMessage.name);
    let msg = aMessage.json;
    if (msg.id != this.id)
      return;
    debug("msg=" + JSON.stringify(msg));

    switch(aMessage.name) {
      case "Activity:FireSuccess":
        debug("FireSuccess");
        Services.DOMRequest.fireSuccess(this.activity,
                                        ObjectWrapper.wrap(msg.result, this.window));
        break;
      case "Activity:FireError":
        debug("FireError");
        Services.DOMRequest.fireError(this.activity, msg.error);
        break;
    }
  },

  cleanup: function actProxy_cleanup() {
    debug("cleanup");
    cpmm.removeMessageListener("Activity:FireSuccess", this);
    cpmm.removeMessageListener("Activity:FireError", this);
  },

  classID: Components.ID("{ba9bd5cb-76a0-4ecf-a7b3-d2f7c43c5949}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIActivityProxy])
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([ActivityProxy]);
