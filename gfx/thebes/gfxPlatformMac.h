/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GFX_PLATFORM_MAC_H
#define GFX_PLATFORM_MAC_H

#include "nsTArray.h"
#include "gfxPlatform.h"

#define MAC_OS_X_VERSION_10_4_HEX 0x00001040
#define MAC_OS_X_VERSION_10_5_HEX 0x00001050
#define MAC_OS_X_VERSION_10_6_HEX 0x00001060
#define MAC_OS_X_VERSION_10_7_HEX 0x00001070

#define MAC_OS_X_MAJOR_VERSION_MASK 0xFFFFFFF0U

class gfxTextRun;
class gfxFontFamily;
class mozilla::gfx::DrawTarget;

class THEBES_API gfxPlatformMac : public gfxPlatform {
public:
    gfxPlatformMac();
    virtual ~gfxPlatformMac();

    static gfxPlatformMac *GetPlatform() {
        return (gfxPlatformMac*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxASurface::gfxContentType contentType);
    virtual already_AddRefed<gfxASurface>
      CreateOffscreenImageSurface(const gfxIntSize& aSize,
                                  gfxASurface::gfxContentType aContentType);
    
    already_AddRefed<gfxASurface> OptimizeImage(gfxImageSurface *aSurface,
                                                gfxASurface::gfxImageFormat format);
    
    mozilla::RefPtr<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont);

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, bool& aAborted);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle,
                                  gfxUserFontSet *aUserFontSet);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    virtual gfxPlatformFontList* CreatePlatformFontList();

    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const uint8_t *aFontData,
                                           uint32_t aLength);

    bool IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags);

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);
    nsresult UpdateFontList();

    virtual void GetCommonFallbackFonts(const uint32_t aCh,
                                        int32_t aRunScript,
                                        nsTArray<const char*>& aFontList);

    // Returns the OS X version as returned from Gestalt(gestaltSystemVersion, ...)
    // Ex: Mac OS X 10.4.x ==> 0x104x
    int32_t OSXVersion();

    bool UseAcceleratedCanvas();

    // lower threshold on font anti-aliasing
    uint32_t GetAntiAliasingThreshold() { return mFontAntiAliasingThreshold; }

    virtual already_AddRefed<gfxASurface>
    GetThebesSurfaceForDrawTarget(mozilla::gfx::DrawTarget *aTarget);
private:
    virtual qcms_profile* GetPlatformCMSOutputProfile();
    
    // read in the pref value for the lower threshold on font anti-aliasing
    static uint32_t ReadAntiAliasingThreshold();    
    
    int32_t mOSXVersion;
    uint32_t mFontAntiAliasingThreshold;
};

#endif /* GFX_PLATFORM_MAC_H */
