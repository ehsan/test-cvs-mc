/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GFX_GDIFONTLIST_H
#define GFX_GDIFONTLIST_H

#include "gfxWindowsPlatform.h"
#include "gfxPlatformFontList.h"
#include "nsGkAtoms.h"

#include <windows.h>

class AutoDC // get the global device context, and auto-release it on destruction
{
public:
    AutoDC() {
        mDC = ::GetDC(NULL);
    }

    ~AutoDC() {
        ::ReleaseDC(NULL, mDC);
    }

    HDC GetDC() {
        return mDC;
    }

private:
    HDC mDC;
};

class AutoSelectFont // select a font into the given DC, and auto-restore
{
public:
    AutoSelectFont(HDC aDC, LOGFONTW *aLogFont)
        : mOwnsFont(false)
    {
        mFont = ::CreateFontIndirectW(aLogFont);
        if (mFont) {
            mOwnsFont = true;
            mDC = aDC;
            mOldFont = (HFONT)::SelectObject(aDC, mFont);
        } else {
            mOldFont = NULL;
        }
    }

    AutoSelectFont(HDC aDC, HFONT aFont)
        : mOwnsFont(false)
    {
        mDC = aDC;
        mFont = aFont;
        mOldFont = (HFONT)::SelectObject(aDC, aFont);
    }

    ~AutoSelectFont() {
        if (mOldFont) {
            ::SelectObject(mDC, mOldFont);
            if (mOwnsFont) {
                ::DeleteObject(mFont);
            }
        }
    }

    bool IsValid() const {
        return mFont != NULL;
    }

    HFONT GetFont() const {
        return mFont;
    }

private:
    HDC    mDC;
    HFONT  mFont;
    HFONT  mOldFont;
    bool mOwnsFont;
};

/**
 * List of different types of fonts we support on Windows.
 * These can generally be lumped in to 3 categories where we have to
 * do special things:  Really old fonts bitmap and vector fonts (device
 * and raster), Type 1 fonts, and TrueType/OpenType fonts.
 * 
 * This list is sorted in order from least prefered to most prefered.
 * We prefer Type1 fonts over OpenType fonts to avoid falling back to
 * things like Arial (opentype) when you ask for Helvetica (type1)
 **/
enum gfxWindowsFontType {
    GFX_FONT_TYPE_UNKNOWN = 0,
    GFX_FONT_TYPE_DEVICE,
    GFX_FONT_TYPE_RASTER,
    GFX_FONT_TYPE_TRUETYPE,
    GFX_FONT_TYPE_PS_OPENTYPE,
    GFX_FONT_TYPE_TT_OPENTYPE,
    GFX_FONT_TYPE_TYPE1
};

// A single member of a font family (i.e. a single face, such as Times Italic)
// represented as a LOGFONT that will resolve to the correct face.
// This replaces FontEntry from gfxWindowsFonts.h/cpp.
class GDIFontEntry : public gfxFontEntry
{
public:
    LPLOGFONTW GetLogFont() { return &mLogFont; }

    nsresult ReadCMAP();

    virtual bool IsSymbolFont();

    void FillLogFont(LOGFONTW *aLogFont, PRUint16 aWeight, gfxFloat aSize,
                     bool aUseCleartype);

    static gfxWindowsFontType DetermineFontType(const NEWTEXTMETRICW& metrics, 
                                                DWORD fontType)
    {
        gfxWindowsFontType feType;
        if (metrics.ntmFlags & NTM_TYPE1)
            feType = GFX_FONT_TYPE_TYPE1;
        else if (metrics.ntmFlags & NTM_PS_OPENTYPE)
            feType = GFX_FONT_TYPE_PS_OPENTYPE;
        else if (metrics.ntmFlags & NTM_TT_OPENTYPE)
            feType = GFX_FONT_TYPE_TT_OPENTYPE;
        else if (fontType == TRUETYPE_FONTTYPE)
            feType = GFX_FONT_TYPE_TRUETYPE;
        else if (fontType == RASTER_FONTTYPE)
            feType = GFX_FONT_TYPE_RASTER;
        else if (fontType == DEVICE_FONTTYPE)
            feType = GFX_FONT_TYPE_DEVICE;
        else
            feType = GFX_FONT_TYPE_UNKNOWN;
        
        return feType;
    }

    bool IsType1() const {
        return (mFontType == GFX_FONT_TYPE_TYPE1);
    }

    bool IsTrueType() const {
        return (mFontType == GFX_FONT_TYPE_TRUETYPE ||
                mFontType == GFX_FONT_TYPE_PS_OPENTYPE ||
                mFontType == GFX_FONT_TYPE_TT_OPENTYPE);
    }

    virtual bool MatchesGenericFamily(const nsACString& aGeneric) const {
        if (aGeneric.IsEmpty()) {
            return true;
        }

        // Japanese 'Mincho' fonts do not belong to FF_MODERN even if
        // they are fixed pitch because they have variable stroke width.
        if (mWindowsFamily == FF_ROMAN && mWindowsPitch & FIXED_PITCH) {
            return aGeneric.EqualsLiteral("monospace");
        }

        // Japanese 'Gothic' fonts do not belong to FF_SWISS even if
        // they are variable pitch because they have constant stroke width.
        if (mWindowsFamily == FF_MODERN && mWindowsPitch & VARIABLE_PITCH) {
            return aGeneric.EqualsLiteral("sans-serif");
        }

        // All other fonts will be grouped correctly using family...
        switch (mWindowsFamily) {
        case FF_DONTCARE:
            return false;
        case FF_ROMAN:
            return aGeneric.EqualsLiteral("serif");
        case FF_SWISS:
            return aGeneric.EqualsLiteral("sans-serif");
        case FF_MODERN:
            return aGeneric.EqualsLiteral("monospace");
        case FF_SCRIPT:
            return aGeneric.EqualsLiteral("cursive");
        case FF_DECORATIVE:
            return aGeneric.EqualsLiteral("fantasy");
        }

        return false;
    }

    virtual bool SupportsLangGroup(nsIAtom* aLangGroup) const {
        if (!aLangGroup || aLangGroup == nsGkAtoms::Unicode) {
            return true;
        }

        PRInt16 bit = -1;

        /* map our langgroup names in to Windows charset bits */
        if (aLangGroup == nsGkAtoms::x_western) {
            bit = ANSI_CHARSET;
        } else if (aLangGroup == nsGkAtoms::Japanese) {
            bit = SHIFTJIS_CHARSET;
        } else if (aLangGroup == nsGkAtoms::ko) {
            bit = HANGEUL_CHARSET;
        } else if (aLangGroup == nsGkAtoms::ko_xxx) {
            bit = JOHAB_CHARSET;
        } else if (aLangGroup == nsGkAtoms::zh_cn) {
            bit = GB2312_CHARSET;
        } else if (aLangGroup == nsGkAtoms::zh_tw) {
            bit = CHINESEBIG5_CHARSET;
        } else if (aLangGroup == nsGkAtoms::el_) {
            bit = GREEK_CHARSET;
        } else if (aLangGroup == nsGkAtoms::tr) {
            bit = TURKISH_CHARSET;
        } else if (aLangGroup == nsGkAtoms::he) {
            bit = HEBREW_CHARSET;
        } else if (aLangGroup == nsGkAtoms::ar) {
            bit = ARABIC_CHARSET;
        } else if (aLangGroup == nsGkAtoms::x_baltic) {
            bit = BALTIC_CHARSET;
        } else if (aLangGroup == nsGkAtoms::x_cyrillic) {
            bit = RUSSIAN_CHARSET;
        } else if (aLangGroup == nsGkAtoms::th) {
            bit = THAI_CHARSET;
        } else if (aLangGroup == nsGkAtoms::x_central_euro) {
            bit = EASTEUROPE_CHARSET;
        } else if (aLangGroup == nsGkAtoms::x_symbol) {
            bit = SYMBOL_CHARSET;
        }

        if (bit != -1) {
            return mCharset.test(bit);
        }

        return false;
    }

    virtual bool SupportsRange(PRUint8 range) {
        return mUnicodeRanges.test(range);
    }

    virtual bool SkipDuringSystemFallback() { 
        return !HasCmapTable(); // explicitly skip non-SFNT fonts
    }

    virtual bool TestCharacterMap(PRUint32 aCh);

    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;

    // create a font entry for a font with a given name
    static GDIFontEntry* CreateFontEntry(const nsAString& aName,
                                         gfxWindowsFontType aFontType,
                                         bool aItalic,
                                         PRUint16 aWeight, PRInt16 aStretch,
                                         gfxUserFontData* aUserFontData,
                                         bool aFamilyHasItalicFace);

    // create a font entry for a font referenced by its fullname
    static GDIFontEntry* LoadLocalFont(const gfxProxyFontEntry &aProxyEntry,
                                       const nsAString& aFullname);

    PRUint8 mWindowsFamily;
    PRUint8 mWindowsPitch;

    gfxWindowsFontType mFontType;
    bool mForceGDI    : 1;

    // For src:local user-fonts, we keep track of whether the platform family
    // contains an italic face, because in this case we can't safely ask GDI
    // to create synthetic italics (oblique) via the LOGFONT.
    // (For other types of font, this is just set to false.)
    bool mFamilyHasItalicFace : 1;

    gfxSparseBitSet mCharset;
    gfxSparseBitSet mUnicodeRanges;

protected:
    friend class gfxWindowsFont;

    GDIFontEntry(const nsAString& aFaceName, gfxWindowsFontType aFontType,
                 bool aItalic, PRUint16 aWeight, PRInt16 aStretch,
                 gfxUserFontData *aUserFontData, bool aFamilyHasItalicFace);

    void InitLogFont(const nsAString& aName, gfxWindowsFontType aFontType);

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold);

    virtual nsresult GetFontTable(PRUint32 aTableTag,
                                  FallibleTArray<PRUint8>& aBuffer);

    LOGFONTW mLogFont;
};

// a single font family, referencing one or more faces
class GDIFontFamily : public gfxFontFamily
{
public:
    GDIFontFamily(nsAString &aName) :
        gfxFontFamily(aName) {}

    virtual void FindStyleVariations();

private:
    static int CALLBACK FamilyAddStylesProc(const ENUMLOGFONTEXW *lpelfe,
                                            const NEWTEXTMETRICEXW *nmetrics,
                                            DWORD fontType, LPARAM data);
};

class gfxGDIFontList : public gfxPlatformFontList {
public:
    static gfxGDIFontList* PlatformFontList() {
        return static_cast<gfxGDIFontList*>(sPlatformFontList);
    }

    // initialize font lists
    virtual nsresult InitFontList();

    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle, bool& aNeedsBold);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData, PRUint32 aLength);

    virtual bool ResolveFontName(const nsAString& aFontName,
                                   nsAString& aResolvedFontName);

    virtual void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;
    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;

private:
    friend class gfxWindowsPlatform;

    gfxGDIFontList();

    void InitializeFontEmbeddingProcs();

    nsresult GetFontSubstitutes();

    static int CALLBACK EnumFontFamExProc(ENUMLOGFONTEXW *lpelfe,
                                          NEWTEXTMETRICEXW *lpntme,
                                          DWORD fontType,
                                          LPARAM lParam);

    typedef nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> FontTable;

    FontTable mFontSubstitutes;
    nsTArray<nsString> mNonExistingFonts;
};

#endif /* GFX_GDIFONTLIST_H */
