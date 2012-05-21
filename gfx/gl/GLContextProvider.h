/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GLCONTEXTPROVIDER_H_
#define GLCONTEXTPROVIDER_H_

#include "GLContext.h"
#include "gfxTypes.h"
#include "gfxPoint.h"
#include "nsAutoPtr.h"

class nsIWidget;
class gfxASurface;

namespace mozilla {
namespace gl {

#define IN_GL_CONTEXT_PROVIDER_H

// Null and OSMesa are always there
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderNull
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME

#define GL_CONTEXT_PROVIDER_NAME GLContextProviderOSMesa
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME

#ifdef XP_WIN
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderWGL
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderWGL
#define DEFAULT_IMPL WGL
#endif

#ifdef XP_MACOSX
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderCGL
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderCGL
#endif

#if defined(ANDROID) || defined(MOZ_PLATFORM_MAEMO) || defined(XP_WIN)
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderEGL
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME

#ifndef GL_CONTEXT_PROVIDER_DEFAULT
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderEGL
#endif
#endif

// X11, with XRender optimizations and no GL layer support
#if defined(MOZ_X11) && defined(MOZ_EGL_XRENDER_COMPOSITE) && !defined(GL_CONTEXT_PROVIDER_DEFAULT)
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderEGL
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderEGL
#endif

// X11, but only if we didn't use EGL above
#if defined(MOZ_X11) && !defined(GL_CONTEXT_PROVIDER_DEFAULT)
#define GL_CONTEXT_PROVIDER_NAME GLContextProviderGLX
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT GLContextProviderGLX
#endif

#ifdef MOZ_GL_PROVIDER
#define GL_CONTEXT_PROVIDER_NAME MOZ_GL_PROVIDER
#include "GLContextProviderImpl.h"
#undef GL_CONTEXT_PROVIDER_NAME
#define GL_CONTEXT_PROVIDER_DEFAULT MOZ_GL_PROVIDER
#endif

#ifdef GL_CONTEXT_PROVIDER_DEFAULT
typedef GL_CONTEXT_PROVIDER_DEFAULT GLContextProvider;
#else
typedef GLContextProviderNull GLContextProvider;
#endif

#undef IN_GL_CONTEXT_PROVIDER_H

}
}

#endif
