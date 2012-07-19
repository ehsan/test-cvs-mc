/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99:
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jsion_ion_alloc_policy_h__
#define jsion_ion_alloc_policy_h__

#include "jscntxt.h"
#include "ds/LifoAlloc.h"

#include "Ion.h"
#include "InlineList.h"

namespace js {
namespace ion {

class IonAllocPolicy
{
  public:
    void *malloc_(size_t bytes) {
        JSContext *cx = GetIonContext()->cx;
        return cx->tempLifoAlloc().alloc(bytes);
    }
    void *realloc_(void *p, size_t oldBytes, size_t bytes) {
        void *n = malloc_(bytes);
        if (!n)
            return n;
        memcpy(n, p, Min(oldBytes, bytes));
        return n;
    }
    void free_(void *p) {
    }
    void reportAllocOverflow() const {
    }
};

class TempAllocator
{
    LifoAlloc *lifoAlloc_;
    void *mark_;

  public:
    TempAllocator(LifoAlloc *lifoAlloc)
      : lifoAlloc_(lifoAlloc),
        mark_(lifoAlloc->mark())
    { }

    ~TempAllocator()
    {
        lifoAlloc_->release(mark_);
    }

    void *allocateInfallible(size_t bytes)
    {
        void *p = lifoAlloc_->allocInfallible(bytes);
        JS_ASSERT(p);
        return p;
    }

    void *allocate(size_t bytes)
    {
        void *p = lifoAlloc_->alloc(bytes);
        if (!ensureBallast())
            return NULL;
        return p;
    }

    bool ensureBallast() {
        // Most infallible Ion allocations are small, so we use a ballast of
        // ~16K for now.
        return lifoAlloc_->ensureUnusedApproximate(16 * 1024);
    }
};

class AutoIonContextAlloc
{
    TempAllocator tempAlloc_;
    IonContext *icx_;
    TempAllocator *prevAlloc_;

  public:
    explicit AutoIonContextAlloc(JSContext *cx)
      : tempAlloc_(&cx->tempLifoAlloc()),
        icx_(GetIonContext()),
        prevAlloc_(icx_->temp)
    {
        icx_->temp = &tempAlloc_;
    }

    ~AutoIonContextAlloc() {
        JS_ASSERT(icx_->temp == &tempAlloc_);
        icx_->temp = prevAlloc_;
    }
};

struct TempObject
{
    inline void *operator new(size_t nbytes) {
        return GetIonContext()->temp->allocateInfallible(nbytes);
    }

  public:
    inline void *operator new(size_t nbytes, void *pos) {
        return pos;
    }
};

template <typename T>
class TempObjectPool
{
    InlineForwardList<T> freed_;

  public:
    T *allocate() {
        if (freed_.empty())
            return new T();
        return freed_.popFront();
    }
    void free(T *obj) {
        freed_.pushFront(obj);
    }
};

} // namespace ion
} // namespace js

#endif // jsion_temp_alloc_policy_h__

