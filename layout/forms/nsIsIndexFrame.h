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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef nsIsIndexFrame_h___
#define nsIsIndexFrame_h___

#include "nsBlockFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIStatefulFrame.h"
#include "nsIUnicodeEncoder.h"
#include "nsIDOMKeyListener.h"

#include "nsTextControlFrame.h"
typedef   nsTextControlFrame nsNewFrame;

class nsIsIndexFrame : public nsBlockFrame,
                       public nsIAnonymousContentCreator,
                       public nsIStatefulFrame
{
public:
  nsIsIndexFrame(nsStyleContext* aContext);
  virtual ~nsIsIndexFrame();

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

private:
  void KeyPress(nsIDOMEvent* aKeyEvent);

  class KeyListener : public nsIDOMKeyListener
  {
    NS_DECL_ISUPPORTS

    KeyListener(nsIsIndexFrame* aOwner) : mOwner(aOwner) { };

    NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent) { return NS_OK; }

    NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent) { return NS_OK; }

    NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent); // we only care when a key is pressed

    NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

    nsIsIndexFrame* mOwner;
  };

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  // nsIFormControlFrame
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  
  virtual PRBool IsLeaf() const;

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  void           SetFocus(PRBool aOn, PRBool aRepaint);

  // nsIAnonymousContentCreator
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter);

  NS_IMETHOD OnSubmit(nsPresContext* aPresContext);

  //nsIStatefulFrame
  NS_IMETHOD SaveState(SpecialStateID aStateID, nsPresState** aState);
  NS_IMETHOD RestoreState(nsPresState* aState);

protected:
  // native anonymous content generated by this frame when
  // asked via the nsIAnonymousContentCreator interface.
  nsCOMPtr<nsIContent> mTextContent;
  nsCOMPtr<nsIContent> mInputContent;
  nsCOMPtr<nsIContent> mPreHr;
  nsCOMPtr<nsIContent> mPostHr;

private:
  nsresult UpdatePromptLabel(PRBool aNotify);
  nsresult GetInputFrame(nsIFormControlFrame** oFrame);
  void GetInputValue(nsString& oString);
  void SetInputValue(const nsString& aString);

  void GetSubmitCharset(nsCString& oCharset);
  NS_IMETHOD GetEncoder(nsIUnicodeEncoder** encoder);
  char* UnicodeToNewBytes(const PRUnichar* aSrc, PRUint32 aLen, nsIUnicodeEncoder* encoder);
  void URLEncode(const nsString& aString, nsIUnicodeEncoder* encoder, nsString& oString);

  nsCOMPtr<KeyListener> mListener;
};

#endif


