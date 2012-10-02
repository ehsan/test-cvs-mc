/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// see http://mxr.mozilla.org/mozilla-central/source/services/sync/Weave.js#76

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

const rph = Services.io.getProtocolHandler("resource").QueryInterface(Ci.nsIResProtocolHandler);

function endsWith(str, end) {
  return str.slice(-end.length) == end;
}

function jar_entries(jarReader, pattern) {
  var entries = [];
  var enumerator = jarReader.findEntries(pattern);
  while (enumerator.hasMore()) {
    entries.push(enumerator.getNext());
  }
  return entries;
}

function dir_entries(baseDir, subpath, ext) {
  var dir = baseDir.clone();
  dir.append(subpath);
  var enumerator = dir.directoryEntries;
  var entries = [];
  while (enumerator.hasMoreElements()) {
    var file = enumerator.getNext().QueryInterface(Ci.nsIFile);
    if (file.isDirectory()) {
      entries = entries.concat(dir_entries(dir, file.leafName, ext).map(function(p) subpath + "/" + p));
    } else if (endsWith(file.leafName, ext)) {
      entries.push(subpath + "/" + file.leafName);
    }
  }
  return entries;
}

function get_modules_under(uri) {
  if (uri instanceof Ci.nsIJARURI) {
    var jar = uri.QueryInterface(Ci.nsIJARURI);
    var jarReader = Cc["@mozilla.org/libjar/zip-reader;1"].createInstance(Ci.nsIZipReader);
    var file = jar.JARFile.QueryInterface(Ci.nsIFileURL);
    jarReader.open(file.file);
    var entries = jar_entries(jarReader, "components/*.js")
                  .concat(jar_entries(jarReader, "modules/*.js"))
                  .concat(jar_entries(jarReader, "modules/*.jsm"));
    jarReader.close();
    return entries;
  } else if (uri instanceof Ci.nsIFileURL){
    var file = uri.QueryInterface(Ci.nsIFileURL);
    return dir_entries(file.file, "components", ".js")
           .concat(dir_entries(file.file, "modules", ".js"))
           .concat(dir_entries(file.file, "modules", ".jsm"));
  } else {
    throw "Expected a nsIJARURI or nsIFileURL";
  }
}

function load_modules_under(spec, uri) {
  var entries = get_modules_under(uri);
  for each (let entry in entries) {
    try {
      dump(spec + entry + "\n");
      Cu.import(spec + entry, null);
    } catch(e) {}
  }
}

function resolveResource(spec) {
  var uri = Services.io.newURI(spec, null, null);
  return Services.io.newURI(rph.resolveURI(uri), null, null);
}

function populate_startupcache() {
  var greURI = resolveResource("resource://gre/");
  var appURI = resolveResource("resource://app/");

  load_modules_under("resource://gre/", greURI);
  if (!appURI.equals(greURI))
    load_modules_under("resource://app/", appURI);
}


