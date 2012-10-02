/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DEVTM_H
#define DEVTM_H

#ifdef DEBUG
static const char DEVTM_CVS_ID[] = "@(#) $RCSfile: devtm.h,v $ $Revision: 1.5 $ $Date: 2012/04/25 14:49:42 $";
#endif /* DEBUG */

/*
 * devtm.h
 *
 * This file contains module-private definitions for the low-level 
 * cryptoki devices.
 */

#ifndef DEVT_H
#include "devt.h"
#endif /* DEVT_H */

PR_BEGIN_EXTERN_C

#define MAX_LOCAL_CACHE_OBJECTS 10

PR_END_EXTERN_C

#endif /* DEVTM_H */
