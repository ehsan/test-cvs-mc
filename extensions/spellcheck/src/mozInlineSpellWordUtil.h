/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsCOMPtr.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsString.h"
#include "nsTArray.h"

//#define DEBUG_SPELLCHECK

class nsRange;
class nsINode;

/**
 *    This class extracts text from the DOM and builds it into a single string.
 *    The string includes whitespace breaks whereever non-inline elements begin
 *    and end. This string is broken into "real words", following somewhat
 *    complex rules; for example substrings that look like URLs or
 *    email addresses are treated as single words, but otherwise many kinds of
 *    punctuation are treated as word separators. GetNextWord provides a way
 *    to iterate over these "real words".
 *
 *    The basic operation is:
 *
 *    1. Call Init with the weak pointer to the editor that you're using.
 *    2. Call SetEnd to set where you want to stop spellchecking. We'll stop
 *       at the word boundary after that. If SetEnd is not called, we'll stop
 *       at the end of the document's root element.
 *    3. Call SetPosition to initialize the current position inside the
 *       previously given range.
 *    4. Call GetNextWord over and over until it returns false.
 */

class mozInlineSpellWordUtil
{
public:
  struct NodeOffset {
    nsINode* mNode;
    PRInt32  mOffset;
    
    NodeOffset(nsINode* aNode, PRInt32 aOffset) :
      mNode(aNode), mOffset(aOffset) {}
  };

  mozInlineSpellWordUtil()
    : mRootNode(nullptr),
      mSoftBegin(nullptr, 0), mSoftEnd(nullptr, 0),
      mNextWordIndex(-1), mSoftTextValid(false) {}

  nsresult Init(nsWeakPtr aWeakEditor);

  nsresult SetEnd(nsINode* aEndNode, PRInt32 aEndOffset);

  // sets the current position, this should be inside the range. If we are in
  // the middle of a word, we'll move to its start.
  nsresult SetPosition(nsINode* aNode, PRInt32 aOffset);

  // Given a point inside or immediately following a word, this returns the
  // DOM range that exactly encloses that word's characters. The current
  // position will be at the end of the word. This will find the previous
  // word if the current position is space, so if you care that the point is
  // inside the word, you should check the range.
  //
  // THIS CHANGES THE CURRENT POSITION AND RANGE. It is designed to be called
  // before you actually generate the range you are interested in and iterate
  // the words in it.
  nsresult GetRangeForWord(nsIDOMNode* aWordNode, PRInt32 aWordOffset,
                           nsRange** aRange);

  // Moves to the the next word in the range, and retrieves it's text and range.
  // An empty word and a NULL range are returned when we are done checking.
  // aSkipChecking will be set if the word is "special" and shouldn't be
  // checked (e.g., an email address).
  nsresult GetNextWord(nsAString& aText, nsRange** aRange,
                       bool* aSkipChecking);

  // Call to normalize some punctuation. This function takes an autostring
  // so we can access characters directly.
  static void NormalizeWord(nsSubstring& aWord);

  nsIDOMDocument* GetDOMDocument() const { return mDOMDocument; }
  nsIDocument* GetDocument() const { return mDocument; }
  nsINode* GetRootNode() { return mRootNode; }
  
private:

  // cached stuff for the editor, set by Init
  nsCOMPtr<nsIDOMDocument> mDOMDocument;
  nsCOMPtr<nsIDocument>         mDocument;

  // range to check, see SetPosition and SetEnd
  nsINode*    mRootNode;
  NodeOffset  mSoftBegin;
  NodeOffset  mSoftEnd;

  // DOM text covering the soft range, with newlines added at block boundaries
  nsString mSoftText;
  // A list of where we extracted text from, ordered by mSoftTextOffset. A given
  // DOM node appears at most once in this list.
  struct DOMTextMapping {
    NodeOffset mNodeOffset;
    PRInt32    mSoftTextOffset;
    PRInt32    mLength;
    
    DOMTextMapping(NodeOffset aNodeOffset, PRInt32 aSoftTextOffset, PRInt32 aLength)
      : mNodeOffset(aNodeOffset), mSoftTextOffset(aSoftTextOffset),
        mLength(aLength) {}
  };
  nsTArray<DOMTextMapping> mSoftTextDOMMapping;
  
  // A list of the "real words" in mSoftText, ordered by mSoftTextOffset
  struct RealWord {
    PRInt32      mSoftTextOffset;
    PRInt32      mLength;
    bool mCheckableWord;
    
    RealWord(PRInt32 aOffset, PRInt32 aLength, bool aCheckable)
      : mSoftTextOffset(aOffset), mLength(aLength), mCheckableWord(aCheckable) {}
    PRInt32 EndOffset() const { return mSoftTextOffset + mLength; }
  };
  nsTArray<RealWord> mRealWords;
  PRInt32            mNextWordIndex;

  bool mSoftTextValid;

  void InvalidateWords() { mSoftTextValid = false; }
  void EnsureWords();
  
  PRInt32 MapDOMPositionToSoftTextOffset(NodeOffset aNodeOffset);
  // Map an offset into mSoftText to a DOM position. Note that two DOM positions
  // can map to the same mSoftText offset, e.g. given nodes A=aaaa and B=bbbb
  // forming aaaabbbb, (A,4) and (B,0) give the same string offset. So,
  // aHintBefore controls which position we return ... if aHint is eEnd
  // then the position indicates the END of a range so we return (A,4). Otherwise
  // the position indicates the START of a range so we return (B,0).
  enum DOMMapHint { HINT_BEGIN, HINT_END };
  NodeOffset MapSoftTextOffsetToDOMPosition(PRInt32 aSoftTextOffset,
                                            DOMMapHint aHint);
  // Finds the index of the real word containing aSoftTextOffset, or -1 if none
  // If it's exactly between two words, then if aHint is HINT_BEGIN, return the
  // later word (favouring the assumption that it's the BEGINning of a word),
  // otherwise return the earlier word (assuming it's the END of a word).
  // If aSearchForward is true, then if we don't find a word at the given
  // position, search forward until we do find a word and return that (if found).
  PRInt32 FindRealWordContaining(PRInt32 aSoftTextOffset, DOMMapHint aHint,
                                 bool aSearchForward);
    
  // build mSoftText and mSoftTextDOMMapping
  void BuildSoftText();
  // Build mRealWords array
  void BuildRealWords();

  void SplitDOMWord(PRInt32 aStart, PRInt32 aEnd);

  // Convenience functions, object must be initialized
  nsresult MakeRange(NodeOffset aBegin, NodeOffset aEnd, nsRange** aRange);
  nsresult MakeRangeForWord(const RealWord& aWord, nsRange** aRange);
};
