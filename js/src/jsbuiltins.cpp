/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=99:
 *
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
 * The Original Code is Mozilla SpiderMonkey JavaScript 1.9 code, released
 * May 28, 2008.
 *
 * The Initial Developer of the Original Code is
 *   Andreas Gal <gal@mozilla.com>
 *
 * Contributor(s):
 *   Brendan Eich <brendan@mozilla.org>
 *   Mike Shaver <shaver@mozilla.org>
 *   David Anderson <danderson@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include <math.h>

#include "jsapi.h"
#include "jsarray.h"
#include "jsbool.h"
#include "jsnum.h"
#include "jsgc.h"
#include "jscntxt.h"
#include "nanojit/avmplus.h"
#include "nanojit/nanojit.h"
#include "jsmath.h"
#include "jsstr.h"
#include "jstracer.h"

using namespace nanojit;

jsdouble FASTCALL builtin_dmod(jsdouble a, jsdouble b)
{
    if (b == 0.0) {
        jsdpun u;
        u.s.hi = JSDOUBLE_HI32_EXPMASK | JSDOUBLE_HI32_MANTMASK;
        u.s.lo = 0xffffffff;
        return u.d;
    }
    jsdouble r;
#ifdef XP_WIN
    /* Workaround MS fmod bug where 42 % (1/0) => NaN, not 42. */
    if (JSDOUBLE_IS_FINITE(a) && JSDOUBLE_IS_INFINITE(b))
        r = a;
    else
#endif
        r = fmod(a, b);
    return r;
}

/* The following boxing/unboxing primitives we can't emit inline because
   they either interact with the GC and depend on Spidermonkey's 32-bit
   integer representation. */

jsval FASTCALL builtin_BoxDouble(JSContext* cx, jsdouble d)
{
    jsint i;
    if (JSDOUBLE_IS_INT(d, i))
        return INT_TO_JSVAL(i);
    if (!cx->doubleFreeList) /* we must be certain the GC won't kick in */
        return JSVAL_ERROR_COOKIE;
    jsval v; /* not rooted but ok here because we know GC won't run */
    if (!js_NewDoubleInRootedValue(cx, d, &v))
        return JSVAL_ERROR_COOKIE;
    return v;
}

jsval FASTCALL builtin_BoxInt32(JSContext* cx, jsint i)
{
    if (JS_LIKELY(INT_FITS_IN_JSVAL(i)))
        return INT_TO_JSVAL(i);
    if (!cx->doubleFreeList) /* we must be certain the GC won't kick in */
        return JSVAL_ERROR_COOKIE;
    jsval v; /* not rooted but ok here because we know GC won't run */
    jsdouble d = (jsdouble)i;
    if (!js_NewDoubleInRootedValue(cx, d, &v))
        return JSVAL_ERROR_COOKIE;
    return v;
} 

jsdouble FASTCALL builtin_UnboxDouble(jsval v)
{
    if (JS_LIKELY(JSVAL_IS_INT(v)))
        return (jsdouble)JSVAL_TO_INT(v);
    return *JSVAL_TO_DOUBLE(v);
}

jsint FASTCALL builtin_UnboxInt32(jsval v)
{
    if (JS_LIKELY(JSVAL_IS_INT(v)))
        return JSVAL_TO_INT(v);
    return js_DoubleToECMAInt32(*JSVAL_TO_DOUBLE(v));
}

int32 FASTCALL builtin_doubleToInt32(jsdouble d)
{
    return js_DoubleToECMAInt32(d);
}

int32 FASTCALL builtin_doubleToUint32(jsdouble d)
{
    return js_DoubleToECMAUint32(d);
}

jsdouble FASTCALL builtin_Math_sin(jsdouble d)
{
    return sin(d);
}

jsdouble FASTCALL builtin_Math_cos(jsdouble d)
{
    return cos(d);
}

jsdouble FASTCALL builtin_Math_pow(jsdouble d, jsdouble p)
{
#ifdef NOTYET
    /* XXX Need to get a NaN here without parameterizing on context all the time. */
    if (!JSDOUBLE_IS_FINITE(p) && (d == 1.0 || d == -1.0))
        return NaN;
#endif
    if (p == 0)
        return 1.0;
    return pow(d, p);
}

jsdouble FASTCALL builtin_Math_sqrt(jsdouble d)
{
    return sqrt(d);
}

bool FASTCALL builtin_Array_dense_setelem(JSContext *cx, JSObject *obj, jsint i, jsval v)
{
    JS_ASSERT(OBJ_IS_DENSE_ARRAY(cx, obj));

    jsuint length = ARRAY_DENSE_LENGTH(obj);
    if ((jsuint)i < length) {
        if (obj->dslots[i] == JSVAL_HOLE) {
            if (i >= obj->fslots[JSSLOT_ARRAY_LENGTH])
                obj->fslots[JSSLOT_ARRAY_LENGTH] = i + 1;
            obj->fslots[JSSLOT_ARRAY_COUNT]++;
        }
        obj->dslots[i] = v;
        return true;
    }
    return OBJ_SET_PROPERTY(cx, obj, INT_TO_JSID(i), &v) ? true : false;
}

JSString* FASTCALL
builtin_String_p_substring(JSContext *cx, JSString *str, jsint begin, jsint end)
{
    JS_ASSERT(end >= begin);
    return js_NewDependentString(cx, str, (size_t)begin, (size_t)(end - begin));
}

JSString* FASTCALL
builtin_String_p_substring_1(JSContext *cx, JSString *str, jsint begin)
{
    jsint end = JSSTRING_LENGTH(str);
    JS_ASSERT(end >= begin);
    return js_NewDependentString(cx, str, (size_t)begin, (size_t)(end - begin));
}

JSString* FASTCALL
builtin_ConcatStrings(JSContext* cx, JSString* left, JSString* right)
{
    /* XXX check for string freelist space */
    return js_ConcatStrings(cx, left, right);
}

JSString* FASTCALL
builtin_String_getelem(JSContext* cx, JSString* str, jsint i)
{
    if ((size_t)i >= JSSTRING_LENGTH(str))
        return NULL;
    /* XXX check for string freelist space */
    str = js_GetUnitString(cx, str, (size_t)i);
    return str;
}

JSString* FASTCALL
builtin_String_fromCharCode(JSContext* cx, jsint i)
{
    jschar c = (jschar)i;
    /* XXX check for string freelist space */
    if (c < UNIT_STRING_LIMIT)
        return js_GetUnitStringForChar(cx, c);
    return js_NewStringCopyN(cx, &c, 1);
}

jsint FASTCALL
builtin_String_p_charCodeAt(JSString* str, jsint i)
{
    if (i < 0 || (jsint)JSSTRING_LENGTH(str) <= i)
        return -1;
    return JSSTRING_CHARS(str)[i];
}

jsdouble FASTCALL
builtin_Math_random(JSRuntime* rt)
{
    JS_LOCK_RUNTIME(rt);
    js_random_init(rt);
    jsdouble z = js_random_nextDouble(rt);
    JS_UNLOCK_RUNTIME(rt);
    return z;
}

bool FASTCALL
builtin_EqualStrings(JSString* str1, JSString* str2)
{
    return js_EqualStrings(str1, str2);
}

jsdouble FASTCALL
builtin_StringToNumber(JSContext* cx, JSString* str)
{
    const jschar* bp;
    const jschar* end;
    const jschar* ep;
    jsdouble d;

    JSSTRING_CHARS_AND_END(str, bp, end);
    if ((!js_strtod(cx, bp, end, &ep, &d) ||
         js_SkipWhiteSpace(ep, end) != end) &&
        (!js_strtointeger(cx, bp, end, &ep, 0, &d) ||
         js_SkipWhiteSpace(ep, end) != end)) {
        return *cx->runtime->jsNaN;
    }
    return d;
}

jsint FASTCALL
builtin_StringToInt32(JSContext* cx, JSString* str)
{
    const jschar* bp;
    const jschar* end;
    const jschar* ep;
    jsdouble d;

    JSSTRING_CHARS_AND_END(str, bp, end);
    if (!js_strtod(cx, bp, end, &ep, &d) || js_SkipWhiteSpace(ep, end) != end)
        return 0;
    return (jsint)d;
}

#define LO ARGSIZE_LO
#define F  ARGSIZE_F
#define Q  ARGSIZE_Q

#ifdef DEBUG
#define NAME(op) ,#op
#else
#define NAME(op)
#endif

#define BUILTIN1(op, at0, atr, tr, t0, cse, fold) \
    { (intptr_t)&builtin_##op, (at0 << 2) | atr, cse, fold NAME(op) },
#define BUILTIN2(op, at0, at1, atr, tr, t0, t1, cse, fold) \
    { (intptr_t)&builtin_##op, (at0 << 4) | (at1 << 2) | atr, cse, fold NAME(op) },
#define BUILTIN3(op, at0, at1, at2, atr, tr, t0, t1, t2, cse, fold) \
    { (intptr_t)&builtin_##op, (at0 << 6) | (at1 << 4) | (at2 << 2) | atr, cse, fold NAME(op) },
#define BUILTIN4(op, at0, at1, at2, at3, atr, tr, t0, t1, t2, t3, cse, fold)    \
    { (intptr_t)&builtin_##op, (at0 << 8) | (at1 << 6) | (at2 << 4) | (at3 << 2) | atr, cse, fold NAME(op) },

struct CallInfo builtins[] = {
#include "builtins.tbl"
};
