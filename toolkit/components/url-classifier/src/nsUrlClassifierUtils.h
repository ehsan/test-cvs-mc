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
 * Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef nsUrlClassifierUtils_h_
#define nsUrlClassifierUtils_h_

#include "nsAutoPtr.h"
#include "nsIUrlClassifierUtils.h"
#include "nsTArray.h"
#include "nsDataHashtable.h"

class nsUrlClassifierUtils : public nsIUrlClassifierUtils
{
private:
  /**
   * A fast, bit-vector map for ascii characters.
   *
   * Internally stores 256 bits in an array of 8 ints.
   * Does quick bit-flicking to lookup needed characters.
   */
  class Charmap
  {
  public:
    Charmap(PRUint32 b0, PRUint32 b1, PRUint32 b2, PRUint32 b3,
            PRUint32 b4, PRUint32 b5, PRUint32 b6, PRUint32 b7)
    {
      mMap[0] = b0; mMap[1] = b1; mMap[2] = b2; mMap[3] = b3;
      mMap[4] = b4; mMap[5] = b5; mMap[6] = b6; mMap[7] = b7;
    }

    /**
     * Do a quick lookup to see if the letter is in the map.
     */
    PRBool Contains(unsigned char c) const
    {
      return mMap[c >> 5] & (1 << (c & 31));
    }

  private:
    // Store the 256 bits in an 8 byte array.
    PRUint32 mMap[8];
  };


public:
  nsUrlClassifierUtils();
  ~nsUrlClassifierUtils() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERUTILS

  nsresult Init();

  nsresult CanonicalizeHostname(const nsACString & hostname,
                                nsACString & _retval);
  nsresult CanonicalizePath(const nsACString & url, nsACString & _retval);

  // This function will encode all "special" characters in typical url encoding,
  // that is %hh where h is a valid hex digit.  The characters which are encoded
  // by this function are any ascii characters under 32(control characters and
  // space), 37(%), and anything 127 or above (special characters).  Url is the
  // string to encode, ret is the encoded string.  Function returns true if
  // ret != url.
  PRBool SpecialEncode(const nsACString & url,
                       PRBool foldSlashes,
                       nsACString & _retval);

  void ParseIPAddress(const nsACString & host, nsACString & _retval);
  void CanonicalNum(const nsACString & num,
                    PRUint32 bytes,
                    PRBool allowOctal,
                    nsACString & _retval);

  // Convert an urlsafe base64 string to a normal base64 string.
  // This method will leave an already-normal base64 string alone.
  static void UnUrlsafeBase64(nsACString & str);

  // Takes an urlsafe-base64 encoded client key and gives back binary
  // key data
  static nsresult DecodeClientKey(const nsACString & clientKey,
                                  nsACString & _retval);
private:
  // Disallow copy constructor
  nsUrlClassifierUtils(const nsUrlClassifierUtils&);

  // Function to tell if we should encode a character.
  PRBool ShouldURLEscape(const unsigned char c) const;

  void CleanupHostname(const nsACString & host, nsACString & _retval);

  nsAutoPtr<Charmap> mEscapeCharmap;
};

// An MRU list of fragments.  This is used by the DB service to
// keep a set of known-clean fragments that don't need a database
// lookup.
class nsUrlClassifierFragmentSet
{
public:
  nsUrlClassifierFragmentSet() : mFirst(nsnull), mLast(nsnull), mCapacity(16) {}

  PRBool Init(PRUint32 maxEntries) {
    mCapacity = maxEntries;
    if (!mEntryStorage.SetCapacity(mCapacity))
      return PR_FALSE;

    if (!mEntries.Init())
      return PR_FALSE;

    return PR_TRUE;
  }

  PRBool Put(const nsACString &fragment) {
    Entry *entry;
    if (mEntries.Get(fragment, &entry)) {
      // Remove this entry from the list, we'll add it back
      // to the front.
      UnlinkEntry(entry);
    } else {
      if (mEntryStorage.Length() < mEntryStorage.Capacity()) {
        entry = mEntryStorage.AppendElement();
        if (!entry)
          return PR_FALSE;
      } else {
        // Reuse the oldest entry.
        entry = mLast;
        UnlinkEntry(entry);
        mEntries.Remove(entry->mFragment);
      }
      entry->mFragment = fragment;
      mEntries.Put(fragment, entry);
    }

    // Add the entry to the front of the list
    entry->mPrev = nsnull;
    entry->mNext = mFirst;
    mFirst = entry;
    if (!mLast) {
      mLast = entry;
    }

    return PR_TRUE;
  }

  PRBool Has(const nsACString &fragment) {
    return mEntries.Get(fragment, nsnull);
  }

  void Clear() {
    mFirst = mLast = nsnull;
    mEntries.Clear();
    mEntryStorage.Clear();
    mEntryStorage.SetCapacity(mCapacity);
  }

private:
  // One entry in the set.  We maintain a doubly-linked list, with
  // the most recently used entry at the front.
  class Entry {
  public:
    Entry() : mNext(nsnull), mPrev(nsnull) {};
    ~Entry() { }

    Entry *mNext;
    Entry *mPrev;
    nsCString mFragment;
  };

  void UnlinkEntry(Entry *entry)
  {
    if (entry->mPrev)
      entry->mPrev->mNext = entry->mNext;
    else
      mFirst = entry->mNext;

    if (entry->mNext)
      entry->mNext->mPrev = entry->mPrev;
    else
      mLast = entry->mPrev;

    entry->mPrev = entry->mNext = nsnull;
  }

  // The newest entry in the cache.
  Entry *mFirst;
  // The oldest entry in the cache.
  Entry *mLast;

  // Max entries in the cache.
  PRUint32 mCapacity;

  // Storage for the entries in this set.
  nsTArray<Entry> mEntryStorage;

  // Entry lookup by fragment.
  nsDataHashtable<nsCStringHashKey, Entry*> mEntries;
};

#endif // nsUrlClassifierUtils_h_
