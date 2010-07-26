/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is tabs.js.
 *
 * The Initial Developer of the Original Code is
 * Atul Varma <avarma@mozilla.com>
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Aza Raskin <aza@mozilla.com>
 * Ian Gilman <ian@iangilman.com>
 * Michael Yoshitaka Erlewine <mitcho@mitcho.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// **********
// Title: tabs.js

(function(){

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

// ##########
// Class: EventListenerMixIns
function EventListenerMixIns(mixInto) {
  var mixIns = {};

  // ----------
  // Function: add
  this.add = function add(options) {
    if (mixIns) {
      if (options.name in mixIns)
        Utils.log("mixIn for", options.name, "already exists.");
      options.mixInto = mixInto;
      mixIns[options.name] = new EventListenerMixIn(options);
    }
  };

  // ----------
  // Function: bubble
  this.bubble = function bubble(name, target, event) {
    if (mixIns)
      mixIns[name].trigger(target, event);
  };

}

// ##########
// Class: EventListenerMixIn
function EventListenerMixIn(options) {
  var listeners = [];

  function onEvent(event, target) {
/*     Utils.log('event = ' + event.type + ', mixInto = ' + options.mixInto); */
/*     Utils.log(options); */
    if (listeners) {
      if (options.filter)
        event = options.filter.call(this, event);
      if (event) {
        if (!target)
          target = options.mixInto;
        var listenersCopy = listeners.slice();
/*         Utils.log(Utils.expandObject(event) + '; ' + listenersCopy.length); */
        for (var i = 0; i < listenersCopy.length; i++)
          try {
/*             Utils.log('telling listener'); */
            listenersCopy[i].call(target, event);
          } catch (e) {
            Utils.log(e);
          }
        if (options.bubbleTo)
          options.bubbleTo.bubble(options.name, target, event);
      }
    }
  };

  options.mixInto[options.name] = function bind(cb) {
//   Utils.trace('bind');
    if (typeof(cb) != "function")
      Utils.log("Callback must be a function.");
    if (listeners)
      listeners.push(cb);
  };

  options.mixInto[options.name].unbind = function unbind(cb) {
    if (listeners) {
      var index = listeners.indexOf(cb);
      if (index != -1)
        listeners.splice(index, 1);
    }
  };

  // ----------
  // Function: trigger
  this.trigger = function trigger(target, event) {
    onEvent(event, target);
  };

  if (options.observe)
    options.observe.addEventListener(options.eventName,
                                     onEvent,
                                     options.useCapture);

}

// ##########
// Class: TabsManager
// Singelton for dealing with the actual tabs in the browser.
window.TabsManager = Utils.extend(new Subscribable(), {
  // ----------
  // Function: init
  // Sets up the TabsManager and window.Tabs
  init: function TabsManager_init() {
    var trackedTabs = [];
    new BrowserWindow();

    window.Tabs = {
      // ----------
      toString: function toString() {
        return "[Tabs]";
      }
    };

    window.Tabs.__proto__ = trackedTabs;

    var tabsMixIns = new EventListenerMixIns(window.Tabs);
    tabsMixIns.add({name: "onReady"});
    tabsMixIns.add({name: "onLoad"});
    tabsMixIns.add({name: "onFocus"});
    tabsMixIns.add({name: "onClose"});
    tabsMixIns.add({name: "onOpen"});
    tabsMixIns.add({name: "onMove"});

    function newBrowserTab(chromeTab) {
      new BrowserTab(chromeTab);
      trackedTabs.push(chromeTab);
    }

    function unloadBrowserTab(chromeTab) {
      let index = trackedTabs.indexOf(chromeTab);
      if (index > -1) {
        trackedTabs.splice(index,1);
      } else {
        Utils.assert("unloadBrowserTab: browserTab not found in trackedTabs",false);
      }
    }

    function BrowserWindow() {
      Array.forEach(gBrowser.tabs, function(tab) newBrowserTab(tab));

      const EVENTS_TO_WATCH = ["TabOpen", "TabMove", "TabClose", "TabSelect"];

      function onEvent(event) {
        // TODO: For some reason, exceptions that are raised outside of this
        // function get eaten, rather than logged, so we're adding our own
        // error logging here.
        try {
          // This is a XUL <tab> element of class tabbrowser-tab.
          var chromeTab = event.originalTarget;

          switch (event.type) {
            case "TabSelect":
              tabsMixIns.bubble("onFocus",
                               chromeTab,
                               true);
              break;

            case "TabOpen":
              newBrowserTab(chromeTab);
              tabsMixIns.bubble("onOpen",
                                chromeTab,
                                true);
              break;

            case "TabMove":
              tabsMixIns.bubble("onMove",
                               chromeTab,
                               true);
              break;

            case "TabClose":
              tabsMixIns.bubble("onClose",
                                chromeTab,
                                true);
              unloadBrowserTab(chromeTab);
              break;
          }
        } catch (e) {
          Utils.log(e);
        }
      }

      EVENTS_TO_WATCH.forEach(
        function(eventType) {
          gBrowser.tabContainer.addEventListener(eventType, onEvent, true);
        });
    }

    function BrowserTab(chromeTab) {
      var browser = chromeTab.linkedBrowser;
      var mixIns = new EventListenerMixIns(this);
      mixIns.add(
        {name: "onReady",
         observe: browser,
         eventName: "DOMContentLoaded",
         useCapture: true,
         bubbleTo: tabsMixIns,
         filter: function(event) {
           // Return the document that just loaded.
           event.tab = chromeTab;
           return event;
         }});

      mixIns.add(
        {name: "onLoad",
         observe: browser,
         eventName: "load",
         useCapture: true,
         bubbleTo: tabsMixIns,
         filter: function(event) {
           // Return the document that just loaded.
           event.tab = chromeTab;
           return event;
         }});
    }

    this._sendToSubscribers('load');
  }
});

// ----------
window.TabsManager.init();

})();
