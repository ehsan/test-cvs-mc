#!/usr/bin/env python

"""
Fix references to source files of the form [LOCpath]
so that they are relative to a given source directory.
"""

import os, sys, re

(srcdir, ) = sys.argv[1:]
srcdir = os.path.realpath(srcdir)

f = re.compile(r'\[LOC(.*?)\]')

def replacer(m):
    file = m.group(1)
    file = os.path.realpath(file)
    if not file.startswith(srcdir):
        raise Exception("File %s doesn't start with %s" % (file, srcdir))

    file = file[len(srcdir) + 1:]
    return file

for line in sys.stdin:
    line = f.sub(replacer, line)
    sys.stdout.write(line)

