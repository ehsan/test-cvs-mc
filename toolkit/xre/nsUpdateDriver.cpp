/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include <stdio.h>
#include "nsUpdateDriver.h"
#include "nsXULAppAPI.h"
#include "nsAppRunner.h"
#include "nsILocalFile.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "prproces.h"
#include "prlog.h"
#include "prenv.h"
#include "nsVersionComparator.h"

#ifdef XP_MACOSX
#include "nsILocalFileMac.h"
#include "nsCommandLineServiceMac.h"
#include "MacLaunchHelper.h"
#endif

#if defined(XP_WIN)
# include <direct.h>
# include <process.h>
# include <windows.h>
# define getcwd(path, size) _getcwd(path, size)
# define getpid() GetCurrentProcessId()
#elif defined(XP_OS2)
# include <unistd.h>
# define INCL_DOSFILEMGR
# include <os2.h>
#elif defined(XP_UNIX)
# include <unistd.h>
#endif

//
// We use execv to spawn the updater process on all UNIX systems except Mac OSX
// since it is known to cause problems on the Mac.  Windows has execv, but it
// is a faked implementation that doesn't really replace the current process.
// Instead it spawns a new process, so we gain nothing from using execv on
// Windows.
//
// On platforms where we are not calling execv, we may need to make the
// updater executable wait for the calling process to exit.  Otherwise, the
// updater may have trouble modifying our executable image (because it might
// still be in use).  This is accomplished by passing our PID to the updater so
// that it can wait for us to exit.  This is not perfect as there is a race
// condition that could bite us.  It's possible that the calling process could
// exit before the updater waits on the specified PID, and in the meantime a
// new process with the same PID could be created.  This situation is unlikely,
// however, given the way most operating systems recycle PIDs.  We'll take our
// chances ;-)
//
// A similar #define lives in updater.cpp and should be kept in sync with this.
//
#if defined(XP_UNIX) && !defined(XP_MACOSX)
#define USE_EXECV
#endif

#ifdef PR_LOGGING
static PRLogModuleInfo *sUpdateLog = PR_NewLogModule("updatedriver");
#endif
#define LOG(args) PR_LOG(sUpdateLog, PR_LOG_DEBUG, args)

#ifdef XP_WIN
static const char kUpdaterBin[] = "updater.exe";
#else
static const char kUpdaterBin[] = "updater";
#endif
static const char kUpdaterINI[] = "updater.ini";
#ifdef XP_MACOSX
static const char kUpdaterApp[] = "updater.app";
#endif
#if defined(XP_UNIX) && !defined(XP_MACOSX)
static const char kUpdaterPNG[] = "updater.png";
#endif

static nsresult
GetCurrentWorkingDir(char *buf, size_t size)
{
  // Cannot use NS_GetSpecialDirectory because XPCOM is not yet initialized.
  // This code is duplicated from xpcom/io/SpecialSystemDirectory.cpp:

#if defined(XP_OS2)
  if (DosQueryPathInfo( ".", FIL_QUERYFULLNAME, buf, size))
    return NS_ERROR_FAILURE;
#elif defined(XP_WIN)
  wchar_t wpath[MAX_PATH];
  if (!_wgetcwd(wpath, size))
    return NS_ERROR_FAILURE;
  NS_ConvertUTF16toUTF8 path(wpath);
  strncpy(buf, path.get(), size);
#else
  if(!getcwd(buf, size))
    return NS_ERROR_FAILURE;
#endif
  return NS_OK;
}

#if defined(XP_MACOSX)
// This is a copy of OS X's XRE_GetBinaryPath from nsAppRunner.cpp with the
// gBinaryPath check removed so that the updater can reload the stub executable
// instead of xulrunner-bin. See bug 349737.
static nsresult
GetXULRunnerStubPath(const char* argv0, nsILocalFile* *aResult)
{
  // Works even if we're not bundled.
  CFBundleRef appBundle = ::CFBundleGetMainBundle();
  if (!appBundle)
    return NS_ERROR_FAILURE;

  CFURLRef bundleURL = ::CFBundleCopyExecutableURL(appBundle);
  if (!bundleURL)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsILocalFileMac> lfm;
  nsresult rv = NS_NewLocalFileWithCFURL(bundleURL, true, getter_AddRefs(lfm));

  ::CFRelease(bundleURL);

  if (NS_FAILED(rv))
    return rv;

  NS_ADDREF(*aResult = static_cast<nsILocalFile*>(lfm.get()));
  return NS_OK;
}
#endif /* XP_MACOSX */

static bool
GetFile(nsIFile *dir, const nsCSubstring &name, nsCOMPtr<nsILocalFile> &result)
{
  nsresult rv;
  
  nsCOMPtr<nsIFile> file;
  rv = dir->Clone(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return false;

  rv = file->AppendNative(name);
  if (NS_FAILED(rv))
    return false;

  result = do_QueryInterface(file, &rv);
  return NS_SUCCEEDED(rv);
}

static bool
GetStatusFile(nsIFile *dir, nsCOMPtr<nsILocalFile> &result)
{
  return GetFile(dir, NS_LITERAL_CSTRING("update.status"), result);
}

static bool
IsPending(nsILocalFile *statusFile)
{
  PRFileDesc *fd = nsnull;
  nsresult rv = statusFile->OpenNSPRFileDesc(PR_RDONLY, 0660, &fd);
  if (NS_FAILED(rv))
    return false;

  char buf[32];
  const PRInt32 n = PR_Read(fd, buf, sizeof(buf));
  PR_Close(fd);

  if (n < 0)
    return false;
  
  const char kPending[] = "pending";
  bool isPending = (strncmp(buf, kPending, sizeof(kPending) - 1) == 0);
  return isPending;
}

static bool
GetVersionFile(nsIFile *dir, nsCOMPtr<nsILocalFile> &result)
{
  return GetFile(dir, NS_LITERAL_CSTRING("update.version"), result);
}

// Compares the current application version with the update's application
// version.
static bool
IsOlderVersion(nsILocalFile *versionFile, const char *appVersion)
{
  PRFileDesc *fd = nsnull;
  nsresult rv = versionFile->OpenNSPRFileDesc(PR_RDONLY, 0660, &fd);
  if (NS_FAILED(rv))
    return true;

  char buf[32];
  const PRInt32 n = PR_Read(fd, buf, sizeof(buf));
  PR_Close(fd);

  if (n < 0)
    return false;

  // Trim off the trailing newline
  if (buf[n - 1] == '\n')
    buf[n - 1] = '\0';

  // If the update xml doesn't provide the application version the file will
  // contain the string "null" and it is assumed that the update is not older.
  const char kNull[] = "null";
  if (strncmp(buf, kNull, sizeof(kNull) - 1) == 0)
    return false;

  if (mozilla::Version(appVersion) > buf)
    return true;

  return false;
}

static bool
CopyFileIntoUpdateDir(nsIFile *parentDir, const char *leafName, nsIFile *updateDir)
{
  nsDependentCString leaf(leafName);
  nsCOMPtr<nsIFile> file;

  // Make sure there is not an existing file in the target location.
  nsresult rv = updateDir->Clone(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return false;
  rv = file->AppendNative(leaf);
  if (NS_FAILED(rv))
    return false;
  file->Remove(false);

  // Now, copy into the target location.
  rv = parentDir->Clone(getter_AddRefs(file));
  if (NS_FAILED(rv))
    return false;
  rv = file->AppendNative(leaf);
  if (NS_FAILED(rv))
    return false;
  rv = file->CopyToNative(updateDir, EmptyCString());
  if (NS_FAILED(rv))
    return false;

  return true;
}

static bool
CopyUpdaterIntoUpdateDir(nsIFile *greDir, nsIFile *appDir, nsIFile *updateDir,
                         nsCOMPtr<nsIFile> &updater)
{
  // Copy the updater application from the GRE and the updater ini from the app
#if defined(XP_MACOSX)
  if (!CopyFileIntoUpdateDir(greDir, kUpdaterApp, updateDir))
    return false;
#else
  if (!CopyFileIntoUpdateDir(greDir, kUpdaterBin, updateDir))
    return false;
#endif
  CopyFileIntoUpdateDir(appDir, kUpdaterINI, updateDir);
#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(ANDROID)
  nsCOMPtr<nsIFile> iconDir;
  appDir->Clone(getter_AddRefs(iconDir));
  iconDir->AppendNative(NS_LITERAL_CSTRING("icons"));
  if (!CopyFileIntoUpdateDir(iconDir, kUpdaterPNG, updateDir))
    return false;
#endif
  // Finally, return the location of the updater binary.
  nsresult rv = updateDir->Clone(getter_AddRefs(updater));
  if (NS_FAILED(rv))
    return false;
#if defined(XP_MACOSX)
  rv  = updater->AppendNative(NS_LITERAL_CSTRING(kUpdaterApp));
  rv |= updater->AppendNative(NS_LITERAL_CSTRING("Contents"));
  rv |= updater->AppendNative(NS_LITERAL_CSTRING("MacOS"));
  if (NS_FAILED(rv))
    return false;
#endif
  rv = updater->AppendNative(NS_LITERAL_CSTRING(kUpdaterBin));
  return NS_SUCCEEDED(rv); 
}

static void
ApplyUpdate(nsIFile *greDir, nsIFile *updateDir, nsILocalFile *statusFile,
            nsIFile *appDir, int appArgc, char **appArgv)
{
  nsresult rv;

  // Steps:
  //  - mark update as 'applying'
  //  - copy updater into update dir
  //  - run updater w/ appDir as the current working dir

  nsCOMPtr<nsIFile> updater;
  if (!CopyUpdaterIntoUpdateDir(greDir, appDir, updateDir, updater)) {
    LOG(("failed copying updater\n"));
    return;
  }

  // We need to use the value returned from XRE_GetBinaryPath when attempting
  // to restart the running application.
  nsCOMPtr<nsILocalFile> appFile;

#if defined(XP_MACOSX)
  // On OS X we need to pass the location of the xulrunner-stub executable
  // rather than xulrunner-bin. See bug 349737.
  GetXULRunnerStubPath(appArgv[0], getter_AddRefs(appFile));
#else
  XRE_GetBinaryPath(appArgv[0], getter_AddRefs(appFile));
#endif

  if (!appFile)
    return;

#ifdef XP_WIN
  nsAutoString appFilePathW;
  rv = appFile->GetPath(appFilePathW);
  if (NS_FAILED(rv))
    return;
  NS_ConvertUTF16toUTF8 appFilePath(appFilePathW);

  nsAutoString updaterPathW;
  rv = updater->GetPath(updaterPathW);
  if (NS_FAILED(rv))
    return;

  NS_ConvertUTF16toUTF8 updaterPath(updaterPathW);

#else
  nsCAutoString appFilePath;
  rv = appFile->GetNativePath(appFilePath);
  if (NS_FAILED(rv))
    return;
  
  nsCAutoString updaterPath;
  rv = updater->GetNativePath(updaterPath);
  if (NS_FAILED(rv))
    return;

#endif

  // Get the directory to which the update will be applied. On Mac OSX we need
  // to apply the update to the Foo.app directory which is the parent of the
  // parent of the appDir. On other platforms we will just apply to the appDir.
#if defined(XP_MACOSX)
  nsCAutoString applyToDir;
  {
    nsCOMPtr<nsIFile> parentDir1, parentDir2;
    rv = appDir->GetParent(getter_AddRefs(parentDir1));
    if (NS_FAILED(rv))
      return;
    rv = parentDir1->GetParent(getter_AddRefs(parentDir2));
    if (NS_FAILED(rv))
      return;
    rv = parentDir2->GetNativePath(applyToDir);
  }
#elif defined(XP_WIN)
  nsAutoString applyToDirW;
  rv = appDir->GetPath(applyToDirW);

  NS_ConvertUTF16toUTF8 applyToDir(applyToDirW);
#else
  nsCAutoString applyToDir;
  rv = appDir->GetNativePath(applyToDir);
#endif
  if (NS_FAILED(rv))
    return;

#if defined(XP_WIN)
  nsAutoString updateDirPathW;
  rv = updateDir->GetPath(updateDirPathW);

  NS_ConvertUTF16toUTF8 updateDirPath(updateDirPathW);
#else
  nsCAutoString updateDirPath;
  rv = updateDir->GetNativePath(updateDirPath);
#endif

  if (NS_FAILED(rv))
    return;

  // Get the current working directory.
  char workingDirPath[MAXPATHLEN];
  rv = GetCurrentWorkingDir(workingDirPath, sizeof(workingDirPath));
  if (NS_FAILED(rv))
    return;

  // We used to write out "Applying" to the update.status file here.
  // Instead we do this from within the updater application now.
  // This is so that we don't overwrite the status of pending-service
  // in the Windows case.  This change was made for all platforms so
  // that it stays consistent across all OS.

  // Construct the PID argument for this process.  If we are using execv, then
  // we pass "0" which is then ignored by the updater.
#if defined(USE_EXECV)
  NS_NAMED_LITERAL_CSTRING(pid, "0");
#else
  nsCAutoString pid;
  pid.AppendInt((PRInt32) getpid());
#endif

  int argc = appArgc + 5;
  char **argv = new char*[argc + 1];
  if (!argv)
    return;
  argv[0] = (char*) updaterPath.get();
  argv[1] = (char*) updateDirPath.get();
  argv[2] = (char*) applyToDir.get();
  argv[3] = (char*) pid.get();
  if (appArgc) {
    argv[4] = workingDirPath;
    argv[5] = (char*) appFilePath.get();
    for (int i = 1; i < appArgc; ++i)
      argv[5 + i] = appArgv[i];
    argc = 5 + appArgc;
    argv[argc] = NULL;
  } else {
    argc = 4;
    argv[4] = NULL;
  }

  if (gSafeMode) {
    PR_SetEnv("MOZ_SAFE_MODE_RESTART=1");
  }

  LOG(("spawning updater process [%s]\n", updaterPath.get()));

#if defined(USE_EXECV)
  execv(updaterPath.get(), argv);
#elif defined(XP_WIN)

  // Launch the update using updater.exe
  if (!WinLaunchChild(updaterPathW.get(), argc, argv)) {
    return;
  }

  // We are going to process an update so we should exit now
  _exit(0);
#elif defined(XP_MACOSX)
  CommandLineServiceMac::SetupMacCommandLine(argc, argv, true);
  // LaunchChildMac uses posix_spawnp and prefers the current
  // architecture when launching. It doesn't require a
  // null-terminated string but it doesn't matter if we pass one.
  LaunchChildMac(argc, argv);
  exit(0);
#else
  PR_CreateProcessDetached(updaterPath.get(), argv, NULL, NULL);
  exit(0);
#endif
}

nsresult
ProcessUpdates(nsIFile *greDir, nsIFile *appDir, nsIFile *updRootDir,
               int argc, char **argv, const char *appVersion)
{
  nsresult rv;

  nsCOMPtr<nsIFile> updatesDir;
  rv = updRootDir->Clone(getter_AddRefs(updatesDir));
  if (NS_FAILED(rv))
    return rv;

  rv = updatesDir->AppendNative(NS_LITERAL_CSTRING("updates"));
  if (NS_FAILED(rv))
    return rv;

  rv = updatesDir->AppendNative(NS_LITERAL_CSTRING("0"));
  if (NS_FAILED(rv))
    return rv;
 
  const char *processingUpdates = PR_GetEnv("MOZ_PROCESS_UPDATES");
  if (processingUpdates && *processingUpdates) {
    // Enable the tests to request us to use a different update root directory
    const char *updRootOverride = PR_GetEnv("MOZ_UPDATE_ROOT_OVERRIDE");
    if (updRootOverride && *updRootOverride) {
      nsCOMPtr<nsILocalFile> overrideDir;
      nsCAutoString path(updRootOverride);
      rv = NS_NewNativeLocalFile(path, false, getter_AddRefs(overrideDir));
      if (NS_FAILED(rv)) {
        return rv;
      }
      updatesDir = do_QueryInterface(overrideDir);
    }
    // Enable the tests to request us to use a different app directory
    const char *appDirOverride = PR_GetEnv("MOZ_UPDATE_APPDIR_OVERRIDE");
    if (appDirOverride && *appDirOverride) {
      nsCOMPtr<nsILocalFile> overrideDir;
      nsCAutoString path(appDirOverride);
      rv = NS_NewNativeLocalFile(path, false, getter_AddRefs(overrideDir));
      if (NS_FAILED(rv)) {
        return rv;
      }
      NS_RELEASE(appDir);
      NS_ADDREF(appDir = overrideDir);
    }
  }

  nsCOMPtr<nsILocalFile> statusFile;
  if (GetStatusFile(updatesDir, statusFile) && 
      IsPending(statusFile)) {
    nsCOMPtr<nsILocalFile> versionFile;
    // Remove the update if the update application version file doesn't exist
    // or if the update's application version is less than the current
    // application version.
    if (!GetVersionFile(updatesDir, versionFile) ||
        IsOlderVersion(versionFile, appVersion)) {
      updatesDir->Remove(true);
    } else {
      ApplyUpdate(greDir, updatesDir, statusFile, appDir, 
                  argc, argv);
    }
  }

  return NS_OK;
}
