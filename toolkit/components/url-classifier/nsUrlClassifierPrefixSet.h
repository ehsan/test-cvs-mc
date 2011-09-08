//* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
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
 * The Original Code is Url Classifier code
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Gian-Carlo Pascutto <gpascutto@mozilla.com>
 *   Mehdi Mulani <mars.martian+bugmail@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef nsUrlClassifierPrefixSet_h_
#define nsUrlClassifierPrefixSet_h_

#include "nsISupportsUtils.h"
#include "nsID.h"
#include "nsIUrlClassifierPrefixSet.h"
#include "nsToolkitCompsCID.h"

class nsUrlClassifierPrefixSet : public nsIUrlClassifierPrefixSet
{
public:
  nsUrlClassifierPrefixSet();
  virtual ~nsUrlClassifierPrefixSet() {};

  // Can send an empty Array to clean the tree
  virtual nsresult SetPrefixes(const PRUint32*, PRUint32);
  // Given prefixes must be in sorted order and bigger than
  // anything currently in the Prefix Tree
  virtual nsresult AddPrefixes(const PRUint32*, PRUint32);
  virtual nsresult Contains(PRUint32, PRBool*);
  virtual nsresult EstimateSize(PRUint32*);

  NS_DECL_ISUPPORTS

protected:
  static const PRUint32 DELTAS_LIMIT = 100;
  static const PRUint32 MAX_INDEX_DIFF = (1 << 16);

  PRUint32 BinSearch(PRUint32 start, PRUint32 end, PRUint32 target);

 //  boolean indicating whether |setPrefixes| has been
  // called with a non-empty array.
  PRBool mHasPrefixes;
  // array containing deltas from indices.
  nsTArray<PRUint16> mDeltas;
  // the prefix for each index.
  nsTArray<PRUint32> mIndexPrefixes;
  // the value corresponds to the beginning of the run
  // (an index in |_deltas|) for the index
  nsTArray<PRUint32> mIndexStarts;
};

#endif
