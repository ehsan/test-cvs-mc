/***********************************************************************
Copyright (c) 2006-2011, Skype Limited. All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, (subject to the limitations in the disclaimer below)
are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
- Neither the name of Skype Limited, nor the names of specific
contributors, may be used to endorse or promote products derived from
this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED
BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

#ifndef SILK_TYPEDEF_H
#define SILK_TYPEDEF_H

#include "opus_types.h"

#ifndef silk_USE_DOUBLE_PRECISION_FLOATS
#define silk_USE_DOUBLE_PRECISION_FLOATS     0
#endif

#include <float.h>
#if defined( __GNUC__ )
#include <stdint.h>
#endif

#define silk_int_ptr_size intptr_t

#if silk_USE_DOUBLE_PRECISION_FLOATS
# define silk_float      double
# define silk_float_MAX  DBL_MAX
#else
# define silk_float      float
# define silk_float_MAX  FLT_MAX
#endif

#ifdef _WIN32
# define silk_STR_CASEINSENSITIVE_COMPARE(x, y) _stricmp(x, y)
#else
# define silk_STR_CASEINSENSITIVE_COMPARE(x, y) strcasecmp(x, y)
#endif

#define silk_int64_MAX   ((opus_int64)0x7FFFFFFFFFFFFFFFLL)   /*  2^63 - 1 */
#define silk_int64_MIN   ((opus_int64)0x8000000000000000LL)   /* -2^63 */
#define silk_int32_MAX   0x7FFFFFFF                           /*  2^31 - 1 =  2147483647 */
#define silk_int32_MIN   ((opus_int32)0x80000000)             /* -2^31     = -2147483648 */
#define silk_int16_MAX   0x7FFF                               /*  2^15 - 1 =  32767 */
#define silk_int16_MIN   ((opus_int16)0x8000)                 /* -2^15     = -32768 */
#define silk_int8_MAX    0x7F                                 /*  2^7 - 1  =  127 */
#define silk_int8_MIN    ((opus_int8)0x80)                    /* -2^7      = -128 */

#define silk_uint32_MAX  0xFFFFFFFF  /* 2^32 - 1 = 4294967295 */
#define silk_uint32_MIN  0x00000000
#define silk_uint16_MAX  0xFFFF      /* 2^16 - 1 = 65535 */
#define silk_uint16_MIN  0x0000
#define silk_uint8_MAX   0xFF        /*  2^8 - 1 = 255 */
#define silk_uint8_MIN   0x00

#define silk_TRUE        1
#define silk_FALSE       0

/* assertions */
#if (defined _WIN32 && !defined _WINCE && !defined(__GNUC__) && !defined(NO_ASSERTS))
# ifndef silk_assert
#  include <crtdbg.h>      /* ASSERTE() */
#  define silk_assert(COND)   _ASSERTE(COND)
# endif
#else
# ifdef ENABLE_ASSERTIONS
#  include <stdio.h>
#  include <stdlib.h>
#define silk_fatal(str) _silk_fatal(str, __FILE__, __LINE__);
#ifdef __GNUC__
__attribute__((noreturn))
#endif
static inline void _silk_fatal(const char *str, const char *file, int line)
{
   fprintf (stderr, "Fatal (internal) error in %s, line %d: %s\n", file, line, str);
   abort();
}
#  define silk_assert(COND) {if (!(COND)) {silk_fatal("assertion failed: " #COND);}}
# else
#  define silk_assert(COND)
# endif
#endif

#endif /* SILK_TYPEDEF_H */