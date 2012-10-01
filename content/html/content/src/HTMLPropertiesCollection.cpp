/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set tw=80 expandtab softtabstop=2 ts=2 sw=2: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HTMLPropertiesCollection.h"
#include "dombindings.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#include "nsGenericHTMLElement.h"
#include "nsVariant.h"
#include "nsDOMSettableTokenList.h"
#include "nsAttrValue.h"
#include "nsWrapperCacheInlines.h"
#include "mozilla/dom/HTMLPropertiesCollectionBinding.h"

DOMCI_DATA(HTMLPropertiesCollection, mozilla::dom::HTMLPropertiesCollection)
DOMCI_DATA(PropertyNodeList, mozilla::dom::PropertyNodeList)

namespace mozilla {
namespace dom {

static PLDHashOperator
TraverseNamedProperties(const nsAString& aKey, PropertyNodeList* aEntry, void* aData)
{
  nsCycleCollectionTraversalCallback* cb = static_cast<nsCycleCollectionTraversalCallback*>(aData);
  cb->NoteXPCOMChild(static_cast<nsIDOMPropertyNodeList*>(aEntry));
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLPropertiesCollection)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(HTMLPropertiesCollection)
  // SetDocument(nullptr) ensures that we remove ourselves as a mutation observer
  tmp->SetDocument(nullptr);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRoot)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mNames)
  tmp->mNamedItemEntries.Clear();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mProperties)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(HTMLPropertiesCollection)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDoc)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRoot)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mNames)
  tmp->mNamedItemEntries.EnumerateRead(TraverseNamedProperties, &cb);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_OF_NSCOMPTR(mProperties)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(HTMLPropertiesCollection)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

HTMLPropertiesCollection::HTMLPropertiesCollection(nsGenericHTMLElement* aRoot)
  : mRoot(aRoot)
  , mDoc(aRoot->GetCurrentDoc())
  , mIsDirty(true)
{
  SetIsDOMBinding();
  mNames = new PropertyStringList(this);
  if (mDoc) {
    mDoc->AddMutationObserver(this);
  }
  mNamedItemEntries.Init();
}

HTMLPropertiesCollection::~HTMLPropertiesCollection()
{
  if (mDoc) {
    mDoc->RemoveMutationObserver(this);
  }
}

NS_INTERFACE_TABLE_HEAD(HTMLPropertiesCollection)
    NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
    NS_INTERFACE_TABLE4(HTMLPropertiesCollection,
                        nsIDOMHTMLPropertiesCollection,
                        nsIDOMHTMLCollection,
                        nsIHTMLCollection,
                        nsIMutationObserver)
    NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(HTMLPropertiesCollection)
    NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HTMLPropertiesCollection)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(HTMLPropertiesCollection)
NS_IMPL_CYCLE_COLLECTING_RELEASE(HTMLPropertiesCollection)


static PLDHashOperator
SetPropertyListDocument(const nsAString& aKey, PropertyNodeList* aEntry, void* aData)
{
  aEntry->SetDocument(static_cast<nsIDocument*>(aData));
  return PL_DHASH_NEXT;
}

void
HTMLPropertiesCollection::SetDocument(nsIDocument* aDocument) {
  if (mDoc) {
    mDoc->RemoveMutationObserver(this);
  }
  mDoc = aDocument;
  if (mDoc) {
    mDoc->AddMutationObserver(this);
  }
  mNamedItemEntries.EnumerateRead(SetPropertyListDocument, aDocument);
  mIsDirty = true;
}

JSObject*
HTMLPropertiesCollection::WrapObject(JSContext* cx, JSObject* scope,
                                     bool* triedToWrap)
{
  JSObject* obj = HTMLPropertiesCollectionBinding::Wrap(cx, scope, this,
                                                        triedToWrap);
  if (obj || *triedToWrap) {
    return obj;
  }

  *triedToWrap = true;
  return oldproxybindings::HTMLPropertiesCollection::create(cx, scope, this);
}

NS_IMETHODIMP
HTMLPropertiesCollection::GetLength(uint32_t* aLength)
{
  EnsureFresh();
  *aLength = mProperties.Length();
  return NS_OK;
}

NS_IMETHODIMP
HTMLPropertiesCollection::Item(uint32_t aIndex, nsIDOMNode** aResult)
{
  nsINode* result = nsIHTMLCollection::Item(aIndex);
  if (result) {
    NS_ADDREF(*aResult = result->AsDOMNode());
  } else {
    *aResult = nullptr;
  }
  return NS_OK;
}

NS_IMETHODIMP
HTMLPropertiesCollection::NamedItem(const nsAString& aName,
                                    nsIDOMNode** aResult)
{
  *aResult = NULL;
  return NS_OK;
}

JSObject*
HTMLPropertiesCollection::NamedItem(JSContext* cx, const nsAString& name,
                                    mozilla::ErrorResult& error)
{
  // HTMLPropertiesCollection.namedItem and the named getter call the NamedItem
  // that returns a PropertyNodeList, calling HTMLCollection.namedItem doesn't
  // make sense so this returns null.
  return nullptr;
}

nsISupports*
HTMLPropertiesCollection::GetNamedItem(const nsAString& aName,
                                       nsWrapperCache **aCache)
{
  if (!IsSupportedNamedProperty(aName)) {
    *aCache = NULL;
    return NULL;
  }

  nsRefPtr<PropertyNodeList> propertyList;
  if (!mNamedItemEntries.Get(aName, getter_AddRefs(propertyList))) {
    propertyList = new PropertyNodeList(this, mRoot, aName);
    mNamedItemEntries.Put(aName, propertyList);
  }
  *aCache = propertyList;
  return static_cast<nsIDOMPropertyNodeList*>(propertyList);
}

nsGenericElement*
HTMLPropertiesCollection::GetElementAt(uint32_t aIndex)
{
  EnsureFresh();
  return mProperties.SafeElementAt(aIndex);
}

nsINode*
HTMLPropertiesCollection::GetParentObject()
{
  return mRoot;
}

PropertyNodeList*
HTMLPropertiesCollection::NamedItem(const nsAString& aName)
{
  EnsureFresh();

  PropertyNodeList* propertyList = mNamedItemEntries.GetWeak(aName);
  if (!propertyList) {
    nsRefPtr<PropertyNodeList> newPropertyList =
      new PropertyNodeList(this, mRoot, aName);
    mNamedItemEntries.Put(aName, newPropertyList);
    propertyList = newPropertyList;
  }
  return propertyList;
}

NS_IMETHODIMP
HTMLPropertiesCollection::NamedItem(const nsAString& aName,
                                    nsIDOMPropertyNodeList** aResult)
{
  NS_ADDREF(*aResult = NamedItem(aName));
  return NS_OK;
}

NS_IMETHODIMP
HTMLPropertiesCollection::GetNames(nsIDOMDOMStringList** aResult)
{
  NS_ADDREF(*aResult = Names());
  return NS_OK;
}

void
HTMLPropertiesCollection::AttributeChanged(nsIDocument *aDocument, Element* aElement,
                                           int32_t aNameSpaceID, nsIAtom* aAttribute,
                                           int32_t aModType)
{
  mIsDirty = true;
}

void
HTMLPropertiesCollection::ContentAppended(nsIDocument* aDocument, nsIContent* aContainer,
                                          nsIContent* aFirstNewContent,
                                          int32_t aNewIndexInContainer)
{
  mIsDirty = true;
}

void
HTMLPropertiesCollection::ContentInserted(nsIDocument *aDocument,
                                          nsIContent* aContainer,
                                          nsIContent* aChild,
                                          int32_t aIndexInContainer)
{
  mIsDirty = true;
}

void
HTMLPropertiesCollection::ContentRemoved(nsIDocument *aDocument,
                                         nsIContent* aContainer,
                                         nsIContent* aChild,
                                         int32_t aIndexInContainer,
                                         nsIContent* aPreviousSibling)
{
  mIsDirty = true;
}

class TreeOrderComparator {
  public:
    bool Equals(const nsGenericHTMLElement* aElem1,
                const nsGenericHTMLElement* aElem2) const {
      return aElem1 == aElem2;
    }
    bool LessThan(const nsGenericHTMLElement* aElem1,
                  const nsGenericHTMLElement* aElem2) const {
      return nsContentUtils::PositionIsBefore(const_cast<nsGenericHTMLElement*>(aElem1),
                                              const_cast<nsGenericHTMLElement*>(aElem2));
    }
};

static PLDHashOperator
MarkDirty(const nsAString& aKey, PropertyNodeList* aEntry, void* aData)
{
  aEntry->SetDirty();
  return PL_DHASH_NEXT;
}

void
HTMLPropertiesCollection::EnsureFresh()
{
  if (mDoc && !mIsDirty) {
    return;
  }
  mIsDirty = false;

  mProperties.Clear();
  mNames->Clear();
  // We don't clear NamedItemEntries because the PropertyNodeLists must be live.
  mNamedItemEntries.EnumerateRead(MarkDirty, NULL);
  if (!mRoot->HasAttr(kNameSpaceID_None, nsGkAtoms::itemscope)) {
    return;
  }

  CrawlProperties();
  TreeOrderComparator comparator;
  mProperties.Sort(comparator);

  // Create the names DOMStringList
  uint32_t count = mProperties.Length();
  for (uint32_t i = 0; i < count; ++i) {
    const nsAttrValue* attr = mProperties.ElementAt(i)->GetParsedAttr(nsGkAtoms::itemprop); 
    for (uint32_t i = 0; i < attr->GetAtomCount(); i++) {
      nsDependentAtomString propName(attr->AtomAt(i));
      // ContainsInternal must not call EnsureFresh
      bool contains = mNames->ContainsInternal(propName);
      if (!contains) {
        mNames->Add(propName);
      }
    }
  }
}
  
static Element*
GetElementByIdForConnectedSubtree(nsIContent* aContent, const nsIAtom* aId)
{
  aContent = static_cast<nsIContent*>(aContent->SubtreeRoot());
  do {
    if (aContent->GetID() == aId) {
      return aContent->AsElement();
    }
    aContent = aContent->GetNextNode();
  } while(aContent);
  
  return NULL;
}

void
HTMLPropertiesCollection::CrawlProperties()
{
  nsIDocument* doc = mRoot->GetCurrentDoc();
 
  const nsAttrValue* attr = mRoot->GetParsedAttr(nsGkAtoms::itemref);
  if (attr) {
    for (uint32_t i = 0; i < attr->GetAtomCount(); i++) {
      nsIAtom* ref = attr->AtomAt(i);
      Element* element;
      if (doc) {
        element = doc->GetElementById(nsDependentAtomString(ref));
      } else {
        element = GetElementByIdForConnectedSubtree(mRoot, ref);
      }
      if (element && element != mRoot) {
        CrawlSubtree(element);
      }
    }
  }
  
  CrawlSubtree(mRoot);
}

void
HTMLPropertiesCollection::CrawlSubtree(Element* aElement)
{
  nsIContent* aContent = aElement;
  while (aContent) {
    // We must check aContent against mRoot because 
    // an element must not be its own property
    if (aContent == mRoot || !aContent->IsHTML()) {
      // Move on to the next node in the tree
      aContent = aContent->GetNextNode(aElement);
    } else {
      MOZ_ASSERT(aContent->IsElement(), "IsHTML() returned true!");
      Element* element = aContent->AsElement();
      if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::itemprop) &&
          !mProperties.Contains(element)) {
        mProperties.AppendElement(static_cast<nsGenericHTMLElement*>(element));
      }                 
                     
      if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::itemscope)) {
        aContent = element->GetNextNonChildNode(aElement);
      } else {          
        aContent = element->GetNextNode(aElement);
      }                 
    }                     
  }                    
}

PropertyNodeList::PropertyNodeList(HTMLPropertiesCollection* aCollection,
                                   nsIContent* aParent, const nsAString& aName)
  : mName(aName),
    mDoc(aParent->GetCurrentDoc()),
    mCollection(aCollection),
    mParent(aParent),
    mIsDirty(true)
{
  SetIsDOMBinding();
  if (mDoc) {
    mDoc->AddMutationObserver(this);
  }
}

PropertyNodeList::~PropertyNodeList()
{
  if (mDoc) {
    mDoc->RemoveMutationObserver(this);
  }
}

void
PropertyNodeList::SetDocument(nsIDocument* aDoc)
{
  if (mDoc) {
    mDoc->RemoveMutationObserver(this);
  }
  mDoc = aDoc;
  if (mDoc) {
    mDoc->AddMutationObserver(this);
  }
  mIsDirty = true;
}

NS_IMETHODIMP
PropertyNodeList::GetLength(uint32_t* aLength)
{
  EnsureFresh();
  *aLength = mElements.Length();
  return NS_OK;
}

NS_IMETHODIMP
PropertyNodeList::Item(uint32_t aIndex, nsIDOMNode** aReturn)
{
  EnsureFresh();
  nsINode* element = mElements.SafeElementAt(aIndex);
  if (!element) {
    *aReturn = NULL;
    return NS_OK;
  }
  return CallQueryInterface(element, aReturn);
}

nsIContent*
PropertyNodeList::GetNodeAt(uint32_t aIndex)
{
  EnsureFresh();
  return mElements.SafeElementAt(aIndex);
}

int32_t
PropertyNodeList::IndexOf(nsIContent* aContent)
{
  EnsureFresh();
  return mElements.IndexOf(aContent);
}

nsINode*
PropertyNodeList::GetParentObject()
{
  return mParent;
}

JSObject*
PropertyNodeList::WrapObject(JSContext *cx, JSObject *scope,
                             bool *triedToWrap)
{
  JSObject* obj = PropertyNodeListBinding::Wrap(cx, scope, this, triedToWrap);
  if (obj || *triedToWrap) {
    return obj;
  }

  *triedToWrap = true;
  return oldproxybindings::PropertyNodeList::create(cx, scope, this);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(PropertyNodeList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(PropertyNodeList)
  // SetDocument(nullptr) ensures that we remove ourselves as a mutation observer
  tmp->SetDocument(nullptr);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCollection)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mElements)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(PropertyNodeList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDoc)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mCollection, nsIDOMHTMLPropertiesCollection)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_OF_NSCOMPTR(mElements)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(PropertyNodeList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(PropertyNodeList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(PropertyNodeList)

NS_INTERFACE_TABLE_HEAD(PropertyNodeList)
    NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
    NS_INTERFACE_TABLE4(PropertyNodeList,
                        nsIDOMPropertyNodeList,
                        nsIDOMNodeList,
                        nsINodeList,
                        nsIMutationObserver)
    NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(PropertyNodeList)
    NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(PropertyNodeList)
NS_INTERFACE_MAP_END

void
PropertyNodeList::GetValues(JSContext* aCx, nsTArray<JS::Value >& aResult,
                            ErrorResult& aError)
{
  EnsureFresh();

  JSObject* wrapper = GetWrapper();
  JSAutoCompartment ac(aCx, wrapper);
  uint32_t length = mElements.Length();
  for (uint32_t i = 0; i < length; ++i) {
    JS::Value v = mElements.ElementAt(i)->GetItemValue(aCx, wrapper, aError);
    if (aError.Failed()) {
      return;
    }
    aResult.AppendElement(v);
  }
}

NS_IMETHODIMP
PropertyNodeList::GetValues(nsIVariant** aValues)
{
  EnsureFresh();
  nsCOMPtr<nsIWritableVariant> out = new nsVariant();

  // We have to use an nsTArray<nsIVariant*> here and do manual refcounting because 
  // nsWritableVariant::SetAsArray takes an nsIVariant**.
  nsTArray<nsIVariant*> values;

  uint32_t length = mElements.Length();
  if (length == 0) {
    out->SetAsEmptyArray();
  } else {
    for (uint32_t i = 0; i < length; ++i) {
      nsIVariant* itemValue;
      mElements.ElementAt(i)->GetItemValue(&itemValue);
      values.AppendElement(itemValue);
    }
    out->SetAsArray(nsIDataType::VTYPE_INTERFACE_IS,
                    &NS_GET_IID(nsIVariant),
                    values.Length(),
                    values.Elements());
  }

  out.forget(aValues);

  for (uint32_t i = 0; i < values.Length(); ++i) {
    NS_RELEASE(values[i]);
  }

  return NS_OK;
}

void
PropertyNodeList::AttributeChanged(nsIDocument* aDocument, Element* aElement,
                                   int32_t aNameSpaceID, nsIAtom* aAttribute,
                                   int32_t aModType)
{
  mIsDirty = true;
}

void
PropertyNodeList::ContentAppended(nsIDocument* aDocument, nsIContent* aContainer,
                                  nsIContent* aFirstNewContent,
                                  int32_t aNewIndexInContainer)
{
  mIsDirty = true;
}

void
PropertyNodeList::ContentInserted(nsIDocument* aDocument,
                                  nsIContent* aContainer,
                                  nsIContent* aChild,
                                  int32_t aIndexInContainer)
{
  mIsDirty = true;
}

void
PropertyNodeList::ContentRemoved(nsIDocument* aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aChild,
                                 int32_t aIndexInContainer,
                                 nsIContent* aPreviousSibling)
{
  mIsDirty = true;
}

void
PropertyNodeList::EnsureFresh()
{
  if (mDoc && !mIsDirty) {
    return;
  }
  mIsDirty = false;

  mCollection->EnsureFresh();
  Clear();

  uint32_t count = mCollection->mProperties.Length();
  for (uint32_t i = 0; i < count; ++i) {
    nsGenericHTMLElement* element = mCollection->mProperties.ElementAt(i);
    const nsAttrValue* attr = element->GetParsedAttr(nsGkAtoms::itemprop);
    if (attr->Contains(mName)) {
      AppendElement(element);
    }
  }
}

PropertyStringList::PropertyStringList(HTMLPropertiesCollection* aCollection)
  : nsDOMStringList()
  , mCollection(aCollection)
{ }

NS_IMPL_CYCLE_COLLECTION_CLASS(PropertyStringList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(PropertyStringList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCollection)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(PropertyStringList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mCollection, nsIDOMHTMLPropertiesCollection)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(PropertyStringList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(PropertyStringList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(PropertyStringList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMStringList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMStringList)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
PropertyStringList::Item(uint32_t aIndex, nsAString& aResult)
{
  mCollection->EnsureFresh();
  return nsDOMStringList::Item(aIndex, aResult);
}

NS_IMETHODIMP
PropertyStringList::GetLength(uint32_t* aLength)
{
  mCollection->EnsureFresh();
  return nsDOMStringList::GetLength(aLength);
}

NS_IMETHODIMP
PropertyStringList::Contains(const nsAString& aString, bool* aResult)
{
  mCollection->EnsureFresh();
  return nsDOMStringList::Contains(aString, aResult);
}

bool
PropertyStringList::ContainsInternal(const nsAString& aString)
{
  // This method should not call EnsureFresh, otherwise we may become stuck in an infinite loop.
  bool result;
  nsDOMStringList::Contains(aString, &result);
  return result;
}

} // namespace dom
} // namespace mozilla
