#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies that files generated by two-steps-removed actions are built before
dependent compile steps.
"""

import os
import sys
import TestGyp

# This test is Ninja-specific in that:
# - the bug only showed nondeterministically in parallel builds;
# - it relies on a ninja-specific output file path.

test = TestGyp.TestGyp(formats=['ninja'])
test.run_gyp('chained-dependency.gyp')
objext = '.obj' if sys.platform == 'win32' else '.o'
test.build('chained-dependency.gyp',
           os.path.join('obj', 'chained.chained' + objext))
# The test passes if the .o file builds successfully.
test.pass_test()
