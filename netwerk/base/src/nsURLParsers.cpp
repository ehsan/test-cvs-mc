/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>

#include "mozilla/RangedPtr.h"

#include "nsURLParsers.h"
#include "nsURLHelper.h"
#include "nsIURI.h"
#include "prtypes.h"
#include "nsString.h"
#include "nsCRT.h"
#include "netCore.h"

using namespace mozilla;

//----------------------------------------------------------------------------

static PRUint32
CountConsecutiveSlashes(const char *str, PRInt32 len)
{
    RangedPtr<const char> p(str, len);
    PRUint32 count = 0;
    while (len-- && *p++ == '/') ++count;
    return count;
}

//----------------------------------------------------------------------------
// nsBaseURLParser implementation
//----------------------------------------------------------------------------

// The URL parser service does not have any internal state; however, it can
// be called from multiple threads, so we must use a threadsafe AddRef and
// Release implementation.
NS_IMPL_THREADSAFE_ISUPPORTS1(nsAuthURLParser, nsIURLParser)
NS_IMPL_THREADSAFE_ISUPPORTS1(nsNoAuthURLParser, nsIURLParser)

#define SET_RESULT(component, pos, len) \
    PR_BEGIN_MACRO \
        if (component ## Pos) \
           *component ## Pos = PRUint32(pos); \
        if (component ## Len) \
           *component ## Len = PRInt32(len); \
    PR_END_MACRO

#define OFFSET_RESULT(component, offset) \
    PR_BEGIN_MACRO \
        if (component ## Pos) \
           *component ## Pos += offset; \
    PR_END_MACRO

NS_IMETHODIMP
nsBaseURLParser::ParseURL(const char *spec, PRInt32 specLen,
                          PRUint32 *schemePos, PRInt32 *schemeLen,
                          PRUint32 *authorityPos, PRInt32 *authorityLen,
                          PRUint32 *pathPos, PRInt32 *pathLen)
{
    NS_PRECONDITION(spec, "null pointer");

    if (specLen < 0)
        specLen = strlen(spec);

    const char *stop = nullptr;
    const char *colon = nullptr;
    const char *slash = nullptr;
    const char *p;
    PRUint32 offset = 0;
    PRInt32 len = specLen;
    for (p = spec; len && *p && !colon && !slash; ++p, --len) {
        // skip leading whitespace
        if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') {
            spec++;
            specLen--;
            offset++;
            continue;
        }
        switch (*p) {
            case ':':
                if (!colon)
                    colon = p;
                break;
            case '/': // start of filepath
            case '?': // start of query
            case '#': // start of ref
                if (!slash)
                    slash = p;
                break;
            case '@': // username@hostname
            case '[': // start of IPv6 address literal
                if (!stop)
                    stop = p;
                break;
        }
    }
    // disregard the first colon if it follows an '@' or a '['
    if (colon && stop && colon > stop)
        colon = nullptr;

    // if the spec only contained whitespace ...
    if (specLen == 0) {
        SET_RESULT(scheme, 0, -1);
        SET_RESULT(authority, 0, 0);
        SET_RESULT(path, 0, 0);
        return NS_OK;
    }

    // ignore trailing whitespace and control characters
    for (p = spec + specLen - 1; ((unsigned char) *p <= ' ') && (p != spec); --p)
        ;

    specLen = p - spec + 1;

    if (colon && (colon < slash || !slash)) {
        //
        // spec = <scheme>:/<the-rest>
        //
        // or
        //
        // spec = <scheme>:<authority>
        // spec = <scheme>:<path-no-slashes>
        //
        if (!net_IsValidScheme(spec, colon - spec) || (*(colon+1) == ':')) {
            NS_WARNING("malformed uri");
            return NS_ERROR_MALFORMED_URI;
        }
        SET_RESULT(scheme, offset, colon - spec);
        if (authorityLen || pathLen) {
            PRUint32 schemeLen = colon + 1 - spec;
            offset += schemeLen;
            ParseAfterScheme(colon + 1, specLen - schemeLen,
                             authorityPos, authorityLen,
                             pathPos, pathLen);
            OFFSET_RESULT(authority, offset);
            OFFSET_RESULT(path, offset);
        }
    }
    else {
        // 
        // spec = <authority-no-port-or-password>/<path>
        // spec = <path>
        //
        // or
        //
        // spec = <authority-no-port-or-password>/<path-with-colon>
        // spec = <path-with-colon>
        //
        // or
        //
        // spec = <authority-no-port-or-password>
        // spec = <path-no-slashes-or-colon>
        //
        SET_RESULT(scheme, 0, -1);
        if (authorityLen || pathLen)
            ParseAfterScheme(spec, specLen,
                             authorityPos, authorityLen,
                             pathPos, pathLen);
            OFFSET_RESULT(authority, offset);
            OFFSET_RESULT(path, offset);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsBaseURLParser::ParseAuthority(const char *auth, PRInt32 authLen,
                                PRUint32 *usernamePos, PRInt32 *usernameLen,
                                PRUint32 *passwordPos, PRInt32 *passwordLen,
                                PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                                PRInt32 *port)
{
    NS_PRECONDITION(auth, "null pointer");

    if (authLen < 0)
        authLen = strlen(auth);

    SET_RESULT(username, 0, -1);
    SET_RESULT(password, 0, -1);
    SET_RESULT(hostname, 0, authLen);
    if (port)
       *port = -1;
    return NS_OK;
}

NS_IMETHODIMP
nsBaseURLParser::ParseUserInfo(const char *userinfo, PRInt32 userinfoLen,
                               PRUint32 *usernamePos, PRInt32 *usernameLen,
                               PRUint32 *passwordPos, PRInt32 *passwordLen)
{
    SET_RESULT(username, 0, -1);
    SET_RESULT(password, 0, -1);
    return NS_OK;
}

NS_IMETHODIMP
nsBaseURLParser::ParseServerInfo(const char *serverinfo, PRInt32 serverinfoLen,
                                 PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                                 PRInt32 *port)
{
    SET_RESULT(hostname, 0, -1);
    if (port)
       *port = -1;
    return NS_OK;
}

NS_IMETHODIMP
nsBaseURLParser::ParsePath(const char *path, PRInt32 pathLen,
                           PRUint32 *filepathPos, PRInt32 *filepathLen,
                           PRUint32 *queryPos, PRInt32 *queryLen,
                           PRUint32 *refPos, PRInt32 *refLen)
{
    NS_PRECONDITION(path, "null pointer");

    if (pathLen < 0)
        pathLen = strlen(path);

    // path = [/]<segment1>/<segment2>/<...>/<segmentN>?<query>#<ref>

    // XXX PL_strnpbrk would be nice, but it's buggy

    // search for first occurrence of either ? or #
    const char *query_beg = 0, *query_end = 0;
    const char *ref_beg = 0;
    const char *p = 0;
    for (p = path; p < path + pathLen; ++p) {
        // only match the query string if it precedes the reference fragment
        if (!ref_beg && !query_beg && *p == '?')
            query_beg = p + 1;
        else if (*p == '#') {
            ref_beg = p + 1;
            if (query_beg)
                query_end = p;
            break;
        }
    }

    if (query_beg) {
        if (query_end)
            SET_RESULT(query, query_beg - path, query_end - query_beg);
        else
            SET_RESULT(query, query_beg - path, pathLen - (query_beg - path));
    }
    else
        SET_RESULT(query, 0, -1);

    if (ref_beg)
        SET_RESULT(ref, ref_beg - path, pathLen - (ref_beg - path));
    else
        SET_RESULT(ref, 0, -1);

    const char *end;
    if (query_beg)
        end = query_beg - 1;
    else if (ref_beg)
        end = ref_beg - 1;
    else
        end = path + pathLen;

    // an empty file path is no file path
    if (end != path)
        SET_RESULT(filepath, 0, end - path);
    else
        SET_RESULT(filepath, 0, -1);
    return NS_OK;
}

NS_IMETHODIMP
nsBaseURLParser::ParseFilePath(const char *filepath, PRInt32 filepathLen,
                               PRUint32 *directoryPos, PRInt32 *directoryLen,
                               PRUint32 *basenamePos, PRInt32 *basenameLen,
                               PRUint32 *extensionPos, PRInt32 *extensionLen)
{
    NS_PRECONDITION(filepath, "null pointer");

    if (filepathLen < 0)
        filepathLen = strlen(filepath);

    if (filepathLen == 0) {
        SET_RESULT(directory, 0, -1);
        SET_RESULT(basename, 0, 0); // assume a zero length file basename
        SET_RESULT(extension, 0, -1);
        return NS_OK;
    }

    const char *p;
    const char *end = filepath + filepathLen;

    // search backwards for filename
    for (p = end - 1; *p != '/' && p > filepath; --p)
        ;
    if (*p == '/') {
        // catch /.. and /.
        if ((p+1 < end && *(p+1) == '.') && 
           (p+2 == end || (*(p+2) == '.' && p+3 == end)))
            p = end - 1;
        // filepath = <directory><filename>.<extension>
        SET_RESULT(directory, 0, p - filepath + 1);
        ParseFileName(p + 1, end - (p + 1),
                      basenamePos, basenameLen,
                      extensionPos, extensionLen);
        OFFSET_RESULT(basename, p + 1 - filepath);
        OFFSET_RESULT(extension, p + 1 - filepath);
    }
    else {
        // filepath = <filename>.<extension>
        SET_RESULT(directory, 0, -1);
        ParseFileName(filepath, filepathLen,
                      basenamePos, basenameLen,
                      extensionPos, extensionLen);
    }
    return NS_OK;
}

nsresult
nsBaseURLParser::ParseFileName(const char *filename, PRInt32 filenameLen,
                               PRUint32 *basenamePos, PRInt32 *basenameLen,
                               PRUint32 *extensionPos, PRInt32 *extensionLen)
{
    NS_PRECONDITION(filename, "null pointer");

    if (filenameLen < 0)
        filenameLen = strlen(filename);

    // no extension if filename ends with a '.'
    if (filename[filenameLen-1] != '.') {
        // ignore '.' at the beginning
        for (const char *p = filename + filenameLen - 1; p > filename; --p) {
            if (*p == '.') {
                // filename = <basename.extension>
                SET_RESULT(basename, 0, p - filename);
                SET_RESULT(extension, p + 1 - filename, filenameLen - (p - filename + 1));
                return NS_OK;
            }
        }
    }
    // filename = <basename>
    SET_RESULT(basename, 0, filenameLen);
    SET_RESULT(extension, 0, -1);
    return NS_OK;
}

//----------------------------------------------------------------------------
// nsNoAuthURLParser implementation
//----------------------------------------------------------------------------

NS_IMETHODIMP
nsNoAuthURLParser::ParseAuthority(const char *auth, PRInt32 authLen,
                                 PRUint32 *usernamePos, PRInt32 *usernameLen,
                                 PRUint32 *passwordPos, PRInt32 *passwordLen,
                                 PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                                 PRInt32 *port)
{
    NS_NOTREACHED("Shouldn't parse auth in a NoAuthURL!");
    return NS_ERROR_UNEXPECTED;
}

void
nsNoAuthURLParser::ParseAfterScheme(const char *spec, PRInt32 specLen,
                                    PRUint32 *authPos, PRInt32 *authLen,
                                    PRUint32 *pathPos, PRInt32 *pathLen)
{
    NS_PRECONDITION(specLen >= 0, "unexpected");

    // everything is the path
    PRUint32 pos = 0;
    switch (CountConsecutiveSlashes(spec, specLen)) {
    case 0:
    case 1:
        break;
    case 2:
        {
            const char *p = nullptr;
            if (specLen > 2) {
                // looks like there is an authority section
#if defined(XP_WIN) || defined(XP_OS2)
                // if the authority looks like a drive number then we
                // really want to treat it as part of the path
                // [a-zA-Z][:|]{/\}
                // i.e one of:   c:   c:\foo  c:/foo  c|  c|\foo  c|/foo
                if ((specLen > 3) && (spec[3] == ':' || spec[3] == '|') &&
                    nsCRT::IsAsciiAlpha(spec[2]) &&
                    ((specLen == 4) || (spec[4] == '/') || (spec[4] == '\\'))) {
                    pos = 1;
                    break;  
                } 
#endif
                // Ignore apparent authority; path is everything after it
                for (p = spec + 2; p < spec + specLen; ++p) {
                    if (*p == '/' || *p == '?' || *p == '#')
                        break;
                }
            }
            SET_RESULT(auth, 0, -1);
            if (p && p != spec+specLen)
                SET_RESULT(path, p - spec, specLen - (p - spec));
            else
                SET_RESULT(path, 0, -1);
            return;
        }
    default:
        pos = 2;
        break;
    }
    SET_RESULT(auth, pos, 0);
    SET_RESULT(path, pos, specLen - pos);
}

#if defined(XP_WIN) || defined(XP_OS2)
NS_IMETHODIMP
nsNoAuthURLParser::ParseFilePath(const char *filepath, PRInt32 filepathLen,
                                 PRUint32 *directoryPos, PRInt32 *directoryLen,
                                 PRUint32 *basenamePos, PRInt32 *basenameLen,
                                 PRUint32 *extensionPos, PRInt32 *extensionLen)
{
    NS_PRECONDITION(filepath, "null pointer");

    if (filepathLen < 0)
        filepathLen = strlen(filepath);

    // look for a filepath consisting of only a drive number, which may or
    // may not have a leading slash.
    if (filepathLen > 1 && filepathLen < 4) {
        const char *end = filepath + filepathLen;
        const char *p = filepath;
        if (*p == '/')
            p++;
        if ((end-p == 2) && (p[1]==':' || p[1]=='|') && nsCRT::IsAsciiAlpha(*p)) {
            // filepath = <drive-number>:
            SET_RESULT(directory, 0, filepathLen);
            SET_RESULT(basename, 0, -1);
            SET_RESULT(extension, 0, -1);
            return NS_OK;
        }
    }

    // otherwise fallback on common implementation
    return nsBaseURLParser::ParseFilePath(filepath, filepathLen,
                                          directoryPos, directoryLen,
                                          basenamePos, basenameLen,
                                          extensionPos, extensionLen);
}
#endif

//----------------------------------------------------------------------------
// nsAuthURLParser implementation
//----------------------------------------------------------------------------

NS_IMETHODIMP
nsAuthURLParser::ParseAuthority(const char *auth, PRInt32 authLen,
                                PRUint32 *usernamePos, PRInt32 *usernameLen,
                                PRUint32 *passwordPos, PRInt32 *passwordLen,
                                PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                                PRInt32 *port)
{
    nsresult rv;

    NS_PRECONDITION(auth, "null pointer");

    if (authLen < 0)
        authLen = strlen(auth);

    if (authLen == 0) {
        SET_RESULT(username, 0, -1);
        SET_RESULT(password, 0, -1);
        SET_RESULT(hostname, 0, 0);
        if (port)
            *port = -1;
        return NS_OK;
    }

    // search backwards for @
    const char *p = auth + authLen - 1;
    for (; (*p != '@') && (p > auth); --p);
    if ( *p == '@' ) {
        // auth = <user-info@server-info>
        rv = ParseUserInfo(auth, p - auth,
                           usernamePos, usernameLen,
                           passwordPos, passwordLen);
        if (NS_FAILED(rv)) return rv;
        rv = ParseServerInfo(p + 1, authLen - (p - auth + 1),
                             hostnamePos, hostnameLen,
                             port);
        if (NS_FAILED(rv)) return rv;
        OFFSET_RESULT(hostname, p + 1 - auth);
    }
    else {
        // auth = <server-info>
        SET_RESULT(username, 0, -1);
        SET_RESULT(password, 0, -1);
        rv = ParseServerInfo(auth, authLen,
                             hostnamePos, hostnameLen,
                             port);
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsAuthURLParser::ParseUserInfo(const char *userinfo, PRInt32 userinfoLen,
                               PRUint32 *usernamePos, PRInt32 *usernameLen,
                               PRUint32 *passwordPos, PRInt32 *passwordLen)
{
    NS_PRECONDITION(userinfo, "null pointer");

    if (userinfoLen < 0)
        userinfoLen = strlen(userinfo);

    if (userinfoLen == 0) {
        SET_RESULT(username, 0, -1);
        SET_RESULT(password, 0, -1);
        return NS_OK;
    }

    const char *p = (const char *) memchr(userinfo, ':', userinfoLen);
    if (p) {
        // userinfo = <username:password>
        if (p == userinfo) {
            // must have a username!
            return NS_ERROR_MALFORMED_URI;
        }
        SET_RESULT(username, 0, p - userinfo);
        SET_RESULT(password, p - userinfo + 1, userinfoLen - (p - userinfo + 1));
    }
    else {
        // userinfo = <username>
        SET_RESULT(username, 0, userinfoLen);
        SET_RESULT(password, 0, -1);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsAuthURLParser::ParseServerInfo(const char *serverinfo, PRInt32 serverinfoLen,
                                 PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                                 PRInt32 *port)
{
    NS_PRECONDITION(serverinfo, "null pointer");

    if (serverinfoLen < 0)
        serverinfoLen = strlen(serverinfo);

    if (serverinfoLen == 0) {
        SET_RESULT(hostname, 0, 0);
        if (port)
            *port = -1;
        return NS_OK;
    }

    // search backwards for a ':' but stop on ']' (IPv6 address literal
    // delimiter).  check for illegal characters in the hostname.
    const char *p = serverinfo + serverinfoLen - 1;
    const char *colon = nullptr, *bracket = nullptr;
    for (; p > serverinfo; --p) {
        switch (*p) {
            case ']':
                bracket = p;
                break;
            case ':':
                if (bracket == nullptr)
                    colon = p;
                break;
            case ' ':
                // hostname must not contain a space
                NS_WARNING("malformed hostname");
                return NS_ERROR_MALFORMED_URI;
        }
    }

    if (colon) {
        // serverinfo = <hostname:port>
        SET_RESULT(hostname, 0, colon - serverinfo);
        if (port) {
            // XXX unfortunately ToInteger is not defined for substrings
            nsCAutoString buf(colon+1, serverinfoLen - (colon + 1 - serverinfo));
            if (buf.Length() == 0) {
                *port = -1;
            }
            else {
                const char* nondigit = NS_strspnp("0123456789", buf.get());
                if (nondigit && *nondigit)
                    return NS_ERROR_MALFORMED_URI;

                PRInt32 err;
                *port = buf.ToInteger(&err);
                if (NS_FAILED(err) || *port <= 0)
                    return NS_ERROR_MALFORMED_URI;
            }
        }
    }
    else {
        // serverinfo = <hostname>
        SET_RESULT(hostname, 0, serverinfoLen);
        if (port)
           *port = -1;
    }

    // In case of IPv6 address check its validity
    if (*hostnameLen > 1 && *(serverinfo + *hostnamePos) == '[' &&
        *(serverinfo + *hostnamePos + *hostnameLen - 1) == ']' &&
        !net_IsValidIPv6Addr(serverinfo + *hostnamePos + 1, *hostnameLen - 2))
            return NS_ERROR_MALFORMED_URI;

    return NS_OK;
}

void
nsAuthURLParser::ParseAfterScheme(const char *spec, PRInt32 specLen,
                                  PRUint32 *authPos, PRInt32 *authLen,
                                  PRUint32 *pathPos, PRInt32 *pathLen)
{
    NS_PRECONDITION(specLen >= 0, "unexpected");

    PRUint32 nslash = CountConsecutiveSlashes(spec, specLen);

    // search for the end of the authority section
    const char *end = spec + specLen;
    const char *p;
    for (p = spec + nslash; p < end; ++p) {
        if (*p == '/' || *p == '?' || *p == '#')
            break;
    }
    if (p < end) {
        // spec = [/]<auth><path>
        SET_RESULT(auth, nslash, p - (spec + nslash));
        SET_RESULT(path, p - spec, specLen - (p - spec));
    }
    else {
        // spec = [/]<auth>
        SET_RESULT(auth, nslash, specLen - nslash);
        SET_RESULT(path, 0, -1);
    }
}

//----------------------------------------------------------------------------
// nsStdURLParser implementation
//----------------------------------------------------------------------------

void
nsStdURLParser::ParseAfterScheme(const char *spec, PRInt32 specLen,
                                 PRUint32 *authPos, PRInt32 *authLen,
                                 PRUint32 *pathPos, PRInt32 *pathLen)
{
    NS_PRECONDITION(specLen >= 0, "unexpected");

    PRUint32 nslash = CountConsecutiveSlashes(spec, specLen);

    // search for the end of the authority section
    const char *end = spec + specLen;
    const char *p;
    for (p = spec + nslash; p < end; ++p) {
        if (strchr("/?#;", *p))
            break;
    }
    switch (nslash) {
    case 0:
    case 2:
        if (p < end) {
            // spec = (//)<auth><path>
            SET_RESULT(auth, nslash, p - (spec + nslash));
            SET_RESULT(path, p - spec, specLen - (p - spec));
        }
        else {
            // spec = (//)<auth>
            SET_RESULT(auth, nslash, specLen - nslash);
            SET_RESULT(path, 0, -1);
        }
        break;
    case 1:
        // spec = /<path>
        SET_RESULT(auth, 0, -1);
        SET_RESULT(path, 0, specLen);
        break;
    default:
        // spec = ///[/]<path>
        SET_RESULT(auth, 2, 0);
        SET_RESULT(path, 2, specLen - 2);
    }
}
