/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsPermissionManager_h__
#define nsPermissionManager_h__

#include "nsIPermissionManager.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsTHashtable.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsPermission.h"

class nsIPermission;
class nsIIDNService;
class mozIStorageConnection;
class mozIStorageStatement;

////////////////////////////////////////////////////////////////////////////////

class nsPermissionEntry
{
public:
  nsPermissionEntry(uint32_t aType, uint32_t aPermission, int64_t aID, 
                    uint32_t aExpireType, int64_t aExpireTime)
   : mType(aType)
   , mPermission(aPermission)
   , mID(aID)
   , mExpireType(aExpireType)
   , mExpireTime(aExpireTime) {}

  uint32_t mType;
  uint32_t mPermission;
  int64_t  mID;
  uint32_t mExpireType;
  int64_t  mExpireTime;
};

class nsHostEntry : public PLDHashEntryHdr
{
public:
  // Hash methods
  typedef const char* KeyType;
  typedef const char* KeyTypePointer;

  nsHostEntry(const char* aHost);
  nsHostEntry(const nsHostEntry& toCopy);

  ~nsHostEntry()
  {
  }

  KeyType GetKey() const
  {
    return mHost;
  }

  bool KeyEquals(KeyTypePointer aKey) const
  {
    return !strcmp(mHost, aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return aKey;
  }

  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    // PL_DHashStringKey doesn't use the table parameter, so we can safely
    // pass nullptr
    return PL_DHashStringKey(nullptr, aKey);
  }

  // force the hashtable to use the copy constructor when shuffling entries
  // around, otherwise the Auto part of our nsAutoTArray won't be happy!
  enum { ALLOW_MEMMOVE = false };

  // Permissions methods
  inline const nsDependentCString GetHost() const
  {
    return nsDependentCString(mHost);
  }

  inline nsTArray<nsPermissionEntry> & GetPermissions()
  {
    return mPermissions;
  }

  inline int32_t GetPermissionIndex(uint32_t aType) const
  {
    for (uint32_t i = 0; i < mPermissions.Length(); ++i)
      if (mPermissions[i].mType == aType)
        return i;

    return -1;
  }

  inline nsPermissionEntry GetPermission(uint32_t aType) const
  {
    for (uint32_t i = 0; i < mPermissions.Length(); ++i)
      if (mPermissions[i].mType == aType)
        return mPermissions[i];

    // unknown permission... return relevant data 
    nsPermissionEntry unk = nsPermissionEntry(aType, nsIPermissionManager::UNKNOWN_ACTION,
                                              -1, nsIPermissionManager::EXPIRE_NEVER, 0);
    return unk;
  }

private:
  const char *mHost;
  nsAutoTArray<nsPermissionEntry, 1> mPermissions;
};


class nsPermissionManager : public nsIPermissionManager,
                            public nsIObserver,
                            public nsSupportsWeakReference
{
public:

  // nsISupports
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPERMISSIONMANAGER
  NS_DECL_NSIOBSERVER

  nsPermissionManager();
  virtual ~nsPermissionManager();
  static nsIPermissionManager* GetXPCOMSingleton();
  nsresult Init();

  // enums for AddInternal()
  enum OperationType {
    eOperationNone,
    eOperationAdding,
    eOperationRemoving,
    eOperationChanging
  };

  enum DBOperationType {
    eNoDBOperation,
    eWriteToDB
  };

  enum NotifyOperationType {
    eDontNotify,
    eNotify
  };

  nsresult AddInternal(const nsAFlatCString &aHost,
                       const nsAFlatCString &aType,
                       uint32_t aPermission,
                       int64_t aID,
                       uint32_t aExpireType,
                       int64_t  aExpireTime,
                       NotifyOperationType aNotifyOperation,
                       DBOperationType aDBOperation);

private:

  int32_t GetTypeIndex(const char *aTypeString,
                       bool        aAdd);

  nsHostEntry *GetHostEntry(const nsAFlatCString &aHost,
                            uint32_t              aType,
                            bool                  aExactHostMatch);

  nsresult CommonTestPermission(nsIURI     *aURI,
                                const char *aType,
                                uint32_t   *aPermission,
                                bool        aExactHostMatch);

  nsresult InitDB(bool aRemoveFile);
  nsresult CreateTable();
  nsresult Import();
  nsresult Read();
  void     NotifyObserversWithPermission(const nsACString &aHost,
                                         const nsCString  &aType,
                                         uint32_t          aPermission,
                                         uint32_t          aExpireType,
                                         int64_t           aExpireTime,
                                         const PRUnichar  *aData);
  void     NotifyObservers(nsIPermission *aPermission, const PRUnichar *aData);

  // Finalize all statements, close the DB and null it.
  // if aRebuildOnSuccess, reinitialize database
  void     CloseDB(bool aRebuildOnSuccess = false);

  nsresult RemoveAllInternal(bool aNotifyObservers);
  nsresult RemoveAllFromMemory();
  nsresult NormalizeToACE(nsCString &aHost);
  nsresult GetHost(nsIURI *aURI, nsACString &aResult);
  static void UpdateDB(OperationType         aOp,
                       mozIStorageStatement* aStmt,
                       int64_t               aID,
                       const nsACString     &aHost,
                       const nsACString     &aType,
                       uint32_t              aPermission,
                       uint32_t              aExpireType,
                       int64_t               aExpireTime);

  nsCOMPtr<nsIObserverService> mObserverService;
  nsCOMPtr<nsIIDNService>      mIDNService;

  nsCOMPtr<mozIStorageConnection> mDBConn;
  nsCOMPtr<mozIStorageStatement> mStmtInsert;
  nsCOMPtr<mozIStorageStatement> mStmtDelete;
  nsCOMPtr<mozIStorageStatement> mStmtUpdate;

  nsTHashtable<nsHostEntry>    mHostTable;
  // a unique, monotonically increasing id used to identify each database entry
  int64_t                      mLargestID;

  // An array to store the strings identifying the different types.
  nsTArray<nsCString>          mTypeArray;

  // Initially, |false|. Set to |true| once shutdown has started, to avoid
  // reopening the database.
  bool mIsShuttingDown;

  friend class DeleteFromMozHostListener;
  friend class CloseDatabaseListener;
};

// {4F6B5E00-0C36-11d5-A535-0010A401EB10}
#define NS_PERMISSIONMANAGER_CID \
{ 0x4f6b5e00, 0xc36, 0x11d5, { 0xa5, 0x35, 0x0, 0x10, 0xa4, 0x1, 0xeb, 0x10 } }

#endif /* nsPermissionManager_h__ */
