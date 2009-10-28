/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sw=2 et tw=78: */
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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Henri Sivonen <hsivonen@iki.fi>
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

#include "nsContentErrors.h"
#include "nsContentCreatorFunctions.h"
#include "nsIDOMDocumentType.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsEvent.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsContentUtils.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIFormControl.h"
#include "nsNodeUtils.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsTraceRefcnt.h"
#include "mozAutoDocUpdate.h"
#include "nsIScriptElement.h"
#include "nsIDTD.h"

// this really should be autogenerated...
jArray<PRUnichar,PRInt32> nsHtml5TreeBuilder::ISINDEX_PROMPT = jArray<PRUnichar,PRInt32>();
nsHtml5TreeBuilder::nsHtml5TreeBuilder(nsHtml5TreeOpExecutor* aExec)
  : scriptingEnabled(PR_FALSE)
  , fragment(PR_FALSE)
  , contextNode(nsnull)
  , formPointer(nsnull)
  , headPointer(nsnull)
  , mExecutor(aExec)
#ifdef DEBUG
  , mActive(PR_FALSE)
#endif
{
  MOZ_COUNT_CTOR(nsHtml5TreeBuilder);
}

nsHtml5TreeBuilder::~nsHtml5TreeBuilder()
{
  MOZ_COUNT_DTOR(nsHtml5TreeBuilder);
  NS_ASSERTION(!mActive, "nsHtml5TreeBuilder deleted without ever calling end() on it!");
  mOpQueue.Clear();
}

nsIContent*
nsHtml5TreeBuilder::createElement(PRInt32 aNamespace, nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes)
{
  nsIContent* newContent;
  nsCOMPtr<nsINodeInfo> nodeInfo = mExecutor->GetNodeInfoManager()->GetNodeInfo(aName, nsnull, aNamespace);
  NS_ASSERTION(nodeInfo, "Got null nodeinfo.");
  NS_NewElement(&newContent, nodeInfo->NamespaceID(), nodeInfo, PR_TRUE);
  NS_ASSERTION(newContent, "Element creation created null pointer.");
  PRInt32 len = aAttributes->getLength();
  for (PRInt32 i = 0; i < len; ++i) {
    newContent->SetAttr(aAttributes->getURI(i), aAttributes->getLocalName(i), aAttributes->getPrefix(i), *(aAttributes->getValue(i)), PR_FALSE);
    // XXX what to do with nsresult?
  }
  if (aNamespace != kNameSpaceID_MathML && (aName == nsHtml5Atoms::style || (aNamespace == kNameSpaceID_XHTML && aName == nsHtml5Atoms::link))) {
    nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(newContent));
    if (ssle) {
      ssle->InitStyleLinkElement(PR_FALSE);
      ssle->SetEnableUpdates(PR_FALSE);
#if 0
      if (!aNodeInfo->Equals(nsGkAtoms::link, kNameSpaceID_XHTML)) {
        ssle->SetLineNumber(aLineNumber);
      }
#endif
    }
  }
  return newContent;
}

nsIContent*
nsHtml5TreeBuilder::createElement(PRInt32 aNamespace, nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes, nsIContent* aFormElement)
{
  nsIContent* content = createElement(aNamespace, aName, aAttributes);
  if (aFormElement) {
    nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(content));
    NS_ASSERTION(formControl, "Form-associated element did not implement nsIFormControl.");
    nsCOMPtr<nsIDOMHTMLFormElement> formElement(do_QueryInterface(aFormElement));
    NS_ASSERTION(formElement, "The form element doesn't implement nsIDOMHTMLFormElement.");
    if (formControl) { // avoid crashing on <output>
      formControl->SetForm(formElement);
    }
  }
  return content;
}

nsIContent*
nsHtml5TreeBuilder::createHtmlElementSetAsRoot(nsHtml5HtmlAttributes* aAttributes)
{
  nsIContent* content = createElement(kNameSpaceID_XHTML, nsHtml5Atoms::html, aAttributes);
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(eTreeOpAppendToDocument, content);
  return content;
}

void
nsHtml5TreeBuilder::detachFromParent(nsIContent* aElement)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(eTreeOpDetach, aElement);
}

void
nsHtml5TreeBuilder::appendElement(nsIContent* aChild, nsIContent* aParent)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(aChild, aParent);
}

void
nsHtml5TreeBuilder::appendChildrenToNewParent(nsIContent* aOldParent, nsIContent* aNewParent)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(eTreeOpAppendChildrenToNewParent, aOldParent, aNewParent);
}

void
nsHtml5TreeBuilder::insertFosterParentedCharacters(PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength, nsIContent* aTable, nsIContent* aStackParent)
{
  nsCOMPtr<nsIContent> text;
  NS_NewTextNode(getter_AddRefs(text), mExecutor->GetNodeInfoManager());
  // XXX nsresult and comment null check?
  text->SetText(aBuffer + aStart, aLength, PR_FALSE);
  // XXX nsresult
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(eTreeOpFosterParent, text, aStackParent, aTable);
}

void
nsHtml5TreeBuilder::insertFosterParentedChild(nsIContent* aChild, nsIContent* aTable, nsIContent* aStackParent)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(eTreeOpFosterParent, aChild, aStackParent, aTable);
}

void
nsHtml5TreeBuilder::appendCharacters(nsIContent* aParent, PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsCOMPtr<nsIContent> text;
  NS_NewTextNode(getter_AddRefs(text), mExecutor->GetNodeInfoManager());
  // XXX nsresult and comment null check?
  text->SetText(aBuffer + aStart, aLength, PR_FALSE);
  // XXX nsresult
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(text, aParent);
}

void
nsHtml5TreeBuilder::appendComment(nsIContent* aParent, PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsCOMPtr<nsIContent> comment;
  NS_NewCommentNode(getter_AddRefs(comment), mExecutor->GetNodeInfoManager());
  // XXX nsresult and comment null check?
  comment->SetText(aBuffer + aStart, aLength, PR_FALSE);
  // XXX nsresult
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(comment, aParent);
}

void
nsHtml5TreeBuilder::appendCommentToDocument(PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsCOMPtr<nsIContent> comment;
  NS_NewCommentNode(getter_AddRefs(comment), mExecutor->GetNodeInfoManager());
  // XXX nsresult and comment null check?
  comment->SetText(aBuffer + aStart, aLength, PR_FALSE);
  // XXX nsresult
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(eTreeOpAppendToDocument, comment);
}

void
nsHtml5TreeBuilder::addAttributesToElement(nsIContent* aElement, nsHtml5HtmlAttributes* aAttributes)
{
  nsIContent* holder = createElement(kNameSpaceID_XHTML, nsHtml5Atoms::div, aAttributes);
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(eTreeOpAddAttributes, holder, aElement);
}

void
nsHtml5TreeBuilder::markMalformedIfScript(nsIContent* elt)
{
  nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(elt);
  if (sele) {
    // Make sure to serialize this script correctly, for nice round tripping.
    sele->SetIsMalformed();
  }
}

void
nsHtml5TreeBuilder::start(PRBool fragment)
{
  // XXX check that timer creation didn't fail in constructor
  if (!fragment) {
    /*
     * If you move the following line, be very careful not to cause 
     * WillBuildModel to be called before the document has had its 
     * script global object set.
     */
    mExecutor->WillBuildModel(eDTDMode_unknown);
  }
  mExecutor->Start();
#ifdef DEBUG
  mActive = PR_TRUE;
#endif
}

void
nsHtml5TreeBuilder::end()
{
  mExecutor->End();
  mOpQueue.Clear();
#ifdef DEBUG
  mActive = PR_FALSE;
#endif
}

void
nsHtml5TreeBuilder::appendDoctypeToDocument(nsIAtom* aName, nsString* aPublicId, nsString* aSystemId)
{
  // Adapted from nsXMLContentSink
  // Create a new doctype node
  nsCOMPtr<nsIDOMDocumentType> docType;
  nsAutoString voidString;
  voidString.SetIsVoid(PR_TRUE);
  NS_NewDOMDocumentType(getter_AddRefs(docType),
                        mExecutor->GetNodeInfoManager(),
                        nsnull,
                        aName,
                        nsnull,
                        nsnull,
                        *aPublicId,
                        *aSystemId,
                        voidString);
  NS_ASSERTION(docType, "Doctype creation failed.");
  nsCOMPtr<nsIContent> content = do_QueryInterface(docType);
  NS_ASSERTION(content, "doctype isn't content?");
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(eTreeOpAppendToDocument, content);
  // nsXMLContentSink can flush here, but what's the point?
  // It can also interrupt here, but we can't.
}

void
nsHtml5TreeBuilder::elementPushed(PRInt32 aNamespace, nsIAtom* aName, nsIContent* aElement)
{
  NS_ASSERTION(aNamespace == kNameSpaceID_XHTML || aNamespace == kNameSpaceID_SVG || aNamespace == kNameSpaceID_MathML, "Element isn't HTML, SVG or MathML!");
  NS_ASSERTION(aName, "Element doesn't have local name!");
  NS_ASSERTION(aElement, "No element!");
  // Give autoloading links a chance to fire
  if (aNamespace == kNameSpaceID_XHTML) {
    if (aName == nsHtml5Atoms::body) {
      nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
      // XXX if null, OOM!
      treeOp->Init(eTreeOpStartLayout, nsnull);
    } else if (aName == nsHtml5Atoms::html) {
      nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
      // XXX if null, OOM!
      treeOp->Init(eTreeOpProcessOfflineManifest, aElement);
      return;
    }
  }
  mExecutor->MaybeSuspend();
}

void
nsHtml5TreeBuilder::elementPopped(PRInt32 aNamespace, nsIAtom* aName, nsIContent* aElement)
{
  NS_ASSERTION(aNamespace == kNameSpaceID_XHTML || aNamespace == kNameSpaceID_SVG || aNamespace == kNameSpaceID_MathML, "Element isn't HTML, SVG or MathML!");
  NS_ASSERTION(aName, "Element doesn't have local name!");
  NS_ASSERTION(aElement, "No element!");
  mExecutor->MaybeSuspend();
  if (aNamespace == kNameSpaceID_MathML) {
    return;
  }
  // we now have only SVG and HTML
  if (aName == nsHtml5Atoms::script) {
    requestSuspension();
    mExecutor->SetScriptElement(aElement);
    return;
  }
  if (aName == nsHtml5Atoms::title) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    // XXX if null, OOM!
    treeOp->Init(eTreeOpDoneAddingChildren, aElement);
    return;
  }
  if (aName == nsHtml5Atoms::style || (aNamespace == kNameSpaceID_XHTML && aName == nsHtml5Atoms::link)) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    // XXX if null, OOM!
    treeOp->Init(eTreeOpUpdateStyleSheet, aElement);
    return;
  }
  if (aNamespace == kNameSpaceID_SVG) {
#if 0
    if (aElement->HasAttr(kNameSpaceID_None, nsHtml5Atoms::onload)) {
      nsEvent event(PR_TRUE, NS_SVG_LOAD);
      event.eventStructType = NS_SVG_EVENT;
      event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
      // Do we care about forcing presshell creation if it hasn't happened yet?
      // That is, should this code flush or something?  Does it really matter?
      // For that matter, do we really want to try getting the prescontext?  Does
      // this event ever want one?
      nsRefPtr<nsPresContext> ctx;
      nsCOMPtr<nsIPresShell> shell = parser->GetDocument()->GetPrimaryShell();
      if (shell) {
        ctx = shell->GetPresContext();
      }
      nsEventDispatcher::Dispatch(aElement, ctx, &event);
    }
#endif
    return;
  }
  // we now have only HTML
  // Some HTML nodes need DoneAddingChildren() called to initialize
  // properly (e.g. form state restoration).
  // XXX expose ElementName group here and do switch
  if (aName == nsHtml5Atoms::select ||
        aName == nsHtml5Atoms::textarea ||
#ifdef MOZ_MEDIA
        aName == nsHtml5Atoms::video ||
        aName == nsHtml5Atoms::audio ||
#endif
        aName == nsHtml5Atoms::object ||
        aName == nsHtml5Atoms::applet) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    // XXX if null, OOM!
    treeOp->Init(eTreeOpDoneAddingChildren, aElement);
    return;
  }
  if (aName == nsHtml5Atoms::input ||
      aName == nsHtml5Atoms::button) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    // XXX if null, OOM!
    treeOp->Init(eTreeOpDoneCreatingElement, aElement);
    return;
  }
  if (aName == nsHtml5Atoms::base) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    // XXX if null, OOM!
    treeOp->Init(eTreeOpProcessBase, aElement);
    return;
  }
  if (aName == nsHtml5Atoms::meta) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    // XXX if null, OOM!
    treeOp->Init(eTreeOpProcessMeta, aElement);
    return;
  }
  return;
}

void
nsHtml5TreeBuilder::accumulateCharacters(PRUnichar* aBuf, PRInt32 aStart, PRInt32 aLength)
{
  PRInt32 newFillLen = charBufferLen + aLength;
  if (newFillLen > charBuffer.length) {
    PRInt32 newAllocLength = newFillLen + (newFillLen >> 1);
    jArray<PRUnichar,PRInt32> newBuf(newAllocLength);
    memcpy(newBuf, charBuffer, sizeof(PRUnichar) * charBufferLen);
    charBuffer.release();
    charBuffer = newBuf;
  }
  memcpy(charBuffer + charBufferLen, aBuf + aStart, sizeof(PRUnichar) * aLength);
  charBufferLen = newFillLen;
}

void
nsHtml5TreeBuilder::DoUnlink()
{
  nsHtml5TreeBuilder* tmp = this;
  NS_IF_RELEASE(contextNode);
  NS_IF_RELEASE(formPointer);
  NS_IF_RELEASE(headPointer);
  while (currentPtr > -1) {
    stack[currentPtr]->release();
    currentPtr--;
  }
  while (listPtr > -1) {
    if (listOfActiveFormattingElements[listPtr]) {
      listOfActiveFormattingElements[listPtr]->release();
    }
    listPtr--;
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mOpQueue);
}

void
nsHtml5TreeBuilder::DoTraverse(nsCycleCollectionTraversalCallback &cb)
{
  nsHtml5TreeBuilder* tmp = this;
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(contextNode);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(formPointer);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(headPointer);
  if (stack) {
    for (PRInt32 i = 0; i <= currentPtr; i++) {
#ifdef DEBUG_CC
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "stack[i]");
#endif
      cb.NoteNativeChild(stack[i], &NS_CYCLE_COLLECTION_NAME(nsHtml5StackNode));
    }
  }
  if (listOfActiveFormattingElements) {
    for (PRInt32 i = 0; i <= listPtr; i++) {
      if (listOfActiveFormattingElements[i]) {
#ifdef DEBUG_CC
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "listOfActiveFormattingElements[i]");
#endif
        cb.NoteNativeChild(listOfActiveFormattingElements[i], &NS_CYCLE_COLLECTION_NAME(nsHtml5StackNode));
      }
    }
  }
  const nsHtml5TreeOperation* start = mOpQueue.Elements();
  const nsHtml5TreeOperation* end = start + mOpQueue.Length();
  for (nsHtml5TreeOperation* iter = (nsHtml5TreeOperation*)start; iter < end; ++iter) {
#ifdef DEBUG_CC
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mOpQueue[i]");
#endif
    iter->DoTraverse(cb);
  }
}

// DocumentModeHandler
void
nsHtml5TreeBuilder::documentMode(nsHtml5DocumentMode m)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  // XXX if null, OOM!
  treeOp->Init(m);
}
