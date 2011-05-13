/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=99 ft=cpp:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef PageAllocation_h
#define PageAllocation_h

#include "wtfbridge.h"
#include "OSAllocator.h"
#include "PageBlock.h"
#include "assembler/wtf/VMTags.h"

#if WTF_OS_DARWIN
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#endif

#if WTF_OS_HAIKU
#include <OS.h>
#endif

#if WTF_OS_WINDOWS
#include <malloc.h>
#include <windows.h>
#endif

#if WTF_OS_SYMBIAN
#include <e32hal.h>
#include <e32std.h>
#endif

#if WTF_HAVE_ERRNO_H
#include <errno.h>
#endif

#if WTF_HAVE_MMAP
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace WTF {

/*
    PageAllocation

    The PageAllocation class provides a cross-platform memory allocation interface
    with similar capabilities to posix mmap/munmap.  Memory is allocated by calling
    PageAllocation::allocate, and deallocated by calling deallocate on the
    PageAllocation object.  The PageAllocation holds the allocation's base pointer
    and size.

    The allocate method is passed the size required (which must be a multiple of
    the system page size, which can be accessed using PageAllocation::pageSize).
    Callers may also optinally provide a flag indicating the usage (for use by
    system memory usage tracking tools, where implemented), and boolean values
    specifying the required protection (defaulting to writable, non-executable).
*/

class PageAllocation : private PageBlock {
public:
    PageAllocation()
    {
    }

    using PageBlock::size;
    using PageBlock::base;

#ifndef __clang__
    using PageBlock::operator bool;
#else
    // FIXME: This is a workaround for <rdar://problem/8876150>, wherein Clang incorrectly emits an access
    // control warning when a client tries to use operator bool exposed above via "using PageBlock::operator bool".
    operator bool() const { return PageBlock::operator bool(); }
#endif

    static PageAllocation allocate(size_t size, OSAllocator::Usage usage = OSAllocator::UnknownUsage, bool writable = true, bool executable = false)
    {
        ASSERT(isPageAligned(size));
        return PageAllocation(OSAllocator::reserveAndCommit(size, usage, writable, executable), size);
    }

    void deallocate()
    {
        // Clear base & size before calling release; if this is *inside* allocation
        // then we won't be able to clear then after deallocating the memory.
        PageAllocation tmp;
        JSC::std::swap(tmp, *this);

        ASSERT(tmp);
        ASSERT(!*this);

        OSAllocator::decommitAndRelease(tmp.base(), tmp.size());
    }

private:
    PageAllocation(void* base, size_t size)
        : PageBlock(base, size)
    {
    }
};

} // namespace WTF

using WTF::PageAllocation;

#endif // PageAllocation_h
