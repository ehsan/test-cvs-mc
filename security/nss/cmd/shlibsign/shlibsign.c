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
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

/*
 * shlibsign creates the checksum (.chk) files for the NSS libraries,
 * libsoftokn3/softokn3 and libfreebl/freebl (platforms can have 
 * multiple freebl variants), that contain the NSS cryptograhic boundary.
 *
 * The generated .chk files must be put in the same directory as
 * the NSS libraries they were generated for.
 *
 * When in FIPS 140 mode, the NSS Internal FIPS PKCS #11 Module will
 * compute the checksum for the NSS cryptographic boundary libraries
 * and compare the checksum with the value in .chk file.
 *
 * $Id: shlibsign.c,v 1.18 2008/11/20 15:44:12 glen.beasley%sun.com Exp $
 */

#ifdef XP_UNIX
#define USES_LINKS 1
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef USES_LINKS
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

/* nspr headers */
#include "prlink.h"
#include "prprf.h"
#include "prenv.h"
#include "plgetopt.h"
#include "prinit.h"
#include "prmem.h"
#include "plstr.h"
#include "prerror.h"

/* softoken headers */
#include "pkcs11.h"
#include "pkcs11t.h"

/* freebl headers */
#include "shsign.h"

#define NUM_ELEM(array) (sizeof(array)/sizeof(array[0]))
CK_BBOOL true = CK_TRUE;
CK_BBOOL false = CK_FALSE;
static PRBool verbose = PR_FALSE;

static void
usage (const char *program_name)
{
    PRFileDesc *debug_out = PR_GetSpecialFD(PR_StandardError);
    PR_fprintf (debug_out,
                "type %s -H for more detail information.\n", program_name);
    PR_fprintf (debug_out,
                "Usage: %s [-v] [-V] [-o outfile] [-d dbdir] [-f pwfile]\n"
                "          [-F] [-p pwd] -[P dbprefix ] "
                "-i shared_library_name\n",
                program_name);
    exit(1);
}

static void 
long_usage(const char *program_name) 
{
    PRFileDesc *debug_out = PR_GetSpecialFD(PR_StandardError);
    PR_fprintf(debug_out, "%s test program usage:\n", program_name);
    PR_fprintf(debug_out, "\t-i <infile>  shared_library_name to process\n");
    PR_fprintf(debug_out, "\t-o <outfile> checksum outfile\n");
    PR_fprintf(debug_out, "\t-d <path>    database path location\n");
    PR_fprintf(debug_out, "\t-P <prefix>  database prefix\n");
    PR_fprintf(debug_out, "\t-f <file>    password File : echo pw > file \n");
    PR_fprintf(debug_out, "\t-F           FIPS mode\n"); 
    PR_fprintf(debug_out, "\t-p <pwd>     password\n");
    PR_fprintf(debug_out, "\t-v           verbose output\n");
    PR_fprintf(debug_out, "\t-V           perform Verify operations\n");
    PR_fprintf(debug_out, "\t-?           short help message\n");
    PR_fprintf(debug_out, "\t-h           short help message\n");
    PR_fprintf(debug_out, "\t-H           this help message\n");
    PR_fprintf(debug_out, "\n\n\tNote: Use of FIPS mode requires your ");
    PR_fprintf(debug_out, "library path is using \n");
    PR_fprintf(debug_out, "\t      pre-existing libraries with generated ");
    PR_fprintf(debug_out, "checksum files\n");
    PR_fprintf(debug_out, "\t      and database in FIPS mode \n");
    exit(1);
}

static char * 
mkoutput(const char *input)
{
    int in_len = strlen(input);
    char *output = PR_Malloc(in_len+sizeof(SGN_SUFFIX));
    int index = in_len + 1 - sizeof("."SHLIB_SUFFIX);

    if ((index > 0) && 
        (PL_strncmp(&input[index],
                 "."SHLIB_SUFFIX,sizeof("."SHLIB_SUFFIX)) == 0)) {
        in_len = index;
    }
    memcpy(output,input,in_len);
    memcpy(&output[in_len],SGN_SUFFIX,sizeof(SGN_SUFFIX));
    return output;
}

static void 
lperror(const char *string) {
    PRErrorCode errorcode;

    errorcode = PR_GetError();
    PR_fprintf(PR_STDERR, "%s: %d: %s\n", string, errorcode,
                PR_ErrorToString(errorcode, PR_LANGUAGE_I_DEFAULT));
}

static void
encodeInt(unsigned char *buf, int val)
{
    buf[3] = (val >> 0) & 0xff;
    buf[2] = (val >>  8) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[0] = (val >> 24) & 0xff;
    return;
}

static PRStatus 
writeItem(PRFileDesc *fd, CK_VOID_PTR pValue,
          CK_ULONG ulValueLen, char *file)
{
    unsigned char buf[4];
    int bytesWritten;
    if (ulValueLen == 0) {
        PR_fprintf(PR_STDERR, "call to writeItem with 0 bytes of data.\n");
        return PR_FAILURE;
    }

    encodeInt(buf,ulValueLen);
    bytesWritten = PR_Write(fd,buf, 4);
    if (bytesWritten != 4) {
        lperror(file);
        return PR_FAILURE;
    }
    bytesWritten = PR_Write(fd, pValue, ulValueLen);
    if (bytesWritten != ulValueLen) {
        lperror(file);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

static const unsigned char prime[] = { 0x00,
   0x97, 0x44, 0x1d, 0xcc, 0x0d, 0x39, 0x0d, 0x8d, 
   0xcb, 0x75, 0xdc, 0x24, 0x25, 0x6f, 0x01, 0x92, 
   0xa1, 0x11, 0x07, 0x6b, 0x70, 0xac, 0x73, 0xd7, 
   0x82, 0x28, 0xdf, 0xab, 0x82, 0x0c, 0x41, 0x0c, 
   0x95, 0xb3, 0x3c, 0x3d, 0xea, 0x8a, 0xe6, 0x44, 
   0x0a, 0xb8, 0xab, 0x90, 0x15, 0x41, 0x11, 0xe8, 
   0x48, 0x7b, 0x8d, 0xb0, 0x9c, 0xd3, 0xf2, 0x69, 
   0x66, 0xff, 0x66, 0x4b, 0x70, 0x2b, 0xbf, 0xfb, 
   0xd6, 0x68, 0x85, 0x76, 0x1e, 0x34, 0xaa, 0xc5, 
   0x57, 0x6e, 0x23, 0x02, 0x08, 0x60, 0x6e, 0xfd, 
   0x67, 0x76, 0xe1, 0x7c, 0xc8, 0xcb, 0x51, 0x77, 
   0xcf, 0xb1, 0x3b, 0x00, 0x2e, 0xfa, 0x21, 0xcd, 
   0x34, 0x76, 0x75, 0x01, 0x19, 0xfe, 0xf8, 0x5d, 
   0x43, 0xc5, 0x34, 0xf3, 0x7a, 0x95, 0xdc, 0xc2, 
   0x58, 0x07, 0x19, 0x2f, 0x1d, 0x6f, 0x9a, 0x77, 
   0x7e, 0x55, 0xaa, 0xe7, 0x5a, 0x50, 0x43, 0xd3 };

static const unsigned char subprime[] = { 0x0,
   0xd8, 0x16, 0x23, 0x34, 0x8a, 0x9e, 0x3a, 0xf5, 
   0xd9, 0x10, 0x13, 0x35, 0xaa, 0xf3, 0xf3, 0x54, 
   0x0b, 0x31, 0x24, 0xf1 };

static const unsigned char base[] = { 
    0x03, 0x3a, 0xad, 0xfa, 0x3a, 0x0c, 0xea, 0x0a, 
    0x4e, 0x43, 0x32, 0x92, 0xbb, 0x87, 0xf1, 0x11, 
    0xc0, 0xad, 0x39, 0x38, 0x56, 0x1a, 0xdb, 0x23, 
    0x66, 0xb1, 0x08, 0xda, 0xb6, 0x19, 0x51, 0x42, 
    0x93, 0x4f, 0xc3, 0x44, 0x43, 0xa8, 0x05, 0xc1, 
    0xf8, 0x71, 0x62, 0x6f, 0x3d, 0xe2, 0xab, 0x6f, 
    0xd7, 0x80, 0x22, 0x6f, 0xca, 0x0d, 0xf6, 0x9f, 
    0x45, 0x27, 0x83, 0xec, 0x86, 0x0c, 0xda, 0xaa, 
    0xd6, 0xe0, 0xd0, 0x84, 0xfd, 0xb1, 0x4f, 0xdc, 
    0x08, 0xcd, 0x68, 0x3a, 0x77, 0xc2, 0xc5, 0xf1, 
    0x99, 0x0f, 0x15, 0x1b, 0x6a, 0x8c, 0x3d, 0x18, 
    0x2b, 0x6f, 0xdc, 0x2b, 0xd8, 0xb5, 0x9b, 0xb8, 
    0x2d, 0x57, 0x92, 0x1c, 0x46, 0x27, 0xaf, 0x6d, 
    0xe1, 0x45, 0xcf, 0x0b, 0x3f, 0xfa, 0x07, 0xcc, 
    0x14, 0x8e, 0xe7, 0xb8, 0xaa, 0xd5, 0xd1, 0x36, 
    0x1d, 0x7e, 0x5e, 0x7d, 0xfa, 0x5b, 0x77, 0x1f };

static const unsigned char h[] = { 
    0x41, 0x87, 0x47, 0x79, 0xd8, 0xba, 0x4e, 0xac, 
    0x44, 0x4f, 0x6b, 0xd2, 0x16, 0x5e, 0x04, 0xc6, 
    0xc2, 0x29, 0x93, 0x5e, 0xbd, 0xc7, 0xa9, 0x8f, 
    0x23, 0xa1, 0xc8, 0xee, 0x80, 0x64, 0xd5, 0x67, 
    0x3c, 0xba, 0x59, 0x9a, 0x06, 0x0c, 0xcc, 0x29, 
    0x56, 0xc0, 0xb2, 0x21, 0xe0, 0x5b, 0x52, 0xcd, 
    0x84, 0x73, 0x57, 0xfd, 0xd8, 0xc3, 0x5b, 0x13, 
    0x54, 0xd7, 0x4a, 0x06, 0x86, 0x63, 0x09, 0xa5, 
    0xb0, 0x59, 0xe2, 0x32, 0x9e, 0x09, 0xa3, 0x9f, 
    0x49, 0x62, 0xcc, 0xa6, 0xf9, 0x54, 0xd5, 0xb2, 
    0xc3, 0x08, 0x71, 0x7e, 0xe3, 0x37, 0x50, 0xd6, 
    0x7b, 0xa7, 0xc2, 0x60, 0xc1, 0xeb, 0x51, 0x32, 
    0xfa, 0xad, 0x35, 0x25, 0x17, 0xf0, 0x7f, 0x23, 
    0xe5, 0xa8, 0x01, 0x52, 0xcf, 0x2f, 0xd9, 0xa9, 
    0xf6, 0x00, 0x21, 0x15, 0xf1, 0xf7, 0x70, 0xb7, 
    0x57, 0x8a, 0xd0, 0x59, 0x6a, 0x82, 0xdc, 0x9c };

static const unsigned char seed[] = { 0x00,
    0xcc, 0x4c, 0x69, 0x74, 0xf6, 0x72, 0x24, 0x68, 
    0x24, 0x4f, 0xd7, 0x50, 0x11, 0x40, 0x81, 0xed, 
    0x19, 0x3c, 0x8a, 0x25, 0xbc, 0x78, 0x0a, 0x85, 
    0x82, 0x53, 0x70, 0x20, 0xf6, 0x54, 0xa5, 0x1b, 
    0xf4, 0x15, 0xcd, 0xff, 0xc4, 0x88, 0xa7, 0x9d, 
    0xf3, 0x47, 0x1c, 0x0a, 0xbe, 0x10, 0x29, 0x83, 
    0xb9, 0x0f, 0x4c, 0xdf, 0x90, 0x16, 0x83, 0xa2, 
    0xb3, 0xe3, 0x2e, 0xc1, 0xc2, 0x24, 0x6a, 0xc4, 
    0x9d, 0x57, 0xba, 0xcb, 0x0f, 0x18, 0x75, 0x00, 
    0x33, 0x46, 0x82, 0xec, 0xd6, 0x94, 0x77, 0xc3, 
    0x4f, 0x4c, 0x58, 0x1c, 0x7f, 0x61, 0x3c, 0x36, 
    0xd5, 0x2f, 0xa5, 0x66, 0xd8, 0x2f, 0xce, 0x6e, 
    0x8e, 0x20, 0x48, 0x4a, 0xbb, 0xe3, 0xe0, 0xb2, 
    0x50, 0x33, 0x63, 0x8a, 0x5b, 0x2d, 0x6a, 0xbe, 
    0x4c, 0x28, 0x81, 0x53, 0x5b, 0xe4, 0xf6, 0xfc, 
    0x64, 0x06, 0x13, 0x51, 0xeb, 0x4a, 0x91, 0x9c };

static const unsigned int counter=1496;

struct tuple_str {
    CK_RV         errNum;
    const char * errString;
};

typedef struct tuple_str tuple_str;

static const tuple_str errStrings[] = {
{CKR_OK                              , "CKR_OK                              "},
{CKR_CANCEL                          , "CKR_CANCEL                          "},
{CKR_HOST_MEMORY                     , "CKR_HOST_MEMORY                     "},
{CKR_SLOT_ID_INVALID                 , "CKR_SLOT_ID_INVALID                 "},
{CKR_GENERAL_ERROR                   , "CKR_GENERAL_ERROR                   "},
{CKR_FUNCTION_FAILED                 , "CKR_FUNCTION_FAILED                 "},
{CKR_ARGUMENTS_BAD                   , "CKR_ARGUMENTS_BAD                   "},
{CKR_NO_EVENT                        , "CKR_NO_EVENT                        "},
{CKR_NEED_TO_CREATE_THREADS          , "CKR_NEED_TO_CREATE_THREADS          "},
{CKR_CANT_LOCK                       , "CKR_CANT_LOCK                       "},
{CKR_ATTRIBUTE_READ_ONLY             , "CKR_ATTRIBUTE_READ_ONLY             "},
{CKR_ATTRIBUTE_SENSITIVE             , "CKR_ATTRIBUTE_SENSITIVE             "},
{CKR_ATTRIBUTE_TYPE_INVALID          , "CKR_ATTRIBUTE_TYPE_INVALID          "},
{CKR_ATTRIBUTE_VALUE_INVALID         , "CKR_ATTRIBUTE_VALUE_INVALID         "},
{CKR_DATA_INVALID                    , "CKR_DATA_INVALID                    "},
{CKR_DATA_LEN_RANGE                  , "CKR_DATA_LEN_RANGE                  "},
{CKR_DEVICE_ERROR                    , "CKR_DEVICE_ERROR                    "},
{CKR_DEVICE_MEMORY                   , "CKR_DEVICE_MEMORY                   "},
{CKR_DEVICE_REMOVED                  , "CKR_DEVICE_REMOVED                  "},
{CKR_ENCRYPTED_DATA_INVALID          , "CKR_ENCRYPTED_DATA_INVALID          "},
{CKR_ENCRYPTED_DATA_LEN_RANGE        , "CKR_ENCRYPTED_DATA_LEN_RANGE        "},
{CKR_FUNCTION_CANCELED               , "CKR_FUNCTION_CANCELED               "},
{CKR_FUNCTION_NOT_PARALLEL           , "CKR_FUNCTION_NOT_PARALLEL           "},
{CKR_FUNCTION_NOT_SUPPORTED          , "CKR_FUNCTION_NOT_SUPPORTED          "},
{CKR_KEY_HANDLE_INVALID              , "CKR_KEY_HANDLE_INVALID              "},
{CKR_KEY_SIZE_RANGE                  , "CKR_KEY_SIZE_RANGE                  "},
{CKR_KEY_TYPE_INCONSISTENT           , "CKR_KEY_TYPE_INCONSISTENT           "},
{CKR_KEY_NOT_NEEDED                  , "CKR_KEY_NOT_NEEDED                  "},
{CKR_KEY_CHANGED                     , "CKR_KEY_CHANGED                     "},
{CKR_KEY_NEEDED                      , "CKR_KEY_NEEDED                      "},
{CKR_KEY_INDIGESTIBLE                , "CKR_KEY_INDIGESTIBLE                "},
{CKR_KEY_FUNCTION_NOT_PERMITTED      , "CKR_KEY_FUNCTION_NOT_PERMITTED      "},
{CKR_KEY_NOT_WRAPPABLE               , "CKR_KEY_NOT_WRAPPABLE               "},
{CKR_KEY_UNEXTRACTABLE               , "CKR_KEY_UNEXTRACTABLE               "},
{CKR_MECHANISM_INVALID               , "CKR_MECHANISM_INVALID               "},
{CKR_MECHANISM_PARAM_INVALID         , "CKR_MECHANISM_PARAM_INVALID         "},
{CKR_OBJECT_HANDLE_INVALID           , "CKR_OBJECT_HANDLE_INVALID           "},
{CKR_OPERATION_ACTIVE                , "CKR_OPERATION_ACTIVE                "},
{CKR_OPERATION_NOT_INITIALIZED       , "CKR_OPERATION_NOT_INITIALIZED       "},
{CKR_PIN_INCORRECT                   , "CKR_PIN_INCORRECT                   "},
{CKR_PIN_INVALID                     , "CKR_PIN_INVALID                     "},
{CKR_PIN_LEN_RANGE                   , "CKR_PIN_LEN_RANGE                   "},
{CKR_PIN_EXPIRED                     , "CKR_PIN_EXPIRED                     "},
{CKR_PIN_LOCKED                      , "CKR_PIN_LOCKED                      "},
{CKR_SESSION_CLOSED                  , "CKR_SESSION_CLOSED                  "},
{CKR_SESSION_COUNT                   , "CKR_SESSION_COUNT                   "},
{CKR_SESSION_HANDLE_INVALID          , "CKR_SESSION_HANDLE_INVALID          "},
{CKR_SESSION_PARALLEL_NOT_SUPPORTED  , "CKR_SESSION_PARALLEL_NOT_SUPPORTED  "},
{CKR_SESSION_READ_ONLY               , "CKR_SESSION_READ_ONLY               "},
{CKR_SESSION_EXISTS                  , "CKR_SESSION_EXISTS                  "},
{CKR_SESSION_READ_ONLY_EXISTS        , "CKR_SESSION_READ_ONLY_EXISTS        "},
{CKR_SESSION_READ_WRITE_SO_EXISTS    , "CKR_SESSION_READ_WRITE_SO_EXISTS    "},
{CKR_SIGNATURE_INVALID               , "CKR_SIGNATURE_INVALID               "},
{CKR_SIGNATURE_LEN_RANGE             , "CKR_SIGNATURE_LEN_RANGE             "},
{CKR_TEMPLATE_INCOMPLETE             , "CKR_TEMPLATE_INCOMPLETE             "},
{CKR_TEMPLATE_INCONSISTENT           , "CKR_TEMPLATE_INCONSISTENT           "},
{CKR_TOKEN_NOT_PRESENT               , "CKR_TOKEN_NOT_PRESENT               "},
{CKR_TOKEN_NOT_RECOGNIZED            , "CKR_TOKEN_NOT_RECOGNIZED            "},
{CKR_TOKEN_WRITE_PROTECTED           , "CKR_TOKEN_WRITE_PROTECTED           "},
{CKR_UNWRAPPING_KEY_HANDLE_INVALID   , "CKR_UNWRAPPING_KEY_HANDLE_INVALID   "},
{CKR_UNWRAPPING_KEY_SIZE_RANGE       , "CKR_UNWRAPPING_KEY_SIZE_RANGE       "},
{CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT, "CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT"},
{CKR_USER_ALREADY_LOGGED_IN          , "CKR_USER_ALREADY_LOGGED_IN          "},
{CKR_USER_NOT_LOGGED_IN              , "CKR_USER_NOT_LOGGED_IN              "},
{CKR_USER_PIN_NOT_INITIALIZED        , "CKR_USER_PIN_NOT_INITIALIZED        "},
{CKR_USER_TYPE_INVALID               , "CKR_USER_TYPE_INVALID               "},
{CKR_USER_ANOTHER_ALREADY_LOGGED_IN  , "CKR_USER_ANOTHER_ALREADY_LOGGED_IN  "},
{CKR_USER_TOO_MANY_TYPES             , "CKR_USER_TOO_MANY_TYPES             "},
{CKR_WRAPPED_KEY_INVALID             , "CKR_WRAPPED_KEY_INVALID             "},
{CKR_WRAPPED_KEY_LEN_RANGE           , "CKR_WRAPPED_KEY_LEN_RANGE           "},
{CKR_WRAPPING_KEY_HANDLE_INVALID     , "CKR_WRAPPING_KEY_HANDLE_INVALID     "},
{CKR_WRAPPING_KEY_SIZE_RANGE         , "CKR_WRAPPING_KEY_SIZE_RANGE         "},
{CKR_WRAPPING_KEY_TYPE_INCONSISTENT  , "CKR_WRAPPING_KEY_TYPE_INCONSISTENT  "},
{CKR_RANDOM_SEED_NOT_SUPPORTED       , "CKR_RANDOM_SEED_NOT_SUPPORTED       "},
{CKR_RANDOM_NO_RNG                   , "CKR_RANDOM_NO_RNG                   "},
{CKR_DOMAIN_PARAMS_INVALID           , "CKR_DOMAIN_PARAMS_INVALID           "},
{CKR_BUFFER_TOO_SMALL                , "CKR_BUFFER_TOO_SMALL                "},
{CKR_SAVED_STATE_INVALID             , "CKR_SAVED_STATE_INVALID             "},
{CKR_INFORMATION_SENSITIVE           , "CKR_INFORMATION_SENSITIVE           "},
{CKR_STATE_UNSAVEABLE                , "CKR_STATE_UNSAVEABLE                "},
{CKR_CRYPTOKI_NOT_INITIALIZED        , "CKR_CRYPTOKI_NOT_INITIALIZED        "},
{CKR_CRYPTOKI_ALREADY_INITIALIZED    , "CKR_CRYPTOKI_ALREADY_INITIALIZED    "},
{CKR_MUTEX_BAD                       , "CKR_MUTEX_BAD                       "},
{CKR_MUTEX_NOT_LOCKED                , "CKR_MUTEX_NOT_LOCKED                "},
{CKR_FUNCTION_REJECTED               , "CKR_FUNCTION_REJECTED               "},
{CKR_VENDOR_DEFINED                  , "CKR_VENDOR_DEFINED                  "},
{0xCE534351                          , "CKR_NETSCAPE_CERTDB_FAILED          "},
{0xCE534352                          , "CKR_NETSCAPE_KEYDB_FAILED           "}

};

static const CK_ULONG numStrings = sizeof(errStrings) / sizeof(tuple_str);

/* Returns constant error string for "CRV".
 * Returns "unknown error" if errNum is unknown.
 */
static const char *
CK_RVtoStr(CK_RV errNum) {
    CK_ULONG low  = 1;
    CK_ULONG high = numStrings - 1;
    CK_ULONG i;
    CK_RV num;
    static int initDone;

    /* make sure table is in  ascending order.
     * binary search depends on it.
     */
    if (!initDone) {
        CK_RV lastNum = CKR_OK;
        for (i = low; i <= high; ++i) {
            num = errStrings[i].errNum;
            if (num <= lastNum) {
                PR_fprintf(PR_STDERR,
                        "sequence error in error strings at item %d\n"
                        "error %d (%s)\n"
                        "should come after \n"
                        "error %d (%s)\n",
                        (int) i, (int) lastNum, errStrings[i-1].errString,
                        (int) num, errStrings[i].errString);
            }
            lastNum = num;
        }
        initDone = 1;
    }

    /* Do binary search of table. */
    while (low + 1 < high) {
        i = (low + high) / 2;
        num = errStrings[i].errNum;
        if (errNum == num)
            return errStrings[i].errString;
        if (errNum < num)
            high = i;
        else
            low = i;
    }
    if (errNum == errStrings[low].errNum)
        return errStrings[low].errString;
    if (errNum == errStrings[high].errNum)
        return errStrings[high].errString;
    return "unknown error";
}

static void 
pk11error(const char *string, CK_RV crv) {
    PRErrorCode errorcode;

    PR_fprintf(PR_STDERR, "%s: 0x%08lX, %-26s\n", string, crv, CK_RVtoStr(crv));

    errorcode = PR_GetError();
    if (errorcode) {
        PR_fprintf(PR_STDERR, "NSPR error code: %d: %s\n", errorcode,
                PR_ErrorToString(errorcode, PR_LANGUAGE_I_DEFAULT));
    }
}

static void 
logIt(const char *fmt, ...) {
    va_list args;

    if (verbose) {
        va_start (args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}

static CK_RV 
softokn_Init(CK_FUNCTION_LIST_PTR pFunctionList, const char * configDir,
            const char * dbPrefix) {

    CK_RV crv = CKR_OK;
    CK_C_INITIALIZE_ARGS initArgs;
    char *moduleSpec = NULL;

    initArgs.CreateMutex = NULL;
    initArgs.DestroyMutex = NULL;
    initArgs.LockMutex = NULL;
    initArgs.UnlockMutex = NULL;
    initArgs.flags = CKF_OS_LOCKING_OK;
    if (configDir) {
        moduleSpec = PR_smprintf("configdir='%s' certPrefix='%s' "
                             "keyPrefix='%s' secmod='secmod.db' flags=ReadOnly ",
                             configDir, dbPrefix, dbPrefix);
    } else {
        moduleSpec = PR_smprintf("configdir='' certPrefix='' keyPrefix='' "
                                 "secmod='' flags=noCertDB, noModDB");
    }
    if (!moduleSpec) {
        PR_fprintf(PR_STDERR, "softokn_Init: out of memory error\n");
        return CKR_HOST_MEMORY;
    } 
    logIt("moduleSpec %s\n", moduleSpec);
    initArgs.LibraryParameters = (CK_CHAR_PTR *) moduleSpec;
    initArgs.pReserved = NULL;

    crv = pFunctionList->C_Initialize(&initArgs);
    if (crv != CKR_OK) {
        pk11error("C_Initialize failed", crv);
        goto cleanup;
    }

cleanup:
    if (moduleSpec) {
        PR_smprintf_free(moduleSpec);
    }

    return crv;
}

static char * 
filePasswd(char *pwFile)
{
    unsigned char phrase[200];
    PRFileDesc *fd;
    PRInt32 nb;
    int i;

    if (!pwFile)
        return 0;

    fd = PR_Open(pwFile, PR_RDONLY, 0);
    if (!fd) {
        lperror(pwFile);
        return NULL;
    }

    nb = PR_Read(fd, phrase, sizeof(phrase));

    PR_Close(fd);
    /* handle the Windows EOL case */
    i = 0;
    while (phrase[i] != '\r' && phrase[i] != '\n' && i < nb) i++;
    phrase[i] = '\0';
    if (nb == 0) {
        PR_fprintf(PR_STDERR,"password file contains no data\n");
        return NULL;
    }
    return (char*) PL_strdup((char*)phrase);
}

static void 
checkPath(char *string)
{
    char *src;
    char *dest;

    /*
     * windows support convert any back slashes to
     * forward slashes.
     */
    for (src=string, dest=string; *src; src++,dest++) {
        if (*src == '\\') {
            *dest = '/';
        }
    }
    dest--;
    /* if the last char is a / set it to 0 */
    if (*dest == '/')
        *dest = 0;

}

static CK_SLOT_ID *
getSlotList(CK_FUNCTION_LIST_PTR pFunctionList,
            CK_ULONG slotIndex) {
    CK_RV crv = CKR_OK;
    CK_SLOT_ID *pSlotList = NULL;
    CK_ULONG slotCount;

    /* Get slot list */
    crv = pFunctionList->C_GetSlotList(CK_FALSE /* all slots */,
                                       NULL, &slotCount);
    if (crv != CKR_OK) {
        pk11error( "C_GetSlotList failed", crv);
        return NULL;
    }

    if (slotIndex >= slotCount) {
        PR_fprintf(PR_STDERR, "provided slotIndex is greater than the slot count.");
        return NULL;
    }

    pSlotList = (CK_SLOT_ID *)PR_Malloc(slotCount * sizeof(CK_SLOT_ID));
    if (!pSlotList) {
        lperror("failed to allocate slot list");
        return NULL;
    }
    crv = pFunctionList->C_GetSlotList(CK_FALSE /* all slots */,
                                       pSlotList, &slotCount);
    if (crv != CKR_OK) {
        pk11error( "C_GetSlotList failed", crv);
        if (pSlotList) PR_Free(pSlotList);
        return NULL;
    }
    return pSlotList;
}

int main(int argc, char **argv)
{
    PLOptState *optstate;
    char *program_name;
    char *libname = NULL;
    PRLibrary *lib;
    PRFileDesc *fd;
    PRStatus rv = PR_SUCCESS;
    const char  *input_file = NULL; /* read/create encrypted data from here */
    char  *output_file = NULL;	/* write new encrypted data here */
    int bytesRead;
    int bytesWritten;
    unsigned char file_buf[512];
    int count=0;
    int i;
    PRBool verify = PR_FALSE;
    static PRBool FIPSMODE = PR_FALSE;

#ifdef USES_LINKS
    int ret;
    struct stat stat_buf;
    char link_buf[MAXPATHLEN+1];
    char *link_file = NULL;
#endif

    char *pwd = NULL;
    char *configDir = NULL;
    char *dbPrefix = NULL;
    char *disableUnload = NULL;

    CK_C_GetFunctionList pC_GetFunctionList;
    CK_TOKEN_INFO tokenInfo;
    CK_FUNCTION_LIST_PTR pFunctionList = NULL;
    CK_RV crv = CKR_OK;
    CK_SESSION_HANDLE hRwSession;
    CK_SLOT_ID *pSlotList = NULL;
    CK_ULONG slotIndex = 0; 
    CK_MECHANISM digestmech;
    CK_ULONG digestLen = 0;
    CK_BYTE digest[20]; /* SHA1_LENGTH */
    CK_BYTE sign[40];   /* DSA SIGNATURE LENGTH */
    CK_ULONG signLen = 0 ;
    CK_MECHANISM signMech = {
        CKM_DSA, NULL, 0
    };

    /*** DSA Key ***/

    CK_MECHANISM dsaKeyPairGenMech;
    CK_ATTRIBUTE dsaPubKeyTemplate[5];
    CK_ATTRIBUTE dsaPrivKeyTemplate[5];
    CK_OBJECT_HANDLE hDSApubKey = CK_INVALID_HANDLE;
    CK_OBJECT_HANDLE hDSAprivKey = CK_INVALID_HANDLE;

    CK_BYTE dsaPubKey[128];
    CK_ATTRIBUTE dsaPubKeyValue;

    /* DSA key init */
    dsaPubKeyTemplate[0].type       = CKA_PRIME;
    dsaPubKeyTemplate[0].pValue     = (CK_VOID_PTR) &prime;
    dsaPubKeyTemplate[0].ulValueLen = sizeof(prime);
    dsaPubKeyTemplate[1].type = CKA_SUBPRIME;
    dsaPubKeyTemplate[1].pValue = (CK_VOID_PTR) &subprime;
    dsaPubKeyTemplate[1].ulValueLen = sizeof(subprime);
    dsaPubKeyTemplate[2].type = CKA_BASE;
    dsaPubKeyTemplate[2].pValue = (CK_VOID_PTR) &base;
    dsaPubKeyTemplate[2].ulValueLen = sizeof(base);
    dsaPubKeyTemplate[3].type = CKA_TOKEN;
    dsaPubKeyTemplate[3].pValue = &false; /* session object */
    dsaPubKeyTemplate[3].ulValueLen = sizeof(false);
    dsaPubKeyTemplate[4].type = CKA_VERIFY;
    dsaPubKeyTemplate[4].pValue = &true;
    dsaPubKeyTemplate[4].ulValueLen = sizeof(true);
    dsaKeyPairGenMech.mechanism      = CKM_DSA_KEY_PAIR_GEN;
    dsaKeyPairGenMech.pParameter = NULL;
    dsaKeyPairGenMech.ulParameterLen = 0;
    dsaPrivKeyTemplate[0].type       = CKA_TOKEN;
    dsaPrivKeyTemplate[0].pValue     = &false; /* session object */
    dsaPrivKeyTemplate[0].ulValueLen = sizeof(false);
    dsaPrivKeyTemplate[1].type       = CKA_PRIVATE;
    dsaPrivKeyTemplate[1].pValue     = &true;
    dsaPrivKeyTemplate[1].ulValueLen = sizeof(true);
    dsaPrivKeyTemplate[2].type       = CKA_SENSITIVE;
    dsaPrivKeyTemplate[2].pValue     = &true; 
    dsaPrivKeyTemplate[2].ulValueLen = sizeof(true);
    dsaPrivKeyTemplate[3].type       = CKA_SIGN,
    dsaPrivKeyTemplate[3].pValue     = &true;
    dsaPrivKeyTemplate[3].ulValueLen = sizeof(true);
    dsaPrivKeyTemplate[4].type       = CKA_EXTRACTABLE;
    dsaPrivKeyTemplate[4].pValue     = &false;
    dsaPrivKeyTemplate[4].ulValueLen = sizeof(false);
    digestmech.mechanism = CKM_SHA_1;
    digestmech.pParameter = NULL;
    digestmech.ulParameterLen = 0;

    program_name = strrchr(argv[0], '/');
    program_name = program_name ? (program_name + 1) : argv[0];
    optstate = PL_CreateOptState (argc, argv, "i:o:f:Fd:hH?p:P:vVs:");
    if (optstate == NULL) {
        lperror("PL_CreateOptState failed");
        return 1;
    }

    while (PL_GetNextOpt (optstate) == PL_OPT_OK) {
        switch (optstate->option) {

            case 'd':
                if (!optstate->value) {
                    PL_DestroyOptState(optstate);
                    usage(program_name);
                }
                configDir = PL_strdup(optstate->value);
                checkPath(configDir);
                break;

                case 'i':
                if (!optstate->value) {
                    PL_DestroyOptState(optstate);
                    usage(program_name);
                }
                input_file = optstate->value;
                break;

                case 'o':
                if (!optstate->value) {
                    PL_DestroyOptState(optstate);
                    usage(program_name);
                }
                output_file = PL_strdup(optstate->value);
                break;

                case 'f':
                if (!optstate->value) {
                    PL_DestroyOptState(optstate);
                    usage(program_name);
                }
                pwd = filePasswd((char *)optstate->value);
                if (!pwd) usage(program_name);
                break;

                case 'F':
                FIPSMODE = PR_TRUE;
                break;

                case 'p':
                if (!optstate->value) {
                    PL_DestroyOptState(optstate);
                    usage(program_name);
                }
                pwd =  PL_strdup(optstate->value);
                break;

                case 'P':
                if (!optstate->value) {
                    PL_DestroyOptState(optstate);
                    usage(program_name);
                }
                dbPrefix = PL_strdup(optstate->value);
                break;

                case 'v':
                verbose = PR_TRUE;
                break;

                case 'V':
                verify = PR_TRUE;
                break;

                case 'H':
                PL_DestroyOptState(optstate);
                long_usage (program_name);
                return 1;
                break;

                case 'h':
                case '?':
                default:
                PL_DestroyOptState(optstate);
                usage(program_name);
                return 1;
                break;
        }
    }
    PL_DestroyOptState(optstate);

    if (!input_file) {
        usage(program_name);
        return 1;
    }

    /* Get the platform-dependent library name of the
     * NSS cryptographic module.
     */
    libname = PR_GetLibraryName(NULL, "softokn3");
    assert(libname != NULL);
    lib = PR_LoadLibrary(libname);
    assert(lib != NULL);
    PR_FreeLibraryName(libname);


    if (FIPSMODE) {
        /* FIPSMODE == FC_GetFunctionList */
        /* library path must be set to an already signed softokn3/freebl */
        pC_GetFunctionList = (CK_C_GetFunctionList)
                             PR_FindFunctionSymbol(lib, "FC_GetFunctionList");
    } else {
        /* NON FIPS mode  == C_GetFunctionList */
        pC_GetFunctionList = (CK_C_GetFunctionList)
                             PR_FindFunctionSymbol(lib, "C_GetFunctionList");
     }
    assert(pC_GetFunctionList != NULL);

    crv = (*pC_GetFunctionList)(&pFunctionList);
    assert(crv == CKR_OK);

    if (configDir) {
    if (!dbPrefix) {
            dbPrefix = PL_strdup("");
        }
        crv = softokn_Init(pFunctionList, configDir, dbPrefix);
        if (crv != CKR_OK) {
            logIt("Failed to use provided database directory "
                  "will just initialize the volatile certdb.\n");
            crv = softokn_Init(pFunctionList, NULL, NULL); /* NoDB Init */
        }
    } else {
        crv = softokn_Init(pFunctionList, NULL, NULL); /* NoDB Init */
    }

    if (crv != CKR_OK) {
        pk11error( "Initiailzing softoken failed", crv);
        goto cleanup;
    }

    pSlotList = getSlotList(pFunctionList, slotIndex);
    if (pSlotList == NULL) {
        PR_fprintf(PR_STDERR, "getSlotList failed");
        goto cleanup;
    }

    crv = pFunctionList->C_OpenSession(pSlotList[slotIndex],
                                       CKF_RW_SESSION | CKF_SERIAL_SESSION,
                                       NULL, NULL, &hRwSession);
    if (crv != CKR_OK) {
        pk11error( "Opening a read/write session failed", crv);
        goto cleanup;
    }

    /* check if a password is needed */
    crv = pFunctionList->C_GetTokenInfo(pSlotList[slotIndex], &tokenInfo);
    if (crv != CKR_OK) {
        pk11error( "C_GetTokenInfo failed", crv);
        goto cleanup;
    }
    if (tokenInfo.flags & CKF_LOGIN_REQUIRED) {
        if (pwd) {
            int pwdLen = strlen((const char*)pwd); 
            crv = pFunctionList->C_Login(hRwSession, CKU_USER, 
                                (CK_UTF8CHAR_PTR) pwd, (CK_ULONG)pwdLen);
            if (crv != CKR_OK) {
                pk11error("C_Login failed", crv);
                goto cleanup;
            }
        } else {
            PR_fprintf(PR_STDERR, "Please provide the password for the token");
            goto cleanup;
        }
    } else if (pwd) {
        logIt("A password was provided but the password was not used.\n");
    }

    /* Generate a DSA key pair */
    logIt("Generate a DSA key pair ... \n");
    crv = pFunctionList->C_GenerateKeyPair(hRwSession, &dsaKeyPairGenMech,
                                           dsaPubKeyTemplate,
                                           NUM_ELEM(dsaPubKeyTemplate),
                                           dsaPrivKeyTemplate,
                                           NUM_ELEM(dsaPrivKeyTemplate),
                                           &hDSApubKey, &hDSAprivKey);
    if (crv != CKR_OK) {
        pk11error("DSA key pair generation failed", crv);
        goto cleanup;
    }

    /* open the shared library */
    fd = PR_OpenFile(input_file,PR_RDONLY,0);
    if (fd == NULL ) {
        lperror(input_file);
        goto cleanup;
    }
#ifdef USES_LINKS
    ret = lstat(input_file, &stat_buf);
    if (ret < 0) {
        perror(input_file);
        goto cleanup;
    }
    if (S_ISLNK(stat_buf.st_mode)) {
        char *dirpath,*dirend;
        ret = readlink(input_file, link_buf, sizeof(link_buf) - 1);
        if (ret < 0) {
            perror(input_file);
            goto cleanup;
        }
        link_buf[ret] = 0;
        link_file = mkoutput(input_file);
        /* get the dirname of input_file */
        dirpath = PL_strdup(input_file);
        dirend = strrchr(dirpath, '/');
        if (dirend) {
            *dirend = '\0';
            ret = chdir(dirpath);
            if (ret < 0) {
                perror(dirpath);
                goto cleanup;
            }
        }
        PL_strfree(dirpath);
        input_file = link_buf;
        /* get the basename of link_file */
        dirend = strrchr(link_file, '/');
        if (dirend) {
            char * tmp_file = NULL;
            tmp_file = PL_strdup(dirend +1 );
            PL_strfree(link_file);
            link_file = tmp_file;
        }
    }
#endif
    if (output_file == NULL) {
        output_file = mkoutput(input_file);
    }

    /* compute the digest */
    memset(digest, 0, sizeof(digest));
    crv = pFunctionList->C_DigestInit(hRwSession, &digestmech);
    if (crv != CKR_OK) {
        pk11error("C_DigestInit failed", crv);
        goto cleanup;
    }

    /* Digest the file */
    while ((bytesRead = PR_Read(fd,file_buf,sizeof(file_buf))) > 0) {
        crv = pFunctionList->C_DigestUpdate(hRwSession, (CK_BYTE_PTR)file_buf,
                                            bytesRead);
        if (crv != CKR_OK) {
            pk11error("C_DigestUpdate failed", crv);
            goto cleanup;
        }
        count += bytesRead;
    }

    /* close the input_File */
    PR_Close(fd);
    fd = NULL;
    if (bytesRead < 0) {
        lperror("0 bytes read from input file");
        goto cleanup;
    }

    digestLen = sizeof(digest);
    crv = pFunctionList->C_DigestFinal(hRwSession, (CK_BYTE_PTR)digest,
                                       &digestLen);
    if (crv != CKR_OK) {
        pk11error("C_DigestFinal failed", crv);
        goto cleanup;
    }

    if (digestLen != sizeof(digest)) {
        PR_fprintf(PR_STDERR, "digestLen has incorrect length %lu "
                "it should be %lu \n",digestLen, sizeof(digest));
        goto cleanup;
    }

    /* sign the hash */
    memset(sign, 0, sizeof(sign));
    /* SignUpdate  */
    crv = pFunctionList->C_SignInit(hRwSession, &signMech, hDSAprivKey);
    if (crv != CKR_OK) {
        pk11error("C_SignInit failed", crv);
        goto cleanup;
    }

    signLen = sizeof(sign);
    crv = pFunctionList->C_Sign(hRwSession, (CK_BYTE * ) digest, digestLen,
                                sign, &signLen);
    if (crv != CKR_OK) {
        pk11error("C_Sign failed", crv);
        goto cleanup;
    }

    if (signLen != sizeof(sign)) {
        PR_fprintf(PR_STDERR, "signLen has incorrect length %lu "
                    "it should be %lu \n", signLen, sizeof(sign));
        goto cleanup;
    }

    if (verify) {
        crv = pFunctionList->C_VerifyInit(hRwSession, &signMech, hDSApubKey);
        if (crv != CKR_OK) {
            pk11error("C_VerifyInit failed", crv);
            goto cleanup;
        }
        crv = pFunctionList->C_Verify(hRwSession, digest, digestLen,
                                      sign, signLen);
        if (crv != CKR_OK) {
            pk11error("C_Verify failed", crv);
            goto cleanup;
        }
    }

    if (verbose) {
        int j;
        PR_fprintf(PR_STDERR,"Library File: %s %d bytes\n",input_file, count);
        PR_fprintf(PR_STDERR,"Check File: %s\n",output_file);
#ifdef USES_LINKS
        if (link_file) {
            PR_fprintf(PR_STDERR,"Link: %s\n",link_file);
        }
#endif
        PR_fprintf(PR_STDERR,"  hash: %lu bytes\n", digestLen);
#define STEP 10
        for (i=0; i < (int) digestLen; i += STEP) {
            PR_fprintf(PR_STDERR,"   ");
            for (j=0; j < STEP && (i+j) < (int) digestLen; j++) {
                PR_fprintf(PR_STDERR," %02x", digest[i+j]);
            }
            PR_fprintf(PR_STDERR,"\n");
        }
        PR_fprintf(PR_STDERR,"  signature: %lu bytes\n", signLen);
        for (i=0; i < (int) signLen; i += STEP) {
            PR_fprintf(PR_STDERR,"   ");
            for (j=0; j < STEP && (i+j) < (int) signLen; j++) {
                PR_fprintf(PR_STDERR," %02x", sign[i+j]);
            }
            PR_fprintf(PR_STDERR,"\n");
        }
    }

    /* open the target signature file */
    fd = PR_OpenFile(output_file,PR_WRONLY|PR_CREATE_FILE|PR_TRUNCATE,0666);
    if (fd == NULL ) {
        lperror(output_file);
        goto cleanup;
    }

    /*
     * we write the key out in a straight binary format because very
     * low level libraries need to read an parse this file. Ideally we should
     * just derEncode the public key (which would be pretty simple, and be
     * more general), but then we'd need to link the ASN.1 decoder with the
     * freebl libraries.
     */

    file_buf[0] = NSS_SIGN_CHK_MAGIC1;
    file_buf[1] = NSS_SIGN_CHK_MAGIC2;
    file_buf[2] = NSS_SIGN_CHK_MAJOR_VERSION;
    file_buf[3] = NSS_SIGN_CHK_MINOR_VERSION;
    encodeInt(&file_buf[4],12);  /* offset to data start */
    encodeInt(&file_buf[8],CKK_DSA);
    bytesWritten = PR_Write(fd,file_buf, 12);
    if (bytesWritten != 12) {
        lperror(output_file);
        goto cleanup;
    }

    /* get DSA Public KeyValue */
    memset(dsaPubKey, 0, sizeof(dsaPubKey));
    dsaPubKeyValue.type =CKA_VALUE;
    dsaPubKeyValue.pValue = (CK_VOID_PTR) &dsaPubKey;
    dsaPubKeyValue.ulValueLen = sizeof(dsaPubKey);

    crv = pFunctionList->C_GetAttributeValue(hRwSession, hDSApubKey,
                                             &dsaPubKeyValue, 1);
    if (crv != CKR_OK && crv != CKR_ATTRIBUTE_TYPE_INVALID) {
        pk11error("C_GetAttributeValue failed", crv);
        goto cleanup;
    }

    /* CKA_PRIME */
    rv = writeItem(fd,dsaPubKeyTemplate[0].pValue,
                   dsaPubKeyTemplate[0].ulValueLen, output_file);
    if (rv != PR_SUCCESS) goto cleanup;
    /* CKA_SUBPRIME */
    rv = writeItem(fd,dsaPubKeyTemplate[1].pValue,
                   dsaPubKeyTemplate[1].ulValueLen, output_file);
    if (rv != PR_SUCCESS) goto cleanup;
    /* CKA_BASE */ 
    rv = writeItem(fd,dsaPubKeyTemplate[2].pValue,
                   dsaPubKeyTemplate[2].ulValueLen, output_file);
    if (rv != PR_SUCCESS) goto cleanup;
    /* DSA Public Key value */
    rv = writeItem(fd,dsaPubKeyValue.pValue,
                   dsaPubKeyValue.ulValueLen, output_file);
    if (rv != PR_SUCCESS) goto cleanup;
    /* DSA SIGNATURE */
    rv = writeItem(fd,&sign, signLen, output_file);
    if (rv != PR_SUCCESS) goto cleanup;
    PR_Close(fd);

#ifdef USES_LINKS
    if (link_file) {
        (void)unlink(link_file);
        ret = symlink(output_file, link_file);
        if (ret < 0) {
            perror(link_file);
            goto cleanup;
        }
    }
#endif

cleanup:
    if (pFunctionList) {
        /* C_Finalize will automatically logout, close session, */
        /* and delete the temp objects on the token */
        crv = pFunctionList->C_Finalize(NULL);
        if (crv != CKR_OK) {
            pk11error("C_Finalize failed", crv);
        }
    }
    if (pSlotList) {
        PR_Free(pSlotList);
    }
    if (pwd) {
        PL_strfree(pwd);
    }
    if (configDir) {
        PL_strfree(configDir);
    }
    if (dbPrefix) {
        PL_strfree(dbPrefix);
    }
    if (output_file) { /* allocated by mkoutput function */
        PL_strfree(output_file); 
    }
#ifdef USES_LINKS
    if (link_file) { /* allocated by mkoutput function */
        PL_strfree(link_file); 
    }
#endif

    disableUnload = PR_GetEnv("NSS_DISABLE_UNLOAD");
    if (!disableUnload) {
        PR_UnloadLibrary(lib);
    }
    PR_Cleanup();

    return crv;
}
