/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsISO88592ToUnicode_h___
#define nsISO88592ToUnicode_h___

#include "nsISupports.h"

/**
 * A character set converter from ISO88592 to Unicode.
 *
 * @created         05/Apr/1999
 * @author  Catalin Rotaru [CATA]
 */
nsresult
nsISO88592ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult);

#endif /* nsISO88592ToUnicode_h___ */
