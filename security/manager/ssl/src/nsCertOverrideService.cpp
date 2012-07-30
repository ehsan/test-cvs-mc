/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsCertOverrideService.h"
#include "nsIX509Cert.h"
#include "nsNSSCertificate.h"
#include "nsCRT.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsStreamUtils.h"
#include "nsNetUtil.h"
#include "nsILineInputStream.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsISupportsPrimitives.h"
#include "nsPromiseFlatString.h"
#include "nsThreadUtils.h"
#include "nsStringBuffer.h"
#include "nsAutoPtr.h"
#include "nspr.h"
#include "pk11pub.h"
#include "certdb.h"
#include "sechash.h"
#include "ssl.h" // For SSL_ClearSessionCache

#include "nsNSSCleaner.h"
NSSCleanupAutoPtrClass(CERTCertificate, CERT_DestroyCertificate)

using namespace mozilla;

static const char kCertOverrideFileName[] = "cert_override.txt";

void
nsCertOverride::convertBitsToString(OverrideBits ob, nsACString &str)
{
  str.Truncate();

  if (ob & ob_Mismatch)
    str.Append('M');

  if (ob & ob_Untrusted)
    str.Append('U');

  if (ob & ob_Time_error)
    str.Append('T');
}

void
nsCertOverride::convertStringToBits(const nsACString &str, OverrideBits &ob)
{
  const nsPromiseFlatCString &flat = PromiseFlatCString(str);
  const char *walk = flat.get();

  ob = ob_None;

  for ( ; *walk; ++walk)
  {
    switch (*walk)
    {
      case 'm':
      case 'M':
        ob = (OverrideBits)(ob | ob_Mismatch);
        break;

      case 'u':
      case 'U':
        ob = (OverrideBits)(ob | ob_Untrusted);
        break;

      case 't':
      case 'T':
        ob = (OverrideBits)(ob | ob_Time_error);
        break;

      default:
        break;
    }
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsCertOverrideService, 
                              nsICertOverrideService,
                              nsIObserver,
                              nsISupportsWeakReference)

nsCertOverrideService::nsCertOverrideService()
  : monitor("nsCertOverrideService.monitor")
{
}

nsCertOverrideService::~nsCertOverrideService()
{
}

nsresult
nsCertOverrideService::Init()
{
  if (!NS_IsMainThread()) {
    NS_NOTREACHED("nsCertOverrideService initialized off main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  mSettingsTable.Init();

  mOidTagForStoringNewHashes = SEC_OID_SHA256;

  SECOidData *od = SECOID_FindOIDByTag(mOidTagForStoringNewHashes);
  if (!od)
    return NS_ERROR_FAILURE;

  char *dotted_oid = CERT_GetOidString(&od->oid);
  if (!dotted_oid)
    return NS_ERROR_FAILURE;

  mDottedOidForStoringNewHashes = dotted_oid;
  PR_smprintf_free(dotted_oid);

  nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();

  // If we cannot add ourselves as a profile change observer, then we will not
  // attempt to read/write any settings file. Otherwise, we would end up
  // reading/writing the wrong settings file after a profile change.
  if (observerService) {
    observerService->AddObserver(this, "profile-before-change", true);
    observerService->AddObserver(this, "profile-do-change", true);
    // simulate a profile change so we read the current profile's settings file
    Observe(nullptr, "profile-do-change", nullptr);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCertOverrideService::Observe(nsISupports     *,
                               const char      *aTopic,
                               const PRUnichar *aData)
{
  // check the topic
  if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    // The profile is about to change,
    // or is going away because the application is shutting down.

    ReentrantMonitorAutoEnter lock(monitor);

    if (!nsCRT::strcmp(aData, NS_LITERAL_STRING("shutdown-cleanse").get())) {
      RemoveAllFromMemory();
      // delete the storage file
      if (mSettingsFile) {
        mSettingsFile->Remove(false);
      }
    } else {
      RemoveAllFromMemory();
    }

  } else if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    // The profile has already changed.
    // Now read from the new profile location.
    // we also need to update the cached file location

    ReentrantMonitorAutoEnter lock(monitor);

    nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mSettingsFile));
    if (NS_SUCCEEDED(rv)) {
      mSettingsFile->AppendNative(NS_LITERAL_CSTRING(kCertOverrideFileName));
    } else {
      mSettingsFile = nullptr;
    }
    Read();

  }

  return NS_OK;
}

void
nsCertOverrideService::RemoveAllFromMemory()
{
  ReentrantMonitorAutoEnter lock(monitor);
  mSettingsTable.Clear();
}

PR_STATIC_CALLBACK(PLDHashOperator)
RemoveTemporariesCallback(nsCertOverrideEntry *aEntry,
                          void *aArg)
{
  if (aEntry && aEntry->mSettings.mIsTemporary) {
    aEntry->mSettings.mCert = nullptr;
    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}

void
nsCertOverrideService::RemoveAllTemporaryOverrides()
{
  {
    ReentrantMonitorAutoEnter lock(monitor);
    mSettingsTable.EnumerateEntries(RemoveTemporariesCallback, nullptr);
    // no need to write, as temporaries are never written to disk
  }
}

nsresult
nsCertOverrideService::Read()
{
  ReentrantMonitorAutoEnter lock(monitor);

  // If we don't have a profile, then we won't try to read any settings file.
  if (!mSettingsFile)
    return NS_OK;

  nsresult rv;
  nsCOMPtr<nsIInputStream> fileInputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), mSettingsFile);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsILineInputStream> lineInputStream = do_QueryInterface(fileInputStream, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCAutoString buffer;
  bool isMore = true;
  PRInt32 hostIndex = 0, algoIndex, fingerprintIndex, overrideBitsIndex, dbKeyIndex;

  /* file format is:
   *
   * host:port \t fingerprint-algorithm \t fingerprint \t override-mask \t dbKey
   *
   *   where override-mask is a sequence of characters,
   *     M meaning hostname-Mismatch-override
   *     U meaning Untrusted-override
   *     T meaning Time-error-override (expired/not yet valid) 
   *
   * if this format isn't respected we move onto the next line in the file.
   */

  while (isMore && NS_SUCCEEDED(lineInputStream->ReadLine(buffer, &isMore))) {
    if (buffer.IsEmpty() || buffer.First() == '#') {
      continue;
    }

    // this is a cheap, cheesy way of parsing a tab-delimited line into
    // string indexes, which can be lopped off into substrings. just for
    // purposes of obfuscation, it also checks that each token was found.
    // todo: use iterators?
    if ((algoIndex         = buffer.FindChar('\t', hostIndex)         + 1) == 0 ||
        (fingerprintIndex  = buffer.FindChar('\t', algoIndex)         + 1) == 0 ||
        (overrideBitsIndex = buffer.FindChar('\t', fingerprintIndex)  + 1) == 0 ||
        (dbKeyIndex        = buffer.FindChar('\t', overrideBitsIndex) + 1) == 0) {
      continue;
    }

    const nsASingleFragmentCString &tmp = Substring(buffer, hostIndex, algoIndex - hostIndex - 1);
    const nsASingleFragmentCString &algo_string = Substring(buffer, algoIndex, fingerprintIndex - algoIndex - 1);
    const nsASingleFragmentCString &fingerprint = Substring(buffer, fingerprintIndex, overrideBitsIndex - fingerprintIndex - 1);
    const nsASingleFragmentCString &bits_string = Substring(buffer, overrideBitsIndex, dbKeyIndex - overrideBitsIndex - 1);
    const nsASingleFragmentCString &db_key = Substring(buffer, dbKeyIndex, buffer.Length() - dbKeyIndex);

    nsCAutoString host(tmp);
    nsCertOverride::OverrideBits bits;
    nsCertOverride::convertStringToBits(bits_string, bits);

    PRInt32 port;
    PRInt32 portIndex = host.RFindChar(':');
    if (portIndex == kNotFound)
      continue; // Ignore broken entries

    nsresult portParseError;
    nsCAutoString portString(Substring(host, portIndex+1));
    port = portString.ToInteger(&portParseError);
    if (portParseError)
      continue; // Ignore broken entries

    host.Truncate(portIndex);
    
    AddEntryToList(host, port, 
                   nullptr, // don't have the cert
                   false, // not temporary
                   algo_string, fingerprint, bits, db_key);
  }

  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
WriteEntryCallback(nsCertOverrideEntry *aEntry,
                   void *aArg)
{
  static const char kTab[] = "\t";

  nsIOutputStream *rawStreamPtr = (nsIOutputStream *)aArg;

  PRUint32 unused;

  if (rawStreamPtr && aEntry)
  {
    const nsCertOverride &settings = aEntry->mSettings;
    if (settings.mIsTemporary)
      return PL_DHASH_NEXT;

    nsCAutoString bits_string;
    nsCertOverride::convertBitsToString(settings.mOverrideBits, 
                                            bits_string);

    rawStreamPtr->Write(aEntry->mHostWithPort.get(), aEntry->mHostWithPort.Length(), &unused);
    rawStreamPtr->Write(kTab, sizeof(kTab) - 1, &unused);
    rawStreamPtr->Write(settings.mFingerprintAlgOID.get(), 
                        settings.mFingerprintAlgOID.Length(), &unused);
    rawStreamPtr->Write(kTab, sizeof(kTab) - 1, &unused);
    rawStreamPtr->Write(settings.mFingerprint.get(), 
                        settings.mFingerprint.Length(), &unused);
    rawStreamPtr->Write(kTab, sizeof(kTab) - 1, &unused);
    rawStreamPtr->Write(bits_string.get(), 
                        bits_string.Length(), &unused);
    rawStreamPtr->Write(kTab, sizeof(kTab) - 1, &unused);
    rawStreamPtr->Write(settings.mDBKey.get(), settings.mDBKey.Length(), &unused);
    rawStreamPtr->Write(NS_LINEBREAK, NS_LINEBREAK_LEN, &unused);
  }

  return PL_DHASH_NEXT;
}

nsresult
nsCertOverrideService::Write()
{
  ReentrantMonitorAutoEnter lock(monitor);

  // If we don't have any profile, then we won't try to write any file
  if (!mSettingsFile) {
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIOutputStream> fileOutputStream;
  rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(fileOutputStream),
                                       mSettingsFile,
                                       -1,
                                       0600);
  if (NS_FAILED(rv)) {
    NS_ERROR("failed to open cert_warn_settings.txt for writing");
    return rv;
  }

  // get a buffered output stream 4096 bytes big, to optimize writes
  nsCOMPtr<nsIOutputStream> bufferedOutputStream;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOutputStream), fileOutputStream, 4096);
  if (NS_FAILED(rv)) {
    return rv;
  }

  static const char kHeader[] =
      "# PSM Certificate Override Settings file" NS_LINEBREAK
      "# This is a generated file!  Do not edit." NS_LINEBREAK;

  /* see ::Read for file format */

  PRUint32 unused;
  bufferedOutputStream->Write(kHeader, sizeof(kHeader) - 1, &unused);

  nsIOutputStream *rawStreamPtr = bufferedOutputStream;
  mSettingsTable.EnumerateEntries(WriteEntryCallback, rawStreamPtr);

  // All went ok. Maybe except for problems in Write(), but the stream detects
  // that for us
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(bufferedOutputStream);
  NS_ASSERTION(safeStream, "expected a safe output stream!");
  if (safeStream) {
    rv = safeStream->Finish();
    if (NS_FAILED(rv)) {
      NS_WARNING("failed to save cert warn settings file! possible dataloss");
      return rv;
    }
  }

  return NS_OK;
}

static nsresult
GetCertFingerprintByOidTag(CERTCertificate* nsscert,
                           SECOidTag aOidTag, 
                           nsCString &fp)
{
  unsigned int hash_len = HASH_ResultLenByOidTag(aOidTag);
  nsStringBuffer* fingerprint = nsStringBuffer::Alloc(hash_len);
  if (!fingerprint)
    return NS_ERROR_OUT_OF_MEMORY;

  PK11_HashBuf(aOidTag, (unsigned char*)fingerprint->Data(), 
               nsscert->derCert.data, nsscert->derCert.len);

  SECItem fpItem;
  fpItem.data = (unsigned char*)fingerprint->Data();
  fpItem.len = hash_len;

  char *tmpstr = CERT_Hexify(&fpItem, 1);
  fp.Assign(tmpstr);
  PORT_Free(tmpstr);
  fingerprint->Release();
  return NS_OK;
}

static nsresult
GetCertFingerprintByOidTag(nsIX509Cert *aCert,
                           SECOidTag aOidTag, 
                           nsCString &fp)
{
  nsCOMPtr<nsIX509Cert2> cert2 = do_QueryInterface(aCert);
  if (!cert2)
    return NS_ERROR_FAILURE;

  CERTCertificate* nsscert = cert2->GetCert();
  if (!nsscert)
    return NS_ERROR_FAILURE;

  CERTCertificateCleaner nsscertCleaner(nsscert);
  return GetCertFingerprintByOidTag(nsscert, aOidTag, fp);
}

static nsresult
GetCertFingerprintByDottedOidString(CERTCertificate* nsscert,
                                    const nsCString &dottedOid, 
                                    nsCString &fp)
{
  SECItem oid;
  oid.data = nullptr;
  oid.len = 0;
  SECStatus srv = SEC_StringToOID(nullptr, &oid, 
                    dottedOid.get(), dottedOid.Length());
  if (srv != SECSuccess)
    return NS_ERROR_FAILURE;

  SECOidTag oid_tag = SECOID_FindOIDTag(&oid);
  SECITEM_FreeItem(&oid, false);

  if (oid_tag == SEC_OID_UNKNOWN)
    return NS_ERROR_FAILURE;

  return GetCertFingerprintByOidTag(nsscert, oid_tag, fp);
}

static nsresult
GetCertFingerprintByDottedOidString(nsIX509Cert *aCert,
                                    const nsCString &dottedOid, 
                                    nsCString &fp)
{
  nsCOMPtr<nsIX509Cert2> cert2 = do_QueryInterface(aCert);
  if (!cert2)
    return NS_ERROR_FAILURE;

  CERTCertificate* nsscert = cert2->GetCert();
  if (!nsscert)
    return NS_ERROR_FAILURE;

  CERTCertificateCleaner nsscertCleaner(nsscert);
  return GetCertFingerprintByDottedOidString(nsscert, dottedOid, fp);
}

NS_IMETHODIMP
nsCertOverrideService::RememberValidityOverride(const nsACString & aHostName, PRInt32 aPort, 
                                                nsIX509Cert *aCert,
                                                PRUint32 aOverrideBits, 
                                                bool aTemporary)
{
  NS_ENSURE_ARG_POINTER(aCert);
  if (aHostName.IsEmpty())
    return NS_ERROR_INVALID_ARG;
  if (aPort < -1)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIX509Cert2> cert2 = do_QueryInterface(aCert);
  if (!cert2)
    return NS_ERROR_FAILURE;

  CERTCertificate* nsscert = cert2->GetCert();
  if (!nsscert)
    return NS_ERROR_FAILURE;

  CERTCertificateCleaner nsscertCleaner(nsscert);

  char* nickname = nsNSSCertificate::defaultServerNickname(nsscert);
  if (!aTemporary && nickname && *nickname)
  {
    PK11SlotInfo *slot = PK11_GetInternalKeySlot();
    if (!slot) {
      PR_Free(nickname);
      return NS_ERROR_FAILURE;
    }
  
    SECStatus srv = PK11_ImportCert(slot, nsscert, CK_INVALID_HANDLE, 
                                    nickname, false);
    PK11_FreeSlot(slot);
  
    if (srv != SECSuccess) {
      PR_Free(nickname);
      return NS_ERROR_FAILURE;
    }
  }
  PR_FREEIF(nickname);

  nsCAutoString fpStr;
  nsresult rv = GetCertFingerprintByOidTag(nsscert, 
                  mOidTagForStoringNewHashes, fpStr);
  if (NS_FAILED(rv))
    return rv;

  char *dbkey = NULL;
  rv = aCert->GetDbKey(&dbkey);
  if (NS_FAILED(rv) || !dbkey)
    return rv;

  // change \n and \r to spaces in the possibly multi-line-base64-encoded key
  for (char *dbkey_walk = dbkey;
       *dbkey_walk;
      ++dbkey_walk) {
    char c = *dbkey_walk;
    if (c == '\r' || c == '\n') {
      *dbkey_walk = ' ';
    }
  }

  {
    ReentrantMonitorAutoEnter lock(monitor);
    AddEntryToList(aHostName, aPort,
                   aTemporary ? aCert : nullptr,
                     // keep a reference to the cert for temporary overrides
                   aTemporary, 
                   mDottedOidForStoringNewHashes, fpStr, 
                   (nsCertOverride::OverrideBits)aOverrideBits, 
                   nsDependentCString(dbkey));
    Write();
  }

  PR_Free(dbkey);
  return NS_OK;
}

NS_IMETHODIMP
nsCertOverrideService::HasMatchingOverride(const nsACString & aHostName, PRInt32 aPort,
                                           nsIX509Cert *aCert, 
                                           PRUint32 *aOverrideBits,
                                           bool *aIsTemporary,
                                           bool *_retval)
{
  if (aHostName.IsEmpty())
    return NS_ERROR_INVALID_ARG;
  if (aPort < -1)
    return NS_ERROR_INVALID_ARG;

  NS_ENSURE_ARG_POINTER(aCert);
  NS_ENSURE_ARG_POINTER(aOverrideBits);
  NS_ENSURE_ARG_POINTER(aIsTemporary);
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = false;
  *aOverrideBits = nsCertOverride::ob_None;

  nsCAutoString hostPort;
  GetHostWithPort(aHostName, aPort, hostPort);
  nsCertOverride settings;

  {
    ReentrantMonitorAutoEnter lock(monitor);
    nsCertOverrideEntry *entry = mSettingsTable.GetEntry(hostPort.get());
  
    if (!entry)
      return NS_OK;
  
    settings = entry->mSettings; // copy
  }

  *aOverrideBits = settings.mOverrideBits;
  *aIsTemporary = settings.mIsTemporary;

  nsCAutoString fpStr;
  nsresult rv;

  if (settings.mFingerprintAlgOID.Equals(mDottedOidForStoringNewHashes)) {
    rv = GetCertFingerprintByOidTag(aCert, mOidTagForStoringNewHashes, fpStr);
  }
  else {
    rv = GetCertFingerprintByDottedOidString(aCert, settings.mFingerprintAlgOID, fpStr);
  }
  if (NS_FAILED(rv))
    return rv;

  *_retval = settings.mFingerprint.Equals(fpStr);
  return NS_OK;
}

NS_IMETHODIMP
nsCertOverrideService::GetValidityOverride(const nsACString & aHostName, PRInt32 aPort,
                                           nsACString & aHashAlg, 
                                           nsACString & aFingerprint, 
                                           PRUint32 *aOverrideBits,
                                           bool *aIsTemporary,
                                           bool *_found)
{
  NS_ENSURE_ARG_POINTER(_found);
  NS_ENSURE_ARG_POINTER(aIsTemporary);
  NS_ENSURE_ARG_POINTER(aOverrideBits);
  *_found = false;
  *aOverrideBits = nsCertOverride::ob_None;

  nsCAutoString hostPort;
  GetHostWithPort(aHostName, aPort, hostPort);
  nsCertOverride settings;

  {
    ReentrantMonitorAutoEnter lock(monitor);
    nsCertOverrideEntry *entry = mSettingsTable.GetEntry(hostPort.get());
  
    if (entry) {
      *_found = true;
      settings = entry->mSettings; // copy
    }
  }

  if (*_found) {
    *aOverrideBits = settings.mOverrideBits;
    *aIsTemporary = settings.mIsTemporary;
    aFingerprint = settings.mFingerprint;
    aHashAlg = settings.mFingerprintAlgOID;
  }

  return NS_OK;
}

nsresult
nsCertOverrideService::AddEntryToList(const nsACString &aHostName, PRInt32 aPort,
                                      nsIX509Cert *aCert,
                                      const bool aIsTemporary,
                                      const nsACString &fingerprintAlgOID, 
                                      const nsACString &fingerprint,
                                      nsCertOverride::OverrideBits ob,
                                      const nsACString &dbKey)
{
  nsCAutoString hostPort;
  GetHostWithPort(aHostName, aPort, hostPort);

  {
    ReentrantMonitorAutoEnter lock(monitor);
    nsCertOverrideEntry *entry = mSettingsTable.PutEntry(hostPort.get());

    if (!entry) {
      NS_ERROR("can't insert a null entry!");
      return NS_ERROR_OUT_OF_MEMORY;
    }

    entry->mHostWithPort = hostPort;

    nsCertOverride &settings = entry->mSettings;
    settings.mAsciiHost = aHostName;
    settings.mPort = aPort;
    settings.mIsTemporary = aIsTemporary;
    settings.mFingerprintAlgOID = fingerprintAlgOID;
    settings.mFingerprint = fingerprint;
    settings.mOverrideBits = ob;
    settings.mDBKey = dbKey;
    settings.mCert = aCert;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCertOverrideService::ClearValidityOverride(const nsACString & aHostName, PRInt32 aPort)
{
  if (aPort == 0 &&
      aHostName.EqualsLiteral("all:temporary-certificates")) {
    RemoveAllTemporaryOverrides();
    return NS_OK;
  }
  nsCAutoString hostPort;
  GetHostWithPort(aHostName, aPort, hostPort);
  {
    ReentrantMonitorAutoEnter lock(monitor);
    mSettingsTable.RemoveEntry(hostPort.get());
    Write();
  }
  SSL_ClearSessionCache();
  return NS_OK;
}

NS_IMETHODIMP
nsCertOverrideService::GetAllOverrideHostsWithPorts(PRUint32 *aCount, 
                                                        PRUnichar ***aHostsWithPortsArray)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

static bool
matchesDBKey(nsIX509Cert *cert, const char *match_dbkey)
{
  char *dbkey = NULL;
  nsresult rv = cert->GetDbKey(&dbkey);
  if (NS_FAILED(rv) || !dbkey)
    return false;

  bool found_mismatch = false;
  const char *key1 = dbkey;
  const char *key2 = match_dbkey;

  // skip over any whitespace when comparing
  while (*key1 && *key2) {
    char c1 = *key1;
    char c2 = *key2;
    
    switch (c1) {
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        ++key1;
        continue;
    }

    switch (c2) {
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        ++key2;
        continue;
    }

    if (c1 != c2) {
      found_mismatch = true;
      break;
    }

    ++key1;
    ++key2;
  }

  PR_Free(dbkey);
  return !found_mismatch;
}

struct nsCertAndBoolsAndInt
{
  nsIX509Cert *cert;
  bool aCheckTemporaries;
  bool aCheckPermanents;
  PRUint32 counter;

  SECOidTag mOidTagForStoringNewHashes;
  nsCString mDottedOidForStoringNewHashes;
};

PR_STATIC_CALLBACK(PLDHashOperator)
FindMatchingCertCallback(nsCertOverrideEntry *aEntry,
                         void *aArg)
{
  nsCertAndBoolsAndInt *cai = (nsCertAndBoolsAndInt *)aArg;

  if (cai && aEntry)
  {
    const nsCertOverride &settings = aEntry->mSettings;
    bool still_ok = true;

    if ((settings.mIsTemporary && !cai->aCheckTemporaries)
        ||
        (!settings.mIsTemporary && !cai->aCheckPermanents)) {
      still_ok = false;
    }

    if (still_ok && matchesDBKey(cai->cert, settings.mDBKey.get())) {
      nsCAutoString cert_fingerprint;
      nsresult rv;
      if (settings.mFingerprintAlgOID.Equals(cai->mDottedOidForStoringNewHashes)) {
        rv = GetCertFingerprintByOidTag(cai->cert,
               cai->mOidTagForStoringNewHashes, cert_fingerprint);
      }
      else {
        rv = GetCertFingerprintByDottedOidString(cai->cert,
               settings.mFingerprintAlgOID, cert_fingerprint);
      }
      if (NS_SUCCEEDED(rv) &&
          settings.mFingerprint.Equals(cert_fingerprint)) {
        cai->counter++;
      }
    }
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsCertOverrideService::IsCertUsedForOverrides(nsIX509Cert *aCert, 
                                              bool aCheckTemporaries,
                                              bool aCheckPermanents,
                                              PRUint32 *_retval)
{
  NS_ENSURE_ARG(aCert);
  NS_ENSURE_ARG(_retval);

  nsCertAndBoolsAndInt cai;
  cai.cert = aCert;
  cai.aCheckTemporaries = aCheckTemporaries;
  cai.aCheckPermanents = aCheckPermanents;
  cai.counter = 0;
  cai.mOidTagForStoringNewHashes = mOidTagForStoringNewHashes;
  cai.mDottedOidForStoringNewHashes = mDottedOidForStoringNewHashes;

  {
    ReentrantMonitorAutoEnter lock(monitor);
    mSettingsTable.EnumerateEntries(FindMatchingCertCallback, &cai);
  }
  *_retval = cai.counter;
  return NS_OK;
}

struct nsCertAndPointerAndCallback
{
  nsIX509Cert *cert;
  void *userdata;
  nsCertOverrideService::CertOverrideEnumerator enumerator;

  SECOidTag mOidTagForStoringNewHashes;
  nsCString mDottedOidForStoringNewHashes;
};

PR_STATIC_CALLBACK(PLDHashOperator)
EnumerateCertOverridesCallback(nsCertOverrideEntry *aEntry,
                               void *aArg)
{
  nsCertAndPointerAndCallback *capac = (nsCertAndPointerAndCallback *)aArg;

  if (capac && aEntry)
  {
    const nsCertOverride &settings = aEntry->mSettings;

    if (!capac->cert) {
      (*capac->enumerator)(settings, capac->userdata);
    }
    else {
      if (matchesDBKey(capac->cert, settings.mDBKey.get())) {
        nsCAutoString cert_fingerprint;
        nsresult rv;
        if (settings.mFingerprintAlgOID.Equals(capac->mDottedOidForStoringNewHashes)) {
          rv = GetCertFingerprintByOidTag(capac->cert,
                 capac->mOidTagForStoringNewHashes, cert_fingerprint);
        }
        else {
          rv = GetCertFingerprintByDottedOidString(capac->cert,
                 settings.mFingerprintAlgOID, cert_fingerprint);
        }
        if (NS_SUCCEEDED(rv) &&
            settings.mFingerprint.Equals(cert_fingerprint)) {
          (*capac->enumerator)(settings, capac->userdata);
        }
      }
    }
  }

  return PL_DHASH_NEXT;
}

nsresult 
nsCertOverrideService::EnumerateCertOverrides(nsIX509Cert *aCert,
                         CertOverrideEnumerator enumerator,
                         void *aUserData)
{
  nsCertAndPointerAndCallback capac;
  capac.cert = aCert;
  capac.userdata = aUserData;
  capac.enumerator = enumerator;
  capac.mOidTagForStoringNewHashes = mOidTagForStoringNewHashes;
  capac.mDottedOidForStoringNewHashes = mDottedOidForStoringNewHashes;

  {
    ReentrantMonitorAutoEnter lock(monitor);
    mSettingsTable.EnumerateEntries(EnumerateCertOverridesCallback, &capac);
  }
  return NS_OK;
}

void
nsCertOverrideService::GetHostWithPort(const nsACString & aHostName, PRInt32 aPort, nsACString& _retval)
{
  nsCAutoString hostPort(aHostName);
  if (aPort == -1) {
    aPort = 443;
  }
  if (!hostPort.IsEmpty()) {
    hostPort.AppendLiteral(":");
    hostPort.AppendInt(aPort);
  }
  _retval.Assign(hostPort);
}

