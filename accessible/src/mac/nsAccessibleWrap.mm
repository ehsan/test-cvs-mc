/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsDocAccessible.h"
#include "nsObjCExceptions.h"

#include "Accessible-inl.h"
#include "Role.h"

#import "mozAccessible.h"
#import "mozActionElements.h"
#import "mozHTMLAccessible.h"
#import "mozTextAccessible.h"

using namespace mozilla::a11y;

nsAccessibleWrap::
  nsAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessible(aContent, aDoc), mNativeObject(nil),  
  mNativeInited(false)
{
}

nsAccessibleWrap::~nsAccessibleWrap()
{
}

mozAccessible* 
nsAccessibleWrap::GetNativeObject()
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NIL;
  
  if (!mNativeInited && !mNativeObject && !IsDefunct() && !AncestorIsFlat())
    mNativeObject = [[GetNativeType() alloc] initWithAccessible:this];
  
  mNativeInited = true;
  
  return mNativeObject;
  
  NS_OBJC_END_TRY_ABORT_BLOCK_NIL;
}

NS_IMETHODIMP
nsAccessibleWrap::GetNativeInterface (void **aOutInterface) 
{
  NS_ENSURE_ARG_POINTER(aOutInterface);

  *aOutInterface = static_cast<void*>(GetNativeObject());
    
  return *aOutInterface ? NS_OK : NS_ERROR_FAILURE;
}

// overridden in subclasses to create the right kind of object. by default we create a generic
// 'mozAccessible' node.
Class
nsAccessibleWrap::GetNativeType () 
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NIL;

  roles::Role role = Role();
  switch (role) {
    case roles::PUSHBUTTON:
    case roles::SPLITBUTTON:
    case roles::TOGGLE_BUTTON:
    {
      // if this button may show a popup, let's make it of the popupbutton type.
      return HasPopup() ? [mozPopupButtonAccessible class] : 
             [mozButtonAccessible class];
    }
    
    case roles::PAGETAB:
      return [mozButtonAccessible class];

    case roles::CHECKBUTTON:
      return [mozCheckboxAccessible class];
      
    case roles::HEADING:
      return [mozHeadingAccessible class];

    case roles::PAGETABLIST:
      return [mozTabsAccessible class];
      
    case roles::ENTRY:
    case roles::STATICTEXT:
    case roles::CAPTION:
    case roles::ACCEL_LABEL:
    case roles::TEXT_LEAF:
    case roles::PASSWORD_TEXT:
      // normal textfield (static or editable)
      return [mozTextAccessible class]; 

    case roles::LINK:
      return [mozLinkAccessible class];

    case roles::COMBOBOX:
      return [mozPopupButtonAccessible class];
      
    default:
      return [mozAccessible class];
  }
  
  return nil;

  NS_OBJC_END_TRY_ABORT_BLOCK_NIL;
}

// this method is very important. it is fired when an accessible object "dies". after this point
// the object might still be around (because some 3rd party still has a ref to it), but it is
// in fact 'dead'.
void
nsAccessibleWrap::Shutdown ()
{
  // this ensure we will not try to re-create the native object.
  mNativeInited = true;

  // we really intend to access the member directly.
  if (mNativeObject) {
    [mNativeObject expire];
    [mNativeObject release];
    mNativeObject = nil;
  }

  nsAccessible::Shutdown();
}

nsresult
nsAccessibleWrap::HandleAccEvent(AccEvent* aEvent)
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NSRESULT;

  nsresult rv = nsAccessible::HandleAccEvent(aEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  return FirePlatformEvent(aEvent);

  NS_OBJC_END_TRY_ABORT_BLOCK_NSRESULT;
}

nsresult
nsAccessibleWrap::FirePlatformEvent(AccEvent* aEvent)
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NSRESULT;

  PRUint32 eventType = aEvent->GetEventType();

  // ignore everything but focus-changed, value-changed, caret and selection
  // events for now.
  if (eventType != nsIAccessibleEvent::EVENT_FOCUS &&
      eventType != nsIAccessibleEvent::EVENT_VALUE_CHANGE &&
      eventType != nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED &&
      eventType != nsIAccessibleEvent::EVENT_TEXT_SELECTION_CHANGED)
    return NS_OK;

  nsAccessible *accessible = aEvent->GetAccessible();
  NS_ENSURE_STATE(accessible);

  mozAccessible *nativeAcc = nil;
  accessible->GetNativeInterface((void**)&nativeAcc);
  if (!nativeAcc)
    return NS_ERROR_FAILURE;

  switch (eventType) {
    case nsIAccessibleEvent::EVENT_FOCUS:
      [nativeAcc didReceiveFocus];
      break;
    case nsIAccessibleEvent::EVENT_VALUE_CHANGE:
      [nativeAcc valueDidChange];
      break;
    case nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED:
    case nsIAccessibleEvent::EVENT_TEXT_SELECTION_CHANGED:
      [nativeAcc selectedTextDidChange];
      break;
  }

  return NS_OK;

  NS_OBJC_END_TRY_ABORT_BLOCK_NSRESULT;
}

void
nsAccessibleWrap::InvalidateChildren()
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

  [GetNativeObject() invalidateChildren];

  nsAccessible::InvalidateChildren();

  NS_OBJC_END_TRY_ABORT_BLOCK;
}

bool
nsAccessibleWrap::AppendChild(nsAccessible *aAccessible)
{
  bool appended = nsAccessible::AppendChild(aAccessible);
  
  if (appended && mNativeObject)
    [mNativeObject appendChild:aAccessible];

  return appended;
}

bool
nsAccessibleWrap::RemoveChild(nsAccessible *aAccessible)
{
  bool removed = nsAccessible::RemoveChild(aAccessible);

  if (removed && mNativeObject)
    [mNativeObject invalidateChildren];

  return removed;
}

// if we for some reason have no native accessible, we should be skipped over (and traversed)
// when fetching all unignored children, etc.  when counting unignored children, we will not be counted.
bool 
nsAccessibleWrap::IsIgnored() 
{
  NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;
  
  mozAccessible* nativeObject = GetNativeObject();
  return (!nativeObject) || [nativeObject accessibilityIsIgnored];
  
  NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(false);
}

void
nsAccessibleWrap::GetUnignoredChildren(nsTArray<nsAccessible*>* aChildrenArray)
{
  // we're flat; there are no children.
  if (nsAccUtils::MustPrune(this))
    return;

  PRInt32 childCount = GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessibleWrap *childAcc =
      static_cast<nsAccessibleWrap*>(GetChildAt(childIdx));

    // If element is ignored, then add its children as substitutes.
    if (childAcc->IsIgnored()) {
      childAcc->GetUnignoredChildren(aChildrenArray);
      continue;
    }

    aChildrenArray->AppendElement(childAcc);
  }
}

nsAccessible*
nsAccessibleWrap::GetUnignoredParent() const
{
  // Go up the chain to find a parent that is not ignored.
  nsAccessibleWrap* parentWrap = static_cast<nsAccessibleWrap*>(Parent());
  while (parentWrap && parentWrap->IsIgnored()) 
    parentWrap = static_cast<nsAccessibleWrap*>(parentWrap->Parent());
    
  return parentWrap;
}

////////////////////////////////////////////////////////////////////////////////
// nsAccessibleWrap protected

bool
nsAccessibleWrap::AncestorIsFlat()
{
  // We don't create a native object if we're child of a "flat" accessible;
  // for example, on OS X buttons shouldn't have any children, because that
  // makes the OS confused. 
  //
  // To maintain a scripting environment where the XPCOM accessible hierarchy
  // look the same on all platforms, we still let the C++ objects be created
  // though.

  nsAccessible* parent = Parent();
  while (parent) {
    if (nsAccUtils::MustPrune(parent))
      return true;

    parent = parent->Parent();
  }
  // no parent was flat
  return false;
}
