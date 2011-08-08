/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is thebes gfx code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Benoit Girard <bgirard@mozilla.com>
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

#define MAC_OS_X_VERSION_MASK     0x0000FFFF // Not supported
#define MAC_OS_X_VERSION_10_4_HEX 0x00001040 // Not supported
#define MAC_OS_X_VERSION_10_5_HEX 0x00001050
#define MAC_OS_X_VERSION_10_6_HEX 0x00001060
#define MAC_OS_X_VERSION_10_7_HEX 0x00001070

// This API will not work for OS X 10.10, see Gestalt.h.

#include "nsCocoaFeatures.h"
#include "nsDebug.h"

#import <Cocoa/Cocoa.h>

PRInt32 nsCocoaFeatures::mOSXVersion = 0;

/* static */ PRInt32
nsCocoaFeatures::OSXVersion()
{
    if (!mOSXVersion) {
        // minor version is not accurate, use gestaltSystemVersionMajor, 
        // gestaltSystemVersionMinor, gestaltSystemVersionBugFix for these
        OSErr err = ::Gestalt(gestaltSystemVersion, reinterpret_cast<SInt32*>(&mOSXVersion));
        if (err != noErr) {
            // This should probably be changed when our minimum version changes
            NS_ERROR("Couldn't determine OS X version, assuming 10.5");
            mOSXVersion = MAC_OS_X_VERSION_10_5_HEX;
        }
        mOSXVersion &= MAC_OS_X_VERSION_MASK;
    }
    return mOSXVersion;
}

/* static */ PRBool
nsCocoaFeatures::OnSnowLeopardOrLater()
{
    return (OSXVersion() >= MAC_OS_X_VERSION_10_6_HEX);
}

/* static */ PRBool
nsCocoaFeatures::OnLionOrLater()
{
    return (OSXVersion() >= MAC_OS_X_VERSION_10_7_HEX);
}

