# 
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
# 
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
#ident	"$Id: Makefile.com,v 1.9 2012/03/06 13:13:40 gerv%gerv.net Exp $"
#

MACH = $(shell mach)

PUBLISH_ROOT = $(DIST)
ifeq ($(MOD_DEPTH),../..)
ROOT = ROOT
else
ROOT = $(subst ../../,,$(MOD_DEPTH))/ROOT
endif

PKGARCHIVE = $(dist_prefix)/pkgarchive
DATAFILES = copyright
FILES = $(DATAFILES) pkginfo

PACKAGE = $(shell basename `pwd`)

PRODUCT_VERSION = $(shell grep PR_VERSION $(dist_includedir)/prinit.h \
		   | sed -e 's/"$$//' -e 's/.*"//' -e 's/ .*//')

LN = /usr/bin/ln
CP = /usr/bin/cp

CLOBBERFILES = $(FILES)

include $(topsrcdir)/config/rules.mk

# vim: ft=make
