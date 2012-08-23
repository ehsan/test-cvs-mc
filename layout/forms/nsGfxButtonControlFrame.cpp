/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsGfxButtonControlFrame.h"
#include "nsWidgetsCID.h"
#include "nsFormControlFrame.h"
#include "nsIFormControl.h"
#include "nsINameSpaceManager.h"
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsContentUtils.h"
// MouseEvent suppression in PP
#include "nsGUIEvent.h"
#include "nsContentList.h"
#include "nsContentCreatorFunctions.h"

#include "nsNodeInfoManager.h"
#include "nsIDOMHTMLInputElement.h"

const nscoord kSuggestedNotSet = -1;

nsGfxButtonControlFrame::nsGfxButtonControlFrame(nsStyleContext* aContext):
  nsHTMLButtonControlFrame(aContext)
{
}

nsIFrame*
NS_NewGfxButtonControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsGfxButtonControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsGfxButtonControlFrame)

void nsGfxButtonControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsContentUtils::DestroyAnonymousContent(&mTextContent);
  nsHTMLButtonControlFrame::DestroyFrom(aDestructRoot);
}

nsIAtom*
nsGfxButtonControlFrame::GetType() const
{
  return nsGkAtoms::gfxButtonControlFrame;
}

// Special check for the browse button of a file input.
//
// We'll return true if type is NS_FORM_INPUT_BUTTON and our parent
// is a file input.
bool
nsGfxButtonControlFrame::IsFileBrowseButton(int32_t type)
{
  bool rv = false;
  if (NS_FORM_INPUT_BUTTON == type) {
    // Check to see if parent is a file input
    nsCOMPtr<nsIFormControl> formCtrl =
      do_QueryInterface(mContent->GetParent());

    rv = formCtrl && formCtrl->GetType() == NS_FORM_INPUT_FILE;
  }
  return rv;
}

#ifdef DEBUG
NS_IMETHODIMP
nsGfxButtonControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ButtonControl"), aResult);
}
#endif

// Create the text content used as label for the button.
// The frame will be generated by the frame constructor.
nsresult
nsGfxButtonControlFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  nsXPIDLString label;
  GetLabel(label);

  // Add a child text content node for the label
  NS_NewTextNode(getter_AddRefs(mTextContent),
                 mContent->NodeInfo()->NodeInfoManager());
  if (!mTextContent)
    return NS_ERROR_OUT_OF_MEMORY;

  // set the value of the text node and add it to the child list
  mTextContent->SetText(label, false);
  if (!aElements.AppendElement(mTextContent))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

void
nsGfxButtonControlFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                                  uint32_t aFilter)
{
  aElements.MaybeAppendElement(mTextContent);
}

// Create the text content used as label for the button.
// The frame will be generated by the frame constructor.
nsIFrame*
nsGfxButtonControlFrame::CreateFrameFor(nsIContent*      aContent)
{
  nsIFrame * newFrame = nullptr;

  if (aContent == mTextContent) {
    nsIFrame * parentFrame = mFrames.FirstChild();

    nsPresContext* presContext = PresContext();
    nsRefPtr<nsStyleContext> textStyleContext;
    textStyleContext = presContext->StyleSet()->
      ResolveStyleForNonElement(mStyleContext);

    if (textStyleContext) {
      newFrame = NS_NewTextFrame(presContext->PresShell(), textStyleContext);
      if (newFrame) {
        // initialize the text frame
        newFrame->Init(mTextContent, parentFrame, nullptr);
        mTextContent->SetPrimaryFrame(newFrame);
      }
    }
  }

  return newFrame;
}

nsresult
nsGfxButtonControlFrame::GetFormProperty(nsIAtom* aName, nsAString& aValue) const
{
  nsresult rv = NS_OK;
  if (nsGkAtoms::defaultLabel == aName) {
    // This property is used by accessibility to get
    // the default label of the button.
    nsXPIDLString temp;
    rv = const_cast<nsGfxButtonControlFrame*>(this)->GetDefaultLabel(temp);
    aValue = temp;
  } else {
    aValue.Truncate();
  }
  return rv;
}

NS_QUERYFRAME_HEAD(nsGfxButtonControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLButtonControlFrame)

// Initially we hardcoded the default strings here.
// Next, we used html.css to store the default label for various types
// of buttons. (nsGfxButtonControlFrame::DoNavQuirksReflow rev 1.20)
// However, since html.css is not internationalized, we now grab the default
// label from a string bundle as is done for all other UI strings.
// See bug 16999 for further details.
nsresult
nsGfxButtonControlFrame::GetDefaultLabel(nsXPIDLString& aString)
{
  nsCOMPtr<nsIFormControl> form = do_QueryInterface(mContent);
  NS_ENSURE_TRUE(form, NS_ERROR_UNEXPECTED);

  int32_t type = form->GetType();
  const char *prop;
  if (type == NS_FORM_INPUT_RESET) {
    prop = "Reset";
  } 
  else if (type == NS_FORM_INPUT_SUBMIT) {
    prop = "Submit";
  } 
  else if (IsFileBrowseButton(type)) {
    prop = "Browse";
  }
  else {
    aString.Truncate();
    return NS_OK;
  }

  return nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                            prop, aString);
}

nsresult
nsGfxButtonControlFrame::GetLabel(nsXPIDLString& aLabel)
{
  // Get the text from the "value" property on our content if there is
  // one; otherwise set it to a default value (localized).
  nsresult rv;
  nsCOMPtr<nsIDOMHTMLInputElement> elt = do_QueryInterface(mContent);
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::value) && elt) {
    rv = elt->GetValue(aLabel);
  } else {
    // Generate localized label.
    // We can't make any assumption as to what the default would be
    // because the value is localized for non-english platforms, thus
    // it might not be the string "Reset", "Submit Query", or "Browse..."
    rv = GetDefaultLabel(aLabel);
  }

  NS_ENSURE_SUCCESS(rv, rv);

  // Compress whitespace out of label if needed.
  if (!GetStyleText()->WhiteSpaceIsSignificant()) {
    aLabel.CompressWhitespace();
  } else if (aLabel.Length() > 2 && aLabel.First() == ' ' &&
             aLabel.CharAt(aLabel.Length() - 1) == ' ') {
    // This is a bit of a hack.  The reason this is here is as follows: we now
    // have default padding on our buttons to make them non-ugly.
    // Unfortunately, IE-windows does not have such padding, so people will
    // stick values like " ok " (with the spaces) in the buttons in an attempt
    // to make them look decent.  Unfortunately, if they do this the button
    // looks way too big in Mozilla.  Worse yet, if they do this _and_ set a
    // fixed width for the button we run into trouble because our focus-rect
    // border/padding and outer border take up 10px of the horizontal button
    // space or so; the result is that the text is misaligned, even with the
    // recentering we do in nsHTMLButtonFrame::Reflow.  So to solve this, even
    // if the whitespace is significant, single leading and trailing _spaces_
    // (and not other whitespace) are removed.  The proper solution, of
    // course, is to not have the focus rect painting taking up 6px of
    // horizontal space. We should do that instead (via XBL form controls or
    // changing the renderer) and remove this.
    aLabel.Cut(0, 1);
    aLabel.Truncate(aLabel.Length() - 1);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGfxButtonControlFrame::AttributeChanged(int32_t         aNameSpaceID,
                                          nsIAtom*        aAttribute,
                                          int32_t         aModType)
{
  nsresult rv = NS_OK;

  // If the value attribute is set, update the text of the label
  if (nsGkAtoms::value == aAttribute) {
    if (mTextContent && mContent) {
      nsXPIDLString label;
      rv = GetLabel(label);
      NS_ENSURE_SUCCESS(rv, rv);
    
      mTextContent->SetText(label, true);
    } else {
      rv = NS_ERROR_UNEXPECTED;
    }

  // defer to HTMLButtonControlFrame
  } else {
    rv = nsHTMLButtonControlFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
  }
  return rv;
}

bool
nsGfxButtonControlFrame::IsLeaf() const
{
  return true;
}

nsIFrame*
nsGfxButtonControlFrame::GetContentInsertionFrame()
{
  return this;
}

NS_IMETHODIMP
nsGfxButtonControlFrame::HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent*     aEvent,
                                      nsEventStatus*  aEventStatus)
{
  // Override the HandleEvent to prevent the nsFrame::HandleEvent
  // from being called. The nsFrame::HandleEvent causes the button label
  // to be selected (Drawn with an XOR rectangle over the label)

  // do we have user-input style?
  const nsStyleUserInterface* uiStyle = GetStyleUserInterface();
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  
  return NS_OK;
}
