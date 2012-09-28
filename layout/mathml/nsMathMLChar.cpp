/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsRenderingContext.h"
#include "gfxPlatform.h"

#include "mozilla/Preferences.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsIPersistentProperties2.h"
#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsNetUtil.h"

#include "mozilla/LookAndFeel.h"
#include "nsCSSRendering.h"
#include "prprf.h"         // For PR_snprintf()

#include "nsDisplayList.h"

#include "nsMathMLOperators.h"
#include "nsMathMLChar.h"

using namespace mozilla;

//#define NOISY_SEARCH 1

// -----------------------------------------------------------------------------
static const PRUnichar   kSpaceCh   = PRUnichar(' ');
static const nsGlyphCode kNullGlyph = {{0, 0}, 0};
typedef enum {eExtension_base, eExtension_variants, eExtension_parts}
  nsMathfontPrefExtension;

// -----------------------------------------------------------------------------
// nsGlyphTable is a class that provides an interface for accessing glyphs
// of stretchy chars. It acts like a table that stores the variants of bigger
// sizes (if any) and the partial glyphs needed to build extensible symbols.
// An instance of nsGlyphTable is associated to one primary font. Extra glyphs
// can be taken in other additional fonts when stretching certain characters.
// These supplementary fonts are referred to as "external" fonts to the table.
//
// A char for which nsGlyphTable::Has(aChar) is true means that the table
// contains some glyphs (bigger and/or partial) that can be used to render
// the char. Bigger sizes (if any) of the char can then be retrieved with
// BigOf(aSize). Partial glyphs can be retrieved with TopOf(), GlueOf(), etc.
//
// A table consists of "nsGlyphCode"s which are viewed either as Unicode
// points or as direct glyph indices, depending on the type of the table.
// XXX The latter is not yet supported.

// General format of MathFont Property Files from which glyph data are
// retrieved:
// -----------------------------------------------------------------------------
// Each font should have its set of glyph data. For example, the glyph data for
// the "Symbol" font and the "MT Extra" font are in "mathfontSymbol.properties"
// and "mathfontMTExtra.properties", respectively. The mathfont property file
// is a set of all the stretchy MathML characters that can be rendered with that
// font using larger and/or partial glyphs. The entry of each stretchy character
// in the mathfont property file gives, in that order, the 4 partial glyphs:
// Top (or Left), Middle, Bottom (or Right), Glue; and the variants of bigger
// sizes (if any).
// A position that is not relevant to a particular character is indicated there
// with the UNICODE REPLACEMENT CHARACTER 0xFFFD.
// -----------------------------------------------------------------------------

#define NS_TABLE_TYPE_UNICODE       0
#define NS_TABLE_TYPE_GLYPH_INDEX   1

#define NS_TABLE_STATE_ERROR       -1
#define NS_TABLE_STATE_EMPTY        0
#define NS_TABLE_STATE_READY        1

// helper to trim off comments from data in a MathFont Property File
static void
Clean(nsString& aValue)
{
  // chop the trailing # comment portion if any ...
  int32_t comment = aValue.RFindChar('#');
  if (comment > 0) aValue.Truncate(comment);
  aValue.CompressWhitespace();
}

// helper to load a MathFont Property File
static nsresult
LoadProperties(const nsString& aName,
               nsCOMPtr<nsIPersistentProperties>& aProperties)
{
  nsAutoString uriStr;
  uriStr.AssignLiteral("resource://gre/res/fonts/mathfont");
  uriStr.Append(aName);
  uriStr.StripWhitespace(); // that may come from aName
  uriStr.AppendLiteral(".properties");
  return NS_LoadPersistentPropertiesFromURISpec(getter_AddRefs(aProperties), 
                                                NS_ConvertUTF16toUTF8(uriStr));
}

// -----------------------------------------------------------------------------

class nsGlyphTable {
public:
  explicit nsGlyphTable(const nsString& aPrimaryFontName)
    : mType(NS_TABLE_TYPE_UNICODE),
      mFontName(1), // ensure space for primary font name.
      mState(NS_TABLE_STATE_EMPTY),
      mCharCache(0)
  {
    MOZ_COUNT_CTOR(nsGlyphTable);
    mFontName.AppendElement(aPrimaryFontName);
  }

  // not a virtual destructor: this class is not intended to be subclassed
  ~nsGlyphTable()
  {
    MOZ_COUNT_DTOR(nsGlyphTable);
  }

  const nsAString& PrimaryFontName() const
  {
    return mFontName[0];
  }

  const nsAString& FontNameFor(const nsGlyphCode& aGlyphCode) const
  {
    return mFontName[aGlyphCode.font];
  }

  // True if this table contains some glyphs (variants and/or parts)
  // or contains child chars that can be used to render this char
  bool Has(nsPresContext* aPresContext, nsMathMLChar* aChar);

  // True if this table contains variants of larger sizes to render this char
  bool HasVariantsOf(nsPresContext* aPresContext, nsMathMLChar* aChar);

  // True if this table contains parts to render this char
  bool HasPartsOf(nsPresContext* aPresContext, nsMathMLChar* aChar);

  // Getters for the parts
  nsGlyphCode TopOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 0);
  }
  nsGlyphCode MiddleOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 1);
  }
  nsGlyphCode BottomOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 2);
  }
  nsGlyphCode GlueOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 3);
  }
  nsGlyphCode BigOf(nsPresContext* aPresContext, nsMathMLChar* aChar,
                    int32_t aSize) {
    return ElementAt(aPresContext, aChar, 4 + aSize);
  }
  nsGlyphCode LeftOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 0);
  }
  nsGlyphCode RightOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 2);
  }

private:
  nsGlyphCode ElementAt(nsPresContext* aPresContext, nsMathMLChar* aChar,
                        uint32_t aPosition);

  // The type is either NS_TABLE_TYPE_UNICODE or NS_TABLE_TYPE_GLYPH_INDEX
  int32_t mType;    
                           
  // mFontName[0] is the primary font associated to this table. The others 
  // are possible "external" fonts for glyphs not in the primary font
  // but which are needed to stretch certain characters in the table
  nsTArray<nsString> mFontName; 
                               
  // Tri-state variable for error/empty/ready
  int32_t mState;

  // The set of glyph data in this table, as provided by the MathFont Property
  // File
  nsCOMPtr<nsIPersistentProperties> mGlyphProperties;

  // For speedy re-use, we always cache the last data used in the table.
  // mCharCache is the Unicode point of the last char that was queried in this
  // table. mGlyphCache is a buffer containing the glyph data associated to
  // that char. For a property line 'key = value' in the MathFont Property File,
  // mCharCache will retain the 'key' -- which is a Unicode point, while
  // mGlyphCache will retain the 'value', which is a consecutive list of
  // nsGlyphCodes, i.e., the pairs of 'code@font' needed by the char -- in
  // which 'code@0' can be specified
  // without the optional '@0'. However, to ease subsequent processing,
  // mGlyphCache excludes the '@' symbol and explicitly inserts all optional '0'
  // that indicates the primary font identifier. Specifically therefore, the
  // k-th glyph is characterized by :
  // 1) mGlyphCache[3*k],mGlyphCache[3*k+1] : its Unicode point (or glyph index
  // -- depending on mType),
  // 2) mGlyphCache[3*k+2] : the numeric identifier of the font where it comes
  // from.
  // A font identifier of '0' means the default primary font associated to this
  // table. Other digits map to the "external" fonts that may have been
  // specified in the MathFont Property File.
  nsString  mGlyphCache;
  PRUnichar mCharCache;
};

nsGlyphCode
nsGlyphTable::ElementAt(nsPresContext* aPresContext, nsMathMLChar* aChar,
                        uint32_t aPosition)
{
  if (mState == NS_TABLE_STATE_ERROR) return kNullGlyph;
  // Load glyph properties if this is the first time we have been here
  if (mState == NS_TABLE_STATE_EMPTY) {
    nsresult rv = LoadProperties(mFontName[0], mGlyphProperties);
#ifdef DEBUG
    nsAutoCString uriStr;
    uriStr.AssignLiteral("resource://gre/res/fonts/mathfont");
    LossyAppendUTF16toASCII(mFontName[0], uriStr);
    uriStr.StripWhitespace(); // that may come from mFontName
    uriStr.AppendLiteral(".properties");
    printf("Loading %s ... %s\n",
            uriStr.get(),
            (NS_FAILED(rv)) ? "Failed" : "Done");
#endif
    if (NS_FAILED(rv)) {
      mState = NS_TABLE_STATE_ERROR; // never waste time with this table again
      return kNullGlyph;
    }
    mState = NS_TABLE_STATE_READY;

    // see if there are external fonts needed for certain chars in this table
    nsAutoCString key;
    nsAutoString value;
    for (int32_t i = 1; ; i++) {
      key.AssignLiteral("external.");
      key.AppendInt(i, 10);
      rv = mGlyphProperties->GetStringProperty(key, value);
      if (NS_FAILED(rv)) break;
      Clean(value);
      mFontName.AppendElement(value); // i.e., mFontName[i] holds this font name
    }
  }

  // Update our cache if it is not associated to this character
  PRUnichar uchar = aChar->mData[0];
  if (mCharCache != uchar) {
    // The key in the property file is interpreted as ASCII and kept
    // as such ...
    char key[10]; PR_snprintf(key, sizeof(key), "\\u%04X", uchar);
    nsAutoString value;
    nsresult rv = mGlyphProperties->GetStringProperty(nsDependentCString(key),
                                                      value);
    if (NS_FAILED(rv)) return kNullGlyph;
    Clean(value);
    // See if this char uses external fonts; e.g., if the 2nd glyph is taken
    // from the external font '1', the property line looks like
    // \uNNNN = \uNNNN\uNNNN@1\uNNNN.
    // This is where mGlyphCache is pre-processed to explicitly store all glyph
    // codes as combined pairs of 'code@font', excluding the '@' separator. This
    // means that mGlyphCache[3*k],mGlyphCache[3*k+1] will later be rendered
    // with mFontName[mGlyphCache[3*k+2]]
    // Note: font identifier is internally an ASCII digit to avoid the null
    // char issue
    nsAutoString buffer;
    int32_t length = value.Length();
    int32_t i = 0; // index in value
    while (i < length) {
      PRUnichar code = value[i];
      ++i;
      buffer.Append(code);
      // Read the next word if we have a non-BMP character.
      if (i < length && NS_IS_HIGH_SURROGATE(code)) {
        code = value[i];
        ++i;
      } else {
        code = PRUnichar('\0');
      }
      buffer.Append(code);

      // See if an external font is needed for the code point.
      // Limit of 9 external fonts
      PRUnichar font = 0;
      if (i+1 < length && value[i] == PRUnichar('@') &&
          value[i+1] >= PRUnichar('0') && value[i+1] <= PRUnichar('9')) {
        ++i;
        font = value[i] - '0';
        ++i;
        if (font >= mFontName.Length()) {
          NS_ERROR("Nonexistent font referenced in glyph table");
          return kNullGlyph;
        }
        // The char cannot be handled if this font is not installed
        if (!mFontName[font].Length()) {
          return kNullGlyph;
        }
      }
      buffer.Append(font);
    }
    // update our cache with the new settings
    mGlyphCache.Assign(buffer);
    mCharCache = uchar;
  }

  // 3* is to account for the code@font pairs
  uint32_t index = 3*aPosition;
  if (index+2 >= mGlyphCache.Length()) return kNullGlyph;
  nsGlyphCode ch;
  ch.code[0] = mGlyphCache.CharAt(index);
  ch.code[1] = mGlyphCache.CharAt(index + 1);
  ch.font = mGlyphCache.CharAt(index + 2);
  return ch.code[0] == PRUnichar(0xFFFD) ? kNullGlyph : ch;
}

bool
nsGlyphTable::Has(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  return HasVariantsOf(aPresContext, aChar) || HasPartsOf(aPresContext, aChar);
}

bool
nsGlyphTable::HasVariantsOf(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  //XXXkt all variants must be in the same file as size 1
  return BigOf(aPresContext, aChar, 1).Exists();
}

bool
nsGlyphTable::HasPartsOf(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  return GlueOf(aPresContext, aChar).Exists() ||
    TopOf(aPresContext, aChar).Exists() ||
    BottomOf(aPresContext, aChar).Exists() ||
    MiddleOf(aPresContext, aChar).Exists();
}

// -----------------------------------------------------------------------------
// This is the list of all the applicable glyph tables.
// We will maintain a single global instance that will only reveal those
// glyph tables that are associated to fonts currently installed on the
// user' system. The class is an XPCOM shutdown observer to allow us to
// free its allocated data at shutdown

class nsGlyphTableList : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsGlyphTable mUnicodeTable;

  nsGlyphTableList()
    : mUnicodeTable(NS_LITERAL_STRING("Unicode"))
  {
    MOZ_COUNT_CTOR(nsGlyphTableList);
  }

  virtual ~nsGlyphTableList()
  {
    MOZ_COUNT_DTOR(nsGlyphTableList);
  }

  nsresult Initialize();
  nsresult Finalize();

  // Add a glyph table in the list, return the new table that was added
  nsGlyphTable*
  AddGlyphTable(const nsString& aPrimaryFontName);

  // Find a glyph table in the list that has a glyph for the given char
  nsGlyphTable*
  GetGlyphTableFor(nsPresContext* aPresContext,
                   nsMathMLChar*  aChar);

  // Find the glyph table in the list corresponding to the given font family.
  nsGlyphTable*
  GetGlyphTableFor(const nsAString& aFamily);

private:
  nsGlyphTable* TableAt(int32_t aIndex) {
    return &mTableList.ElementAt(aIndex);
  }
  int32_t Count() {
    return mTableList.Length();
  }

  // List of glyph tables;
  nsTArray<nsGlyphTable> mTableList;
};

NS_IMPL_ISUPPORTS1(nsGlyphTableList, nsIObserver)

// -----------------------------------------------------------------------------
// Here is the global list of applicable glyph tables that we will be using
static nsGlyphTableList* gGlyphTableList = nullptr;

static bool gInitialized = false;

// XPCOM shutdown observer
NS_IMETHODIMP
nsGlyphTableList::Observe(nsISupports*     aSubject,
                          const char* aTopic,
                          const PRUnichar* someData)
{
  Finalize();
  return NS_OK;
}

// Add an observer to XPCOM shutdown so that we can free our data at shutdown
nsresult
nsGlyphTableList::Initialize()
{
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs)
    return NS_ERROR_FAILURE;

  nsresult rv = obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// Remove our observer and free the memory that were allocated for us
nsresult
nsGlyphTableList::Finalize()
{
  // Remove our observer from the observer service
  nsresult rv = NS_OK;
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs)
    rv = obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
  else
    rv = NS_ERROR_FAILURE;

  gInitialized = false;
  // our oneself will be destroyed when our |Release| is called by the observer
  return rv;
}

nsGlyphTable*
nsGlyphTableList::AddGlyphTable(const nsString& aPrimaryFontName)
{
  // See if there is already a special table for this family.
  nsGlyphTable* glyphTable = GetGlyphTableFor(aPrimaryFontName);
  if (glyphTable != &mUnicodeTable)
    return glyphTable;

  // allocate a table
  glyphTable = mTableList.AppendElement(aPrimaryFontName);
  return glyphTable;
}

nsGlyphTable*
nsGlyphTableList::GetGlyphTableFor(nsPresContext* aPresContext, 
                                   nsMathMLChar*   aChar)
{
  if (mUnicodeTable.Has(aPresContext, aChar))
    return &mUnicodeTable;

  int32_t i;
  for (i = 0; i < Count(); i++) {
    nsGlyphTable* glyphTable = TableAt(i);
    if (glyphTable->Has(aPresContext, aChar)) {
      return glyphTable;
    }
  }
  return nullptr;
}

nsGlyphTable*
nsGlyphTableList::GetGlyphTableFor(const nsAString& aFamily)
{
  for (int32_t i = 0; i < Count(); i++) {
    nsGlyphTable* glyphTable = TableAt(i);
    const nsAString& fontName = glyphTable->PrimaryFontName();
    // TODO: would be nice to consider StripWhitespace and other aliasing
    if (fontName.Equals(aFamily, nsCaseInsensitiveStringComparator())) {
      return glyphTable;
    }
  }
  // Fall back to default Unicode table
  return &mUnicodeTable;
}

// -----------------------------------------------------------------------------

// Lookup the preferences:
// "font.mathfont-family.\uNNNN.base"     -- fonts for the base size
// "font.mathfont-family.\uNNNN.variants" -- fonts for larger glyphs
// "font.mathfont-family.\uNNNN.parts"    -- fonts for partial glyphs
// Given the char code and mode of stretch, retrieve the preferred extension
// font families.
static bool
GetFontExtensionPref(PRUnichar aChar,
                     nsMathfontPrefExtension aExtension, nsString& aValue)
{
  // initialize OUT param
  aValue.Truncate();

  // We are going to try two keys because some users specify their pref as 
  // user_pref("font.mathfont-family.\uNNNN.base", "...") rather than
  // user_pref("font.mathfont-family.\\uNNNN.base", "...").
  // The \uNNNN in the former is interpreted as an UTF16 escape sequence by
  // JavaScript and is converted to the internal UTF8 string that JavaScript
  // uses. 
  // But clueless users who are not savvy of JavaScript have no idea as to what 
  // is going on and are baffled as to why their pref setting is not working.
  // So to save countless explanations, we are going to support both keys.

  static const char* kMathFontPrefix = "font.mathfont-family.";

  nsAutoCString extension;
  switch (aExtension)
  {
    case eExtension_base:
      extension.AssignLiteral(".base");
      break;
    case eExtension_variants:
      extension.AssignLiteral(".variants");
      break;
    case eExtension_parts:
      extension.AssignLiteral(".parts");
      break;
    default:
      return false;
  }

  // .\\uNNNN key
  nsAutoCString key;
  key.AssignASCII(kMathFontPrefix);
  char ustr[10];
  PR_snprintf(ustr, sizeof(ustr), "\\u%04X", aChar);
  key.Append(ustr);
  key.Append(extension);
  // .\uNNNN key
  nsAutoCString alternateKey;
  alternateKey.AssignASCII(kMathFontPrefix);
  NS_ConvertUTF16toUTF8 tmp(&aChar, 1);
  alternateKey.Append(tmp);
  alternateKey.Append(extension);

  aValue = Preferences::GetString(key.get());
  if (aValue.IsEmpty()) {
    aValue = Preferences::GetString(alternateKey.get());
  }
  return !aValue.IsEmpty();
}


static bool
MathFontEnumCallback(const nsString& aFamily, bool aGeneric, void *aData)
{
  if (!gGlyphTableList->AddGlyphTable(aFamily))
    return false; // stop in low-memory situations
  return true; // don't stop
}

static nsresult
InitGlobals(nsPresContext* aPresContext)
{
  NS_ASSERTION(!gInitialized, "Error -- already initialized");
  gInitialized = true;

  // Allocate the placeholders for the preferred parts and variants
  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  gGlyphTableList = new nsGlyphTableList();
  if (gGlyphTableList) {
    rv = gGlyphTableList->Initialize();
  }
  if (NS_FAILED(rv)) {
    delete gGlyphTableList;
    gGlyphTableList = nullptr;
    return rv;
  }
  /*
  else
    The gGlyphTableList has been successfully registered as a shutdown observer.
    It will be deleted at shutdown, even if a failure happens below.
  */

  nsAutoCString key;
  nsAutoString value;
  nsCOMPtr<nsIPersistentProperties> mathfontProp;

  // Add the math fonts in the gGlyphTableList in order of preference ...
  // Note: we only load font-names at this stage. The actual glyph tables will
  // be loaded lazily (see nsGlyphTable::ElementAt()).

  // Load the "mathfont.properties" file
  value.Truncate();
  rv = LoadProperties(value, mathfontProp);
  if (NS_FAILED(rv)) return rv;

  // Get the list of mathfonts having special glyph tables to be used for
  // stretchy characters.
  // We just want to iterate over the font-family list using the
  // callback mechanism that nsFont has...
  nsFont font("", 0, 0, 0, 0, 0, 0);
  NS_NAMED_LITERAL_CSTRING(defaultKey, "font.mathfont-glyph-tables");
  rv = mathfontProp->GetStringProperty(defaultKey, font.name);
  if (NS_FAILED(rv)) return rv;

  // Parse the font list and append an entry for each family to gGlyphTableList
  nsAutoString missingFamilyList;

  font.EnumerateFamilies(MathFontEnumCallback, nullptr);
  return rv;
}

// -----------------------------------------------------------------------------
// And now the implementation of nsMathMLChar

nsStyleContext*
nsMathMLChar::GetStyleContext() const
{
  NS_ASSERTION(mStyleContext, "chars should always have style context");
  return mStyleContext;
}

void
nsMathMLChar::SetStyleContext(nsStyleContext* aStyleContext)
{
  NS_PRECONDITION(aStyleContext, "null ptr");
  if (aStyleContext != mStyleContext) {
    if (mStyleContext)
      mStyleContext->Release();
    if (aStyleContext) {
      mStyleContext = aStyleContext;
      aStyleContext->AddRef();
    }
  }
}

void
nsMathMLChar::SetData(nsPresContext* aPresContext,
                      nsString&       aData)
{
  if (!gInitialized) {
    InitGlobals(aPresContext);
  }
  mData = aData;
  // some assumptions until proven otherwise
  // note that mGlyph is not initialized
  mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
  mBoundingMetrics = nsBoundingMetrics();
  mGlyphTable = nullptr;
  // check if stretching is applicable ...
  if (gGlyphTableList && (1 == mData.Length())) {
    mDirection = nsMathMLOperators::GetStretchyDirection(mData);
    // default tentative table (not the one that is necessarily going
    // to be used)
    mGlyphTable = gGlyphTableList->GetGlyphTableFor(aPresContext, this);
  }
}

// -----------------------------------------------------------------------------
/*
 The Stretch:
 @param aContainerSize - suggested size for the stretched char
 @param aDesiredStretchSize - OUT parameter. The desired size
 after stretching. If no stretching is done, the output will
 simply give the base size.

 How it works?
 Summary:-
 The Stretch() method first looks for a glyph of appropriate
 size; If a glyph is found, it is cached by this object and
 its size is returned in aDesiredStretchSize. The cached
 glyph will then be used at the painting stage.
 If no glyph of appropriate size is found, a search is made
 to see if the char can be built by parts.

 Details:-
 A character gets stretched through the following pipeline :

 1) If the base size of the char is sufficient to cover the
    container' size, we use that. If not, it will still be
    used as a fallback if the other stages in the pipeline fail.
    Issues :
    a) The base size, the parts and the variants of a char can
       be in different fonts. For eg., the base size for '(' should
       come from a normal ascii font if CMEX10 is used, since CMEX10
       only contains the stretched versions. Hence, there are two
       style contexts in use throughout the process. The leaf style
       context of the char holds fonts with which to try to stretch
       the char. The parent style context of the char contains fonts
       for normal rendering. So the parent context is the one used
       to get the initial base size at the start of the pipeline.
    b) For operators that can be largeop's in display mode,
       we will skip the base size even if it fits, so that
       the next stage in the pipeline is given a chance to find
       a largeop variant. If the next stage fails, we fallback
       to the base size.

 2) We search for the first larger variant of the char that fits the
    container' size.  We first search for larger variants using the glyph
    table corresponding to the first existing font specified in the list of
    stretchy fonts held by the leaf style context (from -moz-math-stretchy in
    mathml.css).  Generic fonts are resolved by the preference
    "font.mathfont-family".
    Issues :
    a) the largeop and display settings determine the starting
       size when we do the above search, regardless of whether
       smaller variants already fit the container' size.
    b) if it is a largeopOnly request (i.e., a displaystyle operator
       with largeop=true and stretchy=false), we break after finding
       the first starting variant, regardless of whether that
       variant fits the container's size.

 3) If a variant of appropriate size wasn't found, we see if the char
    can be built by parts using the same glyph table.
    Issue:
       There are chars that have no middle and glue glyphs. For
       such chars, the parts need to be joined using the rule.
       By convention (TeXbook p.225), the descent of the parts is
       zero while their ascent gives the thickness of the rule that
       should be used to join them.

 4) If a match was not found in that glyph table, repeat from 2 to search the
    ordered list of stretchy fonts for the first font with a glyph table that
    provides a fit to the container size.  If no fit is found, the closest fit
    is used.

 Of note:
 When the pipeline completes successfully, the desired size of the
 stretched char can actually be slightly larger or smaller than
 aContainerSize. But it is the responsibility of the caller to
 account for the spacing when setting aContainerSize, and to leave
 any extra margin when placing the stretched char.
*/
// -----------------------------------------------------------------------------


// plain TeX settings (TeXbook p.152)
#define NS_MATHML_DELIMITER_FACTOR             0.901f
#define NS_MATHML_DELIMITER_SHORTFALL_POINTS   5.0f

static bool
IsSizeOK(nsPresContext* aPresContext, nscoord a, nscoord b, uint32_t aHint)
{
  // Normal: True if 'a' is around +/-10% of the target 'b' (10% is
  // 1-DelimiterFactor). This often gives a chance to the base size to
  // win, especially in the context of <mfenced> without tall elements
  // or in sloppy markups without protective <mrow></mrow>
  bool isNormal =
    (aHint & NS_STRETCH_NORMAL)
    && bool(float(NS_ABS(a - b))
              < (1.0f - NS_MATHML_DELIMITER_FACTOR) * float(b));
  // Nearer: True if 'a' is around max{ +/-10% of 'b' , 'b' - 5pt },
  // as documented in The TeXbook, Ch.17, p.152.
  // i.e. within 10% and within 5pt
  bool isNearer = false;
  if (aHint & (NS_STRETCH_NEARER | NS_STRETCH_LARGEOP)) {
    float c = NS_MAX(float(b) * NS_MATHML_DELIMITER_FACTOR,
                     float(b) - nsPresContext::
                     CSSPointsToAppUnits(NS_MATHML_DELIMITER_SHORTFALL_POINTS));
    isNearer = bool(float(NS_ABS(b - a)) <= (float(b) - c));
  }
  // Smaller: Mainly for transitory use, to compare two candidate
  // choices
  bool isSmaller =
    (aHint & NS_STRETCH_SMALLER)
    && bool((float(a) >= (NS_MATHML_DELIMITER_FACTOR * float(b)))
              && (a <= b));
  // Larger: Critical to the sqrt code to ensure that the radical
  // size is tall enough
  bool isLarger =
    (aHint & (NS_STRETCH_LARGER | NS_STRETCH_LARGEOP))
    && bool(a >= b);
  return (isNormal || isSmaller || isNearer || isLarger);
}

static bool
IsSizeBetter(nscoord a, nscoord olda, nscoord b, uint32_t aHint)
{
  if (0 == olda)
    return true;
  if (aHint & (NS_STRETCH_LARGER | NS_STRETCH_LARGEOP))
    return (a >= olda) ? (olda < b) : (a >= b);
  if (aHint & NS_STRETCH_SMALLER)
    return (a <= olda) ? (olda > b) : (a <= b);

  // XXXkt prob want log scale here i.e. 1.5 is closer to 1 than 0.5
  return NS_ABS(a - b) < NS_ABS(olda - b);
}

// We want to place the glyphs even when they don't fit at their
// full extent, i.e., we may clip to tolerate a small amount of
// overlap between the parts. This is important to cater for fonts
// with long glues.
static nscoord
ComputeSizeFromParts(nsPresContext* aPresContext,
                     nsGlyphCode* aGlyphs,
                     nscoord*     aSizes,
                     nscoord      aTargetSize)
{
  enum {first, middle, last, glue};
  // Add the parts that cannot be left out.
  nscoord sum = 0;
  for (int32_t i = first; i <= last; i++) {
    if (aGlyphs[i] != aGlyphs[glue]) {
      sum += aSizes[i];
    }
  }

  // Determine how much is used in joins
  nscoord oneDevPixel = aPresContext->AppUnitsPerDevPixel();
  int32_t joins = aGlyphs[middle] == aGlyphs[glue] ? 1 : 2;

  // Pick a maximum size using a maximum number of glue glyphs that we are
  // prepared to draw for one character.
  const int32_t maxGlyphs = 1000;

  // This also takes into account the fact that, if the glue has no size,
  // then the character can't be lengthened.
  nscoord maxSize = sum - 2 * joins * oneDevPixel + maxGlyphs * aSizes[glue];
  if (maxSize < aTargetSize)
    return maxSize; // settle with the maximum size

  // Get the minimum allowable size using some flex.
  nscoord minSize = NSToCoordRound(NS_MATHML_DELIMITER_FACTOR * sum);

  if (minSize > aTargetSize)
    return minSize; // settle with the minimum size

  // Fill-up the target area
  return aTargetSize;
}

// Insert aFallbackFamilies before the first generic family in or at the end
// of a CSS aFontName.
static void
AddFallbackFonts(nsAString& aFontName, const nsAString& aFallbackFamilies)
{
  if (aFallbackFamilies.IsEmpty())
    return;

  if (aFontName.IsEmpty()) {
    return;
  }

  static const PRUnichar kSingleQuote  = PRUnichar('\'');
  static const PRUnichar kDoubleQuote  = PRUnichar('\"');
  static const PRUnichar kComma        = PRUnichar(',');

  const PRUnichar *p_begin, *p_end;
  aFontName.BeginReading(p_begin);
  aFontName.EndReading(p_end);

  const PRUnichar *p = p_begin;
  const PRUnichar *p_name = nullptr;
  while (p < p_end) {
    while (nsCRT::IsAsciiSpace(*p))
      if (++p == p_end)
        goto insert;

    p_name = p;
    if (*p == kSingleQuote || *p == kDoubleQuote) {
      // quoted font family
      PRUnichar quoteMark = *p;
      if (++p == p_end)
        goto insert;

      // XXX What about CSS character escapes?
      while (*p != quoteMark)
        if (++p == p_end)
          goto insert;

      while (++p != p_end && *p != kComma)
        /* nothing */ ;

    } else {
      // unquoted font family
      const PRUnichar *nameStart = p;
      while (++p != p_end && *p != kComma)
        /* nothing */ ;

      nsAutoString family;
      family = Substring(nameStart, p);
      family.CompressWhitespace(false, true);

      uint8_t generic;
      nsFont::GetGenericID(family, &generic);
      if (generic != kGenericFont_NONE)
        goto insert;
    }

    ++p; // may advance past p_end
  }

  aFontName.Append(NS_LITERAL_STRING(",") + aFallbackFamilies);
  return;

insert:
  if (p_name) {
    aFontName.Insert(aFallbackFamilies + NS_LITERAL_STRING(","),
                     p_name - p_begin);
  }
  else { // whitespace or empty
    aFontName = aFallbackFamilies;
  }
}

// Update the font and rendering context if there is a family change
static bool
SetFontFamily(nsStyleContext*      aStyleContext,
              nsRenderingContext&  aRenderingContext,
              nsFont&              aFont,
              const nsGlyphTable*  aGlyphTable,
              const nsGlyphCode&   aGlyphCode,
              const nsAString&     aDefaultFamily)
{
  const nsAString& family =
    aGlyphCode.font ? aGlyphTable->FontNameFor(aGlyphCode) : aDefaultFamily;
  if (! family.Equals(aFont.name)) {
    nsFont font = aFont;
    font.name = family;
    nsRefPtr<nsFontMetrics> fm;
    aRenderingContext.DeviceContext()->GetMetricsFor(font,
      aStyleContext->GetStyleFont()->mLanguage,
      aStyleContext->PresContext()->GetUserFontSet(),
      *getter_AddRefs(fm));
    // Set the font if it is an unicode table
    // or if the same family name has been found
    if (aGlyphTable == &gGlyphTableList->mUnicodeTable ||
        fm->GetThebesFontGroup()->GetFontAt(0)->GetFontEntry()->
        FamilyName() == family) {
      aFont.name = family;
      aRenderingContext.SetFont(fm);
    } else
        return false; // We did not set the font
  }
  return true;
}

class nsMathMLChar::StretchEnumContext {
public:
  StretchEnumContext(nsMathMLChar*        aChar,
                     nsPresContext*       aPresContext,
                     nsRenderingContext& aRenderingContext,
                     nsStretchDirection   aStretchDirection,
                     nscoord              aTargetSize,
                     uint32_t             aStretchHint,
                     nsBoundingMetrics&   aStretchedMetrics,
                     const nsAString&     aFamilies,
                     bool&              aGlyphFound)
    : mChar(aChar),
      mPresContext(aPresContext),
      mRenderingContext(aRenderingContext),
      mDirection(aStretchDirection),
      mTargetSize(aTargetSize),
      mStretchHint(aStretchHint),
      mBoundingMetrics(aStretchedMetrics),
      mFamilies(aFamilies),
      mTryVariants(true),
      mTryParts(true),
      mGlyphFound(aGlyphFound) {}

  static bool
  EnumCallback(const nsString& aFamily, bool aGeneric, void *aData);

private:
  bool TryVariants(nsGlyphTable* aGlyphTable, const nsAString& aFamily);
  bool TryParts(nsGlyphTable* aGlyphTable, const nsAString& aFamily);

  nsMathMLChar* mChar;
  nsPresContext* mPresContext;
  nsRenderingContext& mRenderingContext;
  const nsStretchDirection mDirection;
  const nscoord mTargetSize;
  const uint32_t mStretchHint;
  nsBoundingMetrics& mBoundingMetrics;
  // Font families to search
  const nsAString& mFamilies;

public:
  bool mTryVariants;
  bool mTryParts;

private:
  nsAutoTArray<nsGlyphTable*,16> mTablesTried;
  nsGlyphTable* mGlyphTable; // for this callback
  bool&       mGlyphFound;
};


// 2. See if there are any glyphs of the appropriate size.
// Returns true if the size is OK, false to keep searching.
// Always updates the char if a better match is found.
bool
nsMathMLChar::StretchEnumContext::TryVariants(nsGlyphTable*    aGlyphTable,
                                              const nsAString& aFamily)
{
  // Use our stretchy style context now that stretching is in progress
  nsStyleContext *sc = mChar->mStyleContext;
  nsFont font = sc->GetStyleFont()->mFont;
  // Ensure mRenderingContext.SetFont will be called:
  font.name.Truncate();

  bool isVertical = (mDirection == NS_STRETCH_DIRECTION_VERTICAL);
  bool largeop = (NS_STRETCH_LARGEOP & mStretchHint) != 0;
  bool largeopOnly =
    largeop && (NS_STRETCH_VARIABLE_MASK & mStretchHint) == 0;
  bool maxWidth = (NS_STRETCH_MAXWIDTH & mStretchHint) != 0;

  nscoord bestSize =
    isVertical ? mBoundingMetrics.ascent + mBoundingMetrics.descent
               : mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing;
  bool haveBetter = false;

  // start at size = 1 (size = 0 is the char at its normal size)
  int32_t size = 1;
#ifdef NOISY_SEARCH
  printf("  searching in %s ...\n",
           NS_LossyConvertUTF16toASCII(aFamily).get());
#endif

  nsGlyphCode ch;
  while ((ch = aGlyphTable->BigOf(mPresContext, mChar, size)).Exists()) {

    if(!SetFontFamily(sc, mRenderingContext, font, aGlyphTable, ch, aFamily)) {
      // if largeopOnly is set, break now
      if (largeopOnly) break;
      ++size;
      continue;
    }

    NS_ASSERTION(maxWidth || ch.code[0] != mChar->mGlyph.code[0] ||
                 ch.code[1] != mChar->mGlyph.code[1] ||
                 !font.name.Equals(mChar->mFamily),
                 "glyph table incorrectly set -- duplicate found");

    nsBoundingMetrics bm = mRenderingContext.GetBoundingMetrics(ch.code,
                                                                ch.Length());
    nscoord charSize =
      isVertical ? bm.ascent + bm.descent
      : bm.rightBearing - bm.leftBearing;

    if (largeopOnly ||
        IsSizeBetter(charSize, bestSize, mTargetSize, mStretchHint)) {
      mGlyphFound = true;
      if (maxWidth) {
        // IsSizeBetter() checked that charSize < maxsize;
        // Leave ascent, descent, and bestsize as these contain maxsize.
        if (mBoundingMetrics.width < bm.width)
          mBoundingMetrics.width = bm.width;
        if (mBoundingMetrics.leftBearing > bm.leftBearing)
          mBoundingMetrics.leftBearing = bm.leftBearing;
        if (mBoundingMetrics.rightBearing < bm.rightBearing)
          mBoundingMetrics.rightBearing = bm.rightBearing;
        // Continue to check other sizes unless largeopOnly
        haveBetter = largeopOnly;
      }
      else {
        mBoundingMetrics = bm;
        haveBetter = true;
        bestSize = charSize;
        mChar->mGlyphTable = aGlyphTable;
        mChar->mGlyph = ch;
        mChar->mFamily = font.name;
      }
#ifdef NOISY_SEARCH
      printf("    size:%d Current best\n", size);
#endif
    }
    else {
#ifdef NOISY_SEARCH
      printf("    size:%d Rejected!\n", size);
#endif
      if (haveBetter)
        break; // Not making an futher progress, stop searching
    }

    // if largeopOnly is set, break now
    if (largeopOnly) break;
    ++size;
  }

  return haveBetter &&
    (largeopOnly ||
     IsSizeOK(mPresContext, bestSize, mTargetSize, mStretchHint));
}

// 3. Build by parts.
// Returns true if the size is OK, false to keep searching.
// Always updates the char if a better match is found.
bool
nsMathMLChar::StretchEnumContext::TryParts(nsGlyphTable*    aGlyphTable,
                                           const nsAString& aFamily)
{
  if (!aGlyphTable->HasPartsOf(mPresContext, mChar))
    return false; // to next table

  // See if the parts of this table fit in the desired space //////////////////

  // Use our stretchy style context now that stretching is in progress
  nsFont font = mChar->mStyleContext->GetStyleFont()->mFont;
  // Ensure mRenderingContext.SetFont will be called:
  font.name.Truncate();

  // Compute the bounding metrics of all partial glyphs
  nsGlyphCode chdata[4];
  nsBoundingMetrics bmdata[4];
  nscoord sizedata[4];
  nsGlyphCode glue = aGlyphTable->GlueOf(mPresContext, mChar);

  bool isVertical = (mDirection == NS_STRETCH_DIRECTION_VERTICAL);
  bool maxWidth = (NS_STRETCH_MAXWIDTH & mStretchHint) != 0;

  for (int32_t i = 0; i < 4; i++) {
    nsGlyphCode ch;
    switch (i) {
    case 0: ch = aGlyphTable->TopOf(mPresContext, mChar);    break;
    case 1: ch = aGlyphTable->MiddleOf(mPresContext, mChar); break;
    case 2: ch = aGlyphTable->BottomOf(mPresContext, mChar); break;
    case 3: ch = glue;                                       break;
    }
    // empty slots are filled with the glue if it is not null
    if (!ch.Exists()) ch = glue;
    chdata[i] = ch;
    if (!ch.Exists()) {
      // Null glue indicates that a rule will be drawn, which can stretch to
      // fill any space.  Leave bounding metrics at 0.
      sizedata[i] = mTargetSize;
    }
    else {
      if (!SetFontFamily(mChar->mStyleContext, mRenderingContext,
                         font, aGlyphTable, ch, aFamily))
        return false;

      nsBoundingMetrics bm = mRenderingContext.GetBoundingMetrics(ch.code,
                                                                  ch.Length());

      // TODO: For the generic Unicode table, ideally we should check that the
      // glyphs are actually found and that they each come from the same
      // font.
      bmdata[i] = bm;
      sizedata[i] = isVertical ? bm.ascent + bm.descent
                               : bm.rightBearing - bm.leftBearing;
    }
  }

  // Build by parts if we have successfully computed the
  // bounding metrics of all parts.
  nscoord computedSize = ComputeSizeFromParts(mPresContext, chdata, sizedata,
                                              mTargetSize);

  nscoord currentSize =
    isVertical ? mBoundingMetrics.ascent + mBoundingMetrics.descent
               : mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing;

  if (!IsSizeBetter(computedSize, currentSize, mTargetSize, mStretchHint)) {
#ifdef NOISY_SEARCH
    printf("    Font %s Rejected!\n",
           NS_LossyConvertUTF16toASCII(fontName).get());
#endif
    return false; // to next table
  }

#ifdef NOISY_SEARCH
  printf("    Font %s Current best!\n",
         NS_LossyConvertUTF16toASCII(fontName).get());
#endif

  // The computed size is the best we have found so far...
  // now is the time to compute and cache our bounding metrics
  if (isVertical) {
    int32_t i;
    nscoord lbearing;
    nscoord rbearing;
    nscoord width;
    if (maxWidth) {
      lbearing = mBoundingMetrics.leftBearing;
      rbearing = mBoundingMetrics.rightBearing;
      width = mBoundingMetrics.width;
      i = 0;
    }
    else {
      lbearing = bmdata[0].leftBearing;
      rbearing = bmdata[0].rightBearing;
      width = bmdata[0].width;
      i = 1;
    }
    for (; i < 4; i++) {
      const nsBoundingMetrics& bm = bmdata[i];
      if (width < bm.width) width = bm.width;
      if (lbearing > bm.leftBearing) lbearing = bm.leftBearing;
      if (rbearing < bm.rightBearing) rbearing = bm.rightBearing;
    }
    mBoundingMetrics.width = width;
    // When maxWidth, updating ascent and descent indicates that no characters
    // larger than this character's minimum size need to be checked as they
    // will not be used.
    mBoundingMetrics.ascent = bmdata[0].ascent; // not used except with descent
                                                // for height
    mBoundingMetrics.descent = computedSize - mBoundingMetrics.ascent;
    mBoundingMetrics.leftBearing = lbearing;
    mBoundingMetrics.rightBearing = rbearing;
  }
  else {
    nscoord ascent = bmdata[0].ascent;
    nscoord descent = bmdata[0].descent;
    for (int32_t i = 1; i < 4; i++) {
      const nsBoundingMetrics& bm = bmdata[i];
      if (ascent < bm.ascent) ascent = bm.ascent;
      if (descent < bm.descent) descent = bm.descent;
    }
    mBoundingMetrics.width = computedSize;
    mBoundingMetrics.ascent = ascent;
    mBoundingMetrics.descent = descent;
    mBoundingMetrics.leftBearing = 0;
    mBoundingMetrics.rightBearing = computedSize;
  }
  mGlyphFound = true;
  if (maxWidth)
    return false; // Continue to check other sizes

  // reset
  mChar->mGlyph = kNullGlyph; // this will tell paint to build by parts
  mChar->mGlyphTable = aGlyphTable;
  mChar->mFamily = aFamily;

  return IsSizeOK(mPresContext, computedSize, mTargetSize, mStretchHint);
}

// This is called for each family, whether it exists or not
bool
nsMathMLChar::StretchEnumContext::EnumCallback(const nsString& aFamily,
                                               bool aGeneric, void *aData)
{
  StretchEnumContext* context = static_cast<StretchEnumContext*>(aData);

  // See if there is a special table for the family, but always use the
  // Unicode table for generic fonts.
  nsGlyphTable* glyphTable = aGeneric ?
    &gGlyphTableList->mUnicodeTable :
    gGlyphTableList->GetGlyphTableFor(aFamily);

  if (context->mTablesTried.Contains(glyphTable))
    return true; // already tried this one

  // Check font family if it is not a generic one
  // We test with the kNullGlyph
  nsStyleContext *sc = context->mChar->mStyleContext;
  nsFont font = sc->GetStyleFont()->mFont;
  if (!aGeneric && !SetFontFamily(sc, context->mRenderingContext,
                                  font, NULL, kNullGlyph, aFamily))
     return true; // Could not set the family

  context->mGlyphTable = glyphTable;

  // Now see if the table has a glyph that matches the container

  // Only try this table once.
  context->mTablesTried.AppendElement(glyphTable);

  // If the unicode table is being used, then search all font families.  If a
  // special table is being used then the font in this family should have the
  // specified glyphs.
  const nsAString& family = glyphTable == &gGlyphTableList->mUnicodeTable ?
    context->mFamilies : aFamily;

  if((context->mTryVariants && context->TryVariants(glyphTable, family)) ||
     (context->mTryParts && context->TryParts(glyphTable, family)))
    return false; // no need to continue

  return true; // true means continue
}

nsresult
nsMathMLChar::StretchInternal(nsPresContext*           aPresContext,
                              nsRenderingContext&     aRenderingContext,
                              nsStretchDirection&      aStretchDirection,
                              const nsBoundingMetrics& aContainerSize,
                              nsBoundingMetrics&       aDesiredStretchSize,
                              uint32_t                 aStretchHint,
                              // These are currently only used when
                              // aStretchHint & NS_STRETCH_MAXWIDTH:
                              float                    aMaxSize,
                              bool                     aMaxSizeIsAbsolute)
{
  // if we have been called before, and we didn't actually stretch, our
  // direction may have been set to NS_STRETCH_DIRECTION_UNSUPPORTED.
  // So first set our direction back to its instrinsic value
  nsStretchDirection direction = nsMathMLOperators::GetStretchyDirection(mData);

  // Set default font and get the default bounding metrics
  // mStyleContext is a leaf context used only when stretching happens.
  // For the base size, the default font should come from the parent context
  nsFont font = mStyleContext->GetParent()->GetStyleFont()->mFont;

  // Override with specific fonts if applicable for this character
  nsAutoString families;
  if (GetFontExtensionPref(mData[0], eExtension_base, families)) {
    font.name = families;
  }

  // Don't modify this nsMathMLChar when doing GetMaxWidth()
  bool maxWidth = (NS_STRETCH_MAXWIDTH & aStretchHint) != 0;
  if (!maxWidth) {
    // Record the families in case there is no stretch.  But don't bother
    // storing families when they are just those from the StyleContext.
    mFamily = families;
  }

  nsRefPtr<nsFontMetrics> fm;
  aRenderingContext.DeviceContext()->GetMetricsFor(font,
    mStyleContext->GetStyleFont()->mLanguage,
    aPresContext->GetUserFontSet(), *getter_AddRefs(fm));
  aRenderingContext.SetFont(fm);
  aDesiredStretchSize =
    aRenderingContext.GetBoundingMetrics(mData.get(), uint32_t(mData.Length()));

  if (!maxWidth) {
    mUnscaledAscent = aDesiredStretchSize.ascent;
  }

  //////////////////////////////////////////////////////////////////////////////
  // 1. Check the common situations where stretching is not actually needed
  //////////////////////////////////////////////////////////////////////////////

  // quick return if there is nothing special about this char
  if ((aStretchDirection != direction &&
       aStretchDirection != NS_STRETCH_DIRECTION_DEFAULT) ||
      (aStretchHint & ~NS_STRETCH_MAXWIDTH) == NS_STRETCH_NONE) {
    mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
    return NS_OK;
  }

  // if no specified direction, attempt to stretch in our preferred direction
  if (aStretchDirection == NS_STRETCH_DIRECTION_DEFAULT) {
    aStretchDirection = direction;
  }

  // see if this is a particular largeop or largeopOnly request
  bool largeop = (NS_STRETCH_LARGEOP & aStretchHint) != 0;
  bool stretchy = (NS_STRETCH_VARIABLE_MASK & aStretchHint) != 0;
  bool largeopOnly = largeop && !stretchy;

  bool isVertical = (direction == NS_STRETCH_DIRECTION_VERTICAL);

  nscoord targetSize =
    isVertical ? aContainerSize.ascent + aContainerSize.descent
    : aContainerSize.rightBearing - aContainerSize.leftBearing;

  if (maxWidth) {
    // See if it is only necessary to consider glyphs up to some maximum size.
    // Set the current height to the maximum size, and set aStretchHint to
    // NS_STRETCH_SMALLER if the size is variable, so that only smaller sizes
    // are considered.  targetSize from GetMaxWidth() is 0.
    if (stretchy) {
      // variable size stretch - consider all sizes < maxsize
      aStretchHint =
        (aStretchHint & ~NS_STRETCH_VARIABLE_MASK) | NS_STRETCH_SMALLER;
    }

    // Use NS_MATHML_DELIMITER_FACTOR to allow some slightly larger glyphs as
    // maxsize is not enforced exactly.
    if (aMaxSize == NS_MATHML_OPERATOR_SIZE_INFINITY) {
      aDesiredStretchSize.ascent = nscoord_MAX;
      aDesiredStretchSize.descent = 0;
    }
    else {
      nscoord height = aDesiredStretchSize.ascent + aDesiredStretchSize.descent;
      if (height == 0) {
        if (aMaxSizeIsAbsolute) {
          aDesiredStretchSize.ascent =
            NSToCoordRound(aMaxSize / NS_MATHML_DELIMITER_FACTOR);
          aDesiredStretchSize.descent = 0;
        }
        // else: leave height as 0
      }
      else {
        float scale = aMaxSizeIsAbsolute ? aMaxSize / height : aMaxSize;
        scale /= NS_MATHML_DELIMITER_FACTOR;
        aDesiredStretchSize.ascent =
          NSToCoordRound(scale * aDesiredStretchSize.ascent);
        aDesiredStretchSize.descent =
          NSToCoordRound(scale * aDesiredStretchSize.descent);
      }
    }
  }

  nsBoundingMetrics initialSize = aDesiredStretchSize;
  nscoord charSize =
    isVertical ? initialSize.ascent + initialSize.descent
    : initialSize.rightBearing - initialSize.leftBearing;

  bool done = (mGlyphTable ? false : true);

  if (!done && !maxWidth && !largeop) {
    // Doing Stretch() not GetMaxWidth(),
    // and not a largeop in display mode; we're done if size fits
    if ((targetSize <= 0) || 
        ((isVertical && charSize >= targetSize) ||
         IsSizeOK(aPresContext, charSize, targetSize, aStretchHint)))
      done = true;
  }

  //////////////////////////////////////////////////////////////////////////////
  // 2/3. Search for a glyph or set of part glyphs of appropriate size
  //////////////////////////////////////////////////////////////////////////////

  bool glyphFound = false;
  nsAutoString cssFamilies;

  if (!done) {
    font = mStyleContext->GetStyleFont()->mFont;
    cssFamilies = font.name;
  }

  // See if there are preferred fonts for the variants of this char
  if (!done && GetFontExtensionPref(mData[0], eExtension_variants, families)) {
    font.name = families;

    StretchEnumContext enumData(this, aPresContext, aRenderingContext,
                                aStretchDirection, targetSize, aStretchHint,
                                aDesiredStretchSize, font.name, glyphFound);
    enumData.mTryParts = false;

    done = !font.EnumerateFamilies(StretchEnumContext::EnumCallback, &enumData);
  }

  // See if there are preferred fonts for the parts of this char
  if (!done && !largeopOnly
      && GetFontExtensionPref(mData[0], eExtension_parts, families)) {
    font.name = families;

    StretchEnumContext enumData(this, aPresContext, aRenderingContext,
                                aStretchDirection, targetSize, aStretchHint,
                                aDesiredStretchSize, font.name, glyphFound);
    enumData.mTryVariants = false;

    done = !font.EnumerateFamilies(StretchEnumContext::EnumCallback, &enumData);
  }

  if (!done) { // normal case
    // Use the css font-family but add preferred fallback fonts.
    font.name = cssFamilies;
    NS_NAMED_LITERAL_CSTRING(defaultKey, "font.mathfont-family");
    nsAdoptingString fallbackFonts = Preferences::GetString(defaultKey.get());
    if (!fallbackFonts.IsEmpty()) {
      AddFallbackFonts(font.name, fallbackFonts);
    }

#ifdef NOISY_SEARCH
    printf("Searching in "%s" for a glyph of appropriate size for: 0x%04X:%c\n",
           font.name, mData[0], mData[0]&0x00FF);
#endif
    StretchEnumContext enumData(this, aPresContext, aRenderingContext,
                                aStretchDirection, targetSize, aStretchHint,
                                aDesiredStretchSize, font.name, glyphFound);
    enumData.mTryParts = !largeopOnly;

    font.EnumerateFamilies(StretchEnumContext::EnumCallback, &enumData);
  }

  if (!maxWidth) {
    // Now, we know how we are going to draw the char. Update the member
    // variables accordingly.
    mDrawNormal = !glyphFound;
    mUnscaledAscent = aDesiredStretchSize.ascent;
  }
    
  // stretchy character
  if (stretchy) {
    if (isVertical) {
      float scale =
        float(aContainerSize.ascent + aContainerSize.descent) /
        (aDesiredStretchSize.ascent + aDesiredStretchSize.descent);
      if (!largeop || scale > 1.0) {
        // make the character match the desired height.
        if (!maxWidth) {
          mScaleY *= scale;
        }
        aDesiredStretchSize.ascent *= scale;
        aDesiredStretchSize.descent *= scale;
      }
    } else {
      float scale =
        float(aContainerSize.rightBearing - aContainerSize.leftBearing) /
        (aDesiredStretchSize.rightBearing - aDesiredStretchSize.leftBearing);
      if (!largeop || scale > 1.0) {
        // make the character match the desired width.
        if (!maxWidth) {
          mScaleX *= scale;
        }
        aDesiredStretchSize.leftBearing *= scale;
        aDesiredStretchSize.rightBearing *= scale;
        aDesiredStretchSize.width *= scale;
      }
    }
  }

  // We do not have a char variant for this largeop in display mode, so we
  // apply a scale transform to the base char.
  if (!glyphFound && largeop) {
    float scale;
    float largeopFactor = M_SQRT2;

    // increase the width if it is not largeopFactor times larger
    // than the initial one.
    if ((aDesiredStretchSize.rightBearing - aDesiredStretchSize.leftBearing) <
        largeopFactor * (initialSize.rightBearing - initialSize.leftBearing)) {
      scale = (largeopFactor *
               (initialSize.rightBearing - initialSize.leftBearing)) /
        (aDesiredStretchSize.rightBearing - aDesiredStretchSize.leftBearing);
      if (!maxWidth) {
        mScaleX *= scale;
      }
      aDesiredStretchSize.leftBearing *= scale;
      aDesiredStretchSize.rightBearing *= scale;
      aDesiredStretchSize.width *= scale;
    }

    // increase the height if it is not largeopFactor times larger
    // than the initial one.
    if (NS_STRETCH_INTEGRAL & aStretchHint) {
      // integrals are drawn taller
      largeopFactor = 2.0;
    }
    if ((aDesiredStretchSize.ascent + aDesiredStretchSize.descent) <
        largeopFactor * (initialSize.ascent + initialSize.descent)) {
      scale = (largeopFactor *
               (initialSize.ascent + initialSize.descent)) /
        (aDesiredStretchSize.ascent + aDesiredStretchSize.descent);
      if (!maxWidth) {
        mScaleY *= scale;
      }
      aDesiredStretchSize.ascent *= scale;
      aDesiredStretchSize.descent *= scale;
    }
  }

  return NS_OK;
}

nsresult
nsMathMLChar::Stretch(nsPresContext*           aPresContext,
                      nsRenderingContext&     aRenderingContext,
                      nsStretchDirection       aStretchDirection,
                      const nsBoundingMetrics& aContainerSize,
                      nsBoundingMetrics&       aDesiredStretchSize,
                      uint32_t                 aStretchHint,
                      bool                     aRTL)
{
  NS_ASSERTION(!(aStretchHint &
                 ~(NS_STRETCH_VARIABLE_MASK | NS_STRETCH_LARGEOP |
                   NS_STRETCH_INTEGRAL)),
               "Unexpected stretch flags");

  mDrawNormal = true;
  mMirrored = aRTL && nsMathMLOperators::IsMirrorableOperator(mData);
  mScaleY = mScaleX = 1.0;
  mDirection = aStretchDirection;
  nsresult rv =
    StretchInternal(aPresContext, aRenderingContext, mDirection,
                    aContainerSize, aDesiredStretchSize, aStretchHint);

  // Record the metrics
  mBoundingMetrics = aDesiredStretchSize;

  return rv;
}

// What happens here is that the StretchInternal algorithm is used but
// modified by passing the NS_STRETCH_MAXWIDTH stretch hint.  That causes
// StretchInternal to return horizontal bounding metrics that are the maximum
// that might be returned from a Stretch.
//
// In order to avoid considering widths of some characters in fonts that will
// not be used for any stretch size, StretchInternal sets the initial height
// to infinity and looks for any characters smaller than this height.  When a
// character built from parts is considered, (it will be used by Stretch for
// any characters greater than its minimum size, so) the height is set to its
// minimum size, so that only widths of smaller subsequent characters are
// considered.
nscoord
nsMathMLChar::GetMaxWidth(nsPresContext* aPresContext,
                          nsRenderingContext& aRenderingContext,
                          uint32_t aStretchHint,
                          float aMaxSize, bool aMaxSizeIsAbsolute)
{
  nsBoundingMetrics bm;
  nsStretchDirection direction = NS_STRETCH_DIRECTION_VERTICAL;
  const nsBoundingMetrics container; // zero target size

  StretchInternal(aPresContext, aRenderingContext, direction, container,
                  bm, aStretchHint | NS_STRETCH_MAXWIDTH);

  return NS_MAX(bm.width, bm.rightBearing) - NS_MIN(0, bm.leftBearing);
}

class nsDisplayMathMLSelectionRect : public nsDisplayItem {
public:
  nsDisplayMathMLSelectionRect(nsDisplayListBuilder* aBuilder,
                               nsIFrame* aFrame, const nsRect& aRect)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLSelectionRect);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLSelectionRect() {
    MOZ_COUNT_DTOR(nsDisplayMathMLSelectionRect);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLSelectionRect", TYPE_MATHML_SELECTION_RECT)
private:
  nsRect    mRect;
};

void nsDisplayMathMLSelectionRect::Paint(nsDisplayListBuilder* aBuilder,
                                         nsRenderingContext* aCtx)
{
  // get color to use for selection from the look&feel object
  nscolor bgColor =
    LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectBackground,
                          NS_RGB(0, 0, 0));
  aCtx->SetColor(bgColor);
  aCtx->FillRect(mRect + ToReferenceFrame());
}

class nsDisplayMathMLCharBackground : public nsDisplayItem {
public:
  nsDisplayMathMLCharBackground(nsDisplayListBuilder* aBuilder,
                                nsIFrame* aFrame, const nsRect& aRect,
                                nsStyleContext* aStyleContext)
    : nsDisplayItem(aBuilder, aFrame), mStyleContext(aStyleContext),
      mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLCharBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLCharBackground() {
    MOZ_COUNT_DTOR(nsDisplayMathMLCharBackground);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLCharBackground", TYPE_MATHML_CHAR_BACKGROUND)
private:
  nsStyleContext* mStyleContext;
  nsRect          mRect;
};

void nsDisplayMathMLCharBackground::Paint(nsDisplayListBuilder* aBuilder,
                                          nsRenderingContext* aCtx)
{
  const nsStyleBorder* border = mStyleContext->GetStyleBorder();
  nsRect rect(mRect + ToReferenceFrame());
  nsCSSRendering::PaintBackgroundWithSC(mFrame->PresContext(), *aCtx, mFrame,
                                        mVisibleRect, rect,
                                        mStyleContext, *border,
                                        aBuilder->GetBackgroundPaintFlags());
}

class nsDisplayMathMLCharForeground : public nsDisplayItem {
public:
  nsDisplayMathMLCharForeground(nsDisplayListBuilder* aBuilder,
                                nsIFrame* aFrame, nsMathMLChar* aChar,
				                uint32_t aIndex, bool aIsSelected)
    : nsDisplayItem(aBuilder, aFrame), mChar(aChar), 
      mIndex(aIndex), mIsSelected(aIsSelected) {
    MOZ_COUNT_CTOR(nsDisplayMathMLCharForeground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLCharForeground() {
    MOZ_COUNT_DTOR(nsDisplayMathMLCharForeground);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
    *aSnap = false;
    nsRect rect;
    mChar->GetRect(rect);
    nsPoint offset = ToReferenceFrame() + rect.TopLeft();
    nsBoundingMetrics bm;
    mChar->GetBoundingMetrics(bm);
    nsRect temp(offset.x + bm.leftBearing, offset.y,
                bm.rightBearing - bm.leftBearing, bm.ascent + bm.descent);
    // Bug 748220
    temp.Inflate(mFrame->PresContext()->AppUnitsPerDevPixel());
    return temp;
  }
  
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx)
  {
    mChar->PaintForeground(mFrame->PresContext(), *aCtx,
                           ToReferenceFrame(), mIsSelected);
  }

  NS_DISPLAY_DECL_NAME("MathMLCharForeground", TYPE_MATHML_CHAR_FOREGROUND)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder)
  {
    bool snap;
    return GetBounds(aBuilder, &snap);
  }
  
  virtual uint32_t GetPerFrameKey() {
    return (mIndex << nsDisplayItem::TYPE_BITS)
      | nsDisplayItem::GetPerFrameKey();
  }

private:
  nsMathMLChar* mChar;
  uint32_t      mIndex;
  bool          mIsSelected;
};

#ifdef DEBUG
class nsDisplayMathMLCharDebug : public nsDisplayItem {
public:
  nsDisplayMathMLCharDebug(nsDisplayListBuilder* aBuilder,
                           nsIFrame* aFrame, const nsRect& aRect)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLCharDebug);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLCharDebug() {
    MOZ_COUNT_DTOR(nsDisplayMathMLCharDebug);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLCharDebug", TYPE_MATHML_CHAR_DEBUG)

private:
  nsRect mRect;
};

void nsDisplayMathMLCharDebug::Paint(nsDisplayListBuilder* aBuilder,
                                     nsRenderingContext* aCtx)
{
  // for visual debug
  int skipSides = 0;
  nsPresContext* presContext = mFrame->PresContext();
  nsStyleContext* styleContext = mFrame->GetStyleContext();
  nsRect rect = mRect + ToReferenceFrame();
  nsCSSRendering::PaintBorder(presContext, *aCtx, mFrame,
                              mVisibleRect, rect, styleContext, skipSides);
  nsCSSRendering::PaintOutline(presContext, *aCtx, mFrame,
                               mVisibleRect, rect, styleContext);
}
#endif


nsresult
nsMathMLChar::Display(nsDisplayListBuilder*   aBuilder,
                      nsIFrame*               aForFrame,
                      const nsDisplayListSet& aLists,
                      uint32_t                aIndex,
                      const nsRect*           aSelectedRect)
{
  nsresult rv = NS_OK;
  nsStyleContext* parentContext = mStyleContext->GetParent();
  nsStyleContext* styleContext = mStyleContext;

  if (mDrawNormal) {
    // normal drawing if there is nothing special about this char
    // Set default context to the parent context
    styleContext = parentContext;
  }

  if (!styleContext->GetStyleVisibility()->IsVisible())
    return NS_OK;

  // if the leaf style context that we use for stretchy chars has a background
  // color we use it -- this feature is mostly used for testing and debugging
  // purposes. Normally, users will set the background on the container frame.
  // paint the selection background -- beware MathML frames overlap a lot
  if (aSelectedRect && !aSelectedRect->IsEmpty()) {
    rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayMathMLSelectionRect(aBuilder, aForFrame, *aSelectedRect));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (mRect.width && mRect.height) {
    const nsStyleBackground* backg = styleContext->GetStyleBackground();
    if (styleContext != parentContext &&
        NS_GET_A(backg->mBackgroundColor) > 0) {
      rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
          nsDisplayMathMLCharBackground(aBuilder, aForFrame, mRect,
                                        styleContext));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    //else
    //  our container frame will take care of painting its background

#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
    // for visual debug
    rv = aLists.BorderBackground()->AppendToTop(new (aBuilder)
        nsDisplayMathMLCharDebug(aBuilder, aForFrame, mRect));
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }
  return aLists.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayMathMLCharForeground(aBuilder, aForFrame, this,
                                      aIndex,
                                      aSelectedRect &&
                                      !aSelectedRect->IsEmpty()));
}

void
nsMathMLChar::ApplyTransforms(nsRenderingContext& aRenderingContext, nsRect &r)
{
  // apply the transforms
  if (mMirrored) {
    aRenderingContext.Translate(r.TopRight());
    aRenderingContext.Scale(-mScaleX, mScaleY);
  } else {
    aRenderingContext.Translate(r.TopLeft());
    aRenderingContext.Scale(mScaleX, mScaleY);
  }

  // update the bounding rectangle.
  r.x = r.y = 0;
  r.width /= mScaleX;
  r.height /= mScaleY;
}

void
nsMathMLChar::PaintForeground(nsPresContext* aPresContext,
                              nsRenderingContext& aRenderingContext,
                              nsPoint aPt,
                              bool aIsSelected)
{
  nsStyleContext* parentContext = mStyleContext->GetParent();
  nsStyleContext* styleContext = mStyleContext;

  if (mDrawNormal) {
    // normal drawing if there is nothing special about this char
    // Set default context to the parent context
    styleContext = parentContext;
  }

  // Set color ...
  nscolor fgColor = styleContext->GetVisitedDependentColor(eCSSProperty_color);
  if (aIsSelected) {
    // get color to use for selection from the look&feel object
    fgColor = LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectForeground,
                                    fgColor);
  }
  aRenderingContext.SetColor(fgColor);

  nsFont theFont(styleContext->GetStyleFont()->mFont);
  if (! mFamily.IsEmpty()) {
    theFont.name = mFamily;
  }
  nsRefPtr<nsFontMetrics> fm;
  aRenderingContext.DeviceContext()->GetMetricsFor(theFont,
    styleContext->GetStyleFont()->mLanguage,
    aPresContext->GetUserFontSet(),
    *getter_AddRefs(fm));
  aRenderingContext.SetFont(fm);

  aRenderingContext.PushState();
  nsRect r = mRect + aPt;
  ApplyTransforms(aRenderingContext, r);

  if (mDrawNormal) {
    // normal drawing if there is nothing special about this char ...
    // Grab some metrics to adjust the placements ...
    uint32_t len = uint32_t(mData.Length());
    aRenderingContext.DrawString(mData.get(), len, 0, mUnscaledAscent);
  }
  else {
    // Grab some metrics to adjust the placements ...
    // if there is a glyph of appropriate size, paint that glyph
    if (mGlyph.Exists()) {
      aRenderingContext.DrawString(mGlyph.code, mGlyph.Length(),
                                   0, mUnscaledAscent);
    }
    else { // paint by parts
      if (NS_STRETCH_DIRECTION_VERTICAL == mDirection)
        PaintVertically(aPresContext, aRenderingContext, theFont, styleContext,
                        mGlyphTable, r);
      else if (NS_STRETCH_DIRECTION_HORIZONTAL == mDirection)
        PaintHorizontally(aPresContext, aRenderingContext, theFont,
                          styleContext, mGlyphTable, r);
    }
  }

  aRenderingContext.PopState();
}

/* =============================================================================
  Helper routines that actually do the job of painting the char by parts
 */

class AutoPushClipRect {
  nsRenderingContext& mCtx;
public:
  AutoPushClipRect(nsRenderingContext& aCtx, const nsRect& aRect)
    : mCtx(aCtx) {
    mCtx.PushState();
    mCtx.IntersectClip(aRect);
  }
  ~AutoPushClipRect() {
    mCtx.PopState();
  }
};

static nsPoint
SnapToDevPixels(const gfxContext* aThebesContext, int32_t aAppUnitsPerGfxUnit,
                const nsPoint& aPt)
{
  gfxPoint pt(NSAppUnitsToFloatPixels(aPt.x, aAppUnitsPerGfxUnit),
              NSAppUnitsToFloatPixels(aPt.y, aAppUnitsPerGfxUnit));
  pt = aThebesContext->UserToDevice(pt);
  pt.Round();
  pt = aThebesContext->DeviceToUser(pt);
  return nsPoint(NSFloatPixelsToAppUnits(pt.x, aAppUnitsPerGfxUnit),
                 NSFloatPixelsToAppUnits(pt.y, aAppUnitsPerGfxUnit));
}

// paint a stretchy char by assembling glyphs vertically
nsresult
nsMathMLChar::PaintVertically(nsPresContext*      aPresContext,
                              nsRenderingContext& aRenderingContext,
                              nsFont&              aFont,
                              nsStyleContext*      aStyleContext,
                              nsGlyphTable*        aGlyphTable,
                              nsRect&              aRect)
{
  // Get the device pixel size in the vertical direction.
  // (This makes no effort to optimize for non-translation transformations.)
  nscoord oneDevPixel = aPresContext->AppUnitsPerDevPixel();

  // get metrics data to be re-used later
  int32_t i = 0;
  nsGlyphCode ch, chdata[4];
  nsBoundingMetrics bmdata[4];
  int32_t glue, bottom;
  nsGlyphCode chGlue = aGlyphTable->GlueOf(aPresContext, this);
  for (int32_t j = 0; j < 4; ++j) {
    switch (j) {
      case 0:
        ch = aGlyphTable->TopOf(aPresContext, this);
        break;
      case 1:
        ch = aGlyphTable->MiddleOf(aPresContext, this);
        if (!ch.Exists())
          continue; // no middle
        break;
      case 2:
        ch = aGlyphTable->BottomOf(aPresContext, this);
        bottom = i;
        break;
      case 3:
        ch = chGlue;
        glue = i;
        break;
    }
    // empty slots are filled with the glue if it is not null
    if (!ch.Exists()) ch = chGlue;
    // if (!ch.Exists()) glue is null, leave bounding metrics at 0
    if (ch.Exists()) {
      SetFontFamily(aStyleContext, aRenderingContext,
                    aFont, aGlyphTable, ch, mFamily);
      bmdata[i] = aRenderingContext.GetBoundingMetrics(ch.code, ch.Length());
    }
    chdata[i] = ch;
    ++i;
  }
  nscoord dx = aRect.x;
  nscoord offset[3], start[3], end[3];
  nsRefPtr<gfxContext> ctx = aRenderingContext.ThebesContext();
  for (i = 0; i <= bottom; ++i) {
    ch = chdata[i];
    const nsBoundingMetrics& bm = bmdata[i];
    nscoord dy;
    if (0 == i) { // top
      dy = aRect.y + bm.ascent;
    }
    else if (bottom == i) { // bottom
      dy = aRect.y + aRect.height - bm.descent;
    }
    else { // middle
      dy = aRect.y + bm.ascent + (aRect.height - (bm.ascent + bm.descent))/2;
    }
    // _cairo_scaled_font_show_glyphs snaps origins to device pixels.
    // Do this now so that we can get the other dimensions right.
    // (This may not achieve much with non-rectangular transformations.)
    dy = SnapToDevPixels(ctx, oneDevPixel, nsPoint(dx, dy)).y;
    // abcissa passed to DrawString
    offset[i] = dy;
    // _cairo_scaled_font_glyph_device_extents rounds outwards to the nearest
    // pixel, so the bm values can include 1 row of faint pixels on each edge.
    // Don't rely on this pixel as it can look like a gap.
    start[i] = dy - bm.ascent + oneDevPixel; // top join
    end[i] = dy + bm.descent - oneDevPixel; // bottom join
  }

  // If there are overlaps, then join at the mid point
  for (i = 0; i < bottom; ++i) {
    if (end[i] > start[i+1]) {
      end[i] = (end[i] + start[i+1]) / 2;
      start[i+1] = end[i];
    }
  }

  nsRect unionRect = aRect;
  unionRect.x += mBoundingMetrics.leftBearing;
  unionRect.width =
    mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing;
  unionRect.Inflate(oneDevPixel, oneDevPixel);

  /////////////////////////////////////
  // draw top, middle, bottom
  for (i = 0; i <= bottom; ++i) {
    ch = chdata[i];
    // glue can be null, and other parts could have been set to glue
    if (ch.Exists()) {
      nscoord dy = offset[i];
      // Draw a glyph in a clipped area so that we don't have hairy chars
      // pending outside
      nsRect clipRect = unionRect;
      // Clip at the join to get a solid edge (without overlap or gap), when
      // this won't change the glyph too much.  If the glyph is too small to
      // clip then we'll overlap rather than have a gap.
      nscoord height = bmdata[i].ascent + bmdata[i].descent;
      if (ch == chGlue ||
          height * (1.0 - NS_MATHML_DELIMITER_FACTOR) > oneDevPixel) {
        if (0 == i) { // top
          clipRect.height = end[i] - clipRect.y;
        }
        else if (bottom == i) { // bottom
          clipRect.height -= start[i] - clipRect.y;
          clipRect.y = start[i];
        }
        else { // middle
          clipRect.y = start[i];
          clipRect.height = end[i] - start[i];
        }
      }
      if (!clipRect.IsEmpty()) {
        AutoPushClipRect clip(aRenderingContext, clipRect);
        SetFontFamily(aStyleContext, aRenderingContext,
                      aFont, aGlyphTable, ch, mFamily);
        aRenderingContext.DrawString(ch.code, ch.Length(), dx, dy);
      }
    }
  }

  ///////////////
  // fill the gap between top and middle, and between middle and bottom.
  if (!chGlue.Exists()) { // null glue : draw a rule
    // figure out the dimensions of the rule to be drawn :
    // set lbearing to rightmost lbearing among the two current successive
    // parts.
    // set rbearing to leftmost rbearing among the two current successive parts.
    // this not only satisfies the convention used for over/underbraces
    // in TeX, but also takes care of broken fonts like the stretchy integral
    // in Symbol for small font sizes in unix.
    nscoord lbearing, rbearing;
    int32_t first = 0, last = 1;
    while (last <= bottom) {
      if (chdata[last].Exists()) {
        lbearing = bmdata[last].leftBearing;
        rbearing = bmdata[last].rightBearing;
        if (chdata[first].Exists()) {
          if (lbearing < bmdata[first].leftBearing)
            lbearing = bmdata[first].leftBearing;
          if (rbearing > bmdata[first].rightBearing)
            rbearing = bmdata[first].rightBearing;
        }
      }
      else if (chdata[first].Exists()) {
        lbearing = bmdata[first].leftBearing;
        rbearing = bmdata[first].rightBearing;
      }
      else {
        NS_ERROR("Cannot stretch - All parts missing");
        return NS_ERROR_UNEXPECTED;
      }
      // paint the rule between the parts
      nsRect rule(aRect.x + lbearing, end[first],
                  rbearing - lbearing, start[last] - end[first]);
      if (!rule.IsEmpty())
        aRenderingContext.FillRect(rule);
      first = last;
      last++;
    }
  }
  else if (bmdata[glue].ascent + bmdata[glue].descent > 0) {
    // glue is present
    nsBoundingMetrics& bm = bmdata[glue];
    // Ensure the stride for the glue is not reduced to less than one pixel
    if (bm.ascent + bm.descent >= 3 * oneDevPixel) {
      // To protect against gaps, pretend the glue is smaller than it is,
      // in order to trim off ends and thus get a solid edge for the join.
      bm.ascent -= oneDevPixel;
      bm.descent -= oneDevPixel;
    }

    SetFontFamily(aStyleContext, aRenderingContext,
                  aFont, aGlyphTable, chGlue, mFamily);
    nsRect clipRect = unionRect;

    for (i = 0; i < bottom; ++i) {
      // Make sure not to draw outside the character
      nscoord dy = NS_MAX(end[i], aRect.y);
      nscoord fillEnd = NS_MIN(start[i+1], aRect.YMost());
      while (dy < fillEnd) {
        clipRect.y = dy;
        clipRect.height = NS_MIN(bm.ascent + bm.descent, fillEnd - dy);
        AutoPushClipRect clip(aRenderingContext, clipRect);
        dy += bm.ascent;
        aRenderingContext.DrawString(chGlue.code, chGlue.Length(), dx, dy);
        dy += bm.descent;
      }
    }
  }
#ifdef DEBUG
  else {
    for (i = 0; i < bottom; ++i) {
      NS_ASSERTION(end[i] >= start[i+1],
                   "gap between parts with missing glue glyph");
    }
  }
#endif
  return NS_OK;
}

// paint a stretchy char by assembling glyphs horizontally
nsresult
nsMathMLChar::PaintHorizontally(nsPresContext*      aPresContext,
                                nsRenderingContext& aRenderingContext,
                                nsFont&              aFont,
                                nsStyleContext*      aStyleContext,
                                nsGlyphTable*        aGlyphTable,
                                nsRect&              aRect)
{
  // Get the device pixel size in the horizontal direction.
  // (This makes no effort to optimize for non-translation transformations.)
  nscoord oneDevPixel = aPresContext->AppUnitsPerDevPixel();

  // get metrics data to be re-used later
  int32_t i = 0;
  nsGlyphCode ch, chdata[4];
  nsBoundingMetrics bmdata[4];
  int32_t glue, right;
  nsGlyphCode chGlue = aGlyphTable->GlueOf(aPresContext, this);
  for (int32_t j = 0; j < 4; ++j) {
    switch (j) {
      case 0:
        ch = aGlyphTable->LeftOf(aPresContext, this);
        break;
      case 1:
        ch = aGlyphTable->MiddleOf(aPresContext, this);
        if (!ch.Exists())
          continue; // no middle
        break;
      case 2:
        ch = aGlyphTable->RightOf(aPresContext, this);
        right = i;
        break;
      case 3:
        ch = chGlue;
        glue = i;
        break;
    }
    // empty slots are filled with the glue if it is not null
    if (!ch.Exists()) ch = chGlue;
    // if (!ch.Exists()) glue is null, leave bounding metrics at 0.
    if (ch.Exists()) {
      SetFontFamily(aStyleContext, aRenderingContext,
                    aFont, aGlyphTable, ch, mFamily);
      bmdata[i] = aRenderingContext.GetBoundingMetrics(ch.code, ch.Length());
    }
    chdata[i] = ch;
    ++i;
  }
  nscoord dy = aRect.y + mBoundingMetrics.ascent;
  nscoord offset[3], start[3], end[3];
  nsRefPtr<gfxContext> ctx = aRenderingContext.ThebesContext();
  for (i = 0; i <= right; ++i) {
    ch = chdata[i];
    const nsBoundingMetrics& bm = bmdata[i];
    nscoord dx;
    if (0 == i) { // left
      dx = aRect.x - bm.leftBearing;
    }
    else if (right == i) { // right
      dx = aRect.x + aRect.width - bm.rightBearing;
    }
    else { // middle
      dx = aRect.x + (aRect.width - bm.width)/2;
    }
    // _cairo_scaled_font_show_glyphs snaps origins to device pixels.
    // Do this now so that we can get the other dimensions right.
    // (This may not achieve much with non-rectangular transformations.)
    dx = SnapToDevPixels(ctx, oneDevPixel, nsPoint(dx, dy)).x;
    // abcissa passed to DrawString
    offset[i] = dx;
    // _cairo_scaled_font_glyph_device_extents rounds outwards to the nearest
    // pixel, so the bm values can include 1 row of faint pixels on each edge.
    // Don't rely on this pixel as it can look like a gap.
    start[i] = dx + bm.leftBearing + oneDevPixel; // left join
    end[i] = dx + bm.rightBearing - oneDevPixel; // right join
  }

  // If there are overlaps, then join at the mid point
  for (i = 0; i < right; ++i) {
    if (end[i] > start[i+1]) {
      end[i] = (end[i] + start[i+1]) / 2;
      start[i+1] = end[i];
    }
  }

  nsRect unionRect = aRect;
  unionRect.Inflate(oneDevPixel, oneDevPixel);

  ///////////////////////////
  // draw left, middle, right
  for (i = 0; i <= right; ++i) {
    ch = chdata[i];
    // glue can be null, and other parts could have been set to glue
    if (ch.Exists()) {
      nscoord dx = offset[i];
      nsRect clipRect = unionRect;
      // Clip at the join to get a solid edge (without overlap or gap), when
      // this won't change the glyph too much.  If the glyph is too small to
      // clip then we'll overlap rather than have a gap.
      nscoord width = bmdata[i].rightBearing - bmdata[i].leftBearing;
      if (ch == chGlue ||
          width * (1.0 - NS_MATHML_DELIMITER_FACTOR) > oneDevPixel) {
        if (0 == i) { // left
          clipRect.width = end[i] - clipRect.x;
        }
        else if (right == i) { // right
          clipRect.width -= start[i] - clipRect.x;
          clipRect.x = start[i];
        }
        else { // middle
          clipRect.x = start[i];
          clipRect.width = end[i] - start[i];
        }
      }
      if (!clipRect.IsEmpty()) {
        AutoPushClipRect clip(aRenderingContext, clipRect);
        SetFontFamily(aStyleContext, aRenderingContext,
                      aFont, aGlyphTable, ch, mFamily);
        aRenderingContext.DrawString(ch.code, ch.Length(), dx, dy);
      }
    }
  }

  ////////////////
  // fill the gap between left and middle, and between middle and right.
  if (!chGlue.Exists()) { // null glue : draw a rule
    // figure out the dimensions of the rule to be drawn :
    // set ascent to lowest ascent among the two current successive parts.
    // set descent to highest descent among the two current successive parts.
    // this satisfies the convention used for over/underbraces, and helps
    // fix broken fonts.
    nscoord ascent, descent;
    int32_t first = 0, last = 1;
    while (last <= right) {
      if (chdata[last].Exists()) {
        ascent = bmdata[last].ascent;
        descent = bmdata[last].descent;
        if (chdata[first].Exists()) {
          if (ascent > bmdata[first].ascent)
            ascent = bmdata[first].ascent;
          if (descent > bmdata[first].descent)
            descent = bmdata[first].descent;
        }
      }
      else if (chdata[first].Exists()) {
        ascent = bmdata[first].ascent;
        descent = bmdata[first].descent;
      }
      else {
        NS_ERROR("Cannot stretch - All parts missing");
        return NS_ERROR_UNEXPECTED;
      }
      // paint the rule between the parts
      nsRect rule(end[first], dy - ascent,
                  start[last] - end[first], ascent + descent);
      if (!rule.IsEmpty())
        aRenderingContext.FillRect(rule);
      first = last;
      last++;
    }
  }
  else if (bmdata[glue].rightBearing - bmdata[glue].leftBearing > 0) {
    // glue is present
    nsBoundingMetrics& bm = bmdata[glue];
    // Ensure the stride for the glue is not reduced to less than one pixel
    if (bm.rightBearing - bm.leftBearing >= 3 * oneDevPixel) {
      // To protect against gaps, pretend the glue is smaller than it is,
      // in order to trim off ends and thus get a solid edge for the join.
      bm.leftBearing += oneDevPixel;
      bm.rightBearing -= oneDevPixel;
    }

    SetFontFamily(aStyleContext, aRenderingContext,
                  aFont, aGlyphTable, chGlue, mFamily);
    nsRect clipRect = unionRect;

    for (i = 0; i < right; ++i) {
      // Make sure not to draw outside the character
      nscoord dx = NS_MAX(end[i], aRect.x);
      nscoord fillEnd = NS_MIN(start[i+1], aRect.XMost());
      while (dx < fillEnd) {
        clipRect.x = dx;
        clipRect.width = NS_MIN(bm.rightBearing - bm.leftBearing, fillEnd - dx);
        AutoPushClipRect clip(aRenderingContext, clipRect);
        dx -= bm.leftBearing;
        aRenderingContext.DrawString(chGlue.code, chGlue.Length(), dx, dy);
        dx += bm.rightBearing;
      }
    }
  }
#ifdef DEBUG
  else { // no glue
    for (i = 0; i < right; ++i) {
      NS_ASSERTION(end[i] >= start[i+1],
                   "gap between parts with missing glue glyph");
    }
  }
#endif
  return NS_OK;
}
