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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jonas Sicking (original author)
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

#include "nsIDOMLinkStyle.h"
#include "nsIDOMStyleSheet.h"
#include "nsIDocument.h"
#include "nsIStyleSheet.h"
#include "nsIURI.h"
#include "nsStyleLinkElement.h"
#include "nsNetUtil.h"
#include "nsXMLProcessingInstruction.h"
#include "nsUnicharUtils.h"
#include "nsParserUtils.h"
#include "nsGkAtoms.h"
#include "nsThreadUtils.h"
#include "nsContentUtils.h"

class nsXMLStylesheetPI : public nsXMLProcessingInstruction,
                          public nsStyleLinkElement
{
public:
  nsXMLStylesheetPI(already_AddRefed<nsINodeInfo> aNodeInfo, const nsAString& aData);
  virtual ~nsXMLStylesheetPI();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_IMETHOD SetNodeValue(const nsAString& aData);

  // nsIContent
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  // nsIStyleSheetLinkingElement
  virtual void OverrideBaseURI(nsIURI* aNewBaseURI);

  // nsStyleLinkElement
  NS_IMETHOD GetCharset(nsAString& aCharset);

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  nsCOMPtr<nsIURI> mOverriddenBaseURI;

  already_AddRefed<nsIURI> GetStyleSheetURL(PRBool* aIsInline);
  void GetStyleSheetInfo(nsAString& aTitle,
                         nsAString& aType,
                         nsAString& aMedia,
                         PRBool* aIsAlternate);
  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              PRBool aCloneText) const;
};

// nsISupports implementation

DOMCI_NODE_DATA(XMLStylesheetProcessingInstruction, nsXMLStylesheetPI)

NS_INTERFACE_TABLE_HEAD(nsXMLStylesheetPI)
  NS_NODE_INTERFACE_TABLE4(nsXMLStylesheetPI, nsIDOMNode,
                           nsIDOMProcessingInstruction, nsIDOMLinkStyle,
                           nsIStyleSheetLinkingElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XMLStylesheetProcessingInstruction)
NS_INTERFACE_MAP_END_INHERITING(nsXMLProcessingInstruction)

NS_IMPL_ADDREF_INHERITED(nsXMLStylesheetPI, nsXMLProcessingInstruction)
NS_IMPL_RELEASE_INHERITED(nsXMLStylesheetPI, nsXMLProcessingInstruction)


nsXMLStylesheetPI::nsXMLStylesheetPI(already_AddRefed<nsINodeInfo> aNodeInfo,
                                     const nsAString& aData)
  : nsXMLProcessingInstruction(aNodeInfo, aData)
{
}

nsXMLStylesheetPI::~nsXMLStylesheetPI()
{
}

// nsIContent

nsresult
nsXMLStylesheetPI::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers)
{
  nsresult rv = nsXMLProcessingInstruction::BindToTree(aDocument, aParent,
                                                       aBindingParent,
                                                       aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  void (nsXMLStylesheetPI::*update)() = &nsXMLStylesheetPI::UpdateStyleSheetInternal;
  nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(this, update));

  return rv;  
}

void
nsXMLStylesheetPI::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();

  nsXMLProcessingInstruction::UnbindFromTree(aDeep, aNullParent);
  UpdateStyleSheetInternal(oldDoc);
}

// nsIDOMNode

NS_IMETHODIMP
nsXMLStylesheetPI::SetNodeValue(const nsAString& aNodeValue)
{
  nsresult rv = nsGenericDOMDataNode::SetNodeValue(aNodeValue);
  if (NS_SUCCEEDED(rv)) {
    UpdateStyleSheetInternal(nsnull, PR_TRUE);
  }
  return rv;
}

// nsStyleLinkElement

NS_IMETHODIMP
nsXMLStylesheetPI::GetCharset(nsAString& aCharset)
{
  return GetAttrValue(nsGkAtoms::charset, aCharset) ? NS_OK : NS_ERROR_FAILURE;
}

/* virtual */ void
nsXMLStylesheetPI::OverrideBaseURI(nsIURI* aNewBaseURI)
{
  mOverriddenBaseURI = aNewBaseURI;
}

already_AddRefed<nsIURI>
nsXMLStylesheetPI::GetStyleSheetURL(PRBool* aIsInline)
{
  *aIsInline = PR_FALSE;

  nsAutoString href;
  if (!GetAttrValue(nsGkAtoms::href, href)) {
    return nsnull;
  }

  nsIURI *baseURL;
  nsCAutoString charset;
  nsIDocument *document = GetOwnerDoc();
  if (document) {
    baseURL = mOverriddenBaseURI ?
              mOverriddenBaseURI.get() :
              document->GetDocBaseURI();
    charset = document->GetDocumentCharacterSet();
  } else {
    baseURL = mOverriddenBaseURI;
  }

  nsCOMPtr<nsIURI> aURI;
  NS_NewURI(getter_AddRefs(aURI), href, charset.get(), baseURL);
  return aURI.forget();
}

void
nsXMLStylesheetPI::GetStyleSheetInfo(nsAString& aTitle,
                                     nsAString& aType,
                                     nsAString& aMedia,
                                     PRBool* aIsAlternate)
{
  aTitle.Truncate();
  aType.Truncate();
  aMedia.Truncate();
  *aIsAlternate = PR_FALSE;

  // xml-stylesheet PI is special only in prolog
  if (!nsContentUtils::InProlog(this)) {
    return;
  }

  nsAutoString data;
  GetData(data);

  nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::title, aTitle);

  nsAutoString alternate;
  nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::alternate, alternate);

  // if alternate, does it have title?
  if (alternate.EqualsLiteral("yes")) {
    if (aTitle.IsEmpty()) { // alternates must have title
      return;
    }

    *aIsAlternate = PR_TRUE;
  }

  nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::media, aMedia);

  nsAutoString type;
  nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::type, type);

  nsAutoString mimeType, notUsed;
  nsParserUtils::SplitMimeType(type, mimeType, notUsed);
  if (!mimeType.IsEmpty() && !mimeType.LowerCaseEqualsLiteral("text/css")) {
    aType.Assign(type);
    return;
  }

  // If we get here we assume that we're loading a css file, so set the
  // type to 'text/css'
  aType.AssignLiteral("text/css");

  return;
}

nsGenericDOMDataNode*
nsXMLStylesheetPI::CloneDataNode(nsINodeInfo *aNodeInfo, PRBool aCloneText) const
{
  nsAutoString data;
  nsGenericDOMDataNode::GetData(data);
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  return new nsXMLStylesheetPI(ni.forget(), data);
}

nsresult
NS_NewXMLStylesheetProcessingInstruction(nsIContent** aInstancePtrResult,
                                         nsNodeInfoManager *aNodeInfoManager,
                                         const nsAString& aData)
{
  NS_PRECONDITION(aNodeInfoManager, "Missing nodeinfo manager");

  *aInstancePtrResult = nsnull;
  
  nsCOMPtr<nsINodeInfo> ni;
  ni = aNodeInfoManager->GetNodeInfo(nsGkAtoms::processingInstructionTagName,
                                     nsnull, kNameSpaceID_None,
                                     nsIDOMNode::PROCESSING_INSTRUCTION_NODE,
                                     nsGkAtoms::xml_stylesheet);
  NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

  nsXMLStylesheetPI *instance = new nsXMLStylesheetPI(ni.forget(), aData);
  if (!instance) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = instance);

  return NS_OK;
}
