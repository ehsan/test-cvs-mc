#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.


# can't do this in manifest.mn because OS_TARGET isn't defined there.
ifeq (,$(filter-out WIN%,$(OS_TARGET)))

# don't want the 32 in the shared library name
SHARED_LIBRARY = $(OBJDIR)/$(DLL_PREFIX)$(LIBRARY_NAME)$(LIBRARY_VERSION).$(DLL_SUFFIX)
IMPORT_LIBRARY = $(OBJDIR)/$(IMPORT_LIB_PREFIX)$(LIBRARY_NAME)$(LIBRARY_VERSION)$(IMPORT_LIB_SUFFIX)

#RES = $(OBJDIR)/$(LIBRARY_NAME).res
#RESNAME = $(LIBRARY_NAME).rc
endif

ifeq ($(OS_TARGET),AIX)
EXTRA_LIBS += -lpthreads
ifdef BUILD_OPT
OPTIMIZER=
endif
endif

ifeq ($(OS_TARGET),SunOS)
OS_LIBS += -lbsm 
endif

ifeq ($(OS_TARGET),Darwin)
# These version numbers come from the -version-info 8:6:8 libtool option in
# sqlite upstream's Makefile.in.  (Given -version-info current:revision:age,
# libtool passes
#     -compatibility_version current+1 -current_version current+1.revision
# to the linker.)  Apple builds the system libsqlite3.dylib with these
# version numbers, so we use the same to be compatible.
DARWIN_DYLIB_VERSIONS = -compatibility_version 9 -current_version 9.6
endif
