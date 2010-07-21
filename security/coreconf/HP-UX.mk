#
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
# The Original Code is the Netscape security libraries.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1994-2000
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
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

#
# Config stuff for HP-UX
#

include $(CORE_DEPTH)/coreconf/UNIX.mk

DEFAULT_COMPILER = cc

ifeq ($(OS_TEST),ia64)
	CPU_ARCH = ia64
	CPU_TAG = _$(CPU_ARCH)
	ifneq ($(USE_64),1)
		64BIT_TAG = _32
	endif
	DLL_SUFFIX = so
else
	CPU_ARCH = hppa
	DLL_SUFFIX = sl
endif
CC         = cc
CCC        = CC
ifndef NS_USE_GCC
OS_CFLAGS  += -Ae
endif
OS_CFLAGS  += $(DSO_CFLAGS) -DHPUX -D$(CPU_ARCH) -D_HPUX_SOURCE -D_USE_BIG_FDS

ifeq ($(DEFAULT_IMPL_STRATEGY),_PTH)
	USE_PTHREADS = 1
	ifeq ($(CLASSIC_NSPR),1)
		USE_PTHREADS =
		IMPL_STRATEGY = _CLASSIC
	endif
	ifeq ($(PTHREADS_USER),1)
		USE_PTHREADS =
		IMPL_STRATEGY = _PTH_USER
	endif
endif

ifdef PTHREADS_USER
	OS_CFLAGS	+= -D_POSIX_C_SOURCE=199506L
endif

LDFLAGS			= -z -Wl,+s

ifdef NS_USE_GCC
LD = $(CC)
endif
MKSHLIB			= $(LD) $(DSO_LDOPTS) $(RPATH)
ifdef MAPFILE
ifndef NS_USE_GCC
MKSHLIB += -c $(MAPFILE)
else
MKSHLIB += -Wl,-c,$(MAPFILE)
endif
endif
PROCESS_MAP_FILE = grep -v ';+' $< | grep -v ';-' | \
         sed -e 's; DATA ;;' -e 's,;;,,' -e 's,;.*,,' -e 's,^,+e ,' > $@

ifndef NS_USE_GCC
DSO_LDOPTS		= -b +h $(notdir $@)
RPATH			= +b '$$ORIGIN'
else
DSO_LDOPTS		= -shared -Wl,+h,$(notdir $@)
RPATH			= -Wl,+b,'$$ORIGIN'
endif
ifneq ($(OS_TEST),ia64)
# pa-risc
ifndef USE_64
RPATH			=
endif
endif

# +Z generates position independent code for use in shared libraries.
ifndef NS_USE_GCC
DSO_CFLAGS = +Z
else
DSO_CFLAGS = -fPIC
ASFLAGS   += -x assembler-with-cpp
endif
