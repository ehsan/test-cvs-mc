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
 * The Original Code is Maintenance service file system monitoring.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Brian R. Bondy <netzen@gmail.com>
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

#include <shlobj.h>
#include <shlwapi.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <shellapi.h>

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "rpcrt4.lib")

#include "nsWindowsHelpers.h"
#include "nsAutoPtr.h"

#include "workmonitor.h"
#include "serviceinstall.h"
#include "servicebase.h"
#include "registrycertificates.h"
#include "uachelper.h"
#include "updatehelper.h"

// Wait 15 minutes for an update operation to run at most.
// Updates usually take less than a minute so this seems like a 
// significantly large and safe amount of time to wait.
static const int TIME_TO_WAIT_ON_UPDATER = 15 * 60 * 1000;
PRUnichar* MakeCommandLine(int argc, PRUnichar **argv);
BOOL WriteStatusFailure(LPCWSTR updateDirPath, int errorCode);
BOOL PathGetSiblingFilePath(LPWSTR destinationBuffer,  LPCWSTR siblingFilePath, 
                            LPCWSTR newFileName);

// The error codes start from 16000 since Windows system error 
// codes only go up to 15999
const int SERVICE_UPDATER_COULD_NOT_BE_STARTED = 16000;
const int SERVICE_NOT_ENOUGH_COMMAND_LINE_ARGS = 16001;
const int SERVICE_UPDATER_SIGN_ERROR = 16002;
const int SERVICE_UPDATER_COMPARE_ERROR = 16003;
const int SERVICE_UPDATER_IDENTITY_ERROR = 16004;

/**
 * Runs an update process as the service using the SYSTEM account.
 *
 * @param  argc           The number of arguments in argv
 * @param  argv           The arguments normally passed to updater.exe
 *                        argv[0] must be the path to updater.exe
 * @param  processStarted Set to TRUE if the process was started.
 * @return TRUE if the update process was run had a return code of 0.
 */
BOOL
StartUpdateProcess(int argc,
                   LPWSTR *argv,
                   BOOL &processStarted)
{
  LOG(("Starting update process as the service in session 0.\n"));
  STARTUPINFO si = {0};
  si.cb = sizeof(STARTUPINFO);
  si.lpDesktop = L"winsta0\\Default";
  PROCESS_INFORMATION pi = {0};

  // The updater command line is of the form:
  // updater.exe update-dir apply [wait-pid [callback-dir callback-path args]]
  LPWSTR cmdLine = MakeCommandLine(argc, argv);

  // If we're about to start the update process from session 0,
  // then we should not show a GUI.  This only really needs to be done
  // on Vista and higher, but it's better to keep everything consistent
  // across all OS if it's of no harm.
  if (argc >= 2 ) {
    // Setting the desktop to blank will ensure no GUI is displayed
    si.lpDesktop = L"";
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
  }

  // We move the updater.ini file out of the way because we will handle 
  // executing PostUpdate through the service.  We handle PostUpdate from
  // the service because there are some per user things that happen that
  // can't run in session 0 which we run updater.exe in.
  // Once we are done running updater.exe we rename updater.ini back so
  // that if there were any errors the next updater.exe will run correctly.
  WCHAR updaterINI[MAX_PATH + 1];
  WCHAR updaterINITemp[MAX_PATH + 1];
  BOOL selfHandlePostUpdate = FALSE;
  // We use the updater.ini from the same directory as the updater.exe
  // because of background updates.
  if (PathGetSiblingFilePath(updaterINI, argv[0], L"updater.ini") &&
      PathGetSiblingFilePath(updaterINITemp, argv[0], L"updater.tmp")) {
    selfHandlePostUpdate = MoveFileEx(updaterINI, updaterINITemp, 
                                      MOVEFILE_REPLACE_EXISTING);
  }

  // Create an environment block for the updater.exe process we're about to 
  // start.  Indicate that MOZ_USING_SERVICE is set so the updater.exe can
  // do anything special that it needs to do for service updates.
  // Search in updater.cpp for more info on MOZ_USING_SERVICE.
  WCHAR envVarString[32];
  wsprintf(envVarString, L"MOZ_USING_SERVICE=1"); 
  _wputenv(envVarString);
  LPVOID environmentBlock = NULL;
  if (!CreateEnvironmentBlock(&environmentBlock, NULL, TRUE)) {
    LOG(("Could not create an environment block, setting it to NULL.\n"));
    environmentBlock = NULL;
  }
  // Empty value on _wputenv is how you remove an env variable in Windows
  _wputenv(L"MOZ_USING_SERVICE=");
  processStarted = CreateProcessW(argv[0], cmdLine, 
                                  NULL, NULL, FALSE, 
                                  CREATE_DEFAULT_ERROR_MODE | 
                                  CREATE_UNICODE_ENVIRONMENT, 
                                  environmentBlock, 
                                  NULL, &si, &pi);
  if (environmentBlock) {
    DestroyEnvironmentBlock(environmentBlock);
  }
  BOOL updateWasSuccessful = FALSE;
  if (processStarted) {
    // Wait for the updater process to finish
    LOG(("Process was started... waiting on result.\n")); 
    DWORD waitRes = WaitForSingleObject(pi.hProcess, TIME_TO_WAIT_ON_UPDATER);
    if (WAIT_TIMEOUT == waitRes) {
      // We waited a long period of time for updater.exe and it never finished
      // so kill it.
      TerminateProcess(pi.hProcess, 1);
    } else {
      // Check the return code of updater.exe to make sure we get 0
      DWORD returnCode;
      if (GetExitCodeProcess(pi.hProcess, &returnCode)) {
        LOG(("Process finished with return code %d.\n", returnCode)); 
        // updater returns 0 if successful.
        updateWasSuccessful = (returnCode == 0);
      } else {
        LOG(("Process finished but could not obtain return code.\n")); 
      }
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  } else {
    DWORD lastError = GetLastError();
    LOG(("Could not create process as current user, "
         "updaterPath: %ls; cmdLine: %l.  (%d)\n", 
         argv[0], cmdLine, lastError));
  }

  // Now that we're done with the update, restore back the updater.ini file
  // We use it ourselves, and also we want it back in case we had any type 
  // of error so that the normal update process can use it.
  if (selfHandlePostUpdate) {
    MoveFileEx(updaterINITemp, updaterINI, MOVEFILE_REPLACE_EXISTING);

    // Only run the PostUpdate if the update was successful
    if (updateWasSuccessful && argc > 2) {
      LPCWSTR installationDir = argv[2];
      LPCWSTR updateInfoDir = argv[1];

      // Launch the PostProcess with admin access in session 0.  This is
      // actually launching the post update process but it takes in the 
      // callback app path to figure out where to apply to.
      // The PostUpdate process with user only access will be done inside
      // the unelevated updater.exe after the update process is complete
      // from the service.  We don't know here which session to start
      // the user PostUpdate process from.
      LOG(("Launching post update process as the service in session 0.\n"));
      if (!LaunchWinPostProcess(installationDir, updateInfoDir, true, NULL)) {
        LOG(("The post update process could not be launched.\n"));
      }
    }
  }

  free(cmdLine);
  return updateWasSuccessful;
}

/**
 * Processes a software update command
 *
 * @param  argc           The number of arguments in argv
 * @param  argv           The arguments normally passed to updater.exe
 *                        argv[0] must be the path to updater.exe
 * @return TRUE if the update was successful.
 */
BOOL
ProessSoftwareUpdateCommand(DWORD argc, LPWSTR *argv)
{
  BOOL result = TRUE;
  if (argc < 3) {
    LOG(("Not enough command line parameters specified. "
         "Updating update.status.\n"));

    // We can only update update.status if argv[1] exists.  argv[1] is
    // the directory where the update.status file exists.
    if (argc > 1 || 
        !WriteStatusFailure(argv[1], 
                            SERVICE_NOT_ENOUGH_COMMAND_LINE_ARGS)) {
      LOG(("Could not write update.status service update failure."
           "Last error: %d\n", GetLastError()));
    }
    return FALSE;
  }

  // Verify that the updater.exe that we are executing is the same
  // as the one in the installation directory which we are updating.
  // The installation dir that we are installing to is argv[2].
  WCHAR installDirUpdater[MAX_PATH + 1];
  wcsncpy(installDirUpdater, argv[2], MAX_PATH);
  if (!PathAppendSafe(installDirUpdater, L"updater.exe")) {
    LOG(("Install directory updater could not be determined.\n"));
    result = FALSE;
  }

  BOOL updaterIsCorrect;
  if (result && !VerifySameFiles(argv[0], installDirUpdater, 
                                 updaterIsCorrect)) {
    LOG(("Error checking if the updaters are the same.\n"
         "Path 1: %ls\nPath 2: %ls\n", argv[0], installDirUpdater));
    result = FALSE;
  }

  if (result && !updaterIsCorrect) {
    LOG(("The updaters do not match, udpater will not run.\n")); 
    result = FALSE;
  }

  if (result) {
    LOG(("updater.exe was compared successfully to the installation directory"
         " updater.exe.\n"));
  } else {
    if (!WriteStatusFailure(argv[1], 
                            SERVICE_UPDATER_COMPARE_ERROR)) {
      LOG(("Could not write update.status updater compare failure.\n"));
    }
    return FALSE;
  }

  // Check to make sure the udpater.exe module has the unique updater identity.
  // This is a security measure to make sure that the signed executable that
  // we will run is actually an updater.
  HMODULE updaterModule = LoadLibrary(argv[0]);
  if (!updaterModule) {
    LOG(("updater.exe module could not be loaded. (%d)\n", GetLastError()));
    result = FALSE;
  } else {
    char updaterIdentity[64];
    if (!LoadStringA(updaterModule, IDS_UPDATER_IDENTITY, 
                     updaterIdentity, sizeof(updaterIdentity))) {
      LOG(("The updater.exe application does not contain the Mozilla"
           " updater identity.\n"));
      result = FALSE;
    }

    if (strcmp(updaterIdentity, UPDATER_IDENTITY_STRING)) {
      LOG(("The updater.exe identity string is not valid.\n"));
      result = FALSE;
    }
    FreeLibrary(updaterModule);
  }

  if (result) {
    LOG(("The updater.exe application contains the Mozilla"
          " updater identity.\n"));
  } else {
    if (!WriteStatusFailure(argv[1], 
                            SERVICE_UPDATER_IDENTITY_ERROR)) {
      LOG(("Could not write update.status no updater identity.\n"));
    }
    return TRUE;
  }

  // Check for updater.exe sign problems
  BOOL updaterSignProblem = FALSE;
#ifndef DISABLE_UPDATER_AUTHENTICODE_CHECK
  updaterSignProblem = !DoesBinaryMatchAllowedCertificates(argv[2],
                                                           argv[0]);
#endif

  // Only proceed with the update if we have no signing problems
  if (!updaterSignProblem) {
    BOOL updateProcessWasStarted = FALSE;
    if (StartUpdateProcess(argc, argv,
                           updateProcessWasStarted)) {
      LOG(("updater.exe was launched and run successfully!\n"));
      StartServiceUpdate(argc, argv);
    } else {
      result = FALSE;
      LOG(("Error running update process. Updating update.status"
           " Last error: %d\n", GetLastError()));

      // If the update process was started, then updater.exe is responsible for
      // setting the failure code.  If it could not be started then we do the 
      // work.  We set an error instead of directly setting status pending 
      // so that the app.update.service.errors pref can be updated when 
      // the callback app restarts.
      if (!updateProcessWasStarted) {
        if (!WriteStatusFailure(argv[1], 
                                SERVICE_UPDATER_COULD_NOT_BE_STARTED)) {
          LOG(("Could not write update.status service update failure."
               "Last error: %d\n", GetLastError()));
        }
      }
    }
  } else {
    result = FALSE;
    LOG(("Could not start process due to certificate check error on "
         "updater.exe. Updating update.status.  Last error: %d\n", GetLastError()));

    // When there is a certificate check error on the updater.exe application,
    // we want to write out the error.
    if (!WriteStatusFailure(argv[1], 
                            SERVICE_UPDATER_SIGN_ERROR)) {
      LOG(("Could not write pending state to update.status.  (%d)\n", 
           GetLastError()));
    }
  }
  LocalFree(argv);
  return result;
}

/**
 * Executes a service command.
 *
 * @param argc The number of arguments in argv
 * @param argv The service command line arguments, argv[0] and argv[1]
 *             and automatically included by Windows.  argv[2] is the
 *             service command.
 *             
 * @return FALSE if there was an error executing the service command.
 */
BOOL
ExecuteServiceCommand(int argc, LPWSTR *argv) 
{
  // Indicate that the service is busy and shouldn't be used by anyone else
  // by opening or creating a named event.  Programs should check if this 
  // event exists before trying to start the service.
  nsAutoHandle serviceRunningEvent(CreateEventW(NULL, TRUE, 
                                                FALSE, SERVICE_EVENT_NAME));

  if (argc < 3) {
    LOG(("Not enough command line arguments to execute a service command\n"));
    SetEvent(serviceRunningEvent);
    StopService();
    return FALSE;
  }

  // The tests work by making sure the log has changed, so we put a 
  // unique ID in the log.
  RPC_WSTR guidString = RPC_WSTR(L"");
  GUID guid;
  HRESULT hr = CoCreateGuid(&guid);
  if (SUCCEEDED(hr)) {
    UuidToString(&guid, &guidString);
  }
  LOG(("Executing service command %ls, ID: %ls\n", 
       argv[2], reinterpret_cast<LPCWSTR>(guidString)));
  RpcStringFree(&guidString);

  BOOL result = FALSE;
  if (!lstrcmpi(argv[2], L"software-update")) {
    result = ProessSoftwareUpdateCommand(argc - 3, argv + 3);
    LOG(("Service command %ls complete.\n", argv[2]));
  } else {
    LOG(("Service command not recognized: %ls.\n", argv[2]));
    // result is already set to FALSE
  }

  LOG(("service command %ls complete with result: %ls.\n", 
       argv[1], (result ? L"Success" : L"Failure")));
  SetEvent(serviceRunningEvent);
  StopService();
  return TRUE;
}
