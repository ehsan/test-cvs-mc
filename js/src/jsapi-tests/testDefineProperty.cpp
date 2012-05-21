/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=99:
 */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "tests.h"

BEGIN_TEST(testDefineProperty_bug564344)
{
    jsvalRoot x(cx);
    EVAL("function f() {}\n"
         "var x = {p: f};\n"
         "x.p();  // brand x's scope\n"
         "x;", x.addr());

    JSObject *obj = JSVAL_TO_OBJECT(x.value());
    for (int i = 0; i < 2; i++)
        CHECK(JS_DefineProperty(cx, obj, "q", JSVAL_VOID, NULL, NULL, JSPROP_SHARED));
    return true;
}
END_TEST(testDefineProperty_bug564344)
