/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Red Hat, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2012
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Kai Engert <kengert@redhat.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsNSSVersion.h"
#include "nsString.h"
#include "prinit.h"
#include "nss.h"
#include "nssutil.h"
#include "ssl.h"
#include "smime.h"

NS_IMPL_ISUPPORTS1(nsNSSVersion, nsINSSVersion)

nsNSSVersion::nsNSSVersion()
{
}

nsNSSVersion::~nsNSSVersion()
{
}

NS_IMETHODIMP
nsNSSVersion::GetNSPR_Version(nsAString & v)
{
    CopyUTF8toUTF16(PR_GetVersion(), v);
    return NS_OK;
}

NS_IMETHODIMP
nsNSSVersion::GetNSS_Version(nsAString & v)
{
    CopyUTF8toUTF16(NSS_GetVersion(), v);
    return NS_OK;
}

NS_IMETHODIMP
nsNSSVersion::GetNSSUTIL_Version(nsAString & v)
{
    CopyUTF8toUTF16(NSSUTIL_GetVersion(), v);
    return NS_OK;
}

NS_IMETHODIMP
nsNSSVersion::GetNSSSSL_Version(nsAString & v)
{
    CopyUTF8toUTF16(NSSSSL_GetVersion(), v);
    return NS_OK;
}

NS_IMETHODIMP
nsNSSVersion::GetNSSSMIME_Version(nsAString & v)
{
    CopyUTF8toUTF16(NSSSMIME_GetVersion(), v);
    return NS_OK;
}

NS_IMETHODIMP
nsNSSVersion::GetNSPR_MinVersion(nsAString & v)
{
    CopyUTF8toUTF16(PR_VERSION, v);
    return NS_OK;
}

NS_IMETHODIMP
nsNSSVersion::GetNSS_MinVersion(nsAString & v)
{
    CopyUTF8toUTF16(NSS_VERSION, v);
    return NS_OK;
}

NS_IMETHODIMP
nsNSSVersion::GetNSSUTIL_MinVersion(nsAString & v)
{
    CopyUTF8toUTF16(NSSUTIL_VERSION, v);
    return NS_OK;
}

NS_IMETHODIMP
nsNSSVersion::GetNSSSSL_MinVersion(nsAString & v)
{
    CopyUTF8toUTF16(NSS_VERSION, v);
    return NS_OK;
}

NS_IMETHODIMP
nsNSSVersion::GetNSSSMIME_MinVersion(nsAString & v)
{
    CopyUTF8toUTF16(NSS_VERSION, v);
    return NS_OK;
}
