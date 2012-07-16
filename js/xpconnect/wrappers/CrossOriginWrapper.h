/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99 ft=cpp:
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __CrossOriginWrapper_h__
#define __CrossOriginWrapper_h__

#include "mozilla/Attributes.h"

#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class WaiveXrayWrapper : public js::CrossCompartmentWrapper {
  public:
    WaiveXrayWrapper(unsigned flags);
    virtual ~WaiveXrayWrapper();

    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                       bool set, js::PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          bool set, js::PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     js::Value *vp) MOZ_OVERRIDE;

    virtual bool call(JSContext *cx, JSObject *wrapper, unsigned argc, js::Value *vp) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JSObject *wrapper,
                           unsigned argc, js::Value *argv, js::Value *rval) MOZ_OVERRIDE;

    static WaiveXrayWrapper singleton;
};

}

#endif
