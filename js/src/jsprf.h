/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jsprf_h___
#define jsprf_h___

/*
** API for PR printf like routines. Supports the following formats
**      %d - decimal
**      %u - unsigned decimal
**      %x - unsigned hex
**      %X - unsigned uppercase hex
**      %o - unsigned octal
**      %hd, %hu, %hx, %hX, %ho - 16-bit versions of above
**      %ld, %lu, %lx, %lX, %lo - 32-bit versions of above
**      %lld, %llu, %llx, %llX, %llo - 64 bit versions of above
**      %s - string
**      %hs - 16-bit version of above (only available if js_CStringsAreUTF8)
**      %c - character
**      %hc - 16-bit version of above (only available if js_CStringsAreUTF8)
**      %p - pointer (deals with machine dependent pointer size)
**      %f - float
**      %g - float
*/
#include "jstypes.h"
#include <stdio.h>
#include <stdarg.h>

JS_BEGIN_EXTERN_C

/*
** sprintf into a fixed size buffer. Guarantees that a NUL is at the end
** of the buffer. Returns the length of the written output, NOT including
** the NUL, or (uint32_t)-1 if an error occurs.
*/
extern JS_PUBLIC_API(uint32_t) JS_snprintf(char *out, uint32_t outlen, const char *fmt, ...);

/*
** sprintf into a malloc'd buffer. Return a pointer to the malloc'd
** buffer on success, NULL on failure. Call "JS_smprintf_free" to release
** the memory returned.
*/
extern JS_PUBLIC_API(char*) JS_smprintf(const char *fmt, ...);

/*
** Free the memory allocated, for the caller, by JS_smprintf
*/
extern JS_PUBLIC_API(void) JS_smprintf_free(char *mem);

/*
** "append" sprintf into a malloc'd buffer. "last" is the last value of
** the malloc'd buffer. sprintf will append data to the end of last,
** growing it as necessary using realloc. If last is NULL, JS_sprintf_append
** will allocate the initial string. The return value is the new value of
** last for subsequent calls, or NULL if there is a malloc failure.
*/
extern JS_PUBLIC_API(char*) JS_sprintf_append(char *last, const char *fmt, ...);

/*
** sprintf into a function. The function "f" is called with a string to
** place into the output. "arg" is an opaque pointer used by the stuff
** function to hold any state needed to do the storage of the output
** data. The return value is a count of the number of characters fed to
** the stuff function, or (uint32_t)-1 if an error occurs.
*/
typedef int (*JSStuffFunc)(void *arg, const char *s, uint32_t slen);

extern JS_PUBLIC_API(uint32_t) JS_sxprintf(JSStuffFunc f, void *arg, const char *fmt, ...);

/*
** va_list forms of the above.
*/
extern JS_PUBLIC_API(uint32_t) JS_vsnprintf(char *out, uint32_t outlen, const char *fmt, va_list ap);
extern JS_PUBLIC_API(char*) JS_vsmprintf(const char *fmt, va_list ap);
extern JS_PUBLIC_API(char*) JS_vsprintf_append(char *last, const char *fmt, va_list ap);
extern JS_PUBLIC_API(uint32_t) JS_vsxprintf(JSStuffFunc f, void *arg, const char *fmt, va_list ap);

JS_END_EXTERN_C

#endif /* jsprf_h___ */
