# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import string

propList = eval(sys.stdin.read())
props = ""
for [prop, pref] in propList:
    extendedAttrs = ["TreatNullAs=EmptyString"]
    if pref is not "":
        extendedAttrs.append("Pref=%s" % pref)
    if not prop.startswith("Moz"):
        prop = prop[0].lower() + prop[1:]
    # Unfortunately, even some of the getters here are fallible
    # (e.g. on nsComputedDOMStyle).
    props += "  [%s] attribute DOMString %s;\n" % (", ".join(extendedAttrs),
                                                   prop)

idlFile = open(sys.argv[1], "r");
idlTemplate = idlFile.read();
idlFile.close();

print ("/* THIS IS AN AUTOGENERATED FILE.  DO NOT EDIT */\n\n" +
       string.Template(idlTemplate).substitute({ "props": props }))
