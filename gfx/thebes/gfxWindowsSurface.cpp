/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Oracle Corporation code.
 *
 * The Initial Developer of the Original Code is Oracle Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Stuart Parmenter <pavlov@pavlov.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "gfxWindowsSurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"

#include "cairo.h"
#include "cairo-win32.h"

#include "nsString.h"

gfxWindowsSurface::gfxWindowsSurface(HWND wnd, PRUint32 flags) :
    mOwnsDC(true), mForPrinting(false), mWnd(wnd)
{
    mDC = ::GetDC(mWnd);
    InitWithDC(flags);
}

gfxWindowsSurface::gfxWindowsSurface(HDC dc, PRUint32 flags) :
    mOwnsDC(false), mForPrinting(false), mDC(dc), mWnd(nsnull)
{
    if (flags & FLAG_TAKE_DC)
        mOwnsDC = true;

#ifdef NS_PRINTING
    if (flags & FLAG_FOR_PRINTING) {
        Init(cairo_win32_printing_surface_create(mDC));
        mForPrinting = true;
    } else
#endif
    InitWithDC(flags);
}

void
gfxWindowsSurface::MakeInvalid(gfxIntSize& size)
{
    size = gfxIntSize(-1, -1);
}

gfxWindowsSurface::gfxWindowsSurface(const gfxIntSize& realSize, gfxImageFormat imageFormat) :
    mOwnsDC(false), mForPrinting(false), mWnd(nsnull)
{
    gfxIntSize size(realSize);
    if (!CheckSurfaceSize(size))
        MakeInvalid(size);

    cairo_surface_t *surf = cairo_win32_surface_create_with_dib((cairo_format_t)imageFormat,
                                                                size.width, size.height);

    Init(surf);

    RecordMemoryUsed(size.width * size.height * 4 + sizeof(gfxWindowsSurface));

    if (CairoStatus() == 0)
        mDC = cairo_win32_surface_get_dc(CairoSurface());
    else
        mDC = nsnull;
}

gfxWindowsSurface::gfxWindowsSurface(HDC dc, const gfxIntSize& realSize, gfxImageFormat imageFormat) :
    mOwnsDC(false), mForPrinting(false), mWnd(nsnull)
{
    gfxIntSize size(realSize);
    if (!CheckSurfaceSize(size))
        MakeInvalid(size);

    cairo_surface_t *surf = cairo_win32_surface_create_with_ddb(dc, (cairo_format_t)imageFormat,
                                                                size.width, size.height);

    Init(surf);

    if (mSurfaceValid) {
        // DDBs will generally only use 3 bytes per pixel when RGB24
        int bytesPerPixel = ((imageFormat == gfxASurface::ImageFormatRGB24) ? 3 : 4);
        RecordMemoryUsed(size.width * size.height * bytesPerPixel + sizeof(gfxWindowsSurface));
    }

    if (CairoStatus() == 0)
        mDC = cairo_win32_surface_get_dc(CairoSurface());
    else
        mDC = nsnull;
}

gfxWindowsSurface::gfxWindowsSurface(cairo_surface_t *csurf) :
    mOwnsDC(false), mForPrinting(false), mWnd(nsnull)
{
    if (cairo_surface_status(csurf) == 0)
        mDC = cairo_win32_surface_get_dc(csurf);
    else
        mDC = nsnull;

    if (cairo_surface_get_type(csurf) == CAIRO_SURFACE_TYPE_WIN32_PRINTING)
        mForPrinting = true;

    Init(csurf, true);
}

void
gfxWindowsSurface::InitWithDC(PRUint32 flags)
{
    if (flags & FLAG_IS_TRANSPARENT) {
        Init(cairo_win32_surface_create_with_alpha(mDC));
    } else {
        Init(cairo_win32_surface_create(mDC));
    }
}

already_AddRefed<gfxASurface>
gfxWindowsSurface::CreateSimilarSurface(gfxContentType aContent,
                                        const gfxIntSize& aSize)
{
    if (!mSurface || !mSurfaceValid) {
        return nsnull;
    }

    cairo_surface_t *surface;
    if (GetContentType() == CONTENT_COLOR_ALPHA) {
        // When creating a similar surface to a transparent surface, ensure
        // the new surface uses a DIB. cairo_surface_create_similar won't
        // use  a DIB for a CONTENT_COLOR surface if this surface doesn't
        // have a DIB (e.g. if we're a transparent window surface). But
        // we need a DIB to perform well if the new surface is composited into
        // a surface that's the result of create_similar(CONTENT_COLOR_ALPHA)
        // (e.g. a backbuffer for the window) --- that new surface *would*
        // have a DIB.
        surface =
          cairo_win32_surface_create_with_dib(cairo_format_t(gfxASurface::FormatFromContent(aContent)),
                                              aSize.width, aSize.height);
    } else {
        surface =
          cairo_surface_create_similar(mSurface, cairo_content_t(aContent),
                                       aSize.width, aSize.height);
    }

    if (cairo_surface_status(surface)) {
        cairo_surface_destroy(surface);
        return nsnull;
    }

    nsRefPtr<gfxASurface> result = Wrap(surface);
    cairo_surface_destroy(surface);
    return result.forget();
}

gfxWindowsSurface::~gfxWindowsSurface()
{
    if (mOwnsDC) {
        if (mWnd)
            ::ReleaseDC(mWnd, mDC);
        else
            ::DeleteDC(mDC);
    }
}

HDC
gfxWindowsSurface::GetDCWithClip(gfxContext *ctx)
{
    return cairo_win32_get_dc_with_clip (ctx->GetCairo());
}

already_AddRefed<gfxImageSurface>
gfxWindowsSurface::GetAsImageSurface()
{
    if (!mSurfaceValid) {
        NS_WARNING ("GetImageSurface on an invalid (null) surface; who's calling this without checking for surface errors?");
        return nsnull;
    }

    NS_ASSERTION(CairoSurface() != nsnull, "CairoSurface() shouldn't be nsnull when mSurfaceValid is TRUE!");

    if (mForPrinting)
        return nsnull;

    cairo_surface_t *isurf = cairo_win32_surface_get_image(CairoSurface());
    if (!isurf)
        return nsnull;

    nsRefPtr<gfxASurface> asurf = gfxASurface::Wrap(isurf);
    gfxImageSurface *imgsurf = (gfxImageSurface*) asurf.get();
    NS_ADDREF(imgsurf);
    return imgsurf;
}

already_AddRefed<gfxWindowsSurface>
gfxWindowsSurface::OptimizeToDDB(HDC dc, const gfxIntSize& size, gfxImageFormat format)
{
    if (mForPrinting)
        return nsnull;

    if (format != ImageFormatRGB24)
        return nsnull;

    nsRefPtr<gfxWindowsSurface> wsurf = new gfxWindowsSurface(dc, size, format);
    if (wsurf->CairoStatus() != 0)
        return nsnull;

    gfxContext tmpCtx(wsurf);
    tmpCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpCtx.SetSource(this);
    tmpCtx.Paint();

    gfxWindowsSurface *raw = (gfxWindowsSurface*) (wsurf.get());
    NS_ADDREF(raw);

    // we let the new DDB surfaces be converted back to dibsections if
    // acquire_source_image is called on them
    cairo_win32_surface_set_can_convert_to_dib(raw->CairoSurface(), TRUE);

    return raw;
}

nsresult
gfxWindowsSurface::BeginPrinting(const nsAString& aTitle,
                                 const nsAString& aPrintToFileName)
{
#ifdef NS_PRINTING
#define DOC_TITLE_LENGTH (MAX_PATH-1)
    DOCINFOW docinfo;

    nsString titleStr(aTitle);
    if (titleStr.Length() > DOC_TITLE_LENGTH) {
        titleStr.SetLength(DOC_TITLE_LENGTH-3);
        titleStr.AppendLiteral("...");
    }

    nsString docName(aPrintToFileName);
    docinfo.cbSize = sizeof(docinfo);
    docinfo.lpszDocName = titleStr.Length() > 0 ? titleStr.get() : L"Mozilla Document";
    docinfo.lpszOutput = docName.Length() > 0 ? docName.get() : nsnull;
    docinfo.lpszDatatype = NULL;
    docinfo.fwType = 0;

    ::StartDocW(mDC, &docinfo);

    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

nsresult
gfxWindowsSurface::EndPrinting()
{
#ifdef NS_PRINTING
    int result = ::EndDoc(mDC);
    if (result <= 0)
        return NS_ERROR_FAILURE;

    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

nsresult
gfxWindowsSurface::AbortPrinting()
{
#ifdef NS_PRINTING
    int result = ::AbortDoc(mDC);
    if (result <= 0)
        return NS_ERROR_FAILURE;
    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

nsresult
gfxWindowsSurface::BeginPage()
{
#ifdef NS_PRINTING
    int result = ::StartPage(mDC);
    if (result <= 0)
        return NS_ERROR_FAILURE;
    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

nsresult
gfxWindowsSurface::EndPage()
{
#ifdef NS_PRINTING
    if (mForPrinting)
        cairo_surface_show_page(CairoSurface());
    int result = ::EndPage(mDC);
    if (result <= 0)
        return NS_ERROR_FAILURE;
    return NS_OK;
#else
    return NS_ERROR_FAILURE;
#endif
}

PRInt32
gfxWindowsSurface::GetDefaultContextFlags() const
{
    if (mForPrinting)
        return gfxContext::FLAG_SIMPLIFY_OPERATORS |
               gfxContext::FLAG_DISABLE_SNAPPING |
               gfxContext::FLAG_DISABLE_COPY_BACKGROUND;

    return 0;
}

const gfxIntSize 
gfxWindowsSurface::GetSize() const
{
    if (!mSurfaceValid) {
        NS_WARNING ("GetImageSurface on an invalid (null) surface; who's calling this without checking for surface errors?");
        return gfxIntSize(-1, -1);
    }

    NS_ASSERTION(mSurface != nsnull, "CairoSurface() shouldn't be nsnull when mSurfaceValid is TRUE!");

    return gfxIntSize(cairo_win32_surface_get_width(mSurface),
                      cairo_win32_surface_get_height(mSurface));
}

gfxASurface::MemoryLocation
gfxWindowsSurface::GetMemoryLocation() const
{
    return MEMORY_IN_PROCESS_NONHEAP;
}
