/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * Implements a workaround for compilers which do not support the C++11 nullptr
 * constant.
 */

#ifndef mozilla_NullPtr_h_
#define mozilla_NullPtr_h_

#if defined(__clang__)
#  ifndef __has_extension
#    define __has_extension __has_feature
#  endif
#  if __has_extension(cxx_nullptr)
#    define MOZ_HAVE_CXX11_NULLPTR
#  endif
#elif defined(__GNUC__)
#  if defined(_GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#    if (__GNUC__ * 1000 + __GNU_MINOR__) >= 4006
#      define MOZ_HAVE_CXX11_NULLPTR
#    endif
#  endif
#elif _MSC_VER >= 1600
# define MOZ_HAVE_CXX11_NULLPTR
#endif

/**
 * Use C++11 nullptr if available; otherwise use __null for gcc, or a 0 literal
 * with the correct size to match the size of a pointer on a given platform.
 */

#ifndef MOZ_HAVE_CXX11_NULLPTR
#  if defined(__GNUC__)
#    define nullptr __null
#  elif defined(_WIN64)
#    define nullptr 0LL
#  else
#    define nullptr 0L
#  endif
#endif

#endif  /* mozilla_NullPtr_h_ */
