/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

// JS shim that contains the callback functions to be triggered from the
// payment provider's code in order to fire DOMRequest events.
const kPaymentShimFile = "chrome://browser/content/payment.js";

// Type of MozChromEvents to handle payment dialogs.
const kOpenPaymentConfirmationEvent = "open-payment-confirmation-dialog";
const kOpenPaymentFlowEvent = "open-payment-flow-dialog";

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

function debug (s) {
  //dump("-*- PaymentGlue: " + s + "\n");
};

function PaymentUI() {
}

PaymentUI.prototype = {

  confirmPaymentRequest: function confirmPaymentRequest(aRequests,
                                                        aSuccessCb,
                                                        aErrorCb) {
    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    let content = browser.getContentWindow();
    if (!content && aErrorCb) {
      aErrorCb.onresult("NO_CONTENT_WINDOW");
      return;
    }

    // The UI should listen for mozChromeEvent 'open-payment-confirmation-dialog'
    // type in order to create and show the payment request confirmation frame
    // embeded within a trusted dialog.
    let id = kOpenPaymentConfirmationEvent + "-" + this.getRandomId();
    let detail = {
      type: kOpenPaymentConfirmationEvent,
      id: id,
      paymentRequests: aRequests
    };

    // Once the user confirm the payment request and makes his choice, we get
    // back to the DOM part to get the appropriate payment flow information
    // based on the selected payment provider.
    content.addEventListener("mozContentEvent", function handleSelection(evt) {
      let msg = evt.detail;
      if (msg.id != id) {
        debug("mozContentEvent. evt.detail.id != " + id);
        content.removeEventListener("mozContentEvent", handleSelection);
        return;
      }

      if (msg.userSelection && aSuccessCb) {
        aSuccessCb.onresult(msg.userSelection);
      } else if (msg.errorMsg && aErrorCb) {
        aErrorCb.onresult(msg.errorMsg);
      }

      content.removeEventListener("mozContentEvent", handleSelection);
    });

    browser.shell.sendChromeEvent(detail);
  },

  showPaymentFlow: function showPaymentFlow(aPaymentFlowInfo, aErrorCb) {
    debug("showPaymentFlow. uri " + aPaymentFlowInfo.uri);
    // We ask the UI to browse to the selected payment flow.
    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    let content = browser.getContentWindow();
    if (!content && aErrorCb) {
      aErrorCb.onresult("NO_CONTENT_WINDOW");
      return;
    }

    let id = kOpenPaymentFlowEvent + "-" + this.getRandomId();
    let detail = {
      type: kOpenPaymentFlowEvent,
      id: id,
      uri: aPaymentFlowInfo.uri,
      method: aPaymentFlowInfo.requestMethod,
      jwt: aPaymentFlowInfo.jwt
    };

    // At some point the UI would send the created iframe back so the
    // callbacks for firing DOMRequest events can be loaded on its
    // content.
    content.addEventListener("mozContentEvent", function loadPaymentShim(evt) {
      if (evt.detail.id != id || !evt.detail.frame) {
        content.removeEventListener("mozContentEvent", loadPaymentShim);
        return;
      }

      // Try to load the payment shim file containing the payment callbacks
      // in the content script.
      let frame = evt.detail.frame;
      let frameLoader = frame.QueryInterface(Ci.nsIFrameLoaderOwner)
                        .frameLoader;
      let mm = frameLoader.messageManager;
      try {
        mm.loadFrameScript(kPaymentShimFile, true);
      } catch (e) {
        debug("Error loading " + kPaymentShimFile + " as a frame script: " + e);
        if (aErrorCb) {
          aErrorCb.onresult("ERROR_LOADING_PAYMENT_SHIM");
        }
      } finally {
        content.removeEventListener("mozContentEvent", loadPaymentShim);
      }
    });

    browser.shell.sendChromeEvent(detail);
  },

  getRandomId: function getRandomId() {
    return uuidgen.generateUUID().toString();
  },

  classID: Components.ID("{8b83eabc-7929-47f4-8b48-4dea8d887e4b}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPaymentUIGlue])
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([PaymentUI]);
