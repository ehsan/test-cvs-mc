/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * A struct that represents the value (type and actual data) of an
 * attribute.
 */

#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "nsIAtom.h"
#include "nsUnicharUtils.h"
#include "mozilla/css/StyleRule.h"
#include "mozilla/css/Declaration.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "prprf.h"
#include "mozilla/HashFunctions.h"
#include "nsHTMLCSSStyleSheet.h"
#include "nsCSSParser.h"
#include "nsStyledElement.h"

using namespace mozilla;

#define MISC_STR_PTR(_cont) \
  reinterpret_cast<void*>((_cont)->mStringBits & NS_ATTRVALUE_POINTERVALUE_MASK)

bool
MiscContainer::GetString(nsDependentString& aString) const
{
  void* ptr = MISC_STR_PTR(this);

  if (!ptr) {
    return false;
  }

  if (static_cast<nsAttrValue::ValueBaseType>(mStringBits &
                                              NS_ATTRVALUE_BASETYPE_MASK) ==
      nsAttrValue::eStringBase) {
    nsStringBuffer* buffer = static_cast<nsStringBuffer*>(ptr);
    if (!buffer) {
      return false;
    }

    aString.Rebind(reinterpret_cast<PRUnichar*>(buffer->Data()),
                   buffer->StorageSize() / sizeof(PRUnichar) - 1);
    return true;
  }

  nsIAtom* atom = static_cast<nsIAtom*>(ptr);
  if (!atom) {
    return false;
  }

  aString.Rebind(atom->GetUTF16String(), atom->GetLength());
  return true;
}

void
MiscContainer::Cache()
{
  // Not implemented for anything else yet.
  MOZ_ASSERT(mType == nsAttrValue::eCSSStyleRule);
  MOZ_ASSERT(IsRefCounted());
  MOZ_ASSERT(mValue.mRefCount > 0);
  MOZ_ASSERT(!mValue.mCached);

  css::StyleRule* rule = mValue.mCSSStyleRule;
  nsHTMLCSSStyleSheet* sheet = rule->GetHTMLCSSStyleSheet();
  if (!sheet) {
    return;
  }

  nsDependentString str;
  bool gotString = GetString(str);
  if (!gotString) {
    return;
  }

  sheet->CacheStyleAttr(str, this);
  mValue.mCached = 1;

  // This has to be immutable once it goes into the cache.
  css::Declaration* decl = rule->GetDeclaration();
  if (decl) {
    decl->SetImmutable();
  }
}

void
MiscContainer::Evict()
{
  // Not implemented for anything else yet.
  MOZ_ASSERT(mType == nsAttrValue::eCSSStyleRule);
  MOZ_ASSERT(IsRefCounted());
  MOZ_ASSERT(mValue.mRefCount == 0);

  if (!mValue.mCached) {
    return;
  }

  css::StyleRule* rule = mValue.mCSSStyleRule;
  nsHTMLCSSStyleSheet* sheet = rule->GetHTMLCSSStyleSheet();
  MOZ_ASSERT(sheet);

  nsDependentString str;
  DebugOnly<bool> gotString = GetString(str);
  MOZ_ASSERT(gotString);

  sheet->EvictStyleAttr(str, this);
  mValue.mCached = 0;
}

nsTArray<const nsAttrValue::EnumTable*>* nsAttrValue::sEnumTableArray = nullptr;

nsAttrValue::nsAttrValue()
    : mBits(0)
{
}

nsAttrValue::nsAttrValue(const nsAttrValue& aOther)
    : mBits(0)
{
  SetTo(aOther);
}

nsAttrValue::nsAttrValue(const nsAString& aValue)
    : mBits(0)
{
  SetTo(aValue);
}

nsAttrValue::nsAttrValue(nsIAtom* aValue)
    : mBits(0)
{
  SetTo(aValue);
}

nsAttrValue::nsAttrValue(css::StyleRule* aValue, const nsAString* aSerialized)
    : mBits(0)
{
  SetTo(aValue, aSerialized);
}

nsAttrValue::nsAttrValue(const nsIntMargin& aValue)
    : mBits(0)
{
  SetTo(aValue);
}

nsAttrValue::~nsAttrValue()
{
  ResetIfSet();
}

/* static */
nsresult
nsAttrValue::Init()
{
  NS_ASSERTION(!sEnumTableArray, "nsAttrValue already initialized");

  sEnumTableArray = new nsTArray<const EnumTable*>;
  NS_ENSURE_TRUE(sEnumTableArray, NS_ERROR_OUT_OF_MEMORY);
  
  return NS_OK;
}

/* static */
void
nsAttrValue::Shutdown()
{
  delete sEnumTableArray;
  sEnumTableArray = nullptr;
}

nsAttrValue::ValueType
nsAttrValue::Type() const
{
  switch (BaseType()) {
    case eIntegerBase:
    {
      return static_cast<ValueType>(mBits & NS_ATTRVALUE_INTEGERTYPE_MASK);
    }
    case eOtherBase:
    {
      return GetMiscContainer()->mType;
    }
    default:
    {
      return static_cast<ValueType>(static_cast<uint16_t>(BaseType()));
    }
  }
}

void
nsAttrValue::Reset()
{
  switch(BaseType()) {
    case eStringBase:
    {
      nsStringBuffer* str = static_cast<nsStringBuffer*>(GetPtr());
      if (str) {
        str->Release();
      }

      break;
    }
    case eOtherBase:
    {
      MiscContainer* cont = GetMiscContainer();
      if (cont->IsRefCounted() && cont->mValue.mRefCount > 1) {
        NS_RELEASE(cont);
        break;
      }

      delete ClearMiscContainer();

      break;
    }
    case eAtomBase:
    {
      nsIAtom* atom = GetAtomValue();
      NS_RELEASE(atom);

      break;
    }
    case eIntegerBase:
    {
      break;
    }
  }

  mBits = 0;
}

void
nsAttrValue::SetTo(const nsAttrValue& aOther)
{
  if (this == &aOther) {
    return;
  }

  switch (aOther.BaseType()) {
    case eStringBase:
    {
      ResetIfSet();
      nsStringBuffer* str = static_cast<nsStringBuffer*>(aOther.GetPtr());
      if (str) {
        str->AddRef();
        SetPtrValueAndType(str, eStringBase);
      }
      return;
    }
    case eOtherBase:
    {
      break;
    }
    case eAtomBase:
    {
      ResetIfSet();
      nsIAtom* atom = aOther.GetAtomValue();
      NS_ADDREF(atom);
      SetPtrValueAndType(atom, eAtomBase);
      return;
    }
    case eIntegerBase:
    {
      ResetIfSet();
      mBits = aOther.mBits;
      return;      
    }
  }

  MiscContainer* otherCont = aOther.GetMiscContainer();
  if (otherCont->IsRefCounted()) {
    delete ClearMiscContainer();
    NS_ADDREF(otherCont);
    SetPtrValueAndType(otherCont, eOtherBase);
    return;
  }

  MiscContainer* cont = EnsureEmptyMiscContainer();
  switch (otherCont->mType) {
    case eInteger:
    {
      cont->mValue.mInteger = otherCont->mValue.mInteger;
      break;
    }
    case eEnum:
    {
      cont->mValue.mEnumValue = otherCont->mValue.mEnumValue;
      break;
    }
    case ePercent:
    {
      cont->mValue.mPercent = otherCont->mValue.mPercent;
      break;
    }
    case eColor:
    {
      cont->mValue.mColor = otherCont->mValue.mColor;
      break;
    }
    case eCSSStyleRule:
    {
      MOZ_NOT_REACHED("These should be refcounted!");
      break;
    }
    case eURL:
    {
      NS_ADDREF(cont->mValue.mURL = otherCont->mValue.mURL);
      break;
    }
    case eImage:
    {
      NS_ADDREF(cont->mValue.mImage = otherCont->mValue.mImage);
      break;
    }
    case eAtomArray:
    {
      if (!EnsureEmptyAtomArray() ||
          !GetAtomArrayValue()->AppendElements(*otherCont->mValue.mAtomArray)) {
        Reset();
        return;
      }
      break;
    }
    case eDoubleValue:
    {
      cont->mDoubleValue = otherCont->mDoubleValue;
      break;
    }
    case eIntMarginValue:
    {
      if (otherCont->mValue.mIntMargin)
        cont->mValue.mIntMargin =
          new nsIntMargin(*otherCont->mValue.mIntMargin);
      break;
    }
    default:
    {
      if (IsSVGType(otherCont->mType)) {
        // All SVG types are just pointers to classes and will therefore have
        // the same size so it doesn't really matter which one we assign
        cont->mValue.mSVGAngle = otherCont->mValue.mSVGAngle;
      } else {
        NS_NOTREACHED("unknown type stored in MiscContainer");
      }
      break;
    }
  }

  void* otherPtr = MISC_STR_PTR(otherCont);
  if (otherPtr) {
    if (static_cast<ValueBaseType>(otherCont->mStringBits & NS_ATTRVALUE_BASETYPE_MASK) ==
        eStringBase) {
      static_cast<nsStringBuffer*>(otherPtr)->AddRef();
    } else {
      static_cast<nsIAtom*>(otherPtr)->AddRef();
    }
    cont->mStringBits = otherCont->mStringBits;
  }
  // Note, set mType after switch-case, otherwise EnsureEmptyAtomArray doesn't
  // work correctly.
  cont->mType = otherCont->mType;
}

void
nsAttrValue::SetTo(const nsAString& aValue)
{
  ResetIfSet();
  nsStringBuffer* buf = GetStringBuffer(aValue);
  if (buf) {
    SetPtrValueAndType(buf, eStringBase);
  }
}

void
nsAttrValue::SetTo(nsIAtom* aValue)
{
  ResetIfSet();
  if (aValue) {
    NS_ADDREF(aValue);
    SetPtrValueAndType(aValue, eAtomBase);
  }
}

void
nsAttrValue::SetTo(int16_t aInt)
{
  ResetIfSet();
  SetIntValueAndType(aInt, eInteger, nullptr);
}

void
nsAttrValue::SetTo(int32_t aInt, const nsAString* aSerialized)
{
  ResetIfSet();
  SetIntValueAndType(aInt, eInteger, aSerialized);
}

void
nsAttrValue::SetTo(double aValue, const nsAString* aSerialized)
{
  MiscContainer* cont = EnsureEmptyMiscContainer();
  cont->mDoubleValue = aValue;
  cont->mType = eDoubleValue;
  SetMiscAtomOrString(aSerialized);
}

void
nsAttrValue::SetTo(css::StyleRule* aValue, const nsAString* aSerialized)
{
  MiscContainer* cont = EnsureEmptyMiscContainer();
  MOZ_ASSERT(cont->mValue.mRefCount == 0);
  NS_ADDREF(cont->mValue.mCSSStyleRule = aValue);
  cont->mType = eCSSStyleRule;
  NS_ADDREF(cont);
  SetMiscAtomOrString(aSerialized);
  MOZ_ASSERT(cont->mValue.mRefCount == 1);
}

void
nsAttrValue::SetTo(css::URLValue* aValue, const nsAString* aSerialized)
{
  MiscContainer* cont = EnsureEmptyMiscContainer();
  NS_ADDREF(cont->mValue.mURL = aValue);
  cont->mType = eURL;
  SetMiscAtomOrString(aSerialized);
}

void
nsAttrValue::SetTo(const nsIntMargin& aValue)
{
  MiscContainer* cont = EnsureEmptyMiscContainer();
  cont->mValue.mIntMargin = new nsIntMargin(aValue);
  cont->mType = eIntMarginValue;
}

void
nsAttrValue::SetToSerialized(const nsAttrValue& aOther)
{
  if (aOther.Type() != nsAttrValue::eString &&
      aOther.Type() != nsAttrValue::eAtom) {
    nsAutoString val;
    aOther.ToString(val);
    SetTo(val);
  } else {
    SetTo(aOther);
  }
}

void
nsAttrValue::SetTo(const nsSVGAngle& aValue, const nsAString* aSerialized)
{
  SetSVGType(eSVGAngle, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const nsSVGIntegerPair& aValue, const nsAString* aSerialized)
{
  SetSVGType(eSVGIntegerPair, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const nsSVGLength2& aValue, const nsAString* aSerialized)
{
  SetSVGType(eSVGLength, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const SVGLengthList& aValue,
                   const nsAString* aSerialized)
{
  // While an empty string will parse as a length list, there's no need to store
  // it (and SetMiscAtomOrString will assert if we try)
  if (aSerialized && aSerialized->IsEmpty()) {
    aSerialized = nullptr;
  }
  SetSVGType(eSVGLengthList, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const SVGNumberList& aValue,
                   const nsAString* aSerialized)
{
  // While an empty string will parse as a number list, there's no need to store
  // it (and SetMiscAtomOrString will assert if we try)
  if (aSerialized && aSerialized->IsEmpty()) {
    aSerialized = nullptr;
  }
  SetSVGType(eSVGNumberList, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const nsSVGNumberPair& aValue, const nsAString* aSerialized)
{
  SetSVGType(eSVGNumberPair, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const SVGPathData& aValue,
                   const nsAString* aSerialized)
{
  // While an empty string will parse as path data, there's no need to store it
  // (and SetMiscAtomOrString will assert if we try)
  if (aSerialized && aSerialized->IsEmpty()) {
    aSerialized = nullptr;
  }
  SetSVGType(eSVGPathData, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const SVGPointList& aValue,
                   const nsAString* aSerialized)
{
  // While an empty string will parse as a point list, there's no need to store
  // it (and SetMiscAtomOrString will assert if we try)
  if (aSerialized && aSerialized->IsEmpty()) {
    aSerialized = nullptr;
  }
  SetSVGType(eSVGPointList, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const SVGAnimatedPreserveAspectRatio& aValue,
                   const nsAString* aSerialized)
{
  SetSVGType(eSVGPreserveAspectRatio, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const SVGStringList& aValue,
                   const nsAString* aSerialized)
{
  // While an empty string will parse as a string list, there's no need to store
  // it (and SetMiscAtomOrString will assert if we try)
  if (aSerialized && aSerialized->IsEmpty()) {
    aSerialized = nullptr;
  }
  SetSVGType(eSVGStringList, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const SVGTransformList& aValue,
                   const nsAString* aSerialized)
{
  // While an empty string will parse as a transform list, there's no need to
  // store it (and SetMiscAtomOrString will assert if we try)
  if (aSerialized && aSerialized->IsEmpty()) {
    aSerialized = nullptr;
  }
  SetSVGType(eSVGTransformList, &aValue, aSerialized);
}

void
nsAttrValue::SetTo(const nsSVGViewBox& aValue, const nsAString* aSerialized)
{
  SetSVGType(eSVGViewBox, &aValue, aSerialized);
}

void
nsAttrValue::SwapValueWith(nsAttrValue& aOther)
{
  PtrBits tmp = aOther.mBits;
  aOther.mBits = mBits;
  mBits = tmp;
}

void
nsAttrValue::ToString(nsAString& aResult) const
{
  MiscContainer* cont = nullptr;
  if (BaseType() == eOtherBase) {
    cont = GetMiscContainer();

    nsDependentString str;
    if (cont->GetString(str)) {
      aResult = str;
      return;
    }
  }

  switch(Type()) {
    case eString:
    {
      nsStringBuffer* str = static_cast<nsStringBuffer*>(GetPtr());
      if (str) {
        str->ToString(str->StorageSize()/sizeof(PRUnichar) - 1, aResult);
      }
      else {
        aResult.Truncate();
      }
      break;
    }
    case eAtom:
    {
      nsIAtom *atom = static_cast<nsIAtom*>(GetPtr());
      atom->ToString(aResult);

      break;
    }
    case eInteger:
    {
      nsAutoString intStr;
      intStr.AppendInt(GetIntegerValue());
      aResult = intStr;

      break;
    }
#ifdef DEBUG
    case eColor:
    {
      NS_NOTREACHED("color attribute without string data");
      aResult.Truncate();
      break;
    }
#endif
    case eEnum:
    {
      GetEnumString(aResult, false);
      break;
    }
    case ePercent:
    {
      nsAutoString intStr;
      intStr.AppendInt(cont ? cont->mValue.mPercent : GetIntInternal());
      aResult = intStr + NS_LITERAL_STRING("%");

      break;
    }
    case eCSSStyleRule:
    {
      aResult.Truncate();
      MiscContainer *container = GetMiscContainer();
      css::Declaration *decl =
        container->mValue.mCSSStyleRule->GetDeclaration();
      if (decl) {
        decl->ToString(aResult);
      }
      const_cast<nsAttrValue*>(this)->SetMiscAtomOrString(&aResult);

      break;
    }
    case eDoubleValue:
    {
      aResult.Truncate();
      aResult.AppendFloat(GetDoubleValue());
      break;
    }
    case eSVGAngle:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGAngle,
                                    aResult);
      break;
    }
    case eSVGIntegerPair:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGIntegerPair,
                                    aResult);
      break;
    }
    case eSVGLength:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGLength,
                                    aResult);
      break;
    }
    case eSVGLengthList:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGLengthList,
                                    aResult);
      break;
    }
    case eSVGNumberList:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGNumberList,
                                    aResult);
      break;
    }
    case eSVGNumberPair:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGNumberPair,
                                    aResult);
      break;
    }
    case eSVGPathData:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGPathData,
                                    aResult);
      break;
    }
    case eSVGPointList:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGPointList,
                                    aResult);
      break;
    }
    case eSVGPreserveAspectRatio:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGPreserveAspectRatio,
                                    aResult);
      break;
    }
    case eSVGStringList:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGStringList,
                                    aResult);
      break;
    }
    case eSVGTransformList:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGTransformList,
                                    aResult);
      break;
    }
    case eSVGViewBox:
    {
      SVGAttrValueWrapper::ToString(GetMiscContainer()->mValue.mSVGViewBox,
                                    aResult);
      break;
    }
    default:
    {
      aResult.Truncate();
      break;
    }
  }
}

already_AddRefed<nsIAtom>
nsAttrValue::GetAsAtom() const
{
  switch (Type()) {
    case eString:
      return do_GetAtom(GetStringValue());

    case eAtom:
      {
        nsIAtom* atom = GetAtomValue();
        NS_ADDREF(atom);
        return atom;
      }

    default:
      {
        nsAutoString val;
        ToString(val);
        return do_GetAtom(val);
      }
  }
}

const nsCheapString
nsAttrValue::GetStringValue() const
{
  NS_PRECONDITION(Type() == eString, "wrong type");

  return nsCheapString(static_cast<nsStringBuffer*>(GetPtr()));
}

bool
nsAttrValue::GetColorValue(nscolor& aColor) const
{
  if (Type() != eColor) {
    // Unparseable value, treat as unset.
    NS_ASSERTION(Type() == eString, "unexpected type for color-valued attr");
    return false;
  }

  aColor = GetMiscContainer()->mValue.mColor;
  return true;
}

void
nsAttrValue::GetEnumString(nsAString& aResult, bool aRealTag) const
{
  NS_PRECONDITION(Type() == eEnum, "wrong type");

  uint32_t allEnumBits =
    (BaseType() == eIntegerBase) ? static_cast<uint32_t>(GetIntInternal())
                                   : GetMiscContainer()->mValue.mEnumValue;
  int16_t val = allEnumBits >> NS_ATTRVALUE_ENUMTABLEINDEX_BITS;
  const EnumTable* table = sEnumTableArray->
    ElementAt(allEnumBits & NS_ATTRVALUE_ENUMTABLEINDEX_MASK);

  while (table->tag) {
    if (table->value == val) {
      aResult.AssignASCII(table->tag);
      if (!aRealTag && allEnumBits & NS_ATTRVALUE_ENUMTABLE_VALUE_NEEDS_TO_UPPER) {
        nsContentUtils::ASCIIToUpper(aResult);
      }
      return;
    }
    table++;
  }

  NS_NOTREACHED("couldn't find value in EnumTable");
}

uint32_t
nsAttrValue::GetAtomCount() const
{
  ValueType type = Type();

  if (type == eAtom) {
    return 1;
  }

  if (type == eAtomArray) {
    return GetAtomArrayValue()->Length();
  }

  return 0;
}

nsIAtom*
nsAttrValue::AtomAt(int32_t aIndex) const
{
  NS_PRECONDITION(aIndex >= 0, "Index must not be negative");
  NS_PRECONDITION(GetAtomCount() > uint32_t(aIndex), "aIndex out of range");
  
  if (BaseType() == eAtomBase) {
    return GetAtomValue();
  }

  NS_ASSERTION(Type() == eAtomArray, "GetAtomCount must be confused");
  
  return GetAtomArrayValue()->ElementAt(aIndex);
}

uint32_t
nsAttrValue::HashValue() const
{
  switch(BaseType()) {
    case eStringBase:
    {
      nsStringBuffer* str = static_cast<nsStringBuffer*>(GetPtr());
      if (str) {
        uint32_t len = str->StorageSize()/sizeof(PRUnichar) - 1;
        return HashString(static_cast<PRUnichar*>(str->Data()), len);
      }

      return 0;
    }
    case eOtherBase:
    {
      break;
    }
    case eAtomBase:
    case eIntegerBase:
    {
      // mBits and uint32_t might have different size. This should silence
      // any warnings or compile-errors. This is what the implementation of
      // NS_PTR_TO_INT32 does to take care of the same problem.
      return mBits - 0;
    }
  }

  MiscContainer* cont = GetMiscContainer();
  if (static_cast<ValueBaseType>(cont->mStringBits & NS_ATTRVALUE_BASETYPE_MASK)
      == eAtomBase) {
    return cont->mStringBits - 0;
  }

  switch (cont->mType) {
    case eInteger:
    {
      return cont->mValue.mInteger;
    }
    case eEnum:
    {
      return cont->mValue.mEnumValue;
    }
    case ePercent:
    {
      return cont->mValue.mPercent;
    }
    case eColor:
    {
      return cont->mValue.mColor;
    }
    case eCSSStyleRule:
    {
      return NS_PTR_TO_INT32(cont->mValue.mCSSStyleRule);
    }
    // Intentionally identical, so that loading the image does not change the
    // hash code.
    case eURL:
    case eImage:
    {
      nsString str;
      ToString(str);
      return HashString(str);
    }
    case eAtomArray:
    {
      uint32_t hash = 0;
      uint32_t count = cont->mValue.mAtomArray->Length();
      for (nsCOMPtr<nsIAtom> *cur = cont->mValue.mAtomArray->Elements(),
                             *end = cur + count;
           cur != end; ++cur) {
        hash = AddToHash(hash, cur->get());
      }
      return hash;
    }
    case eDoubleValue:
    {
      // XXX this is crappy, but oh well
      return cont->mDoubleValue;
    }
    case eIntMarginValue:
    {
      return NS_PTR_TO_INT32(cont->mValue.mIntMargin);
    }
    default:
    {
      if (IsSVGType(cont->mType)) {
        // All SVG types are just pointers to classes so we can treat them alike
        return NS_PTR_TO_INT32(cont->mValue.mSVGAngle);
      }
      NS_NOTREACHED("unknown type stored in MiscContainer");
      return 0;
    }
  }
}

bool
nsAttrValue::Equals(const nsAttrValue& aOther) const
{
  if (BaseType() != aOther.BaseType()) {
    return false;
  }

  switch(BaseType()) {
    case eStringBase:
    {
      return GetStringValue().Equals(aOther.GetStringValue());
    }
    case eOtherBase:
    {
      break;
    }
    case eAtomBase:
    case eIntegerBase:
    {
      return mBits == aOther.mBits;
    }
  }

  MiscContainer* thisCont = GetMiscContainer();
  MiscContainer* otherCont = aOther.GetMiscContainer();
  if (thisCont == otherCont) {
    return true;
  }

  if (thisCont->mType != otherCont->mType) {
    return false;
  }

  bool needsStringComparison = false;

  switch (thisCont->mType) {
    case eInteger:
    {
      if (thisCont->mValue.mInteger == otherCont->mValue.mInteger) {
        needsStringComparison = true;
      }
      break;
    }
    case eEnum:
    {
      if (thisCont->mValue.mEnumValue == otherCont->mValue.mEnumValue) {
        needsStringComparison = true;
      }
      break;
    }
    case ePercent:
    {
      if (thisCont->mValue.mPercent == otherCont->mValue.mPercent) {
        needsStringComparison = true;
      }
      break;
    }
    case eColor:
    {
      if (thisCont->mValue.mColor == otherCont->mValue.mColor) {
        needsStringComparison = true;
      }
      break;
    }
    case eCSSStyleRule:
    {
      return thisCont->mValue.mCSSStyleRule == otherCont->mValue.mCSSStyleRule;
    }
    case eURL:
    {
      return thisCont->mValue.mURL == otherCont->mValue.mURL;
    }
    case eImage:
    {
      return thisCont->mValue.mImage == otherCont->mValue.mImage;
    }
    case eAtomArray:
    {
      // For classlists we could be insensitive to order, however
      // classlists are never mapped attributes so they are never compared.

      if (!(*thisCont->mValue.mAtomArray == *otherCont->mValue.mAtomArray)) {
        return false;
      }

      needsStringComparison = true;
      break;
    }
    case eDoubleValue:
    {
      return thisCont->mDoubleValue == otherCont->mDoubleValue;
    }
    case eIntMarginValue:
    {
      return thisCont->mValue.mIntMargin == otherCont->mValue.mIntMargin;
    }
    default:
    {
      if (IsSVGType(thisCont->mType)) {
        // Currently this method is never called for nsAttrValue objects that
        // point to SVG data types.
        // If that changes then we probably want to add methods to the
        // corresponding SVG types to compare their base values.
        // As a shortcut, however, we can begin by comparing the pointers.
        NS_ABORT_IF_FALSE(false, "Comparing nsAttrValues that point to SVG "
          "data");
        return false;
      }
      NS_NOTREACHED("unknown type stored in MiscContainer");
      return false;
    }
  }
  if (needsStringComparison) {
    if (thisCont->mStringBits == otherCont->mStringBits) {
      return true;
    }
    if ((static_cast<ValueBaseType>(thisCont->mStringBits & NS_ATTRVALUE_BASETYPE_MASK) ==
         eStringBase) &&
        (static_cast<ValueBaseType>(otherCont->mStringBits & NS_ATTRVALUE_BASETYPE_MASK) ==
         eStringBase)) {
      return nsCheapString(reinterpret_cast<nsStringBuffer*>(thisCont->mStringBits)).Equals(
        nsCheapString(reinterpret_cast<nsStringBuffer*>(otherCont->mStringBits)));
    }
  }
  return false;
}

bool
nsAttrValue::Equals(const nsAString& aValue,
                    nsCaseTreatment aCaseSensitive) const
{
  switch (BaseType()) {
    case eStringBase:
    {
      nsStringBuffer* str = static_cast<nsStringBuffer*>(GetPtr());
      if (str) {
        nsDependentString dep(static_cast<PRUnichar*>(str->Data()),
                              str->StorageSize()/sizeof(PRUnichar) - 1);
        return aCaseSensitive == eCaseMatters ? aValue.Equals(dep) :
          aValue.Equals(dep, nsCaseInsensitiveStringComparator());
      }
      return aValue.IsEmpty();
    }
    case eAtomBase:
      if (aCaseSensitive == eCaseMatters) {
        return static_cast<nsIAtom*>(GetPtr())->Equals(aValue);
      }
      return nsDependentAtomString(static_cast<nsIAtom*>(GetPtr())).
        Equals(aValue, nsCaseInsensitiveStringComparator());
    default:
      break;
  }

  nsAutoString val;
  ToString(val);
  return aCaseSensitive == eCaseMatters ? val.Equals(aValue) :
    val.Equals(aValue, nsCaseInsensitiveStringComparator());
}

bool
nsAttrValue::Equals(nsIAtom* aValue, nsCaseTreatment aCaseSensitive) const
{
  if (aCaseSensitive != eCaseMatters) {
    // Need a better way to handle this!
    nsAutoString value;
    aValue->ToString(value);
    return Equals(value, aCaseSensitive);
  }
  
  switch (BaseType()) {
    case eStringBase:
    {
      nsStringBuffer* str = static_cast<nsStringBuffer*>(GetPtr());
      if (str) {
        nsDependentString dep(static_cast<PRUnichar*>(str->Data()),
                              str->StorageSize()/sizeof(PRUnichar) - 1);
        return aValue->Equals(dep);
      }
      return aValue == nsGkAtoms::_empty;
    }
    case eAtomBase:
    {
      return static_cast<nsIAtom*>(GetPtr()) == aValue;
    }
    default:
      break;
  }

  nsAutoString val;
  ToString(val);
  return aValue->Equals(val);
}

bool
nsAttrValue::EqualsAsStrings(const nsAttrValue& aOther) const
{
  if (Type() == aOther.Type()) {
    return Equals(aOther);
  }

  // We need to serialize at least one nsAttrValue before passing to
  // Equals(const nsAString&), but we can avoid unnecessarily serializing both
  // by checking if one is already of a string type.
  bool thisIsString = (BaseType() == eStringBase || BaseType() == eAtomBase);
  const nsAttrValue& lhs = thisIsString ? *this : aOther;
  const nsAttrValue& rhs = thisIsString ? aOther : *this;

  switch (rhs.BaseType()) {
    case eAtomBase:
      return lhs.Equals(rhs.GetAtomValue(), eCaseMatters);

    case eStringBase:
      return lhs.Equals(rhs.GetStringValue(), eCaseMatters);

    default:
    {
      nsAutoString val;
      rhs.ToString(val);
      return lhs.Equals(val, eCaseMatters);
    }
  }
}

bool
nsAttrValue::Contains(nsIAtom* aValue, nsCaseTreatment aCaseSensitive) const
{
  switch (BaseType()) {
    case eAtomBase:
    {
      nsIAtom* atom = GetAtomValue();

      if (aCaseSensitive == eCaseMatters) {
        return aValue == atom;
      }

      // For performance reasons, don't do a full on unicode case insensitive
      // string comparison. This is only used for quirks mode anyway.
      return
        nsContentUtils::EqualsIgnoreASCIICase(nsDependentAtomString(aValue),
                                              nsDependentAtomString(atom));
    }
    default:
    {
      if (Type() == eAtomArray) {
        AtomArray* array = GetAtomArrayValue();
        if (aCaseSensitive == eCaseMatters) {
          return array->Contains(aValue);
        }

        nsDependentAtomString val1(aValue);

        for (nsCOMPtr<nsIAtom> *cur = array->Elements(),
                               *end = cur + array->Length();
             cur != end; ++cur) {
          // For performance reasons, don't do a full on unicode case
          // insensitive string comparison. This is only used for quirks mode
          // anyway.
          if (nsContentUtils::EqualsIgnoreASCIICase(val1,
                nsDependentAtomString(*cur))) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

struct AtomArrayStringComparator {
  bool Equals(nsIAtom* atom, const nsAString& string) const {
    return atom->Equals(string);
  }
};

bool
nsAttrValue::Contains(const nsAString& aValue) const
{
  switch (BaseType()) {
    case eAtomBase:
    {
      nsIAtom* atom = GetAtomValue();
      return atom->Equals(aValue);
    }
    default:
    {
      if (Type() == eAtomArray) {
        AtomArray* array = GetAtomArrayValue();
        return array->Contains(aValue, AtomArrayStringComparator());
      }
    }
  }

  return false;
}

void
nsAttrValue::ParseAtom(const nsAString& aValue)
{
  ResetIfSet();

  nsIAtom* atom = NS_NewAtom(aValue);
  if (atom) {
    SetPtrValueAndType(atom, eAtomBase);
  }
}

void
nsAttrValue::ParseAtomArray(const nsAString& aValue)
{
  nsAString::const_iterator iter, end;
  aValue.BeginReading(iter);
  aValue.EndReading(end);
  bool hasSpace = false;
  
  // skip initial whitespace
  while (iter != end && nsContentUtils::IsHTMLWhitespace(*iter)) {
    hasSpace = true;
    ++iter;
  }

  if (iter == end) {
    SetTo(aValue);
    return;
  }

  nsAString::const_iterator start(iter);

  // get first - and often only - atom
  do {
    ++iter;
  } while (iter != end && !nsContentUtils::IsHTMLWhitespace(*iter));

  nsCOMPtr<nsIAtom> classAtom = do_GetAtom(Substring(start, iter));
  if (!classAtom) {
    Reset();
    return;
  }

  // skip whitespace
  while (iter != end && nsContentUtils::IsHTMLWhitespace(*iter)) {
    hasSpace = true;
    ++iter;
  }

  if (iter == end && !hasSpace) {
    // we only found one classname and there was no whitespace so
    // don't bother storing a list
    ResetIfSet();
    nsIAtom* atom = nullptr;
    classAtom.swap(atom);
    SetPtrValueAndType(atom, eAtomBase);
    return;
  }

  if (!EnsureEmptyAtomArray()) {
    return;
  }

  AtomArray* array = GetAtomArrayValue();
  
  if (!array->AppendElement(classAtom)) {
    Reset();
    return;
  }

  // parse the rest of the classnames
  while (iter != end) {
    start = iter;

    do {
      ++iter;
    } while (iter != end && !nsContentUtils::IsHTMLWhitespace(*iter));

    classAtom = do_GetAtom(Substring(start, iter));

    if (!array->AppendElement(classAtom)) {
      Reset();
      return;
    }

    // skip whitespace
    while (iter != end && nsContentUtils::IsHTMLWhitespace(*iter)) {
      ++iter;
    }
  }

  SetMiscAtomOrString(&aValue);
  return;
}

void
nsAttrValue::ParseStringOrAtom(const nsAString& aValue)
{
  uint32_t len = aValue.Length();
  // Don't bother with atoms if it's an empty string since
  // we can store those efficently anyway.
  if (len && len <= NS_ATTRVALUE_MAX_STRINGLENGTH_ATOM) {
    ParseAtom(aValue);
  }
  else {
    SetTo(aValue);
  }
}

void
nsAttrValue::SetIntValueAndType(int32_t aValue, ValueType aType,
                                const nsAString* aStringValue)
{
  if (aStringValue || aValue > NS_ATTRVALUE_INTEGERTYPE_MAXVALUE ||
      aValue < NS_ATTRVALUE_INTEGERTYPE_MINVALUE) {
    MiscContainer* cont = EnsureEmptyMiscContainer();
    switch (aType) {
      case eInteger:
      {
        cont->mValue.mInteger = aValue;
        break;
      }
      case ePercent:
      {
        cont->mValue.mPercent = aValue;
        break;
      }
      case eEnum:
      {
        cont->mValue.mEnumValue = aValue;
        break;
      }
      default:
      {
        NS_NOTREACHED("unknown integer type");
        break;
      }
    }
    cont->mType = aType;
    SetMiscAtomOrString(aStringValue);
  } else {
    NS_ASSERTION(!mBits, "Reset before calling SetIntValueAndType!");
    mBits = (aValue * NS_ATTRVALUE_INTEGERTYPE_MULTIPLIER) | aType;
  }
}

int16_t
nsAttrValue::GetEnumTableIndex(const EnumTable* aTable)
{
  int16_t index = sEnumTableArray->IndexOf(aTable);
  if (index < 0) {
    index = sEnumTableArray->Length();
    NS_ASSERTION(index <= NS_ATTRVALUE_ENUMTABLEINDEX_MAXVALUE,
        "too many enum tables");
    sEnumTableArray->AppendElement(aTable);
  }

  return index;
}

int32_t
nsAttrValue::EnumTableEntryToValue(const EnumTable* aEnumTable,
                                   const EnumTable* aTableEntry)
{
  int16_t index = GetEnumTableIndex(aEnumTable);
  int32_t value = (aTableEntry->value << NS_ATTRVALUE_ENUMTABLEINDEX_BITS) +
                  index;
  return value;
}

bool
nsAttrValue::ParseEnumValue(const nsAString& aValue,
                            const EnumTable* aTable,
                            bool aCaseSensitive,
                            const EnumTable* aDefaultValue)
{
  ResetIfSet();
  const EnumTable* tableEntry = aTable;

  while (tableEntry->tag) {
    if (aCaseSensitive ? aValue.EqualsASCII(tableEntry->tag) :
                         aValue.LowerCaseEqualsASCII(tableEntry->tag)) {
      int32_t value = EnumTableEntryToValue(aTable, tableEntry);

      bool equals = aCaseSensitive || aValue.EqualsASCII(tableEntry->tag);
      if (!equals) {
        nsAutoString tag;
        tag.AssignASCII(tableEntry->tag);
        nsContentUtils::ASCIIToUpper(tag);
        if ((equals = tag.Equals(aValue))) {
          value |= NS_ATTRVALUE_ENUMTABLE_VALUE_NEEDS_TO_UPPER;
        }
      }
      SetIntValueAndType(value, eEnum, equals ? nullptr : &aValue);
      NS_ASSERTION(GetEnumValue() == tableEntry->value,
                   "failed to store enum properly");

      return true;
    }
    tableEntry++;
  }

  if (aDefaultValue) {
    NS_PRECONDITION(aTable <= aDefaultValue && aDefaultValue < tableEntry,
                    "aDefaultValue not inside aTable?");
    SetIntValueAndType(EnumTableEntryToValue(aTable, aDefaultValue),
                       eEnum, &aValue);
    return true;
  }

  return false;
}

bool
nsAttrValue::ParseSpecialIntValue(const nsAString& aString)
{
  ResetIfSet();

  nsresult ec;
  bool strict;
  bool isPercent = false;
  nsAutoString tmp(aString);
  int32_t originalVal = StringToInteger(aString, &strict, &ec, true, &isPercent);

  if (NS_FAILED(ec)) {
    return false;
  }

  int32_t val = NS_MAX(originalVal, 0);

  // % (percent)
  if (isPercent || tmp.RFindChar('%') >= 0) {
    isPercent = true;
  }

  strict = strict && (originalVal == val);

  SetIntValueAndType(val,
                     isPercent ? ePercent : eInteger,
                     strict ? nullptr : &aString);
  return true;
}

bool
nsAttrValue::ParseIntWithBounds(const nsAString& aString,
                                int32_t aMin, int32_t aMax)
{
  NS_PRECONDITION(aMin < aMax, "bad boundaries");

  ResetIfSet();

  nsresult ec;
  bool strict;
  int32_t originalVal = StringToInteger(aString, &strict, &ec);
  if (NS_FAILED(ec)) {
    return false;
  }

  int32_t val = NS_MAX(originalVal, aMin);
  val = NS_MIN(val, aMax);
  strict = strict && (originalVal == val);
  SetIntValueAndType(val, eInteger, strict ? nullptr : &aString);

  return true;
}

bool
nsAttrValue::ParseNonNegativeIntValue(const nsAString& aString)
{
  ResetIfSet();

  nsresult ec;
  bool strict;
  int32_t originalVal = StringToInteger(aString, &strict, &ec);
  if (NS_FAILED(ec) || originalVal < 0) {
    return false;
  }

  SetIntValueAndType(originalVal, eInteger, strict ? nullptr : &aString);

  return true;
}

bool
nsAttrValue::ParsePositiveIntValue(const nsAString& aString)
{
  ResetIfSet();

  nsresult ec;
  bool strict;
  int32_t originalVal = StringToInteger(aString, &strict, &ec);
  if (NS_FAILED(ec) || originalVal <= 0) {
    return false;
  }

  SetIntValueAndType(originalVal, eInteger, strict ? nullptr : &aString);

  return true;
}

void
nsAttrValue::SetColorValue(nscolor aColor, const nsAString& aString)
{
  nsStringBuffer* buf = GetStringBuffer(aString);
  if (!buf) {
    return;
  }

  MiscContainer* cont = EnsureEmptyMiscContainer();
  cont->mValue.mColor = aColor;
  cont->mType = eColor;

  // Save the literal string we were passed for round-tripping.
  cont->mStringBits = reinterpret_cast<PtrBits>(buf) | eStringBase;
}

bool
nsAttrValue::ParseColor(const nsAString& aString)
{
  ResetIfSet();

  // FIXME (partially, at least): HTML5's algorithm says we shouldn't do
  // the whitespace compression, trimming, or the test for emptiness.
  // (I'm a little skeptical that we shouldn't do the whitespace
  // trimming; WebKit also does it.)
  nsAutoString colorStr(aString);
  colorStr.CompressWhitespace(true, true);
  if (colorStr.IsEmpty()) {
    return false;
  }

  nscolor color;
  // No color names begin with a '#'; in standards mode, all acceptable
  // numeric colors do.
  if (colorStr.First() == '#') {
    nsDependentString withoutHash(colorStr.get() + 1, colorStr.Length() - 1);
    if (NS_HexToRGB(withoutHash, &color)) {
      SetColorValue(color, aString);
      return true;
    }
  } else {
    if (NS_ColorNameToRGB(colorStr, &color)) {
      SetColorValue(color, aString);
      return true;
    }
  }

  // FIXME (maybe): HTML5 says we should handle system colors.  This
  // means we probably need another storage type, since we'd need to
  // handle dynamic changes.  However, I think this is a bad idea:
  // http://lists.whatwg.org/pipermail/whatwg-whatwg.org/2010-May/026449.html

  // Use NS_LooseHexToRGB as a fallback if nothing above worked.
  if (NS_LooseHexToRGB(colorStr, &color)) {
    SetColorValue(color, aString);
    return true;
  }

  return false;
}

bool nsAttrValue::ParseDoubleValue(const nsAString& aString)
{
  ResetIfSet();

  nsresult ec;
  double val = PromiseFlatString(aString).ToDouble(&ec);
  if (NS_FAILED(ec)) {
    return false;
  }

  MiscContainer* cont = EnsureEmptyMiscContainer();
  cont->mDoubleValue = val;
  cont->mType = eDoubleValue;
  nsAutoString serializedFloat;
  serializedFloat.AppendFloat(val);
  SetMiscAtomOrString(serializedFloat.Equals(aString) ? nullptr : &aString);
  return true;
}

bool
nsAttrValue::ParseIntMarginValue(const nsAString& aString)
{
  ResetIfSet();

  nsIntMargin margins;
  if (!nsContentUtils::ParseIntMarginValue(aString, margins))
    return false;

  MiscContainer* cont = EnsureEmptyMiscContainer();
  cont->mValue.mIntMargin = new nsIntMargin(margins);
  cont->mType = eIntMarginValue;
  SetMiscAtomOrString(&aString);
  return true;
}

void
nsAttrValue::LoadImage(nsIDocument* aDocument)
{
  NS_ASSERTION(Type() == eURL, "wrong type");

#ifdef DEBUG
  {
    nsString val;
    ToString(val);
    NS_ASSERTION(!val.IsEmpty(),
                 "How did we end up with an empty string for eURL");
  }
#endif

  MiscContainer* cont = GetMiscContainer();
  mozilla::css::URLValue* url = cont->mValue.mURL;
  mozilla::css::ImageValue* image = 
    new css::ImageValue(url->GetURI(), url->mString, url->mReferrer,
                        url->mOriginPrincipal, aDocument);

  NS_ADDREF(image);
  cont->mValue.mImage = image;
  NS_RELEASE(url);
  cont->mType = eImage;
}

bool
nsAttrValue::ParseStyleAttribute(const nsAString& aString,
                                 nsStyledElementNotElementCSSInlineStyle* aElement)
{
  nsIDocument* ownerDoc = aElement->OwnerDoc();
  nsHTMLCSSStyleSheet* sheet = ownerDoc->GetInlineStyleSheet();
  nsCOMPtr<nsIURI> baseURI = aElement->GetBaseURI();
  nsIURI* docURI = ownerDoc->GetDocumentURI();

  NS_ASSERTION(aElement->NodePrincipal() == ownerDoc->NodePrincipal(),
               "This is unexpected");

  // If the (immutable) document URI does not match the element's base URI
  // (the common case is that they do match) do not cache the rule.  This is
  // because the results of the CSS parser are dependent on these URIs, and we
  // do not want to have to account for the URIs in the hash lookup.
  bool cachingAllowed = sheet && baseURI == docURI;
  if (cachingAllowed) {
    MiscContainer* cont = sheet->LookupStyleAttr(aString);
    if (cont) {
      // Set our MiscContainer to the cached one.
      NS_ADDREF(cont);
      SetPtrValueAndType(cont, eOtherBase);
      return true;
    }
  }

  css::Loader* cssLoader = ownerDoc->CSSLoader();
  nsCSSParser cssParser(cssLoader);

  nsRefPtr<css::StyleRule> rule;
  cssParser.ParseStyleAttribute(aString, docURI, baseURI,
                                aElement->NodePrincipal(),
                                getter_AddRefs(rule));
  if (rule) {
    rule->SetHTMLCSSStyleSheet(sheet);
    SetTo(rule, &aString);
    if (cachingAllowed) {
      MiscContainer* cont = GetMiscContainer();
      cont->Cache();
    }

    return true;
  }

  return false;
}

void
nsAttrValue::SetMiscAtomOrString(const nsAString* aValue)
{
  NS_ASSERTION(GetMiscContainer(), "Must have MiscContainer!");
  NS_ASSERTION(!GetMiscContainer()->mStringBits,
               "Trying to re-set atom or string!");
  if (aValue) {
    uint32_t len = aValue->Length();
    // * We're allowing eCSSStyleRule attributes to store empty strings as it
    //   can be beneficial to store an empty style attribute as a parsed rule.
    // * We're allowing enumerated values because sometimes the empty
    //   string corresponds to a particular enumerated value, especially
    //   for enumerated values that are not limited enumerated.
    // Add other types as needed.
    NS_ASSERTION(len || Type() == eCSSStyleRule || Type() == eEnum,
                 "Empty string?");
    MiscContainer* cont = GetMiscContainer();
    if (len <= NS_ATTRVALUE_MAX_STRINGLENGTH_ATOM) {
      nsIAtom* atom = NS_NewAtom(*aValue);
      if (atom) {
        cont->mStringBits = reinterpret_cast<PtrBits>(atom) | eAtomBase;
      }
    } else {
      nsStringBuffer* buf = GetStringBuffer(*aValue);
      if (buf) {
        cont->mStringBits = reinterpret_cast<PtrBits>(buf) | eStringBase;
      }
    }
  }
}

void
nsAttrValue::ResetMiscAtomOrString()
{
  MiscContainer* cont = GetMiscContainer();
  void* ptr = MISC_STR_PTR(cont);
  if (ptr) {
    if (static_cast<ValueBaseType>(cont->mStringBits & NS_ATTRVALUE_BASETYPE_MASK) ==
        eStringBase) {
      static_cast<nsStringBuffer*>(ptr)->Release();
    } else {
      static_cast<nsIAtom*>(ptr)->Release();
    }
    cont->mStringBits = 0;
  }
}

void
nsAttrValue::SetSVGType(ValueType aType, const void* aValue,
                        const nsAString* aSerialized) {
  NS_ABORT_IF_FALSE(IsSVGType(aType), "Not an SVG type");

  MiscContainer* cont = EnsureEmptyMiscContainer();
  // All SVG types are just pointers to classes so just setting any of them
  // will do. We'll lose type-safety but the signature of the calling
  // function should ensure we don't get anything unexpected, and once we
  // stick aValue in a union we lose type information anyway.
  cont->mValue.mSVGAngle = static_cast<const nsSVGAngle*>(aValue);
  cont->mType = aType;
  SetMiscAtomOrString(aSerialized);
}

MiscContainer*
nsAttrValue::ClearMiscContainer()
{
  MiscContainer* cont = nullptr;
  if (BaseType() == eOtherBase) {
    cont = GetMiscContainer();
    if (cont->IsRefCounted() && cont->mValue.mRefCount > 1) {
      // This MiscContainer is shared, we need a new one.
      NS_RELEASE(cont);

      cont = new MiscContainer;
      SetPtrValueAndType(cont, eOtherBase);
    }
    else {
      switch (cont->mType) {
        case eCSSStyleRule:
        {
          MOZ_ASSERT(cont->mValue.mRefCount == 1);
          cont->Release();
          cont->Evict();
          NS_RELEASE(cont->mValue.mCSSStyleRule);
          break;
        }
        case eURL:
        {
          NS_RELEASE(cont->mValue.mURL);
          break;
        }
        case eImage:
        {
          NS_RELEASE(cont->mValue.mImage);
          break;
        }
        case eAtomArray:
        {
          delete cont->mValue.mAtomArray;
          break;
        }
        case eIntMarginValue:
        {
          delete cont->mValue.mIntMargin;
          break;
        }
        default:
        {
          break;
        }
      }
    }
    ResetMiscAtomOrString();
  }
  else {
    ResetIfSet();
  }

  return cont;
}

MiscContainer*
nsAttrValue::EnsureEmptyMiscContainer()
{
  MiscContainer* cont = ClearMiscContainer();
  if (cont) {
    MOZ_ASSERT(BaseType() == eOtherBase);
    ResetMiscAtomOrString();
    cont = GetMiscContainer();
  }
  else {
    cont = new MiscContainer;
    SetPtrValueAndType(cont, eOtherBase);
  }

  return cont;
}

bool
nsAttrValue::EnsureEmptyAtomArray()
{
  if (Type() == eAtomArray) {
    ResetMiscAtomOrString();
    GetAtomArrayValue()->Clear();
    return true;
  }

  AtomArray* array = new AtomArray;
  if (!array) {
    Reset();
    return false;
  }

  MiscContainer* cont = EnsureEmptyMiscContainer();
  cont->mValue.mAtomArray = array;
  cont->mType = eAtomArray;

  return true;
}

nsStringBuffer*
nsAttrValue::GetStringBuffer(const nsAString& aValue) const
{
  uint32_t len = aValue.Length();
  if (!len) {
    return nullptr;
  }

  nsStringBuffer* buf = nsStringBuffer::FromString(aValue);
  if (buf && (buf->StorageSize()/sizeof(PRUnichar) - 1) == len) {
    buf->AddRef();
    return buf;
  }

  buf = nsStringBuffer::Alloc((len + 1) * sizeof(PRUnichar));
  if (!buf) {
    return nullptr;
  }
  PRUnichar *data = static_cast<PRUnichar*>(buf->Data());
  CopyUnicodeTo(aValue, 0, data, len);
  data[len] = PRUnichar(0);
  return buf;
}

int32_t
nsAttrValue::StringToInteger(const nsAString& aValue, bool* aStrict,
                             nsresult* aErrorCode,
                             bool aCanBePercent,
                             bool* aIsPercent) const
{
  *aStrict = true;
  *aErrorCode = NS_ERROR_ILLEGAL_VALUE;
  if (aCanBePercent) {
    *aIsPercent = false;
  }

  nsAString::const_iterator iter, end;
  aValue.BeginReading(iter);
  aValue.EndReading(end);

  while (iter != end && nsContentUtils::IsHTMLWhitespace(*iter)) {
    *aStrict = false;
    ++iter;
  }

  if (iter == end) {
    return 0;
  }

  bool negate = false;
  if (*iter == PRUnichar('-')) {
    negate = true;
    ++iter;
  } else if (*iter == PRUnichar('+')) {
    *aStrict = false;
    ++iter;
  }

  int32_t value = 0;
  int32_t pValue = 0; // Previous value, used to check integer overflow
  while (iter != end) {
    if (*iter >= PRUnichar('0') && *iter <= PRUnichar('9')) {
      value = (value * 10) + (*iter - PRUnichar('0'));
      ++iter;
      // Checking for integer overflow.
      if (pValue > value) {
        *aStrict = false;
        *aErrorCode = NS_ERROR_ILLEGAL_VALUE;
        break;
      } else {
        pValue = value;
        *aErrorCode = NS_OK;
      }
    } else if (aCanBePercent && *iter == PRUnichar('%')) {
      ++iter;
      *aIsPercent = true;
      if (iter != end) {
        *aStrict = false;
        break;
      }
    } else {
      *aStrict = false;
      break;
    }
  }
  if (negate) {
    value = -value;
    // Checking the special case of -0.
    if (!value) {
      *aStrict = false;
    }
  }

  return value;
}

size_t
nsAttrValue::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  size_t n = 0;

  switch (BaseType()) {
    case eStringBase:
    {
      nsStringBuffer* str = static_cast<nsStringBuffer*>(GetPtr());
      n += str ? str->SizeOfIncludingThisIfUnshared(aMallocSizeOf) : 0;
      break;
    }
    case eOtherBase:
    {
      MiscContainer* container = GetMiscContainer();
      if (!container) {
        break;
      }
      n += aMallocSizeOf(container);

      void* otherPtr = MISC_STR_PTR(container);
      // We only count the size of the object pointed by otherPtr if it's a
      // string. When it's an atom, it's counted separatly.
      if (otherPtr &&
          static_cast<ValueBaseType>(container->mStringBits & NS_ATTRVALUE_BASETYPE_MASK) == eStringBase) {
        nsStringBuffer* str = static_cast<nsStringBuffer*>(otherPtr);
        n += str ? str->SizeOfIncludingThisIfUnshared(aMallocSizeOf) : 0;
      }

      if (Type() == eCSSStyleRule && container->mValue.mCSSStyleRule) {
        // TODO: mCSSStyleRule might be owned by another object which would
        //       make us count them twice, bug 677493.
        //n += container->mCSSStyleRule->SizeOfIncludingThis(aMallocSizeOf);
      } else if (Type() == eAtomArray && container->mValue.mAtomArray) {
        // Don't measure each nsIAtom, they are measured separatly.
        n += container->mValue.mAtomArray->SizeOfIncludingThis(aMallocSizeOf);
      }
      break;
    }
    case eAtomBase:    // Atoms are counted separately.
    case eIntegerBase: // The value is in mBits, nothing to do.
      break;
  }

  return n;
}

