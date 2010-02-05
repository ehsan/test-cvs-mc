/* -*- Mode: C++; c-basic-offset: 2; tab-width: 8; indent-tabs-mode: nil; -*- */
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
 * The Original Code is Fennec Installer for WinCE.
 *
 * The Initial Developer of the Original Code is The Mozilla Foundation.
 *
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Pakhotin <alexp@mozilla.com> (original author)
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

#pragma once

/**
 * nsInstallerDlg dialog
 */
class nsInstallerDlg : public nsExtractorProgress
{
public:
  static nsInstallerDlg* GetInstance();
  void Init(HINSTANCE hInst);
  int DoModal();
  virtual void Progress(int n); // gets progress notifications
  INT_PTR DlgMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
  const WCHAR* GetExtractPath() { return m_sInstallPath; }

private:
  nsInstallerDlg();
  BOOL OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
  void FindMemCards();
  BOOL OnDialogCreated(HWND hDlg);
  BOOL OnBtnExtract();
  LRESULT SendMessageToControl(int nCtlID, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
  void SetControlWindowText(int nCtlID, const WCHAR *sText);

  BOOL PostExtract();
  BOOL StoreInstallPath();
  BOOL CreateShortcut();
  BOOL MoveSetupStrings();
  BOOL SilentFirstRun();

  BOOL GetInstallPath(WCHAR *sPath);
  BOOL RunUninstall(BOOL *pbCancelled);

  void AddErrorMsg(WCHAR* sErr);
  BOOL FastStartFileExists();

  static const int c_nMaxErrorLen = 2048;

  HINSTANCE m_hInst;
  HWND m_hDlg;
  BOOL m_bFastStart;
  WCHAR m_sProgramFiles[MAX_PATH];
  WCHAR m_sExtractPath[MAX_PATH];
  WCHAR m_sInstallPath[MAX_PATH];
  WCHAR m_sErrorMsg[c_nMaxErrorLen];
};
