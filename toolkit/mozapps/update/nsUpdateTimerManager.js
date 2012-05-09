/*
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Update timer Manager.
#
# The Initial Developer of the Original Code is Ben Goodger.
# Portions created by the Initial Developer are Copyright (C) 2004
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#  Ben Goodger <ben@mozilla.org> (Original Author)
#  Robert Strong <robert.bugzilla@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****
*/
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

const PREF_APP_UPDATE_LASTUPDATETIME_FMT  = "app.update.lastUpdateTime.%ID%";
const PREF_APP_UPDATE_TIMERMINIMUMDELAY   = "app.update.timerMinimumDelay";
const PREF_APP_UPDATE_TIMERFIRSTINTERVAL  = "app.update.timerFirstInterval";
const PREF_APP_UPDATE_LOG                 = "app.update.log";

const CATEGORY_UPDATE_TIMER               = "update-timer";

XPCOMUtils.defineLazyGetter(this, "gLogEnabled", function tm_gLogEnabled() {
  return getPref("getBoolPref", PREF_APP_UPDATE_LOG, false);
});

/**
#  Gets a preference value, handling the case where there is no default.
#  @param   func
#           The name of the preference function to call, on nsIPrefBranch
#  @param   preference
#           The name of the preference
#  @param   defaultValue
#           The default value to return in the event the preference has
#           no setting
#  @returns The value of the preference, or undefined if there was no
#           user or default value.
 */
function getPref(func, preference, defaultValue) {
  try {
    return Services.prefs[func](preference);
  }
  catch (e) {
  }
  return defaultValue;
}

/**
#  Logs a string to the error console.
#  @param   string
#           The string to write to the error console.
 */
function LOG(string) {
  if (gLogEnabled) {
    dump("*** UTM:SVC " + string + "\n");
    Services.console.logStringMessage("UTM:SVC " + string);
  }
}

/**
#  A manager for timers. Manages timers that fire over long periods of time
#  (e.g. days, weeks, months).
#  @constructor
 */
function TimerManager() {
  Services.obs.addObserver(this, "xpcom-shutdown", false);
}
TimerManager.prototype = {
  /**
   * The Checker Timer
   */
  _timer: null,

  /**
#    The Checker Timer minimum delay interval as specified by the
#    app.update.timerMinimumDelay pref. If the app.update.timerMinimumDelay
#    pref doesn't exist this will default to 120000.
   */
   _timerMinimumDelay: null,

  /**
   * The set of registered timers.
   */
  _timers: { },

  /**
   * See nsIObserver.idl
   */
  observe: function TM_observe(aSubject, aTopic, aData) {
    // Prevent setting the timer interval to a value of less than 60 seconds.
    var minInterval = 60000;
    // Prevent setting the first timer interval to a value of less than 10
    // seconds.
    var minFirstInterval = 10000;
    switch (aTopic) {
    case "utm-test-init":
      // Enforce a minimum timer interval of 500 ms for tests and fall through
      // to profile-after-change to initialize the timer.
      minInterval = 500;
      minFirstInterval = 500;
    case "profile-after-change":
      // Cancel the timer if it has already been initialized. This is primarily
      // for tests.
      this._timerMinimumDelay = Math.max(1000 * getPref("getIntPref", PREF_APP_UPDATE_TIMERMINIMUMDELAY, 120),
                                         minInterval);
      let firstInterval = Math.max(getPref("getIntPref", PREF_APP_UPDATE_TIMERFIRSTINTERVAL,
                                           this._timerMinimumDelay), minFirstInterval);
      this._canEnsureTimer = true;
      this._ensureTimer(firstInterval);
      break;
    case "xpcom-shutdown":
      Services.obs.removeObserver(this, "xpcom-shutdown");

      // Release everything we hold onto.
      this._cancelTimer();
      for (var timerID in this._timers)
        delete this._timers[timerID];
      this._timers = null;
      break;
    }
  },

  /**
#    Called when the checking timer fires.
#
#    We only fire one notification each time, so that the operations are
#    staggered. We don't want too many to happen at once, which could
#    negatively impact responsiveness.
#
#    @param   timer
#             The checking timer that fired.
   */
  notify: function TM_notify(timer) {
    var nextDelay = null;
    function updateNextDelay(delay) {
      if (nextDelay === null || delay < nextDelay)
        nextDelay = delay;
    }

    // Each timer calls tryFire(), which figures out which is the the one that
    // wanted to be called earliest. That one will be fired; the others are
    // skipped and will be done later.
    var now = Math.round(Date.now() / 1000);

    var callbackToFire = null;
    var earliestIntendedTime = null;
    var skippedFirings = false;
    function tryFire(callback, intendedTime) {
      var selected = false;
      if (intendedTime <= now) {
        if (intendedTime < earliestIntendedTime ||
            earliestIntendedTime === null) {
          callbackToFire = callback;
          earliestIntendedTime = intendedTime;
          selected = true;
        }
        else if (earliestIntendedTime !== null)
          skippedFirings = true;
      }
      // We do not need to updateNextDelay for the timer that actually fires;
      // we'll update right after it fires, with the proper intended time.
      // Note that we might select one, then select another later (with an
      // earlier intended time); it is still ok that we did not update for
      // the first one, since if we have skipped firings, the next delay
      // will be the minimum delay anyhow.
      if (!selected)
        updateNextDelay(intendedTime - now);
    }

    var catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    var entries = catMan.enumerateCategory(CATEGORY_UPDATE_TIMER);
    while (entries.hasMoreElements()) {
      let entry = entries.getNext().QueryInterface(Ci.nsISupportsCString).data;
      let value = catMan.getCategoryEntry(CATEGORY_UPDATE_TIMER, entry);
      let [cid, method, timerID, prefInterval, defaultInterval] = value.split(",");
      let lastUpdateTime;

      defaultInterval = parseInt(defaultInterval);
      // cid and method are validated below when calling notify.
      if (!timerID || !defaultInterval || isNaN(defaultInterval)) {
        LOG("TimerManager:notify - update-timer category registered" +
            (cid ? " for " + cid : "") + " without required parameters - " +
             "skipping");
        continue;
      }

      let interval = getPref("getIntPref", prefInterval, defaultInterval);
      let prefLastUpdate = PREF_APP_UPDATE_LASTUPDATETIME_FMT.replace(/%ID%/,
                                                                  timerID);
      if (Services.prefs.prefHasUserValue(prefLastUpdate)) {
        lastUpdateTime = Services.prefs.getIntPref(prefLastUpdate);
      }
      else {
        // Initialize the last update time to 0 so that we check for
        // an update the first time the interval expires.
        lastUpdateTime = 0;
        Services.prefs.setIntPref(prefLastUpdate, lastUpdateTime);
      }

      tryFire(function() {
        try {
          Components.classes[cid][method](Ci.nsITimerCallback).notify(timer);
          LOG("TimerManager:notify - notified " + cid);
        }
        catch (e) {
          LOG("TimerManager:notify - error notifying component id: " +
              cid + " ,error: " + e);
        }
        lastUpdateTime = now;
        Services.prefs.setIntPref(prefLastUpdate, lastUpdateTime);
        updateNextDelay(lastUpdateTime + interval - now);
      }, lastUpdateTime + interval);
    }

    for (let _timerID in this._timers) {
      let timerID = _timerID; // necessary for the closure to work properly
      let timerData = this._timers[timerID];
      tryFire(function() {
        if (timerData.callback instanceof Ci.nsITimerCallback) {
          try {
            timerData.callback.notify(timer);
            LOG("TimerManager:notify - notified timerID: " + timerID);
          }
          catch (e) {
            LOG("TimerManager:notify - error notifying timerID: " + timerID +
                ", error: " + e);
          }
        }
        else {
          LOG("TimerManager:notify - timerID: " + timerID + " doesn't " +
              "implement nsITimerCallback - skipping");
        }
        lastUpdateTime = now;
        timerData.lastUpdateTime = lastUpdateTime;
        var prefLastUpdate = PREF_APP_UPDATE_LASTUPDATETIME_FMT.replace(/%ID%/, timerID);
        Services.prefs.setIntPref(prefLastUpdate, lastUpdateTime);
        updateNextDelay(timerData.lastUpdateTime + timerData.interval - now);
      }, timerData.lastUpdateTime + timerData.interval);
    }

    if (callbackToFire)
      callbackToFire();

    if (nextDelay !== null) {
      if (skippedFirings)
        timer.delay = this._timerMinimumDelay;
      else
        timer.delay = Math.max(nextDelay * 1000, this._timerMinimumDelay);  
      this.lastTimerReset = Date.now();
    } else {
      this._cancelTimer();
    }
  },

  /**
   * Starts the timer, if necessary, and ensures that it will fire soon enough
   * to happen after time |interval| (in milliseconds).
   */
  _ensureTimer: function(interval) {
    if (!this._canEnsureTimer)
      return;
    if (!this._timer) {
      this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this._timer.initWithCallback(this, interval,
                                   Ci.nsITimer.TYPE_REPEATING_SLACK);
      this.lastTimerReset = Date.now();
    } else {
      if (Date.now() + interval < this.lastTimerReset + this._timer.delay) 
        this._timer.delay = this.lastTimerReset + interval - Date.now();
    }
  },

  /**
   * Stops the timer, if it is running.
   */
  _cancelTimer: function() {
    if (this._timer) {
      this._timer.cancel();
      this._timer = null;
    }
  },

  /**
   * See nsIUpdateTimerManager.idl
   */
  registerTimer: function TM_registerTimer(id, callback, interval) {
    LOG("TimerManager:registerTimer - id: " + id);
    var prefLastUpdate = PREF_APP_UPDATE_LASTUPDATETIME_FMT.replace(/%ID%/, id);
    var lastUpdateTime;
    if (Services.prefs.prefHasUserValue(prefLastUpdate)) {
      lastUpdateTime = Services.prefs.getIntPref(prefLastUpdate);
    } else {
      lastUpdateTime = Math.round(Date.now() / 1000);
      Services.prefs.setIntPref(prefLastUpdate, lastUpdateTime);
    }
    this._timers[id] = { callback       : callback,
                         interval       : interval,
                         lastUpdateTime : lastUpdateTime };

    this._ensureTimer(interval * 1000);
  },

  classID: Components.ID("{B322A5C0-A419-484E-96BA-D7182163899F}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdateTimerManager,
                                         Ci.nsITimerCallback,
                                         Ci.nsIObserver])
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([TimerManager]);
