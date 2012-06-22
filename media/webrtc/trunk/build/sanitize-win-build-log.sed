#!/bin/echo Use sanitize-win-build-log.sh or sed -f

# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Use this sed script to reduce a Windows build log into something
# machine-parsable.

# Drop uninformative lines.
/The operation completed successfully./d

# Drop parallelization indicators on lines.
s/^[0-9]\+>//
