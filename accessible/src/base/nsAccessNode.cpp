/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Original Author: Aaron Leventhal (aaronl@netscape.com)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsDocAccessible.h"

#include "nsIAccessible.h"

#include "nsAccCache.h"
#include "nsAccessibilityAtoms.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"

#include "nsHashtable.h"
#include "nsAccessibilityService.h"
#include "nsApplicationAccessibleWrap.h"
#include "nsIAccessibleDocument.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocument.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIFrame.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsRootAccessible.h"
#include "nsFocusManager.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"

/* For documentation of the accessibility architecture, 
 * see http://lxr.mozilla.org/seamonkey/source/accessible/accessible-docs.html
 */

nsIStringBundle *nsAccessNode::gStringBundle = 0;
nsIStringBundle *nsAccessNode::gKeyStringBundle = 0;
nsINode *nsAccessNode::gLastFocusedNode = nsnull;

PRBool nsAccessNode::gIsCacheDisabled = PR_FALSE;
PRBool nsAccessNode::gIsFormFillEnabled = PR_FALSE;

nsApplicationAccessible *nsAccessNode::gApplicationAccessible = nsnull;

/*
 * Class nsAccessNode
 */
 
////////////////////////////////////////////////////////////////////////////////
// nsAccessible. nsISupports

NS_IMPL_CYCLE_COLLECTION_0(nsAccessNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsAccessNode)
  NS_INTERFACE_MAP_ENTRY(nsIAccessNode)
  NS_INTERFACE_MAP_ENTRY(nsAccessNode)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAccessNode)
NS_INTERFACE_MAP_END
 
NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsAccessNode, nsIAccessNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE_FULL(nsAccessNode, nsIAccessNode,
                                      LastRelease())

////////////////////////////////////////////////////////////////////////////////
// nsAccessNode construction/desctruction

nsAccessNode::
  nsAccessNode(nsIContent *aContent, nsIWeakReference *aShell) :
  mContent(aContent), mWeakShell(aShell)
{
#ifdef DEBUG_A11Y
  mIsInitialized = PR_FALSE;
#endif
}

nsAccessNode::~nsAccessNode()
{
  NS_ASSERTION(!mWeakShell, "LastRelease was never called!?!");
}

void nsAccessNode::LastRelease()
{
  // First cleanup if needed...
  if (mWeakShell) {
    Shutdown();
    NS_ASSERTION(!mWeakShell, "A Shutdown() impl forgot to call its parent's Shutdown?");
  }
  // ... then die.
  NS_DELETEXPCOM(this);
}

////////////////////////////////////////////////////////////////////////////////
// nsAccessNode public

nsresult
nsAccessNode::Init()
{
  // We have to put this here, instead of constructor, otherwise
  // we don't have the virtual GetUniqueID() method for the hash key.
  // We need that for accessibles that don't have DOM nodes

#ifdef DEBUG_A11Y
  NS_ASSERTION(!mIsInitialized, "Initialized twice!");
#endif
  nsDocAccessible *docAcc = nsnull;
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
  if (presShell)
    docAcc = GetAccService()->GetDocAccessible(presShell->GetDocument());

  NS_ASSERTION(docAcc, "Cannot cache new nsAccessNode");
  if (!docAcc) {
#ifdef DEBUG
    docAcc = GetAccService()->GetDocAccessible(presShell->GetDocument());
#endif
    return NS_ERROR_FAILURE;
  }

  void* uniqueID;
  GetUniqueID(&uniqueID);

  if (!docAcc->CacheAccessNode(uniqueID, this))
    return NS_ERROR_OUT_OF_MEMORY;

  // Make sure an ancestor in real content is cached
  // so that nsDocAccessible::RefreshNodes() can find the anonymous subtree to release when
  // the root node goes away
  if (mContent && mContent->IsInAnonymousSubtree()) {
    // Specific examples of where this is used: <input type="file"> and <xul:findbar>
    nsAccessible *parent = GetAccService()->GetContainerAccessible(mContent,
                                                                   PR_TRUE);
    if (parent)
      parent->EnsureChildren();
  }

#ifdef DEBUG_A11Y
  mIsInitialized = PR_TRUE;
#endif

  return NS_OK;
}


nsresult
nsAccessNode::Shutdown()
{
  mContent = nsnull;
  mWeakShell = nsnull;

  return NS_OK;
}

// nsIAccessNode
NS_IMETHODIMP nsAccessNode::GetUniqueID(void **aUniqueID)
{
  *aUniqueID = static_cast<void*>(GetNode());
  return NS_OK;
}

// nsIAccessNode
NS_IMETHODIMP nsAccessNode::GetOwnerWindow(void **aWindow)
{
  *aWindow = nsnull;
  nsCOMPtr<nsIAccessibleDocument> docAccessible(GetDocAccessible());
  if (!docAccessible)
    return NS_ERROR_FAILURE; // This node or doc accessible is shut down
  return docAccessible->GetWindowHandle(aWindow);
}

nsApplicationAccessible*
nsAccessNode::GetApplicationAccessible()
{
  NS_ASSERTION(!nsAccessibilityService::IsShutdown(),
               "Accessibility wasn't initialized!");

  if (!gApplicationAccessible) {
    nsApplicationAccessibleWrap::PreCreate();

    gApplicationAccessible = new nsApplicationAccessibleWrap();
    if (!gApplicationAccessible)
      return nsnull;

    // Addref on create. Will Release in ShutdownXPAccessibility()
    NS_ADDREF(gApplicationAccessible);

    nsresult rv = gApplicationAccessible->Init();
    if (NS_FAILED(rv)) {
      gApplicationAccessible->Shutdown();
      NS_RELEASE(gApplicationAccessible);
      return nsnull;
    }
  }

  return gApplicationAccessible;
}

void nsAccessNode::InitXPAccessibility()
{
  nsCOMPtr<nsIStringBundleService> stringBundleService =
    mozilla::services::GetStringBundleService();
  if (stringBundleService) {
    // Static variables are released in ShutdownAllXPAccessibility();
    stringBundleService->CreateBundle(ACCESSIBLE_BUNDLE_URL, 
                                      &gStringBundle);
    stringBundleService->CreateBundle(PLATFORM_KEYS_BUNDLE_URL, 
                                      &gKeyStringBundle);
  }

  nsAccessibilityAtoms::AddRefAtoms();

  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefBranch) {
    prefBranch->GetBoolPref("accessibility.disablecache", &gIsCacheDisabled);
    prefBranch->GetBoolPref("browser.formfill.enable", &gIsFormFillEnabled);
  }

  NotifyA11yInitOrShutdown(PR_TRUE);
}

// nsAccessNode protected static
void nsAccessNode::NotifyA11yInitOrShutdown(PRBool aIsInit)
{
  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  NS_ASSERTION(obsService, "No observer service to notify of a11y init/shutdown");
  if (!obsService)
    return;

  static const PRUnichar kInitIndicator[] = { '1', 0 };
  static const PRUnichar kShutdownIndicator[] = { '0', 0 }; 
  obsService->NotifyObservers(nsnull, "a11y-init-or-shutdown",
                              aIsInit ? kInitIndicator  : kShutdownIndicator);
}

void nsAccessNode::ShutdownXPAccessibility()
{
  // Called by nsAccessibilityService::Shutdown()
  // which happens when xpcom is shutting down
  // at exit of program

  NS_IF_RELEASE(gStringBundle);
  NS_IF_RELEASE(gKeyStringBundle);
  NS_IF_RELEASE(gLastFocusedNode);

  // Release gApplicationAccessible after everything else is shutdown
  // so we don't accidently create it again while tearing down root accessibles
  nsApplicationAccessibleWrap::Unload();
  if (gApplicationAccessible) {
    gApplicationAccessible->Shutdown();
    NS_RELEASE(gApplicationAccessible);
  }

  NotifyA11yInitOrShutdown(PR_FALSE);
}

PRBool
nsAccessNode::IsDefunct()
{
  if (!mContent)
    return PR_TRUE;

  // Call GetPresShell() since the accessible may be shut down in it.
  nsCOMPtr<nsIPresShell> presShell(GetPresShell());
  return !presShell;
}

already_AddRefed<nsIPresShell> nsAccessNode::GetPresShell()
{
  nsIPresShell *presShell = nsnull;
  if (mWeakShell)
    CallQueryReferent(mWeakShell.get(), &presShell);
  if (!presShell) {
    if (mWeakShell) {
      // If our pres shell has died, but we're still holding onto
      // a weak reference, our accessibles are no longer relevant
      // and should be shut down
      Shutdown();
    }
    return nsnull;
  }
  return presShell;
}

// nsAccessNode protected
nsPresContext* nsAccessNode::GetPresContext()
{
  nsCOMPtr<nsIPresShell> presShell(GetPresShell());
  if (!presShell) {
    return nsnull;
  }
  return presShell->GetPresContext();
}

already_AddRefed<nsRootAccessible> nsAccessNode::GetRootAccessible()
{
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mContent);
  NS_ASSERTION(docShellTreeItem, "No docshell tree item for mContent");
  if (!docShellTreeItem) {
    return nsnull;
  }
  nsCOMPtr<nsIDocShellTreeItem> root;
  docShellTreeItem->GetRootTreeItem(getter_AddRefs(root));
  NS_ASSERTION(root, "No root content tree item");
  if (!root) {
    return nsnull;
  }

  nsDocAccessible *docAcc = nsAccUtils::GetDocAccessibleFor(root);
  nsRefPtr<nsRootAccessible> rootAcc = do_QueryObject(docAcc);
  return rootAcc.forget();
}

nsIFrame*
nsAccessNode::GetFrame()
{
  return mContent ? mContent->GetPrimaryFrame() : nsnull;
}

#ifdef DEBUG
PRBool
nsAccessNode::IsInCache()
{
  nsDocAccessible *docAccessible = nsAccUtils::GetDocAccessibleFor(mWeakShell);
  if (!docAccessible)
    return nsnull;

  void* uniqueID = nsnull;
  GetUniqueID(&uniqueID);

  return docAccessible->GetCachedAccessNode(uniqueID) ? PR_TRUE : PR_FALSE;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// nsIAccessNode

NS_IMETHODIMP
nsAccessNode::GetDOMNode(nsIDOMNode **aDOMNode)
{
  NS_ENSURE_ARG_POINTER(aDOMNode);
  *aDOMNode = nsnull;

  nsINode *node = GetNode();
  if (node)
    CallQueryInterface(node, aDOMNode);

  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetNumChildren(PRInt32 *aNumChildren)
{
  NS_ENSURE_ARG_POINTER(aNumChildren);
  *aNumChildren = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aNumChildren = GetNode()->GetChildCount();
  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetDocument(nsIAccessibleDocument **aDocument)
{
  NS_ENSURE_ARG_POINTER(aDocument);

  NS_IF_ADDREF(*aDocument = nsAccUtils::GetDocAccessibleFor(mWeakShell));
  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetRootDocument(nsIAccessibleDocument **aRootDocument)
{
  NS_ENSURE_ARG_POINTER(aRootDocument);

  nsRefPtr<nsRootAccessible> rootDocument = GetRootAccessible();
  NS_IF_ADDREF(*aRootDocument = rootDocument.get());
  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetInnerHTML(nsAString& aInnerHTML)
{
  aInnerHTML.Truncate();

  nsCOMPtr<nsIDOMNSHTMLElement> domNSElement(do_QueryInterface(mContent));
  NS_ENSURE_TRUE(domNSElement, NS_ERROR_NULL_POINTER);

  return domNSElement->GetInnerHTML(aInnerHTML);
}

NS_IMETHODIMP
nsAccessNode::ScrollTo(PRUint32 aScrollType)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> shell(GetPresShell());
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

  nsIFrame *frame = GetFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  nsCOMPtr<nsIContent> content = frame->GetContent();
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  PRInt16 vPercent, hPercent;
  nsCoreUtils::ConvertScrollTypeToPercents(aScrollType, &vPercent, &hPercent);
  return shell->ScrollContentIntoView(content, vPercent, hPercent);
}

NS_IMETHODIMP
nsAccessNode::ScrollToPoint(PRUint32 aCoordinateType, PRInt32 aX, PRInt32 aY)
{
  nsIFrame *frame = GetFrame();
  if (!frame)
    return NS_ERROR_FAILURE;

  nsIntPoint coords;
  nsresult rv = nsAccUtils::ConvertToScreenCoords(aX, aY, aCoordinateType,
                                                  this, &coords);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIFrame *parentFrame = frame;
  while ((parentFrame = parentFrame->GetParent()))
    nsCoreUtils::ScrollFrameToPoint(parentFrame, frame, coords);

  return NS_OK;
}

// nsAccessNode protected
nsAccessNode *
nsAccessNode::MakeAccessNode(nsINode *aNode)
{
  nsAccessNode *accessNode = GetAccService()->GetCachedAccessNode(aNode,
                                                                  mWeakShell);

  if (!accessNode)
    accessNode = GetAccService()->GetAccessibleInWeakShell(aNode, mWeakShell);

  if (accessNode)
    return accessNode;

  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (!content)
    return nsnull;

  nsAccessNode *newAccessNode = new nsAccessNode(content, mWeakShell);
  if (!newAccessNode)
    return nsnull;

  // Initialize and cache it.
  if (NS_FAILED(newAccessNode->Init())) {
    newAccessNode->Shutdown();
    delete newAccessNode;
    return nsnull;
  }

  return newAccessNode;
}

NS_IMETHODIMP
nsAccessNode::GetFirstChildNode(nsIAccessNode **aAccessNode)
{
  NS_ENSURE_ARG_POINTER(aAccessNode);
  *aAccessNode = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsIContent* childNode = GetNode()->GetChildAt(0);
  if (childNode)
    NS_IF_ADDREF(*aAccessNode = MakeAccessNode(childNode));

  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetLastChildNode(nsIAccessNode **aAccessNode)
{
  NS_ENSURE_ARG_POINTER(aAccessNode);
  *aAccessNode = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRUint32 childCount = GetNode()->GetChildCount();
  nsIContent* childNode = GetNode()->GetChildAt(childCount - 1);
  if (childNode)
    NS_IF_ADDREF(*aAccessNode = MakeAccessNode(childNode));

  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetParentNode(nsIAccessNode **aAccessNode)
{
  NS_ENSURE_ARG_POINTER(aAccessNode);
  *aAccessNode = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsINode* parentNode = GetNode()->GetNodeParent();
  if (parentNode)
    NS_IF_ADDREF(*aAccessNode = MakeAccessNode(parentNode));

  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetPreviousSiblingNode(nsIAccessNode **aAccessNode)
{
  NS_ENSURE_ARG_POINTER(aAccessNode);
  *aAccessNode = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsINode* parentNode = GetNode()->GetNodeParent();
  PRInt32 indexOf = parentNode->IndexOf(GetNode());
  if (indexOf == -1)
    return NS_OK;

  nsIContent *siblingNode = parentNode->GetChildAt(indexOf - 1);
  if (siblingNode)
    NS_IF_ADDREF(*aAccessNode = MakeAccessNode(siblingNode));

  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetNextSiblingNode(nsIAccessNode **aAccessNode)
{
  NS_ENSURE_ARG_POINTER(aAccessNode);
  *aAccessNode = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsINode* parentNode = GetNode()->GetNodeParent();
  PRInt32 indexOf = parentNode->IndexOf(GetNode());
  if (indexOf == -1)
    return NS_OK;

  nsIContent *siblingNode = parentNode->GetChildAt(indexOf + 1);
  if (siblingNode)
    NS_IF_ADDREF(*aAccessNode = MakeAccessNode(siblingNode));

  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetChildNodeAt(PRInt32 aChildNum, nsIAccessNode **aAccessNode)
{
  NS_ENSURE_ARG_POINTER(aAccessNode);
  *aAccessNode = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsIContent* childNode = GetNode()->GetChildAt(aChildNum);
  if (childNode)
    NS_IF_ADDREF(*aAccessNode = MakeAccessNode(childNode));

  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetComputedStyleValue(const nsAString& aPseudoElt,
                                    const nsAString& aPropertyName,
                                    nsAString& aValue)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCSSStyleDeclaration> styleDecl =
    nsCoreUtils::GetComputedStyleDeclaration(aPseudoElt, mContent);
  NS_ENSURE_TRUE(styleDecl, NS_ERROR_FAILURE);

  return styleDecl->GetPropertyValue(aPropertyName, aValue);
}

NS_IMETHODIMP
nsAccessNode::GetComputedStyleCSSValue(const nsAString& aPseudoElt,
                                       const nsAString& aPropertyName,
                                       nsIDOMCSSPrimitiveValue **aCSSValue)
{
  NS_ENSURE_ARG_POINTER(aCSSValue);
  *aCSSValue = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCSSStyleDeclaration> styleDecl =
    nsCoreUtils::GetComputedStyleDeclaration(aPseudoElt, mContent);
  NS_ENSURE_STATE(styleDecl);

  nsCOMPtr<nsIDOMCSSValue> cssValue;
  styleDecl->GetPropertyCSSValue(aPropertyName, getter_AddRefs(cssValue));
  NS_ENSURE_TRUE(cssValue, NS_ERROR_FAILURE);

  return CallQueryInterface(cssValue, aCSSValue);
}

// nsAccessNode public
already_AddRefed<nsINode>
nsAccessNode::GetCurrentFocus()
{
  // XXX: consider to use nsFocusManager directly, it allows us to avoid
  // unnecessary query interface calls.
  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  NS_ENSURE_TRUE(shell, nsnull);
  nsIDocument *doc = shell->GetDocument();
  NS_ENSURE_TRUE(doc, nsnull);

  nsIDOMWindow* win = doc->GetWindow();

  nsCOMPtr<nsIDOMWindow> focusedWindow;
  nsCOMPtr<nsIDOMElement> focusedElement;
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm)
    fm->GetFocusedElementForWindow(win, PR_TRUE, getter_AddRefs(focusedWindow),
                                   getter_AddRefs(focusedElement));

  nsINode *focusedNode = nsnull;
  if (focusedElement) {
    CallQueryInterface(focusedElement, &focusedNode);
  }
  else if (focusedWindow) {
    nsCOMPtr<nsIDOMDocument> doc;
    focusedWindow->GetDocument(getter_AddRefs(doc));
    if (doc)
      CallQueryInterface(doc, &focusedNode);
  }

  return focusedNode;
}

NS_IMETHODIMP
nsAccessNode::GetLanguage(nsAString& aLanguage)
{
  aLanguage.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCoreUtils::GetLanguageFor(mContent, nsnull, aLanguage);

  if (aLanguage.IsEmpty()) { // Nothing found, so use document's language
    nsIDocument *doc = mContent->GetOwnerDoc();
    if (doc) {
      doc->GetHeaderData(nsAccessibilityAtoms::headerContentLanguage, aLanguage);
    }
  }
 
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsAccessNode protected

nsDocAccessible*
nsAccessNode::GetDocAccessible() const
{
  return nsAccUtils::GetDocAccessibleFor(mWeakShell);
}
