/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: manchor.c,v $ $Revision: 1.2 $ $Date: 2012/04/25 14:49:40 $";
#endif /* DEBUG */

/*
 * nssmkey/manchor.c
 *
 * This file "anchors" the actual cryptoki entry points in this module's
 * shared library, which is required for dynamic loading.  See the 
 * comments in nssck.api for more information.
 */

#include "ckmk.h"

#define MODULE_NAME ckmk
#define INSTANCE_NAME (NSSCKMDInstance *)&nss_ckmk_mdInstance
#include "nssck.api"
