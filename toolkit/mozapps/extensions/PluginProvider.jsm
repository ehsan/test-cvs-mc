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
# The Original Code is the Extension Manager.
#
# The Initial Developer of the Original Code is
# the Mozilla Foundation.
# Portions created by the Initial Developer are Copyright (C) 2010
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Dave Townsend <dtownsend@oxymoronical.com>
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

const Cc = Components.classes;
const Ci = Components.interfaces;

var EXPORTED_SYMBOLS = [];

Components.utils.import("resource://gre/modules/AddonManager.jsm");

/**
 * Logs a debug message.
 *
 * @param  aStr
 *         The string to log
 */
function LOG(aStr) {
  dump("*** addons.plugins: " + aStr + "\n");
}

/**
 * Logs a warning message.
 *
 * @param  aStr
 *         The string to log
 */
function WARN(aStr) {
  LOG(aStr);
}

/**
 * Logs an error message.
 *
 * @param  aStr
 *         The string to log
 */
function ERROR(aStr) {
  LOG(aStr);
}

var PluginProvider = {
  // A dictionary mapping IDs to names and descriptions
  plugins: null,

  /**
   * Called to get an Addon with a particular ID.
   *
   * @param  aId
   *         The ID of the add-on to retrieve
   * @param  aCallback
   *         A callback to pass the Addon to
   */
  getAddon: function PL_getAddon(aId, aCallback) {
    if (!this.plugins)
      this.buildPluginList();

    if (aId in this.plugins) {
      let name = this.plugins[aId].name;
      let description = this.plugins[aId].description;

      let tags = Cc["@mozilla.org/plugin/host;1"].
                 getService(Ci.nsIPluginHost).
                 getPluginTags({});
      let selected = [];
      tags.forEach(function(aTag) {
        if (aTag.name == name && aTag.description == description)
          selected.push(aTag);
      }, this);

      aCallback(new PluginWrapper(aId, name, description, selected));
    }
    else {
      aCallback(null);
    }
  },

  /**
   * Called to get Addons of a particular type.
   *
   * @param  aTypes
   *         An array of types to fetch. Can be null to get all types.
   * @param  callback
   *         A callback to pass an array of Addons to
   */
  getAddonsByTypes: function PL_getAddonsByTypes(aTypes, aCallback) {
    if (aTypes && aTypes.indexOf("plugin") < 0) {
      aCallback([]);
      return;
    }

    if (!this.plugins)
      this.buildPluginList();

    let results = [];

    for (let id in this.plugins) {
      this.getAddon(id, function(aAddon) {
        results.push(aAddon);
      });
    }

    aCallback(results);
  },

  /**
   * Called to get Addons that have pending operations.
   *
   * @param  aTypes
   *         An array of types to fetch. Can be null to get all types
   * @param  aCallback
   *         A callback to pass an array of Addons to
   */
  getAddonsWithPendingOperations: function PL_getAddonsWithPendingOperations(aTypes, aCallback) {
    aCallback([]);
  },

  /**
   * Called to get the current AddonInstalls, optionally restricting by type.
   *
   * @param  aTypes
   *         An array of types or null to get all types
   * @param  aCallback
   *         A callback to pass the array of AddonInstalls to
   */
  getInstalls: function PL_getInstalls(aTypes, aCallback) {
    aCallback([]);
  },

  buildPluginList: function PL_buildPluginList() {
    let tags = Cc["@mozilla.org/plugin/host;1"].
               getService(Ci.nsIPluginHost).
               getPluginTags({});

    this.plugins = {};
    let seen = {};
    tags.forEach(function(aTag) {
      if (!(aTag.name in seen))
        seen[aTag.name] = {};
      if (!(aTag.description in seen[aTag.name])) {
        let id = Cc["@mozilla.org/uuid-generator;1"].
                 getService(Ci.nsIUUIDGenerator).
                 generateUUID();
        this.plugins[id] = {
          name: aTag.name,
          description: aTag.description
        };
        seen[aTag.name][aTag.description] = true;
      }
    }, this);
  }
};

/**
 * The PluginWrapper wraps a set of nsIPluginTags to provide the data visible to
 * public callers through the API.
 */
function PluginWrapper(aId, aName, aDescription, aTags) {
  let safedesc = aDescription.replace(/<\/?[a-z][^>]*>/gi, " ");
  let homepageURL = null;
  if (/<A\s+HREF=[^>]*>/i.test(aDescription))
    homepageURL = /<A\s+HREF=["']?([^>"'\s]*)/i.exec(aDescription)[1];

  this.__defineGetter__("id", function() aId);
  this.__defineGetter__("type", function() "plugin");
  this.__defineGetter__("name", function() aName);
  this.__defineGetter__("description", function() safedesc);
  this.__defineGetter__("version", function() aTags[0].version);
  this.__defineGetter__("homepageURL", function() homepageURL);

  this.__defineGetter__("isActive", function() !aTags[0].blocklisted && !aTags[0].disabled);
  this.__defineGetter__("isCompatible", function() true);
  this.__defineGetter__("appDisabled", function() aTags[0].blocklisted);
  this.__defineGetter__("userDisabled", function() aTags[0].disabled);
  this.__defineSetter__("userDisabled", function(aVal) {
    if (aTags[0].disabled == aVal)
      return;

    aTags.forEach(function(aTag) {
      aTag.disabled = aVal;
    });
    AddonManagerPrivate.callAddonListeners(aVal ? "onDisabling" : "onEnabling", this, false);
    AddonManagerPrivate.callAddonListeners(aVal ? "onDisabled" : "onEnabled", this);
    return aVal;
  });

  this.__defineGetter__("pendingOperations", function() {
    return 0;
  });

  this.__defineGetter__("permissions", function() {
    let permissions = 0;
    if (!this.appDisabled) {
      if (this.userDisabled)
        permissions |= AddonManager.PERM_CAN_ENABLE;
      else
        permissions |= AddonManager.PERM_CAN_DISABLE;
    }
    return permissions;
  });

  this.uninstall = function() {
    throw new Error("Cannot uninstall plugins");
  };

  this.cancelUninstall = function() {
    throw new Error("Plugin is not marked to be uninstalled");
  };

  this.findUpdates = function(aListener, aReason, aAppVersion, aPlatformVersion) {
    throw new Error("Cannot search for updates for plugins");
  };

  this.hasResource = function(aPath) {
    return false;
  },

  this.getResourceURL = function(aPath) {
    return null;
  }
}

PluginWrapper.prototype = { };

AddonManagerPrivate.registerProvider(PluginProvider);
