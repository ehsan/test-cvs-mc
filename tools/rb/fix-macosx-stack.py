#!/usr/bin/python
# vim:sw=4:ts=4:et:
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
# The Original Code is fix-linux-stack.pl.
#
# The Initial Developer of the Original Code is L. David Baron.
# Portions created by the Initial Developer are Copyright (C) 2003
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   L. David Baron <dbaron@dbaron.org> (original author)
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

# This script uses atos to process the output of nsTraceRefcnt's Mac OS
# X stack walking code.  This is useful for two things:
#  (1) Getting line number information out of
#      |nsTraceRefcntImpl::WalkTheStack|'s output in debug builds.
#  (2) Getting function names out of |nsTraceRefcntImpl::WalkTheStack|'s
#      output on all builds (where it mostly prints UNKNOWN because only
#      a handful of symbols are exported from component libraries).
#
# Use the script by piping output containing stacks (such as raw stacks
# or make-tree.pl balance trees) through this script.

import subprocess
import sys
import re
import os.path

def separate_debug_file_for(file):
    return None

address_adjustments = {}
def address_adjustment(file):
    if not file in address_adjustments:
        result = None
        otool = subprocess.Popen(["otool", "-l", file], stdout=subprocess.PIPE)
        while True:
            line = otool.stdout.readline()
            if line == "":
                break
            if line == "  segname __TEXT\n":
                line = otool.stdout.readline()
                if not line.startswith("   vmaddr "):
                    raise StandardError("unexpected otool output")
                result = int(line[10:], 16)
                break
        otool.stdout.close()

        if result is None:
            raise StandardError("unexpected otool output")

        address_adjustments[file] = result

    return address_adjustments[file]

# Return a Popen object for an atos process that gives symbol
# information for a file.
atoses = {}
def atos_proc(file):
    pipe = None
    if not file in atoses:
        debug_file = separate_debug_file_for(file) or file
        pipe = subprocess.Popen(['/usr/bin/atos', '-o', debug_file],
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE)
        # COMMENTED OUT DUE TO BUFFERING PROBLEMS
        #atoses[file] = pipe
    else:
        pipe = atoses[file]
    return pipe

cxxfilt_proc = None
def cxxfilt(sym):
    if cxxfilt_proc is None:
        globals()["cxxfilt_proc"] = subprocess.Popen(['c++filt',
                                                      '--no-strip-underscores',
                                                      '--format', 'gnu-v3'],
                                                     stdin=subprocess.PIPE,
                                                     stdout=subprocess.PIPE)
    # strip underscores ourselves (workes better than c++filt's
    # --strip-underscores
    cxxfilt_proc.stdin.write(sym[1:] + "\n")
    return cxxfilt_proc.stdout.readline().rstrip("\n")

line_re = re.compile("^([ \|0-9-]*)(.*) ?\[([^ ]*) \+(0x[0-9A-F]{1,8})\](.*)$")
atos_sym_re = re.compile("^(\S+) \(in ([^)]+)\) \((.+)\)$")
for line in sys.stdin:
    result = line_re.match(line)
    if result is not None:
        # before allows preservation of balance trees
        # after allows preservation of counts
        (before, badsymbol, file, address, after) = result.groups()
        address = int(address, 16)

        if os.path.exists(file) and os.path.isfile(file):
            atos = atos_proc(file)
            address += address_adjustment(file)

            atos.stdin.write("0x%X\n" % address)
            atos.stdin.flush()
            # close() TO WORK AROUND BUFFERING PROBLEMS
            # (SEE COMMENTED OUT CACHING ABOVE)
            atos.stdin.close()
            info = atos.stdout.readline().rstrip("\n")

            # atos output seems to have three forms:
            #   address
            #   address (in foo.dylib)
            #   symbol (in foo.dylib) (file:line)
            symresult = atos_sym_re.match(info)
            if symresult is not None:
                # Print the first two forms as-is, and transform the third
                (symbol, library, fileline) = symresult.groups()
                symbol = cxxfilt(symbol)
                info = "%s (%s, in %s)" % (symbol, fileline, library)

            sys.stdout.write(before + info + after + "\n")
        else:
            sys.stderr.write("Warning: File \"" + file + "\" does not exist.\n")
            sys.stdout.write(line)
    else:
        sys.stdout.write(line)
