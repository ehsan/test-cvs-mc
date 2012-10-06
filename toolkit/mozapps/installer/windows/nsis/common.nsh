# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.


################################################################################
# Helper defines and macros for toolkit applications

/**
 * Avoid creating macros / functions that overwrite registers (see the
 * GetLongPath macro for one way to avoid this)!
 *
 * Before using the registers exchange the passed in params and save existing
 * register values to the stack.
 *
 * Exch $R9 ; exhange the original $R9 with the top of the stack
 * Exch 1   ; exchange the top of the stack with 1 below the top of the stack
 * Exch $R8 ; exchange the original $R8 with the top of the stack
 * Exch 2   ; exchange the top of the stack with 2 below the top of the stack
 * Exch $R7 ; exchange the original $R7 with the top of the stack
 * Push $R6 ; push the original $R6 onto the top of the stack
 * Push $R5 ; push the original $R5 onto the top of the stack
 * Push $R4 ; push the original $R4 onto the top of the stack
 *
 * <do stuff>
 *
 * ; Restore the values.
 * Pop $R4  ; restore the value for $R4 from the top of the stack
 * Pop $R5  ; restore the value for $R5 from the top of the stack
 * Pop $R6  ; restore the value for $R6 from the top of the stack
 * Exch $R7 ; exchange the new $R7 value with the top of the stack
 * Exch 2   ; exchange the top of the stack with 2 below the top of the stack
 * Exch $R8 ; exchange the new $R8 value with the top of the stack
 * Exch 1   ; exchange the top of the stack with 2 below the top of the stack
 * Exch $R9 ; exchange the new $R9 value with the top of the stack
 *
 *
 * When inserting macros in common.nsh from another macro in common.nsh that
 * can be used from the uninstaller _MOZFUNC_UN will be undefined when it is
 * inserted. Use the following to redefine _MOZFUNC_UN with its original value
 * (see the RegCleanMain macro for an example).
 *
 * !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
 * !insertmacro ${_MOZFUNC_UN_TMP}FileJoin
 * !insertmacro ${_MOZFUNC_UN_TMP}LineFind
 * !insertmacro ${_MOZFUNC_UN_TMP}TextCompareNoDetails
 * !insertmacro ${_MOZFUNC_UN_TMP}TrimNewLines
 * !undef _MOZFUNC_UN
 * !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
 * !undef _MOZFUNC_UN_TMP
 */

; When including a file provided by NSIS check if its verbose macro is defined
; to prevent loading the file a second time.
!ifmacrondef TEXTFUNC_VERBOSE
  !include TextFunc.nsh
!endif

!ifmacrondef FILEFUNC_VERBOSE
  !include FileFunc.nsh
!endif

!ifmacrondef LOGICLIB_VERBOSITY
  !include LogicLib.nsh
!endif

!ifndef WINMESSAGES_INCLUDED
  !include WinMessages.nsh
!endif

; When including WinVer.nsh check if ___WINVER__NSH___ is defined to prevent
; loading the file a second time. NSIS versions prior to 2.21 didn't include
; WinVer.nsh so include it with the /NOFATAL option.
!ifndef ___WINVER__NSH___
  !include /NONFATAL WinVer.nsh
!endif

!include x64.nsh

; NSIS provided macros that we have overridden.
!include overrides.nsh

!define SHORTCUTS_LOG "shortcuts_log.ini"
!define TO_BE_DELETED "to_be_deleted"


################################################################################
# Macros for debugging

/**
 * The following two macros assist with verifying that a macro doesn't
 * overwrite any registers.
 *
 * Usage:
 * ${debugSetRegisters}
 * <do stuff>
 * ${debugDisplayRegisters}
 */

/**
 * Sets all register values to their name to assist with verifying that a macro
 * doesn't overwrite any registers.
 */
!macro debugSetRegisters
  StrCpy $0 "$$0"
  StrCpy $1 "$$1"
  StrCpy $2 "$$2"
  StrCpy $3 "$$3"
  StrCpy $4 "$$4"
  StrCpy $5 "$$5"
  StrCpy $6 "$$6"
  StrCpy $7 "$$7"
  StrCpy $8 "$$8"
  StrCpy $9 "$$9"
  StrCpy $R0 "$$R0"
  StrCpy $R1 "$$R1"
  StrCpy $R2 "$$R2"
  StrCpy $R3 "$$R3"
  StrCpy $R4 "$$R4"
  StrCpy $R5 "$$R5"
  StrCpy $R6 "$$R6"
  StrCpy $R7 "$$R7"
  StrCpy $R8 "$$R8"
  StrCpy $R9 "$$R9"
!macroend
!define debugSetRegisters "!insertmacro debugSetRegisters"

/**
 * Displays all register values to assist with verifying that a macro doesn't
 * overwrite any registers.
 */
!macro debugDisplayRegisters
  MessageBox MB_OK \
      "Register Values:$\n\
       $$0 = $0$\n$$1 = $1$\n$$2 = $2$\n$$3 = $3$\n$$4 = $4$\n\
       $$5 = $5$\n$$6 = $6$\n$$7 = $7$\n$$8 = $8$\n$$9 = $9$\n\
       $$R0 = $R0$\n$$R1 = $R1$\n$$R2 = $R2$\n$$R3 = $R3$\n$$R4 = $R4$\n\
       $$R5 = $R5$\n$$R6 = $R6$\n$$R7 = $R7$\n$$R8 = $R8$\n$$R9 = $R9"
!macroend
!define debugDisplayRegisters "!insertmacro debugDisplayRegisters"


################################################################################
# Modern User Interface (MUI) override macros

; Removed macros in nsis 2.33u (ported from nsis 2.22)
;  MUI_LANGUAGEFILE_DEFINE
;  MUI_LANGUAGEFILE_LANGSTRING_PAGE
;  MUI_LANGUAGEFILE_MULTILANGSTRING_PAGE
;  MUI_LANGUAGEFILE_LANGSTRING_DEFINE
;  MUI_LANGUAGEFILE_UNLANGSTRING_PAGE

!macro MOZ_MUI_LANGUAGEFILE_DEFINE DEFINE NAME

  !ifndef "${DEFINE}"
    !define "${DEFINE}" "${${NAME}}"
  !endif
  !undef "${NAME}"

!macroend

!macro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE PAGE NAME

  !ifdef MUI_${PAGE}PAGE
    LangString "${NAME}" 0 "${${NAME}}"
    !undef "${NAME}"
  !else
    !undef "${NAME}"
  !endif

!macroend

!macro MOZ_MUI_LANGUAGEFILE_MULTILANGSTRING_PAGE PAGE NAME

  !ifdef MUI_${PAGE}PAGE | MUI_UN${PAGE}PAGE
    LangString "${NAME}" 0 "${${NAME}}"
    !undef "${NAME}"
  !else
    !undef "${NAME}"
  !endif

!macroend

!macro MOZ_MUI_LANGUAGEFILE_LANGSTRING_DEFINE DEFINE NAME

  !ifdef "${DEFINE}"
    LangString "${NAME}" 0 "${${NAME}}"
  !endif
  !undef "${NAME}"

!macroend

!macro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE PAGE NAME

  !ifdef MUI_UNINSTALLER
    !ifdef MUI_UN${PAGE}PAGE
      LangString "${NAME}" 0 "${${NAME}}"
      !undef "${NAME}"
    !else
      !undef "${NAME}"
    !endif
  !else
    !undef "${NAME}"
  !endif

!macroend

; Modified version of the following MUI macros to support Mozilla localization.
; MUI_LANGUAGE
; MUI_LANGUAGEFILE_BEGIN
; MOZ_MUI_LANGUAGEFILE_END
; See <NSIS App Dir>/Contrib/Modern UI/System.nsh for more information
!define MUI_INSTALLOPTIONS_READ "!insertmacro MUI_INSTALLOPTIONS_READ"

!macro MOZ_MUI_LANGUAGE LANGUAGE
  !verbose push
  !verbose ${MUI_VERBOSE}
  !include "${LANGUAGE}.nsh"
  !verbose pop
!macroend

!macro MOZ_MUI_LANGUAGEFILE_BEGIN LANGUAGE
  !insertmacro MUI_INSERT
  !ifndef "MUI_LANGUAGEFILE_${LANGUAGE}_USED"
    !define "MUI_LANGUAGEFILE_${LANGUAGE}_USED"
    LoadLanguageFile "${LANGUAGE}.nlf"
  !else
    !error "Modern UI language file ${LANGUAGE} included twice!"
  !endif
!macroend

; Custom version of MUI_LANGUAGEFILE_END. The macro to add the default MUI
; strings and the macros for several strings that are part of the NSIS MUI and
; not in our locale files have been commented out.
!macro MOZ_MUI_LANGUAGEFILE_END

#  !include "${NSISDIR}\Contrib\Modern UI\Language files\Default.nsh"
  !ifdef MUI_LANGUAGEFILE_DEFAULT_USED
    !undef MUI_LANGUAGEFILE_DEFAULT_USED
    !warning "${LANGUAGE} Modern UI language file version doesn't match. Using default English texts for missing strings."
  !endif

  !insertmacro MOZ_MUI_LANGUAGEFILE_DEFINE "MUI_${LANGUAGE}_LANGNAME" "MUI_LANGNAME"

  !ifndef MUI_LANGDLL_PUSHLIST
    !define MUI_LANGDLL_PUSHLIST "'${MUI_${LANGUAGE}_LANGNAME}' ${LANG_${LANGUAGE}} "
  !else
    !ifdef MUI_LANGDLL_PUSHLIST_TEMP
      !undef MUI_LANGDLL_PUSHLIST_TEMP
    !endif
    !define MUI_LANGDLL_PUSHLIST_TEMP "${MUI_LANGDLL_PUSHLIST}"
    !undef MUI_LANGDLL_PUSHLIST
    !define MUI_LANGDLL_PUSHLIST "'${MUI_${LANGUAGE}_LANGNAME}' ${LANG_${LANGUAGE}} ${MUI_LANGDLL_PUSHLIST_TEMP}"
  !endif

  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE WELCOME "MUI_TEXT_WELCOME_INFO_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE WELCOME "MUI_TEXT_WELCOME_INFO_TEXT"

!ifdef MUI_TEXT_LICENSE_TITLE
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE LICENSE "MUI_TEXT_LICENSE_TITLE"
!endif
!ifdef MUI_TEXT_LICENSE_SUBTITLE
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE LICENSE "MUI_TEXT_LICENSE_SUBTITLE"
!endif
!ifdef MUI_INNERTEXT_LICENSE_TOP
  !insertmacro MOZ_MUI_LANGUAGEFILE_MULTILANGSTRING_PAGE LICENSE "MUI_INNERTEXT_LICENSE_TOP"
!endif

#  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE LICENSE "MUI_INNERTEXT_LICENSE_BOTTOM"

!ifdef MUI_INNERTEXT_LICENSE_BOTTOM_CHECKBOX
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE LICENSE "MUI_INNERTEXT_LICENSE_BOTTOM_CHECKBOX"
!endif

!ifdef MUI_INNERTEXT_LICENSE_BOTTOM_RADIOBUTTONS
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE LICENSE "MUI_INNERTEXT_LICENSE_BOTTOM_RADIOBUTTONS"
!endif

  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE COMPONENTS "MUI_TEXT_COMPONENTS_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE COMPONENTS "MUI_TEXT_COMPONENTS_SUBTITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_MULTILANGSTRING_PAGE COMPONENTS "MUI_INNERTEXT_COMPONENTS_DESCRIPTION_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_MULTILANGSTRING_PAGE COMPONENTS "MUI_INNERTEXT_COMPONENTS_DESCRIPTION_INFO"

  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE DIRECTORY "MUI_TEXT_DIRECTORY_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE DIRECTORY "MUI_TEXT_DIRECTORY_SUBTITLE"

  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE STARTMENU "MUI_TEXT_STARTMENU_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE STARTMENU "MUI_TEXT_STARTMENU_SUBTITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE STARTMENU "MUI_INNERTEXT_STARTMENU_TOP"
#  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE STARTMENU "MUI_INNERTEXT_STARTMENU_CHECKBOX"

  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE INSTFILES "MUI_TEXT_INSTALLING_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE INSTFILES "MUI_TEXT_INSTALLING_SUBTITLE"

  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE INSTFILES "MUI_TEXT_FINISH_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE INSTFILES "MUI_TEXT_FINISH_SUBTITLE"

  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE INSTFILES "MUI_TEXT_ABORT_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE INSTFILES "MUI_TEXT_ABORT_SUBTITLE"

  !insertmacro MOZ_MUI_LANGUAGEFILE_MULTILANGSTRING_PAGE FINISH "MUI_BUTTONTEXT_FINISH"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE FINISH "MUI_TEXT_FINISH_INFO_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE FINISH "MUI_TEXT_FINISH_INFO_TEXT"
  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_PAGE FINISH "MUI_TEXT_FINISH_INFO_REBOOT"
  !insertmacro MOZ_MUI_LANGUAGEFILE_MULTILANGSTRING_PAGE FINISH "MUI_TEXT_FINISH_REBOOTNOW"
  !insertmacro MOZ_MUI_LANGUAGEFILE_MULTILANGSTRING_PAGE FINISH "MUI_TEXT_FINISH_REBOOTLATER"
#  !insertmacro MOZ_MUI_LANGUAGEFILE_MULTILANGSTRING_PAGE FINISH "MUI_TEXT_FINISH_RUN"
#  !insertmacro MOZ_MUI_LANGUAGEFILE_MULTILANGSTRING_PAGE FINISH "MUI_TEXT_FINISH_SHOWREADME"

; Support for using the existing MUI_TEXT_ABORTWARNING string
!ifdef MOZ_MUI_CUSTOM_ABORT
    LangString MOZ_MUI_TEXT_ABORTWARNING 0 "${MUI_TEXT_ABORTWARNING}"
!endif

  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_DEFINE MUI_ABORTWARNING "MUI_TEXT_ABORTWARNING"


  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE WELCOME "MUI_UNTEXT_WELCOME_INFO_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE WELCOME "MUI_UNTEXT_WELCOME_INFO_TEXT"

  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE CONFIRM "MUI_UNTEXT_CONFIRM_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE CONFIRM "MUI_UNTEXT_CONFIRM_SUBTITLE"

#  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE LICENSE "MUI_UNTEXT_LICENSE_TITLE"
#  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE LICENSE "MUI_UNTEXT_LICENSE_SUBTITLE"

#  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE LICENSE "MUI_UNINNERTEXT_LICENSE_BOTTOM"
#  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE LICENSE "MUI_UNINNERTEXT_LICENSE_BOTTOM_CHECKBOX"
#  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE LICENSE "MUI_UNINNERTEXT_LICENSE_BOTTOM_RADIOBUTTONS"

#  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE COMPONENTS "MUI_UNTEXT_COMPONENTS_TITLE"
#  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE COMPONENTS "MUI_UNTEXT_COMPONENTS_SUBTITLE"

#  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE DIRECTORY "MUI_UNTEXT_DIRECTORY_TITLE"
#  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE DIRECTORY  "MUI_UNTEXT_DIRECTORY_SUBTITLE"

  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE INSTFILES "MUI_UNTEXT_UNINSTALLING_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE INSTFILES "MUI_UNTEXT_UNINSTALLING_SUBTITLE"

  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE INSTFILES "MUI_UNTEXT_FINISH_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE INSTFILES "MUI_UNTEXT_FINISH_SUBTITLE"

  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE INSTFILES "MUI_UNTEXT_ABORT_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE INSTFILES "MUI_UNTEXT_ABORT_SUBTITLE"

  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE FINISH "MUI_UNTEXT_FINISH_INFO_TITLE"
  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE FINISH "MUI_UNTEXT_FINISH_INFO_TEXT"
  !insertmacro MOZ_MUI_LANGUAGEFILE_UNLANGSTRING_PAGE FINISH "MUI_UNTEXT_FINISH_INFO_REBOOT"

  !insertmacro MOZ_MUI_LANGUAGEFILE_LANGSTRING_DEFINE MUI_UNABORTWARNING "MUI_UNTEXT_ABORTWARNING"

  !ifndef MUI_LANGDLL_LANGUAGES
    !define MUI_LANGDLL_LANGUAGES "'${LANGFILE_${LANGUAGE}_NAME}' '${LANG_${LANGUAGE}}' "
    !define MUI_LANGDLL_LANGUAGES_CP "'${LANGFILE_${LANGUAGE}_NAME}' '${LANG_${LANGUAGE}}' '${LANG_${LANGUAGE}_CP}' "
  !else
    !ifdef MUI_LANGDLL_LANGUAGES_TEMP
      !undef MUI_LANGDLL_LANGUAGES_TEMP
    !endif
    !define MUI_LANGDLL_LANGUAGES_TEMP "${MUI_LANGDLL_LANGUAGES}"
    !undef MUI_LANGDLL_LANGUAGES

    !ifdef MUI_LANGDLL_LANGUAGES_CP_TEMP
      !undef MUI_LANGDLL_LANGUAGES_CP_TEMP
    !endif
    !define MUI_LANGDLL_LANGUAGES_CP_TEMP "${MUI_LANGDLL_LANGUAGES_CP}"
    !undef MUI_LANGDLL_LANGUAGES_CP

    !define MUI_LANGDLL_LANGUAGES "'${LANGFILE_${LANGUAGE}_NAME}' '${LANG_${LANGUAGE}}' ${MUI_LANGDLL_LANGUAGES_TEMP}"
    !define MUI_LANGDLL_LANGUAGES_CP "'${LANGFILE_${LANGUAGE}_NAME}' '${LANG_${LANGUAGE}}' '${LANG_${LANGUAGE}_CP}' ${MUI_LANGDLL_LANGUAGES_CP_TEMP}"
  !endif

!macroend

/**
 * Creates an InstallOptions file with a UTF-16LE BOM and adds the RTL value
 * to the Settings section.
 *
 * @param   _FILE
 *          The name of the file to be created in $PLUGINSDIR.
 */
!macro InitInstallOptionsFile _FILE
  Push $R9

  FileOpen $R9 "$PLUGINSDIR\${_FILE}" w
  FileWriteWord $R9 "65279"
  FileClose $R9
  WriteIniStr "$PLUGINSDIR\${_FILE}" "Settings" "RTL" "$(^RTL)"

  Pop $R9
!macroend


################################################################################
# Macros for handling files in use

/**
 * Checks for files in use in the $INSTDIR directory. To check files in
 * sub-directories this macro would need to be rewritten to create
 * sub-directories in the temporary directory used to backup the files that are
 * checked.
 *
 * Example usage:
 *
 *  ; The first string to be pushed onto the stack MUST be "end" to indicate
 *  ; that there are no more files in the $INSTDIR directory to check.
 *  Push "end"
 *  Push "freebl3.dll"
 *  ; The last file pushed should be the app's main exe so if it is in use this
 *  ; macro will return after the first check.
 *  Push "${FileMainEXE}"
 *  ${CheckForFilesInUse} $R9
 *
 * !IMPORTANT - this macro uses the $R7, $R8, and $R9 registers and makes no
 *              attempt to restore their original values.
 *
 * @return  _RESULT
 *          false if all of the files popped from the stack are not in use.
 *          True if any of the files popped from the stack are in use.
 * $R7 = Temporary backup directory where the files will be copied to.
 * $R8 = value popped from the stack. This will either be a file name for a file
 *       in the $INSTDIR directory or "end" to indicate that there are no
 *       additional files to check.
 * $R9 = _RESULT
 */
!macro CheckForFilesInUse

  !ifndef ${_MOZFUNC_UN}CheckForFilesInUse
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}CheckForFilesInUse "!insertmacro ${_MOZFUNC_UN}CheckForFilesInUseCall"

    Function ${_MOZFUNC_UN}CheckForFilesInUse
      ; Create a temporary backup directory.
      GetTempFileName $R7 "$INSTDIR"
      Delete "$R7"
      SetOutPath "$R7"
      StrCpy $R9 "false"

      Pop $R8
      ${While} $R8 != "end"
        ${Unless} ${FileExists} "$INSTDIR\$R8"
          Pop $R8 ; get next file to check before continuing
          ${Continue}
        ${EndUnless}

        ClearErrors
        CopyFiles /SILENT "$INSTDIR\$R8" "$R7\$R8" ; try to copy
        ${If} ${Errors}
          ; File is in use
          StrCpy $R9 "true"
          ${Break}
        ${EndIf}

        Delete "$INSTDIR\$R8" ; delete original
        ${If} ${Errors}
          ; File is in use
          StrCpy $R9 "true"
          Delete "$R7\$R8" ; delete temp copy
          ${Break}
        ${EndIf}

        Pop $R8 ; get next file to check
      ${EndWhile}

      ; clear stack
      ${While} $R8 != "end"
        Pop $R8
      ${EndWhile}

      ; restore everything
      SetOutPath "$INSTDIR"
      CopyFiles /SILENT "$R7\*" "$INSTDIR\"
      RmDir /r "$R7"
      SetOutPath "$EXEDIR"
      ClearErrors

      Push $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro CheckForFilesInUseCall _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call CheckForFilesInUse
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro un.CheckForFilesInUseCall _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.CheckForFilesInUse
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro un.CheckForFilesInUse
  !ifndef un.CheckForFilesInUse
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro CheckForFilesInUse

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * The macros below will automatically prepend un. to the function names when
 * they are defined (e.g. !define un.RegCleanMain).
 */
!verbose push
!verbose 3
!ifndef _MOZFUNC_VERBOSE
  !define _MOZFUNC_VERBOSE 3
!endif
!verbose ${_MOZFUNC_VERBOSE}
!define MOZFUNC_VERBOSE "!insertmacro MOZFUNC_VERBOSE"
!define _MOZFUNC_UN
!define _MOZFUNC_S
!verbose pop

!macro MOZFUNC_VERBOSE _VERBOSE
  !verbose push
  !verbose 3
  !undef _MOZFUNC_VERBOSE
  !define _MOZFUNC_VERBOSE ${_VERBOSE}
  !verbose pop
!macroend

/**
 * Displays a MessageBox and then calls abort to prevent continuing to the
 * next page when the specified Window Class is found.
 *
 * @param   _WINDOW_CLASS
 *          The Window Class to search for with FindWindow.
 * @param   _MSG
 *          The message text to display in the message box.
 *
 * $R7 = return value from FindWindow
 * $R8 = _WINDOW_CLASS
 * $R9 = _MSG
 */
!macro ManualCloseAppPrompt

  !ifndef ${_MOZFUNC_UN}ManualCloseAppPrompt
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}ManualCloseAppPrompt "!insertmacro ${_MOZFUNC_UN}ManualCloseAppPromptCall"

    Function ${_MOZFUNC_UN}ManualCloseAppPrompt
      Exch $R9
      Exch 1
      Exch $R8
      Push $R7

      FindWindow $R7 "$R8"
      ${If} $R7 <> 0 ; integer comparison
        MessageBox MB_OK|MB_ICONQUESTION "$R9"
        Abort
      ${EndIf}

      Pop $R7
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro ManualCloseAppPromptCall _WINDOW_CLASS _MSG
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_WINDOW_CLASS}"
  Push "${_MSG}"
  Call ManualCloseAppPrompt
  !verbose pop
!macroend

!macro un.ManualCloseAppPromptCall _WINDOW_CLASS _MSG
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_WINDOW_CLASS}"
  Push "${_MSG}"
  Call un.ManualCloseAppPrompt
  !verbose pop
!macroend

!macro un.ManualCloseAppPrompt
  !ifndef un.ManualCloseAppPrompt
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro ManualCloseAppPrompt

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend


################################################################################
# Macros for working with the registry

/**
 * Writes a registry string using SHCTX and the supplied params and logs the
 * action to the install log and the uninstall log if _LOG_UNINSTALL equals 1.
 *
 * Define NO_LOG to prevent all logging when calling this from the uninstaller.
 *
 * @param   _ROOT
 *          The registry key root as defined by NSIS (e.g. HKLM, HKCU, etc.).
 *          This will only be used for logging.
 * @param   _KEY
 *          The subkey in relation to the key root.
 * @param   _NAME
 *          The key value name to write to.
 * @param   _STR
 *          The string to write to the key value name.
 * @param   _LOG_UNINSTALL
 *          0 = don't add to uninstall log, 1 = add to uninstall log.
 *
 * $R5 = _ROOT
 * $R6 = _KEY
 * $R7 = _NAME
 * $R8 = _STR
 * $R9 = _LOG_UNINSTALL
 */
!macro WriteRegStr2

  !ifndef ${_MOZFUNC_UN}WriteRegStr2
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}WriteRegStr2 "!insertmacro ${_MOZFUNC_UN}WriteRegStr2Call"

    Function ${_MOZFUNC_UN}WriteRegStr2
      Exch $R9
      Exch 1
      Exch $R8
      Exch 2
      Exch $R7
      Exch 3
      Exch $R6
      Exch 4
      Exch $R5

      ClearErrors
      WriteRegStr SHCTX "$R6" "$R7" "$R8"

      !ifndef NO_LOG
        ${If} ${Errors}
          ${LogMsg} "** ERROR Adding Registry String: $R5 | $R6 | $R7 | $R8 **"
        ${Else}
          ${If} $R9 == 1 ; add to the uninstall log?
            ${LogUninstall} "RegVal: $R5 | $R6 | $R7"
          ${EndIf}
          ${LogMsg} "Added Registry String: $R5 | $R6 | $R7 | $R8"
        ${EndIf}
      !endif

      Exch $R5
      Exch 4
      Exch $R6
      Exch 3
      Exch $R7
      Exch 2
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro WriteRegStr2Call _ROOT _KEY _NAME _STR _LOG_UNINSTALL
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_ROOT}"
  Push "${_KEY}"
  Push "${_NAME}"
  Push "${_STR}"
  Push "${_LOG_UNINSTALL}"
  Call WriteRegStr2
  !verbose pop
!macroend

!macro un.WriteRegStr2Call _ROOT _KEY _NAME _STR _LOG_UNINSTALL
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_ROOT}"
  Push "${_KEY}"
  Push "${_NAME}"
  Push "${_STR}"
  Push "${_LOG_UNINSTALL}"
  Call un.WriteRegStr2
  !verbose pop
!macroend

!macro un.WriteRegStr2
  !ifndef un.WriteRegStr2
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro WriteRegStr2

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Writes a registry dword using SHCTX and the supplied params and logs the
 * action to the install log and the uninstall log if _LOG_UNINSTALL equals 1.
 *
 * Define NO_LOG to prevent all logging when calling this from the uninstaller.
 *
 * @param   _ROOT
 *          The registry key root as defined by NSIS (e.g. HKLM, HKCU, etc.).
 *          This will only be used for logging.
 * @param   _KEY
 *          The subkey in relation to the key root.
 * @param   _NAME
 *          The key value name to write to.
 * @param   _DWORD
 *          The dword to write to the key value name.
 * @param   _LOG_UNINSTALL
 *          0 = don't add to uninstall log, 1 = add to uninstall log.
 *
 * $R5 = _ROOT
 * $R6 = _KEY
 * $R7 = _NAME
 * $R8 = _DWORD
 * $R9 = _LOG_UNINSTALL
 */
!macro WriteRegDWORD2

  !ifndef ${_MOZFUNC_UN}WriteRegDWORD2
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}WriteRegDWORD2 "!insertmacro ${_MOZFUNC_UN}WriteRegDWORD2Call"

    Function ${_MOZFUNC_UN}WriteRegDWORD2
      Exch $R9
      Exch 1
      Exch $R8
      Exch 2
      Exch $R7
      Exch 3
      Exch $R6
      Exch 4
      Exch $R5

      ClearErrors
      WriteRegDWORD SHCTX "$R6" "$R7" "$R8"

      !ifndef NO_LOG
        ${If} ${Errors}
          ${LogMsg} "** ERROR Adding Registry DWord: $R5 | $R6 | $R7 | $R8 **"
        ${Else}
          ${If} $R9 == 1 ; add to the uninstall log?
            ${LogUninstall} "RegVal: $R5 | $R6 | $R7"
          ${EndIf}
          ${LogMsg} "Added Registry DWord: $R5 | $R6 | $R7 | $R8"
        ${EndIf}
      !endif

      Exch $R5
      Exch 4
      Exch $R6
      Exch 3
      Exch $R7
      Exch 2
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro WriteRegDWORD2Call _ROOT _KEY _NAME _DWORD _LOG_UNINSTALL
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_ROOT}"
  Push "${_KEY}"
  Push "${_NAME}"
  Push "${_DWORD}"
  Push "${_LOG_UNINSTALL}"
  Call WriteRegDWORD2
  !verbose pop
!macroend

!macro un.WriteRegDWORD2Call _ROOT _KEY _NAME _DWORD _LOG_UNINSTALL
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_ROOT}"
  Push "${_KEY}"
  Push "${_NAME}"
  Push "${_DWORD}"
  Push "${_LOG_UNINSTALL}"
  Call un.WriteRegDWORD2
  !verbose pop
!macroend

!macro un.WriteRegDWORD2
  !ifndef un.WriteRegDWORD2
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro WriteRegDWORD2

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Writes a registry string to HKCR using the supplied params and logs the
 * action to the install log and the uninstall log if _LOG_UNINSTALL equals 1.
 *
 * Define NO_LOG to prevent all logging when calling this from the uninstaller.
 *
 * @param   _ROOT
 *          The registry key root as defined by NSIS (e.g. HKLM, HKCU, etc.).
 *          This will only be used for logging.
 * @param   _KEY
 *          The subkey in relation to the key root.
 * @param   _NAME
 *          The key value name to write to.
 * @param   _STR
 *          The string to write to the key value name.
 * @param   _LOG_UNINSTALL
 *          0 = don't add to uninstall log, 1 = add to uninstall log.
 *
 * $R5 = _ROOT
 * $R6 = _KEY
 * $R7 = _NAME
 * $R8 = _STR
 * $R9 = _LOG_UNINSTALL
 */
!macro WriteRegStrHKCR

  !ifndef ${_MOZFUNC_UN}WriteRegStrHKCR
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}WriteRegStrHKCR "!insertmacro ${_MOZFUNC_UN}WriteRegStrHKCRCall"

    Function ${_MOZFUNC_UN}WriteRegStrHKCR
      Exch $R9
      Exch 1
      Exch $R8
      Exch 2
      Exch $R7
      Exch 3
      Exch $R6
      Exch 4
      Exch $R5

      ClearErrors
      WriteRegStr HKCR "$R6" "$R7" "$R8"

      !ifndef NO_LOG
        ${If} ${Errors}
          ${LogMsg} "** ERROR Adding Registry String: $R5 | $R6 | $R7 | $R8 **"
        ${Else}
          ${If} $R9 == 1 ; add to the uninstall log?
            ${LogUninstall} "RegVal: $R5 | $R6 | $R7"
          ${EndIf}
          ${LogMsg} "Added Registry String: $R5 | $R6 | $R7 | $R8"
        ${EndIf}
      !endif

      Exch $R5
      Exch 4
      Exch $R6
      Exch 3
      Exch $R7
      Exch 2
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro WriteRegStrHKCRCall _ROOT _KEY _NAME _STR _LOG_UNINSTALL
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_ROOT}"
  Push "${_KEY}"
  Push "${_NAME}"
  Push "${_STR}"
  Push "${_LOG_UNINSTALL}"
  Call WriteRegStrHKCR
  !verbose pop
!macroend

!macro un.WriteRegStrHKCRCall _ROOT _KEY _NAME _STR _LOG_UNINSTALL
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_ROOT}"
  Push "${_KEY}"
  Push "${_NAME}"
  Push "${_STR}"
  Push "${_LOG_UNINSTALL}"
  Call un.WriteRegStrHKCR
  !verbose pop
!macroend

!macro un.WriteRegStrHKCR
  !ifndef un.WriteRegStrHKCR
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro WriteRegStrHKCR

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

!define KEY_SET_VALUE 0x0002
!define KEY_WOW64_64KEY 0x0100
!ifndef HAVE_64BIT_OS
  !define CREATE_KEY_SAM ${KEY_SET_VALUE}
!else
  !define CREATE_KEY_SAM ${KEY_SET_VALUE}|${KEY_WOW64_64KEY}
!endif

/**
 * Creates a registry key. This will log the actions to the install and
 * uninstall logs. Alternatively you can set a registry value to create the key
 * and then delete the value.
 *
 * Define NO_LOG to prevent all logging when calling this from the uninstaller.
 *
 * @param   _ROOT
 *          The registry key root as defined by NSIS (e.g. HKLM, HKCU, etc.).
 * @param   _KEY
 *          The subkey in relation to the key root.
 * @param   _LOG_UNINSTALL
 *          0 = don't add to uninstall log, 1 = add to uninstall log.
 *
 * $R4 = [out] handle to newly created registry key. If this is not a key
 *       located in one of the predefined registry keys this must be closed
 *       with RegCloseKey (this should not be needed unless someone decides to
 *       do something extremely squirrelly with NSIS).
 * $R5 = return value from RegCreateKeyExW (represented by R5 in the system call).
 * $R6 = [in] hKey passed to RegCreateKeyExW.
 * $R7 = _ROOT
 * $R8 = _KEY
 * $R9 = _LOG_UNINSTALL
 */
!macro CreateRegKey

  !ifndef ${_MOZFUNC_UN}CreateRegKey
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}CreateRegKey "!insertmacro ${_MOZFUNC_UN}CreateRegKeyCall"

    Function ${_MOZFUNC_UN}CreateRegKey
      Exch $R9
      Exch 1
      Exch $R8
      Exch 2
      Exch $R7
      Push $R6
      Push $R5
      Push $R4

      StrCmp $R7 "HKCR" +1 +2
      StrCpy $R6 "0x80000000"
      StrCmp $R7 "HKCU" +1 +2
      StrCpy $R6 "0x80000001"
      StrCmp $R7 "HKLM" +1 +2
      StrCpy $R6 "0x80000002"

      ; see definition of RegCreateKey
      System::Call "Advapi32::RegCreateKeyExW(i R6, w R8, i 0, i 0, i 0,\
                                              i ${CREATE_KEY_SAM}, i 0, *i .R4,\
                                              i 0) i .R5"

      !ifndef NO_LOG
        ; if $R5 is not 0 then there was an error creating the registry key.
        ${If} $R5 <> 0
          ${LogMsg} "** ERROR Adding Registry Key: $R7 | $R8 **"
        ${Else}
          ${If} $R9 == 1 ; add to the uninstall log?
            ${LogUninstall} "RegKey: $R7 | $R8"
          ${EndIf}
          ${LogMsg} "Added Registry Key: $R7 | $R8"
        ${EndIf}
      !endif

      StrCmp $R5 0 +1 +2
      System::Call "Advapi32::RegCloseKey(iR4)"

      Pop $R4
      Pop $R5
      Pop $R6
      Exch $R7
      Exch 2
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro CreateRegKeyCall _ROOT _KEY _LOG_UNINSTALL
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_ROOT}"
  Push "${_KEY}"
  Push "${_LOG_UNINSTALL}"
  Call CreateRegKey
  !verbose pop
!macroend

!macro un.CreateRegKeyCall _ROOT _KEY _LOG_UNINSTALL
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_ROOT}"
  Push "${_KEY}"
  Push "${_LOG_UNINSTALL}"
  Call un.CreateRegKey
  !verbose pop
!macroend

!macro un.CreateRegKey
  !ifndef un.CreateRegKey
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro CreateRegKey

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Helper for checking for the existence of a registry key.
 * SHCTX is the root key to search.
 *
 * @param   _MAIN_KEY
 *          Sub key to iterate for the key in question
 * @param   _KEY
 *          Key name to search for
 * @return  _RESULT
 *          'true' / 'false' result
 */
!macro CheckIfRegistryKeyExists
  !ifndef CheckIfRegistryKeyExists
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define CheckIfRegistryKeyExists "!insertmacro CheckIfRegistryKeyExistsCall"

    Function CheckIfRegistryKeyExists
      ; stack: main key, key
      Exch $R9 ; main key, stack: old R9, key
      Exch 1   ; stack: key, old R9
      Exch $R8 ; key, stack: old R8, old R9
      Push $R7
      Push $R6
      Push $R5

      StrCpy $R5 "false"
      StrCpy $R7 "0" # loop index
      ${Do}
        EnumRegKey $R6 SHCTX "$R9" "$R7"
        ${If} "$R6" == "$R8"
          StrCpy $R5 "true"
          ${Break}
        ${EndIf}
        IntOp $R7 $R7 + 1
      ${LoopWhile} $R6 != ""
      ClearErrors

      StrCpy $R9 $R5

      Pop $R5
      Pop $R6
      Pop $R7 ; stack: old R8, old R9
      Pop $R8 ; stack: old R9
      Exch $R9 ; stack: result
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro CheckIfRegistryKeyExistsCall _MAIN_KEY _KEY _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Push "${_MAIN_KEY}"
  Call CheckIfRegistryKeyExists
  Pop ${_RESULT}
  !verbose pop
!macroend

################################################################################
# Macros for adding file and protocol handlers

/**
 * Writes common registry values for a handler using SHCTX.
 *
 * @param   _KEY
 *          The subkey in relation to the key root.
 * @param   _VALOPEN
 *          The path and args to launch the application.
 * @param   _VALICON
 *          The path to the binary that contains the icon group for the default icon
 *          followed by a comma and either the icon group's resource index or the icon
 *          group's resource id prefixed with a minus sign
 * @param   _DISPNAME
 *          The display name for the handler. If emtpy no value will be set.
 * @param   _ISPROTOCOL
 *          Sets protocol handler specific registry values when "true".
 * @param   _ISDDE
 *          Sets DDE specific registry values when "true".
 *
 * $R3 = string value of the current registry key path.
 * $R4 = _KEY
 * $R5 = _VALOPEN
 * $R6 = _VALICON
 * $R7 = _DISPNAME
 * $R8 = _ISPROTOCOL
 * $R9 = _ISDDE
 */
!macro AddHandlerValues

  !ifndef ${_MOZFUNC_UN}AddHandlerValues
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}AddHandlerValues "!insertmacro ${_MOZFUNC_UN}AddHandlerValuesCall"

    Function ${_MOZFUNC_UN}AddHandlerValues
      Exch $R9
      Exch 1
      Exch $R8
      Exch 2
      Exch $R7
      Exch 3
      Exch $R6
      Exch 4
      Exch $R5
      Exch 5
      Exch $R4
      Push $R3

      StrCmp "$R7" "" +6 +1
      ReadRegStr $R3 SHCTX "$R4" "FriendlyTypeName"

      StrCmp "$R3" "" +1 +3
      WriteRegStr SHCTX "$R4" "" "$R7"
      WriteRegStr SHCTX "$R4" "FriendlyTypeName" "$R7"

      StrCmp "$R8" "true" +1 +2
      WriteRegStr SHCTX "$R4" "URL Protocol" ""
      StrCpy $R3 ""
      ReadRegDWord $R3 SHCTX "$R4" "EditFlags"
      StrCmp $R3 "" +1 +3  ; Only add EditFlags if a value doesn't exist
      DeleteRegValue SHCTX "$R4" "EditFlags"
      WriteRegDWord SHCTX "$R4" "EditFlags" 0x00000002

      StrCmp "$R6" "" +2 +1
      WriteRegStr SHCTX "$R4\DefaultIcon" "" "$R6"

      StrCmp "$R5" "" +2 +1
      WriteRegStr SHCTX "$R4\shell\open\command" "" "$R5"

!ifdef DDEApplication
      StrCmp "$R9" "true" +1 +11
      WriteRegStr SHCTX "$R4\shell\open\ddeexec" "" "$\"%1$\",,0,0,,,,"
      WriteRegStr SHCTX "$R4\shell\open\ddeexec" "NoActivateHandler" ""
      WriteRegStr SHCTX "$R4\shell\open\ddeexec\Application" "" "${DDEApplication}"
      WriteRegStr SHCTX "$R4\shell\open\ddeexec\Topic" "" "WWW_OpenURL"
      ; The ifexec key may have been added by another application so try to
      ; delete it to prevent it from breaking this app's shell integration.
      ; Also, IE 6 and below doesn't remove this key when it sets itself as the
      ; default handler and if this key exists IE's shell integration breaks.
      DeleteRegKey HKLM "$R4\shell\open\ddeexec\ifexec"
      DeleteRegKey HKCU "$R4\shell\open\ddeexec\ifexec"
!endif

      ClearErrors

      Pop $R3
      Exch $R4
      Exch 5
      Exch $R5
      Exch 4
      Exch $R6
      Exch 3
      Exch $R7
      Exch 2
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro AddHandlerValuesCall _KEY _VALOPEN _VALICON _DISPNAME _ISPROTOCOL _ISDDE
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Push "${_VALOPEN}"
  Push "${_VALICON}"
  Push "${_DISPNAME}"
  Push "${_ISPROTOCOL}"
  Push "${_ISDDE}"
  Call AddHandlerValues
  !verbose pop
!macroend

!macro un.AddHandlerValuesCall _KEY _VALOPEN _VALICON _DISPNAME _ISPROTOCOL _ISDDE
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Push "${_VALOPEN}"
  Push "${_VALICON}"
  Push "${_DISPNAME}"
  Push "${_ISPROTOCOL}"
  Push "${_ISDDE}"
  Call un.AddHandlerValues
  !verbose pop
!macroend

!macro un.AddHandlerValues
  !ifndef un.AddHandlerValues
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro AddHandlerValues

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Writes common registry values for a handler that uses DDE using SHCTX.
 *
 * @param   _KEY
 *          The key name in relation to the HKCR root. SOFTWARE\Classes is
 *          prefixed to this value when using SHCTX.
 * @param   _VALOPEN
 *          The path and args to launch the application.
 * @param   _VALICON
 *          The path to the binary that contains the icon group for the default icon
 *          followed by a comma and either the icon group's resource index or the icon
 *          group's resource id prefixed with a minus sign
 * @param   _DISPNAME
 *          The display name for the handler. If emtpy no value will be set.
 * @param   _ISPROTOCOL
 *          Sets protocol handler specific registry values when "true".
 * @param   _DDE_APPNAME
 *          Sets DDE specific registry values when not an empty string.
 *
 * $R0 = storage for SOFTWARE\Classes
 * $R1 = string value of the current registry key path.
 * $R2 = _KEY
 * $R3 = _VALOPEN
 * $R4 = _VALICON
 * $R5 = _DISPNAME
 * $R6 = _ISPROTOCOL
 * $R7 = _DDE_APPNAME
 * $R8 = _DDE_DEFAULT
 * $R9 = _DDE_TOPIC
 */
!macro AddDDEHandlerValues

  !ifndef ${_MOZFUNC_UN}AddDDEHandlerValues
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}AddDDEHandlerValues "!insertmacro ${_MOZFUNC_UN}AddDDEHandlerValuesCall"

    Function ${_MOZFUNC_UN}AddDDEHandlerValues
      Exch $R9
      Exch 1
      Exch $R8
      Exch 2
      Exch $R7
      Exch 3
      Exch $R6
      Exch 4
      Exch $R5
      Exch 5
      Exch $R4
      Exch 6
      Exch $R3
      Exch 7
      Exch $R2
      Push $R1
      Push $R0

      StrCpy $R0 "SOFTWARE\Classes"
      StrCmp "$R5" "" +6 +1
      ReadRegStr $R1 SHCTX "$R2" "FriendlyTypeName"

      StrCmp "$R1" "" +1 +3
      WriteRegStr SHCTX "$R0\$R2" "" "$R5"
      WriteRegStr SHCTX "$R0\$R2" "FriendlyTypeName" "$R5"

      StrCmp "$R6" "true" +1 +2
      WriteRegStr SHCTX "$R0\$R2" "URL Protocol" ""
      StrCpy $R1 ""
      ReadRegDWord $R1 SHCTX "$R0\$R2" "EditFlags"
      StrCmp $R1 "" +1 +3  ; Only add EditFlags if a value doesn't exist
      DeleteRegValue SHCTX "$R0\$R2" "EditFlags"
      WriteRegDWord SHCTX "$R0\$R2" "EditFlags" 0x00000002

      StrCmp "$R4" "" +2 +1
      WriteRegStr SHCTX "$R0\$R2\DefaultIcon" "" "$R4"

      WriteRegStr SHCTX "$R0\$R2\shell" "" "open"
      WriteRegStr SHCTX "$R0\$R2\shell\open\command" "" "$R3"

      WriteRegStr SHCTX "$R0\$R2\shell\open\ddeexec" "" "$R8"
      WriteRegStr SHCTX "$R0\$R2\shell\open\ddeexec" "NoActivateHandler" ""
      WriteRegStr SHCTX "$R0\$R2\shell\open\ddeexec\Application" "" "$R7"
      WriteRegStr SHCTX "$R0\$R2\shell\open\ddeexec\Topic" "" "$R9"

      ; The ifexec key may have been added by another application so try to
      ; delete it to prevent it from breaking this app's shell integration.
      ; Also, IE 6 and below doesn't remove this key when it sets itself as the
      ; default handler and if this key exists IE's shell integration breaks.
      DeleteRegKey HKLM "$R0\$R2\shell\open\ddeexec\ifexec"
      DeleteRegKey HKCU "$R0\$R2\shell\open\ddeexec\ifexec"
      ClearErrors

      Pop $R0
      Pop $R1
      Exch $R2
      Exch 7
      Exch $R3
      Exch 6
      Exch $R4
      Exch 5
      Exch $R5
      Exch 4
      Exch $R6
      Exch 3
      Exch $R7
      Exch 2
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro AddDDEHandlerValuesCall _KEY _VALOPEN _VALICON _DISPNAME _ISPROTOCOL _DDE_APPNAME _DDE_DEFAULT _DDE_TOPIC
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Push "${_VALOPEN}"
  Push "${_VALICON}"
  Push "${_DISPNAME}"
  Push "${_ISPROTOCOL}"
  Push "${_DDE_APPNAME}"
  Push "${_DDE_DEFAULT}"
  Push "${_DDE_TOPIC}"
  Call AddDDEHandlerValues
  !verbose pop
!macroend

!macro un.AddDDEHandlerValuesCall _KEY _VALOPEN _VALICON _DISPNAME _ISPROTOCOL _DDE_APPNAME _DDE_DEFAULT _DDE_TOPIC
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Push "${_VALOPEN}"
  Push "${_VALICON}"
  Push "${_DISPNAME}"
  Push "${_ISPROTOCOL}"
  Push "${_DDE_APPNAME}"
  Push "${_DDE_DEFAULT}"
  Push "${_DDE_TOPIC}"
  Call un.AddDDEHandlerValues
  !verbose pop
!macroend

!macro un.AddDDEHandlerValues
  !ifndef un.AddDDEHandlerValues
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro AddDDEHandlerValues

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Writes common registry values for a handler that DOES NOT use DDE using SHCTX.
 *
 * @param   _KEY
 *          The key name in relation to the HKCR root. SOFTWARE\Classes is
 *          prefixed to this value when using SHCTX.
 * @param   _VALOPEN
 *          The path and args to launch the application.
 * @param   _VALICON
 *          The path to the binary that contains the icon group for the default icon
 *          followed by a comma and either the icon group's resource index or the icon
 *          group's resource id prefixed with a minus sign
 * @param   _DISPNAME
 *          The display name for the handler. If emtpy no value will be set.
 * @param   _ISPROTOCOL
 *          Sets protocol handler specific registry values when "true".
 *
 * $R3 = storage for SOFTWARE\Classes
 * $R4 = string value of the current registry key path.
 * $R5 = _KEY
 * $R6 = _VALOPEN
 * $R7 = _VALICON
 * $R8 = _DISPNAME
 * $R9 = _ISPROTOCOL
 */
!macro AddDisabledDDEHandlerValues

  !ifndef ${_MOZFUNC_UN}AddDisabledDDEHandlerValues
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}AddDisabledDDEHandlerValues "!insertmacro ${_MOZFUNC_UN}AddDisabledDDEHandlerValuesCall"

    Function ${_MOZFUNC_UN}AddDisabledDDEHandlerValues
      Exch $R9 ; true if a protocol handler
      Exch 1
      Exch $R8 ; FriendlyTypeName
      Exch 2
      Exch $R7 ; icon index
      Exch 3
      Exch $R6 ; shell\open\command
      Exch 4
      Exch $R5 ; reg key
      Push $R4 ;
      Push $R3 ; base reg class

      StrCpy $R3 "SOFTWARE\Classes"
      StrCmp "$R8" "" +6 +1
      ReadRegStr $R4 SHCTX "$R5" "FriendlyTypeName"

      StrCmp "$R4" "" +1 +3
      WriteRegStr SHCTX "$R3\$R5" "" "$R8"
      WriteRegStr SHCTX "$R3\$R5" "FriendlyTypeName" "$R8"

      StrCmp "$R9" "true" +1 +2
      WriteRegStr SHCTX "$R3\$R5" "URL Protocol" ""
      StrCpy $R4 ""
      ReadRegDWord $R4 SHCTX "$R3\$R5" "EditFlags"
      StrCmp $R4 "" +1 +3  ; Only add EditFlags if a value doesn't exist
      DeleteRegValue SHCTX "$R3\$R5" "EditFlags"
      WriteRegDWord SHCTX "$R3\$R5" "EditFlags" 0x00000002

      StrCmp "$R7" "" +2 +1
      WriteRegStr SHCTX "$R3\$R5\DefaultIcon" "" "$R7"

      ; Main command handler for the app
      WriteRegStr SHCTX "$R3\$R5\shell\open\command" "" "$R6"

      ; Drop support for DDE (bug 491947), and remove old dde entries if
      ; they exist.
      ;
      ; Note, changes in SHCTX should propegate to hkey classes root when
      ; current user or local machine entries are written. Windows will also
      ; attempt to propegate entries when a handler is used. CR entries are a
      ; combination of LM and CU, with CU taking priority.
      ;
      ; To disable dde, an empty shell/ddeexec key must be created in current
      ; user or local machine. Unfortunately, settings have various different
      ; behaviors depending on the windows version. The following code attempts
      ; to address these differences.
      ;
      ; On XP (no SP, SP1, SP2), Vista: An empty default string
      ; must be set under ddeexec. Empty strings propagate to CR.
      ;
      ; Win7: IE does not configure ddeexec, so issues with left over ddeexec keys
      ; in LM are reduced. We configure an empty ddeexec key with an empty default
      ; string in CU to be sure.
      ;
      DeleteRegKey SHCTX "SOFTWARE\Classes\$R5\shell\open\ddeexec"
      WriteRegStr SHCTX "SOFTWARE\Classes\$R5\shell\open\ddeexec" "" ""

      ClearErrors

      Pop $R3
      Pop $R4
      Exch $R5
      Exch 4
      Exch $R6
      Exch 3
      Exch $R7
      Exch 2
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro AddDisabledDDEHandlerValuesCall _KEY _VALOPEN _VALICON _DISPNAME _ISPROTOCOL
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Push "${_VALOPEN}"
  Push "${_VALICON}"
  Push "${_DISPNAME}"
  Push "${_ISPROTOCOL}"
  Call AddDisabledDDEHandlerValues
  !verbose pop
!macroend

!macro un.AddDisabledDDEHandlerValuesCall _KEY _VALOPEN _VALICON _DISPNAME _ISPROTOCOL
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Push "${_VALOPEN}"
  Push "${_VALICON}"
  Push "${_DISPNAME}"
  Push "${_ISPROTOCOL}"
  Call un.AddDisabledDDEHandlerValues
  !verbose pop
!macroend

!macro un.AddDisabledDDEHandlerValues
  !ifndef un.AddDisabledDDEHandlerValues
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro AddDisabledDDEHandlerValues

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend


################################################################################
# Macros for handling DLL registration

!macro RegisterDLL DLL

  ; The x64 regsvr32.exe registers x86 DLL's properly on Windows Vista and above
  ; (not on Windows XP http://support.microsoft.com/kb/282747) so just use it
  ; when installing on an x64 systems even when installing an x86 application.
  ${If} ${RunningX64}
    ${DisableX64FSRedirection}
    ExecWait '"$SYSDIR\regsvr32.exe" /s "${DLL}"'
    ${EnableX64FSRedirection}
  ${Else}
    RegDLL "${DLL}"
  ${EndIf}

!macroend

!macro UnregisterDLL DLL

  ; The x64 regsvr32.exe registers x86 DLL's properly on Windows Vista and above
  ; (not on Windows XP http://support.microsoft.com/kb/282747) so just use it
  ; when installing on an x64 systems even when installing an x86 application.
  ${If} ${RunningX64}
    ${DisableX64FSRedirection}
    ExecWait '"$SYSDIR\regsvr32.exe" /s /u "${DLL}"'
    ${EnableX64FSRedirection}
  ${Else}
    UnRegDLL "${DLL}"
  ${EndIf}

!macroend

!define RegisterDLL `!insertmacro RegisterDLL`
!define UnregisterDLL `!insertmacro UnregisterDLL`


################################################################################
# Macros for retrieving existing install paths

/**
 * Finds a second installation of the application so we can make informed
 * decisions about registry operations. This uses SHCTX to determine the
 * registry hive so you must call SetShellVarContext first.
 *
 * @param   _KEY
 *          The registry subkey (typically this will be Software\Mozilla).
 * @return  _RESULT
 *          false if a second install isn't found, path to the main exe if a
 *          second install is found.
 *
 * $R3 = stores the long path to $INSTDIR
 * $R4 = counter for the outer loop's EnumRegKey
 * $R5 = return value from ReadRegStr and RemoveQuotesFromPath
 * $R6 = return value from GetParent
 * $R7 = return value from the loop's EnumRegKey
 * $R8 = storage for _KEY
 * $R9 = _KEY and _RESULT
 */
!macro GetSecondInstallPath

  !ifndef ${_MOZFUNC_UN}GetSecondInstallPath
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !insertmacro ${_MOZFUNC_UN_TMP}GetParent
    !insertmacro ${_MOZFUNC_UN_TMP}RemoveQuotesFromPath
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}GetSecondInstallPath "!insertmacro ${_MOZFUNC_UN}GetSecondInstallPathCall"

    Function ${_MOZFUNC_UN}GetSecondInstallPath
      Exch $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4
      Push $R3

      ${${_MOZFUNC_UN}GetLongPath} "$INSTDIR" $R3

      StrCpy $R4 0       ; set the counter for the loop to 0
      StrCpy $R8 "$R9"   ; Registry key path to search
      StrCpy $R9 "false" ; default return value

      loop:
      EnumRegKey $R7 SHCTX $R8 $R4
      StrCmp $R7 "" end +1  ; if empty there are no more keys to enumerate
      IntOp $R4 $R4 + 1     ; increment the loop's counter
      ClearErrors
      ReadRegStr $R5 SHCTX "$R8\$R7\bin" "PathToExe"
      IfErrors loop

      ${${_MOZFUNC_UN}RemoveQuotesFromPath} "$R5" $R5

      IfFileExists "$R5" +1 loop
      ${${_MOZFUNC_UN}GetLongPath} "$R5" $R5
      ${${_MOZFUNC_UN}GetParent} "$R5" $R6
      StrCmp "$R6" "$R3" loop +1
      StrCmp "$R6\${FileMainEXE}" "$R5" +1 loop
      StrCpy $R9 "$R5"

      end:
      ClearErrors

      Pop $R3
      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro GetSecondInstallPathCall _KEY _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Call GetSecondInstallPath
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro un.GetSecondInstallPathCall _KEY _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Call un.GetSecondInstallPath
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro un.GetSecondInstallPath
  !ifndef un.GetSecondInstallPath
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro GetSecondInstallPath

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Finds an existing installation path for the application based on the
 * application's executable name so we can default to using this path for the
 * install. If there is zero or more than one installation of the application
 * then we default to the default installation path. This uses SHCTX to
 * determine the registry hive to read from so you must call SetShellVarContext
 * first.
 *
 * @param   _KEY
 *          The registry subkey (typically this will be Software\Mozilla\App Name).
 * @return  _RESULT
 *          false if a single install location for this app name isn't found,
 *          path to the install directory if a single install location is found.
 *
 * $R5 = counter for the loop's EnumRegKey
 * $R6 = return value from EnumRegKey
 * $R7 = return value from ReadRegStr
 * $R8 = storage for _KEY
 * $R9 = _KEY and _RESULT
 */
!macro GetSingleInstallPath

  !ifndef ${_MOZFUNC_UN}GetSingleInstallPath
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !insertmacro ${_MOZFUNC_UN_TMP}GetParent
    !insertmacro ${_MOZFUNC_UN_TMP}RemoveQuotesFromPath
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}GetSingleInstallPath "!insertmacro ${_MOZFUNC_UN}GetSingleInstallPathCall"

    Function ${_MOZFUNC_UN}GetSingleInstallPath
      Exch $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5

      StrCpy $R8 $R9
      StrCpy $R9 "false"
      StrCpy $R5 0  ; set the counter for the loop to 0

      loop:
      ClearErrors
      EnumRegKey $R6 SHCTX $R8 $R5
      IfErrors cleanup
      StrCmp $R6 "" cleanup +1  ; if empty there are no more keys to enumerate
      IntOp $R5 $R5 + 1         ; increment the loop's counter
      ClearErrors
      ReadRegStr $R7 SHCTX "$R8\$R6\Main" "PathToExe"
      IfErrors loop
      ${${_MOZFUNC_UN}RemoveQuotesFromPath} "$R7" $R7
      GetFullPathName $R7 "$R7"
      IfErrors loop

      StrCmp "$R9" "false" +1 +3
      StrCpy $R9 "$R7"
      GoTo Loop

      StrCpy $R9 "false"

      cleanup:
      StrCmp $R9 "false" end +1
      ${${_MOZFUNC_UN}GetLongPath} "$R9" $R9
      ${${_MOZFUNC_UN}GetParent} "$R9" $R9

      end:
      ClearErrors

      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro GetSingleInstallPathCall _KEY _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Call GetSingleInstallPath
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro un.GetSingleInstallPathCall _KEY _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Call un.GetSingleInstallPath
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro un.GetSingleInstallPath
  !ifndef un.GetSingleInstallPath
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro GetSingleInstallPath

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend


################################################################################
# Macros for working with the file system

/**
 * Attempts to delete a file if it exists. This will fail if the file is in use.
 *
 * @param   _FILE
 *          The path to the file that is to be deleted.
 */
!macro DeleteFile _FILE
  ${If} ${FileExists} "${_FILE}"
    Delete "${_FILE}"
  ${EndIf}
!macroend
!define DeleteFile "!insertmacro DeleteFile"

/**
 * Removes a directory if it exists and is empty.
 *
 * @param   _DIR
 *          The path to the directory that is to be removed.
 */
!macro RemoveDir _DIR
  ${If} ${FileExists} "${_DIR}"
    RmDir "${_DIR}"
  ${EndIf}
!macroend
!define RemoveDir "!insertmacro RemoveDir"

/**
 * Checks whether it is possible to create and delete a directory and a file in
 * the install directory. Creation and deletion of files and directories are
 * checked since a user may have rights for one and not the other. If creation
 * and deletion of a file and a directory are successful this macro will return
 * true... if not, this it return false.
 *
 * @return  _RESULT
 *          true if files and directories can be created and deleted in the
 *          install directory otherwise false.
 *
 * $R8 = temporary filename in the installation directory returned from
 *       GetTempFileName.
 * $R9 = _RESULT
 */
!macro CanWriteToInstallDir

  !ifndef ${_MOZFUNC_UN}CanWriteToInstallDir
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}CanWriteToInstallDir "!insertmacro ${_MOZFUNC_UN}CanWriteToInstallDirCall"

    Function ${_MOZFUNC_UN}CanWriteToInstallDir
      Push $R9
      Push $R8

      StrCpy $R9 "true"

      ; IfFileExists returns false for $INSTDIR when $INSTDIR is the root of a
      ; UNC path so always try to create $INSTDIR
      CreateDirectory "$INSTDIR\"
      GetTempFileName $R8 "$INSTDIR\"

      ${Unless} ${FileExists} $R8 ; Can files be created?
        StrCpy $R9 "false"
        Goto done
      ${EndUnless}

      Delete $R8
      ${If} ${FileExists} $R8 ; Can files be deleted?
        StrCpy $R9 "false"
        Goto done
      ${EndIf}

      CreateDirectory $R8
      ${Unless} ${FileExists} $R8  ; Can directories be created?
        StrCpy $R9 "false"
        Goto done
      ${EndUnless}

      RmDir $R8
      ${If} ${FileExists} $R8  ; Can directories be deleted?
        StrCpy $R9 "false"
        Goto done
      ${EndIf}

      done:

      RmDir "$INSTDIR\" ; Only remove $INSTDIR if it is empty
      ClearErrors

      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro CanWriteToInstallDirCall _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call CanWriteToInstallDir
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro un.CanWriteToInstallDirCall _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.CanWriteToInstallDir
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro un.CanWriteToInstallDir
  !ifndef un.CanWriteToInstallDir
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro CanWriteToInstallDir

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Checks whether there is sufficient free space available for the installation
 * directory using GetDiskFreeSpaceExW which respects disk quotas. This macro
 * will calculate the size of all sections that are selected, compare that with
 * the free space available, and if there is sufficient free space it will
 * return true... if not, it will return false.
 *
 * @return  _RESULT
 *          "true" if there is sufficient free space otherwise "false".
 *
 * $R5 = return value from SectionGetSize
 * $R6 = return value from SectionGetFlags
 *       return value from an 'and' comparison of SectionGetFlags (1=selected)
 *       return value for lpFreeBytesAvailable from GetDiskFreeSpaceExW
 *       return value for System::Int64Op $R6 / 1024
 *       return value for System::Int64Op $R6 > $R8
 * $R7 = the counter for enumerating the sections
 *       the temporary file name for the directory created under $INSTDIR passed
 *       to GetDiskFreeSpaceExW.
 * $R8 = sum in KB of all selected sections
 * $R9 = _RESULT
 */
!macro CheckDiskSpace

  !ifndef ${_MOZFUNC_UN}CheckDiskSpace
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}CheckDiskSpace "!insertmacro ${_MOZFUNC_UN}CheckDiskSpaceCall"

    Function ${_MOZFUNC_UN}CheckDiskSpace
      Push $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5

      ClearErrors

      StrCpy $R9 "true" ; default return value
      StrCpy $R8 "0"    ; sum in KB of all selected sections
      StrCpy $R7 "0"    ; counter for enumerating sections

      ; Enumerate the sections and sum up the sizes of the sections that are
      ; selected.
      SectionGetFlags $R7 $R6
      IfErrors +7 +1
      IntOp $R6 ${SF_SELECTED} & $R6
      IntCmp $R6 0 +3 +1 +1
      SectionGetSize $R7 $R5
      IntOp $R8 $R8 + $R5
      IntOp $R7 $R7 + 1
      GoTo -7

      ; The directory passed to GetDiskFreeSpaceExW must exist for the call to
      ; succeed.  Since the CanWriteToInstallDir macro is called prior to this
      ; macro the call to CreateDirectory will always succeed.

      ; IfFileExists returns false for $INSTDIR when $INSTDIR is the root of a
      ; UNC path so always try to create $INSTDIR
      CreateDirectory "$INSTDIR\"
      GetTempFileName $R7 "$INSTDIR\"
      Delete "$R7"
      CreateDirectory "$R7"

      System::Call 'kernel32::GetDiskFreeSpaceExW(w, *l, *l, *l) i(R7, .R6, ., .) .'

      ; Convert to KB for comparison with $R8 which is in KB
      System::Int64Op $R6 / 1024
      Pop $R6

      System::Int64Op $R6 > $R8
      Pop $R6

      IntCmp $R6 1 end +1 +1
      StrCpy $R9 "false"

      end:
      RmDir "$R7"
      RmDir "$INSTDIR\" ; Only remove $INSTDIR if it is empty

      ClearErrors

      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro CheckDiskSpaceCall _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call CheckDiskSpace
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro un.CheckDiskSpaceCall _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.CheckDiskSpace
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro un.CheckDiskSpace
  !ifndef un.CheckDiskSpace
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro CheckDiskSpace

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
* Returns the path found within a passed in string. The path is quoted or not
* with the exception of an unquoted non 8dot3 path without arguments that is
* also not a DefaultIcon path, is a 8dot3 path or not, has command line
* arguments, or is a registry DefaultIcon path (e.g. <path to binary>,# where #
* is the icon's resuorce id). The string does not need to be a valid path or
* exist. It is up to the caller to pass in a string of one of the forms noted
* above and to verify existence if necessary.
*
* Examples:
* In:  C:\PROGRA~1\MOZILL~1\FIREFOX.EXE -flag "%1"
* In:  C:\PROGRA~1\MOZILL~1\FIREFOX.EXE,0
* In:  C:\PROGRA~1\MOZILL~1\FIREFOX.EXE
* In:  "C:\PROGRA~1\MOZILL~1\FIREFOX.EXE"
* In:  "C:\PROGRA~1\MOZILL~1\FIREFOX.EXE" -flag "%1"
* Out: C:\PROGRA~1\MOZILL~1\FIREFOX.EXE
*
* In:  "C:\Program Files\Mozilla Firefox\firefox.exe" -flag "%1"
* In:  C:\Program Files\Mozilla Firefox\firefox.exe,0
* In:  "C:\Program Files\Mozilla Firefox\firefox.exe"
* Out: C:\Program Files\Mozilla Firefox\firefox.exe
*
* @param   _IN_PATH
*          The string containing the path.
* @param   _OUT_PATH
*          The register to store the path to.
*
* $R7 = counter for the outer loop's EnumRegKey
* $R8 = return value from ReadRegStr
* $R9 = _IN_PATH and _OUT_PATH
*/
!macro GetPathFromString

  !ifndef ${_MOZFUNC_UN}GetPathFromString
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}GetPathFromString "!insertmacro ${_MOZFUNC_UN}GetPathFromStringCall"

    Function ${_MOZFUNC_UN}GetPathFromString
      Exch $R9
      Push $R8
      Push $R7

      StrCpy $R7 0          ; Set the counter to 0.

      ; Handle quoted paths with arguments.
      StrCpy $R8 $R9 1      ; Copy the first char.
      StrCmp $R8 '"' +2 +1  ; Is it a "?
      StrCmp $R8 "'" +1 +9  ; Is it a '?
      StrCpy $R9 $R9 "" 1   ; Remove the first char.
      IntOp $R7 $R7 + 1     ; Increment the counter.
      StrCpy $R8 $R9 1 $R7  ; Starting from the counter copy the next char.
      StrCmp $R8 "" end +1  ; Are there no more chars?
      StrCmp $R8 '"' +2 +1  ; Is it a " char?
      StrCmp $R8 "'" +1 -4  ; Is it a ' char?
      StrCpy $R9 $R9 $R7    ; Copy chars up to the counter.
      GoTo end

      ; Handle DefaultIcon paths. DefaultIcon paths are not quoted and end with
      ; a , and a number.
      IntOp $R7 $R7 - 1     ; Decrement the counter.
      StrCpy $R8 $R9 1 $R7  ; Copy one char from the end minus the counter.
      StrCmp $R8 '' +4 +1   ; Are there no more chars?
      StrCmp $R8 ',' +1 -3  ; Is it a , char?
      StrCpy $R9 $R9 $R7    ; Copy chars up to the end minus the counter.
      GoTo end

      ; Handle unquoted paths with arguments. An unquoted path with arguments
      ; must be an 8dot3 path.
      StrCpy $R7 -1          ; Set the counter to -1 so it will start at 0.
      IntOp $R7 $R7 + 1      ; Increment the counter.
      StrCpy $R8 $R9 1 $R7   ; Starting from the counter copy the next char.
      StrCmp $R8 "" end +1   ; Are there no more chars?
      StrCmp $R8 " " +1 -3   ; Is it a space char?
      StrCpy $R9 $R9 $R7     ; Copy chars up to the counter.

      end:
      ClearErrors

      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro GetPathFromStringCall _IN_PATH _OUT_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_IN_PATH}"
  Call GetPathFromString
  Pop ${_OUT_PATH}
  !verbose pop
!macroend

!macro un.GetPathFromStringCall _IN_PATH _OUT_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_IN_PATH}"
  Call un.GetPathFromString
  Pop ${_OUT_PATH}
  !verbose pop
!macroend

!macro un.GetPathFromString
  !ifndef un.GetPathFromString
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro GetPathFromString

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Removes the quotes from each end of a string if present.
 *
 * @param   _IN_PATH
 *          The string containing the path.
 * @param   _OUT_PATH
 *          The register to store the long path.
 *
 * $R7 = storage for single character comparison
 * $R8 = storage for _IN_PATH
 * $R9 = _IN_PATH and _OUT_PATH
 */
!macro RemoveQuotesFromPath

  !ifndef ${_MOZFUNC_UN}RemoveQuotesFromPath
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}RemoveQuotesFromPath "!insertmacro ${_MOZFUNC_UN}RemoveQuotesFromPathCall"

    Function ${_MOZFUNC_UN}RemoveQuotesFromPath
      Exch $R9
      Push $R8
      Push $R7

      StrCpy $R7 "$R9" 1
      StrCmp $R7 "$\"" +1 +2
      StrCpy $R9 "$R9" "" 1

      StrCpy $R7 "$R9" "" -1
      StrCmp $R7 "$\"" +1 +2
      StrCpy $R9 "$R9" -1

      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro RemoveQuotesFromPathCall _IN_PATH _OUT_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_IN_PATH}"
  Call RemoveQuotesFromPath
  Pop ${_OUT_PATH}
  !verbose pop
!macroend

!macro un.RemoveQuotesFromPathCall _IN_PATH _OUT_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_IN_PATH}"
  Call un.RemoveQuotesFromPath
  Pop ${_OUT_PATH}
  !verbose pop
!macroend

!macro un.RemoveQuotesFromPath
  !ifndef un.RemoveQuotesFromPath
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro RemoveQuotesFromPath

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Returns the long path for an existing file or directory. GetLongPathNameW
 * may not be available on Win95 if Microsoft Layer for Unicode is not
 * installed and GetFullPathName only returns a long path for the last file or
 * directory that doesn't end with a \ in the path that it is passed. If the
 * path does not exist on the file system this will return an empty string. To
 * provide a consistent result trailing back-slashes are always removed.
 *
 * Note: 1024 used by GetLongPathNameW is the maximum NSIS string length.
 *
 * @param   _IN_PATH
 *          The string containing the path.
 * @param   _OUT_PATH
 *          The register to store the long path.
 *
 * $R4 = counter value when the previous \ was found
 * $R5 = directory or file name found during loop
 * $R6 = return value from GetLongPathNameW and loop counter
 * $R7 = long path from GetLongPathNameW and single char from path for comparison
 * $R8 = storage for _IN_PATH
 * $R9 = _IN_PATH _OUT_PATH
 */
!macro GetLongPath

  !ifndef ${_MOZFUNC_UN}GetLongPath
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}GetLongPath "!insertmacro ${_MOZFUNC_UN}GetLongPathCall"

    Function ${_MOZFUNC_UN}GetLongPath
      Exch $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4

      ClearErrors

      GetFullPathName $R8 "$R9"
      IfErrors end_GetLongPath +1 ; If the path doesn't exist return an empty string.

      System::Call 'kernel32::GetLongPathNameW(w R8, w .R7, i 1024)i .R6'
      StrCmp "$R7" "" +4 +1 ; Empty string when GetLongPathNameW is not present.
      StrCmp $R6 0 +3 +1    ; Should never equal 0 since the path exists.
      StrCpy $R9 "$R7"
      GoTo end_GetLongPath

      ; Do it the hard way.
      StrCpy $R4 0     ; Stores the position in the string of the last \ found.
      StrCpy $R6 -1    ; Set the counter to -1 so it will start at 0.

      loop_GetLongPath:
      IntOp $R6 $R6 + 1      ; Increment the counter.
      StrCpy $R7 $R8 1 $R6   ; Starting from the counter copy the next char.
      StrCmp $R7 "" +2 +1    ; Are there no more chars?
      StrCmp $R7 "\" +1 -3   ; Is it a \?

      ; Copy chars starting from the previously found \ to the counter.
      StrCpy $R5 $R8 $R6 $R4

      ; If this is the first \ found we want to swap R9 with R5 so a \ will
      ; be appended to the drive letter and colon (e.g. C: will become C:\).
      StrCmp $R4 0 +1 +3
      StrCpy $R9 $R5
      StrCpy $R5 ""

      GetFullPathName $R9 "$R9\$R5"

      StrCmp $R7 "" end_GetLongPath +1 ; Are there no more chars?

      ; Store the counter for the current \ and prefix it for StrCpy operations.
      StrCpy $R4 "+$R6"
      IntOp $R6 $R6 + 1      ; Increment the counter so we skip over the \.
      StrCpy $R8 $R8 "" $R6  ; Copy chars starting from the counter to the end.
      StrCpy $R6 -1          ; Reset the counter to -1 so it will start over at 0.
      GoTo loop_GetLongPath

      end_GetLongPath:
      ; If there is a trailing slash remove it
      StrCmp $R9 "" +4 +1
      StrCpy $R8 "$R9" "" -1
      StrCmp $R8 "\" +1 +2
      StrCpy $R9 "$R9" -1

      ClearErrors

      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro GetLongPathCall _IN_PATH _OUT_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_IN_PATH}"
  Call GetLongPath
  Pop ${_OUT_PATH}
  !verbose pop
!macroend

!macro un.GetLongPathCall _IN_PATH _OUT_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_IN_PATH}"
  Call un.GetLongPath
  Pop ${_OUT_PATH}
  !verbose pop
!macroend

!macro un.GetLongPath
  !ifndef un.GetLongPath
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro GetLongPath

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend


################################################################################
# Macros for cleaning up the registry and file system

/**
 * Removes registry keys that reference this install location and for paths that
 * no longer exist. This uses SHCTX to determine the registry hive so you must
 * call SetShellVarContext first.
 *
 * @param   _KEY
 *          The registry subkey (typically this will be Software\Mozilla).
 *
 * XXXrstrong - there is the potential for Key: Software/Mozilla/AppName,
 * ValueName: CurrentVersion, ValueData: AppVersion to reference a key that is
 * no longer available due to this cleanup. This should be no worse than prior
 * to this reg cleanup since the referenced key would be for an app that is no
 * longer installed on the system.
 *
 * $R0 = on x64 systems set to 'false' at the beginning of the macro when
 *       enumerating the x86 registry view and set to 'true' when enumerating
 *       the x64 registry view.
 * $R1 = stores the long path to $INSTDIR
 * $R2 = return value from the stack from the GetParent and GetLongPath macros
 * $R3 = return value from the outer loop's EnumRegKey
 * $R4 = return value from the inner loop's EnumRegKey
 * $R5 = return value from ReadRegStr
 * $R6 = counter for the outer loop's EnumRegKey
 * $R7 = counter for the inner loop's EnumRegKey
 * $R8 = return value from the stack from the RemoveQuotesFromPath macro
 * $R9 = _KEY
 */
!macro RegCleanMain

  !ifndef ${_MOZFUNC_UN}RegCleanMain
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetParent
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !insertmacro ${_MOZFUNC_UN_TMP}RemoveQuotesFromPath
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}RegCleanMain "!insertmacro ${_MOZFUNC_UN}RegCleanMainCall"

    Function ${_MOZFUNC_UN}RegCleanMain
      Exch $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4
      Push $R3
      Push $R2
      Push $R1
      Push $R0

      ${${_MOZFUNC_UN}GetLongPath} "$INSTDIR" $R1
      StrCpy $R6 0  ; set the counter for the outer loop to 0

      ${If} ${RunningX64}
        StrCpy $R0 "false"
        ; Set the registry to the 32 bit registry for 64 bit installations or to
        ; the 64 bit registry for 32 bit installations at the beginning so it can
        ; easily be set back to the correct registry view when finished.
        !ifdef HAVE_64BIT_OS
          SetRegView 32
        !else
          SetRegView 64
        !endif
      ${EndIf}

      outerloop:
      EnumRegKey $R3 SHCTX $R9 $R6
      StrCmp $R3 "" end +1  ; if empty there are no more keys to enumerate
      IntOp $R6 $R6 + 1     ; increment the outer loop's counter
      ClearErrors
      ReadRegStr $R5 SHCTX "$R9\$R3\bin" "PathToExe"
      IfErrors 0 outercontinue
      StrCpy $R7 0  ; set the counter for the inner loop to 0

      innerloop:
      EnumRegKey $R4 SHCTX "$R9\$R3" $R7
      StrCmp $R4 "" outerloop +1  ; if empty there are no more keys to enumerate
      IntOp $R7 $R7 + 1  ; increment the inner loop's counter
      ClearErrors
      ReadRegStr $R5 SHCTX "$R9\$R3\$R4\Main" "PathToExe"
      IfErrors innerloop

      ${${_MOZFUNC_UN}RemoveQuotesFromPath} "$R5" $R8
      ${${_MOZFUNC_UN}GetParent} "$R8" $R2
      ${${_MOZFUNC_UN}GetLongPath} "$R2" $R2
      IfFileExists "$R2" +1 innerloop
      StrCmp "$R2" "$R1" +1 innerloop

      ClearErrors
      DeleteRegKey SHCTX "$R9\$R3\$R4"
      IfErrors innerloop
      IntOp $R7 $R7 - 1 ; decrement the inner loop's counter when the key is deleted successfully.
      ClearErrors
      DeleteRegKey /ifempty SHCTX "$R9\$R3"
      IfErrors innerloop outerdecrement

      outercontinue:
      ${${_MOZFUNC_UN}RemoveQuotesFromPath} "$R5" $R8
      ${${_MOZFUNC_UN}GetParent} "$R8" $R2
      ${${_MOZFUNC_UN}GetLongPath} "$R2" $R2
      IfFileExists "$R2" +1 outerloop
      StrCmp "$R2" "$R1" +1 outerloop

      ClearErrors
      DeleteRegKey SHCTX "$R9\$R3"
      IfErrors outerloop

      outerdecrement:
      IntOp $R6 $R6 - 1 ; decrement the outer loop's counter when the key is deleted successfully.
      GoTo outerloop

      end:
      ${If} ${RunningX64}
      ${AndIf} "$R0" == "false"
        ; Set the registry to the correct view.
        !ifdef HAVE_64BIT_OS
          SetRegView 64
        !else
          SetRegView 32
        !endif

        StrCpy $R6 0  ; set the counter for the outer loop to 0
        StrCpy $R0 "true"
        GoTo outerloop
      ${EndIf}

      ClearErrors

      Pop $R0
      Pop $R1
      Pop $R2
      Pop $R3
      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro RegCleanMainCall _KEY
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Call RegCleanMain
  !verbose pop
!macroend

!macro un.RegCleanMainCall _KEY
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_KEY}"
  Call un.RegCleanMain
  !verbose pop
!macroend

!macro un.RegCleanMain
  !ifndef un.RegCleanMain
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro RegCleanMain

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Removes all registry keys from \Software\Windows\CurrentVersion\Uninstall
 * that reference this install location in both the 32 bit and 64 bit registry
 * view. This macro uses SHCTX to determine the registry hive so you must call
 * SetShellVarContext first.
 *
 * $R3 = on x64 systems set to 'false' at the beginning of the macro when
 *       enumerating the x86 registry view and set to 'true' when enumerating
 *       the x64 registry view.
 * $R4 = stores the long path to $INSTDIR
 * $R5 = return value from ReadRegStr
 * $R6 = string for the base reg key (e.g. Software\Microsoft\Windows\CurrentVersion\Uninstall)
 * $R7 = return value from EnumRegKey
 * $R8 = counter for EnumRegKey
 * $R9 = return value from the stack from the RemoveQuotesFromPath and GetLongPath macros
 */
!macro RegCleanUninstall

  !ifndef ${_MOZFUNC_UN}RegCleanUninstall
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !insertmacro ${_MOZFUNC_UN_TMP}RemoveQuotesFromPath
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}RegCleanUninstall "!insertmacro ${_MOZFUNC_UN}RegCleanUninstallCall"

    Function ${_MOZFUNC_UN}RegCleanUninstall
      Push $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4
      Push $R3

      ${${_MOZFUNC_UN}GetLongPath} "$INSTDIR" $R4
      StrCpy $R6 "Software\Microsoft\Windows\CurrentVersion\Uninstall"
      StrCpy $R7 ""
      StrCpy $R8 0

      ${If} ${RunningX64}
        StrCpy $R3 "false"
        ; Set the registry to the 32 bit registry for 64 bit installations or to
        ; the 64 bit registry for 32 bit installations at the beginning so it can
        ; easily be set back to the correct registry view when finished.
        !ifdef HAVE_64BIT_OS
          SetRegView 32
        !else
          SetRegView 64
        !endif
      ${EndIf}

      loop:
      EnumRegKey $R7 SHCTX $R6 $R8
      StrCmp $R7 "" end +1
      IntOp $R8 $R8 + 1 ; Increment the counter
      ClearErrors
      ReadRegStr $R5 SHCTX "$R6\$R7" "InstallLocation"
      IfErrors loop
      ${${_MOZFUNC_UN}RemoveQuotesFromPath} "$R5" $R9
      ${${_MOZFUNC_UN}GetLongPath} "$R9" $R9
      StrCmp "$R9" "$R4" +1 loop
      ClearErrors
      DeleteRegKey SHCTX "$R6\$R7"
      IfErrors loop +1
      IntOp $R8 $R8 - 1 ; Decrement the counter on successful deletion
      GoTo loop

      end:
      ${If} ${RunningX64}
      ${AndIf} "$R3" == "false"
        ; Set the registry to the correct view.
        !ifdef HAVE_64BIT_OS
          SetRegView 64
        !else
          SetRegView 32
        !endif

        StrCpy $R7 ""
        StrCpy $R8 0
        StrCpy $R3 "true"
        GoTo loop
      ${EndIf}

      ClearErrors

      Pop $R3
      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Pop $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro RegCleanUninstallCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call RegCleanUninstall
  !verbose pop
!macroend

!macro un.RegCleanUninstallCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.RegCleanUninstall
  !verbose pop
!macroend

!macro un.RegCleanUninstall
  !ifndef un.RegCleanUninstall
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro RegCleanUninstall

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Removes an application specific handler registry key under Software\Classes
 * for both HKCU and HKLM when its open command refers to this install
 * location or the install location doesn't exist.
 *
 * @param   _HANDLER_NAME
 *          The registry name for the handler.
 *
 * $R7 = stores the long path to the $INSTDIR
 * $R8 = stores the path to the open command's parent directory
 * $R9 = _HANDLER_NAME
 */
!macro RegCleanAppHandler

  !ifndef ${_MOZFUNC_UN}RegCleanAppHandler
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !insertmacro ${_MOZFUNC_UN_TMP}GetParent
    !insertmacro ${_MOZFUNC_UN_TMP}GetPathFromString
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}RegCleanAppHandler "!insertmacro ${_MOZFUNC_UN}RegCleanAppHandlerCall"

    Function ${_MOZFUNC_UN}RegCleanAppHandler
      Exch $R9
      Push $R8
      Push $R7

      ClearErrors
      ReadRegStr $R8 HKCU "Software\Classes\$R9\shell\open\command" ""
      IfErrors next +1
      ${${_MOZFUNC_UN}GetPathFromString} "$R8" $R8
      ${${_MOZFUNC_UN}GetParent} "$R8" $R8
      IfFileExists "$R8" +3 +1
      DeleteRegKey HKCU "Software\Classes\$R9"
      GoTo next

      ${${_MOZFUNC_UN}GetLongPath} "$R8" $R8
      ${${_MOZFUNC_UN}GetLongPath} "$INSTDIR" $R7
      StrCmp "$R7" "$R8" +1 next
      DeleteRegKey HKCU "Software\Classes\$R9"

      next:
      ReadRegStr $R8 HKLM "Software\Classes\$R9\shell\open\command" ""
      IfErrors end
      ${${_MOZFUNC_UN}GetPathFromString} "$R8" $R8
      ${${_MOZFUNC_UN}GetParent} "$R8" $R8
      IfFileExists "$R8" +3 +1
      DeleteRegKey HKLM "Software\Classes\$R9"
      GoTo end

      ${${_MOZFUNC_UN}GetLongPath} "$R8" $R8
      ${${_MOZFUNC_UN}GetLongPath} "$INSTDIR" $R7
      StrCmp "$R7" "$R8" +1 end
      DeleteRegKey HKLM "Software\Classes\$R9"

      end:

      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro RegCleanAppHandlerCall _HANDLER_NAME
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_HANDLER_NAME}"
  Call RegCleanAppHandler
  !verbose pop
!macroend

!macro un.RegCleanAppHandlerCall _HANDLER_NAME
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_HANDLER_NAME}"
  Call un.RegCleanAppHandler
  !verbose pop
!macroend

!macro un.RegCleanAppHandler
  !ifndef un.RegCleanAppHandler
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro RegCleanAppHandler

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Cleans up the registry for a protocol handler when its open command
 * refers to this install location. For HKCU the registry key is deleted
 * and for HKLM the values set by the application are deleted.
 *
 * @param   _HANDLER_NAME
 *          The registry name for the handler.
 *
 * $R7 = stores the long path to $INSTDIR
 * $R8 = stores the the long path to the open command's parent directory
 * $R9 = _HANDLER_NAME
 */
!macro un.RegCleanProtocolHandler

  !ifndef un.RegCleanProtocolHandler
    !insertmacro un.GetLongPath
    !insertmacro un.GetParent
    !insertmacro un.GetPathFromString

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define un.RegCleanProtocolHandler "!insertmacro un.RegCleanProtocolHandlerCall"

    Function un.RegCleanProtocolHandler
      Exch $R9
      Push $R8
      Push $R7

      ReadRegStr $R8 HKCU "Software\Classes\$R9\shell\open\command" ""
      ${un.GetLongPath} "$INSTDIR" $R7

      StrCmp "$R8" "" next +1
      ${un.GetPathFromString} "$R8" $R8
      ${un.GetParent} "$R8" $R8
      ${un.GetLongPath} "$R8" $R8
      StrCmp "$R7" "$R8" +1 next
      DeleteRegKey HKCU "Software\Classes\$R9"

      next:
      ReadRegStr $R8 HKLM "Software\Classes\$R9\shell\open\command" ""
      StrCmp "$R8" "" end +1
      ${un.GetLongPath} "$INSTDIR" $R7
      ${un.GetPathFromString} "$R8" $R8
      ${un.GetParent} "$R8" $R8
      ${un.GetLongPath} "$R8" $R8
      StrCmp "$R7" "$R8" +1 end
      DeleteRegValue HKLM "Software\Classes\$R9\DefaultIcon" ""
      DeleteRegValue HKLM "Software\Classes\$R9\shell\open" ""
      DeleteRegValue HKLM "Software\Classes\$R9\shell\open\command" ""
      DeleteRegValue HKLM "Software\Classes\$R9\shell\ddeexec" ""
      DeleteRegValue HKLM "Software\Classes\$R9\shell\ddeexec\Application" ""
      DeleteRegValue HKLM "Software\Classes\$R9\shell\ddeexec\Topic" ""

      end:

      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro un.RegCleanProtocolHandlerCall _HANDLER_NAME
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_HANDLER_NAME}"
  Call un.RegCleanProtocolHandler
  !verbose pop
!macroend

/**
 * Cleans up the registry for a file handler when the passed in value equals
 * the default value for the file handler. For HKCU the registry key is deleted
 * and for HKLM the default value is deleted.
 *
 * @param   _HANDLER_NAME
 *          The registry name for the handler.
 * @param   _DEFAULT_VALUE
 *          The value to check for against the handler's default value.
 *
 * $R6 = stores the long path to $INSTDIR
 * $R7 = _DEFAULT_VALUE
 * $R9 = _HANDLER_NAME
 */
!macro RegCleanFileHandler

  !ifndef ${_MOZFUNC_UN}RegCleanFileHandler
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !insertmacro ${_MOZFUNC_UN_TMP}GetParent
    !insertmacro ${_MOZFUNC_UN_TMP}GetPathFromString
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}RegCleanFileHandler "!insertmacro ${_MOZFUNC_UN}RegCleanFileHandlerCall"

    Function ${_MOZFUNC_UN}RegCleanFileHandler
      Exch $R9
      Exch 1
      Exch $R8
      Push $R7

      ReadRegStr $R7 HKCU "Software\Classes\$R9" ""
      StrCmp "$R7" "$R8" +1 +2
      DeleteRegKey HKCU "Software\Classes\$R9"

      ReadRegStr $R7 HKLM "Software\Classes\$R9" ""
      StrCmp "$R7" "$R8" +1 +2
      DeleteRegValue HKLM "Software\Classes\$R9" ""

      ClearErrors

      Pop $R7
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro RegCleanFileHandlerCall _HANDLER_NAME _DEFAULT_VALUE
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_DEFAULT_VALUE}"
  Push "${_HANDLER_NAME}"
  Call RegCleanFileHandler
  !verbose pop
!macroend

!macro un.RegCleanFileHandlerCall _HANDLER_NAME _DEFAULT_VALUE
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_DEFAULT_VALUE}"
  Push "${_HANDLER_NAME}"
  Call un.RegCleanFileHandler
  !verbose pop
!macroend

!macro un.RegCleanFileHandler
  !ifndef un.RegCleanFileHandler
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro RegCleanFileHandler

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Checks if a handler's open command points to this installation directory.
 * Uses SHCTX to determine the registry hive (e.g. HKLM or HKCU) to check.
 *
 * @param   _HANDLER_NAME
 *          The registry name for the handler.
 * @param   _RESULT
 *          true if it is the handler's open command points to this
 *          installation directory and false if it does not.
 *
 * $R7 = stores the value of the open command and the path macros return values
 * $R8 = stores the handler's registry key name
 * $R9 = _DEFAULT_VALUE and _RESULT
 */
!macro IsHandlerForInstallDir

  !ifndef ${_MOZFUNC_UN}IsHandlerForInstallDir
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !insertmacro ${_MOZFUNC_UN_TMP}GetParent
    !insertmacro ${_MOZFUNC_UN_TMP}GetPathFromString
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}IsHandlerForInstallDir "!insertmacro ${_MOZFUNC_UN}IsHandlerForInstallDirCall"

    Function ${_MOZFUNC_UN}IsHandlerForInstallDir
      Exch $R9
      Push $R8
      Push $R7

      StrCpy $R8 "$R9"
      StrCpy $R9 "false"
      ReadRegStr $R7 SHCTX "Software\Classes\$R8\shell\open\command" ""

      ${If} $R7 != ""
        ${GetPathFromString} "$R7" $R7
        ${GetParent} "$R7" $R7
        ${GetLongPath} "$R7" $R7
        ${If} $R7 == $INSTDIR
          StrCpy $R9 "true"
        ${EndIf}
      ${EndIf}

      ClearErrors

      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro IsHandlerForInstallDirCall _HANDLER_NAME _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_HANDLER_NAME}"
  Call IsHandlerForInstallDir
  Pop "${_RESULT}"
  !verbose pop
!macroend

!macro un.IsHandlerForInstallDirCall _HANDLER_NAME _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_HANDLER_NAME}"
  Call un.IsHandlerForInstallDir
  Pop "${_RESULT}"
  !verbose pop
!macroend

!macro un.IsHandlerForInstallDir
  !ifndef un.IsHandlerForInstallDir
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro IsHandlerForInstallDir

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Removes the application's VirtualStore directory if present when the
 * installation directory is a sub-directory of the program files directory.
 *
 * $R4 = $PROGRAMFILES/$PROGRAMFILES64 for CleanVirtualStore_Internal
 * $R5 = various path values.
 * $R6 = length of the long path to $PROGRAMFILES32 or $PROGRAMFILES64
 * $R7 = long path to $PROGRAMFILES32 or $PROGRAMFILES64
 * $R8 = length of the long path to $INSTDIR
 * $R9 = long path to $INSTDIR
 */
!macro CleanVirtualStore

  !ifndef ${_MOZFUNC_UN}CleanVirtualStore
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}CleanVirtualStore "!insertmacro ${_MOZFUNC_UN}CleanVirtualStoreCall"

    Function ${_MOZFUNC_UN}CleanVirtualStore
      Push $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4

      ${${_MOZFUNC_UN}GetLongPath} "$INSTDIR" $R9
      ${If} "$R9" != ""
        StrLen $R8 "$R9"

        StrCpy $R4 $PROGRAMFILES32
        Call ${_MOZFUNC_UN}CleanVirtualStore_Internal

        ${If} ${RunningX64}
          StrCpy $R4 $PROGRAMFILES64
          Call ${_MOZFUNC_UN}CleanVirtualStore_Internal
        ${EndIf}

      ${EndIf}

      ClearErrors

      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Pop $R9
    FunctionEnd

    Function ${_MOZFUNC_UN}CleanVirtualStore_Internal
      ${${_MOZFUNC_UN}GetLongPath} "" $R7
      ${If} "$R7" != ""
        StrLen $R6 "$R7"
        ${If} $R8 < $R6
          ; Copy from the start of $INSTDIR the length of $PROGRAMFILES64
          StrCpy $R5 "$R9" $R6
          ${If} "$R5" == "$R7"
            ; Remove the drive letter and colon from the $INSTDIR long path
            StrCpy $R5 "$R9" "" 2
            StrCpy $R5 "$LOCALAPPDATA\VirtualStore$R5"
            ${${_MOZFUNC_UN}GetLongPath} "$R5" $R5
            ${If} "$R5" != ""
            ${AndIf} ${FileExists} "$R5"
              RmDir /r "$R5"
            ${EndIf}
          ${EndIf}
        ${EndIf}
      ${EndIf}
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro CleanVirtualStoreCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call CleanVirtualStore
  !verbose pop
!macroend

!macro un.CleanVirtualStoreCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.CleanVirtualStore
  !verbose pop
!macroend

!macro un.CleanVirtualStore
  !ifndef un.CleanVirtualStore
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro CleanVirtualStore

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * If present removes the updates directory located in the profile's local
 * directory for this installation.
 *
 * @param   _REL_PROFILE_PATH
 *          The relative path to the profile directory from $LOCALAPPDATA.
 *
 * $R4 = various path values.
 * $R5 = length of the long path to $PROGRAMFILES
 * $R6 = length of the long path to $INSTDIR
 * $R7 = long path to $PROGRAMFILES
 * $R8 = long path to $INSTDIR
 * $R9 = _REL_PROFILE_PATH
 */
!macro CleanUpdatesDir

  !ifndef ${_MOZFUNC_UN}CleanUpdatesDir
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}CleanUpdatesDir "!insertmacro ${_MOZFUNC_UN}CleanUpdatesDirCall"

    Function ${_MOZFUNC_UN}CleanUpdatesDir
      Exch $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4

      StrCmp $R9 "" end +1 ; The relative path to the app's profiles is required
      ${${_MOZFUNC_UN}GetLongPath} "$INSTDIR" $R8
      StrCmp $R8 "" end +1
      ${${_MOZFUNC_UN}GetLongPath} "$PROGRAMFILES" $R7
      StrCmp $R7 "" end +1

      StrLen $R6 "$R8"
      StrLen $R5 "$R7"
      ; Only continue If the length of $INSTDIR is greater than the length of
      ; $PROGRAMFILES
      IntCmp $R6 $R5 end end +1

      ; Copy from the start of $INSTDIR the length of $PROGRAMFILES
      StrCpy $R4 "$R8" $R5
      StrCmp "$R4" "$R7" +1 end ; Check if $INSTDIR is under $PROGRAMFILES

      ; Copy the relative path to $INSTDIR from $PROGRAMFILES
      StrCpy $R4 "$R8" "" $R5

      ; Concatenate the path to $LOCALAPPDATA the relative profile path and the
      ; relative path to $INSTDIR from $PROGRAMFILES
      StrCpy $R4 "$LOCALAPPDATA\$R9$R4"
      ${${_MOZFUNC_UN}GetLongPath} "$R4" $R4
      StrCmp $R4 "" end +1

      IfFileExists "$R4\updates" +1 end
      RmDir /r "$R4"

      end:
      ClearErrors

      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro CleanUpdatesDirCall _REL_PROFILE_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_REL_PROFILE_PATH}"
  Call CleanUpdatesDir
  !verbose pop
!macroend

!macro un.CleanUpdatesDirCall _REL_PROFILE_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_REL_PROFILE_PATH}"
  Call un.CleanUpdatesDir
  !verbose pop
!macroend

!macro un.CleanUpdatesDir
  !ifndef un.CleanUpdatesDir
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro CleanUpdatesDir

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Deletes all relative profiles specified in an application's profiles.ini and
 * performs various other cleanup.
 *
 * @param   _REL_PROFILE_PATH
 *          The relative path to the profile directory.
 *
 * $R6 = value of IsRelative read from profiles.ini
 * $R7 = value of Path to profile read from profiles.ini
 * $R8 = counter for reading profiles (e.g. Profile0, Profile1, etc.)
 * $R9 = _REL_PROFILE_PATH
 */
!macro DeleteRelativeProfiles

  !ifndef ${_MOZFUNC_UN}DeleteRelativeProfiles
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}WordReplace
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}DeleteRelativeProfiles "!insertmacro ${_MOZFUNC_UN}DeleteRelativeProfilesCall"

    Function ${_MOZFUNC_UN}DeleteRelativeProfiles
      Exch $R9
      Push $R8
      Push $R7
      Push $R6

      SetShellVarContext current
      StrCpy $R8 -1

      loop:
      IntOp $R8 $R8 + 1  ; Increment the counter.
      ReadINIStr $R7 "$APPDATA\$R9\profiles.ini" "Profile$R8" "Path"
      IfErrors end +1

      ; Only remove relative profiles
      ReadINIStr $R6 "$APPDATA\$R9\profiles.ini" "Profile$R8" "IsRelative"
      StrCmp "$R6" "1" +1 loop

      ; Relative paths in profiles.ini use / as a separator
      ${${_MOZFUNC_UN}WordReplace} "$R7" "/" "\" "+" $R7

      IfFileExists "$LOCALAPPDATA\$R9\$R7" +1 +2
      RmDir /r "$LOCALAPPDATA\$R9\$R7"
      IfFileExists "$APPDATA\$R9\$R7" +1 +2
      RmDir /r "$APPDATA\$R9\$R7"
      GoTo loop

      end:
      ; Remove profiles directory under LOCALAPPDATA (e.g. cache, etc.) since
      ; they are at times abandoned.
      RmDir /r "$LOCALAPPDATA\$R9\Profiles"
      RmDir /r "$APPDATA\$R9\Crash Reports"
      Delete "$APPDATA\$R9\profiles.ini"
      Delete "$APPDATA\$R9\console.log"
      Delete "$APPDATA\$R9\pluginreg.dat"
      RmDir "$APPDATA\$R9\Profiles"
      RmDir "$APPDATA\$R9"

      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro DeleteRelativeProfilesCall _REL_PROFILE_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_REL_PROFILE_PATH}"
  Call DeleteRelativeProfiles
  !verbose pop
!macroend

!macro un.DeleteRelativeProfilesCall _REL_PROFILE_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_REL_PROFILE_PATH}"
  Call un.DeleteRelativeProfiles
  !verbose pop
!macroend

!macro un.DeleteRelativeProfiles
  !ifndef un.DeleteRelativeProfiles
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro DeleteRelativeProfiles

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Deletes shortcuts and Start Menu directories under Programs as specified by
 * the shortcuts log ini file and on Windows 7 unpins TaskBar and Start Menu
 * shortcuts. The shortcuts will not be deleted if the shortcut target isn't for
 * this install location which is determined by the shortcut having a target of
 * $INSTDIR\${FileMainEXE}. The context (All Users or Current User) of the
 * $DESKTOP and $SMPROGRAMS constants depends on the
 * SetShellVarContext setting and must be set by the caller of this macro. There
 * is no All Users context for $QUICKLAUNCH but this will not cause a problem
 * since the macro will just continue past the $QUICKLAUNCH shortcut deletion
 * section on subsequent calls.
 *
 * The ini file sections must have the following format (the order of the
 * sections in the ini file is not important):
 * [SMPROGRAMS]
 * ; RelativePath is the directory relative from the Start Menu
 * ; Programs directory.
 * RelativePath=Mozilla App
 * ; Shortcut1 is the first shortcut, Shortcut2 is the second shortcut, and so
 * ; on. There must not be a break in the sequence of the numbers.
 * Shortcut1=Mozilla App.lnk
 * Shortcut2=Mozilla App (Safe Mode).lnk
 * [DESKTOP]
 * ; Shortcut1 is the first shortcut, Shortcut2 is the second shortcut, and so
 * ; on. There must not be a break in the sequence of the numbers.
 * Shortcut1=Mozilla App.lnk
 * Shortcut2=Mozilla App (Safe Mode).lnk
 * [QUICKLAUNCH]
 * ; Shortcut1 is the first shortcut, Shortcut2 is the second shortcut, and so
 * ; on. There must not be a break in the sequence of the numbers for the
 * ; suffix.
 * Shortcut1=Mozilla App.lnk
 * Shortcut2=Mozilla App (Safe Mode).lnk
 * [STARTMENU]
 * ; Shortcut1 is the first shortcut, Shortcut2 is the second shortcut, and so
 * ; on. There must not be a break in the sequence of the numbers for the
 * ; suffix.
 * Shortcut1=Mozilla App.lnk
 * Shortcut2=Mozilla App (Safe Mode).lnk
 *
 * $R4 = counter for appending to Shortcut for enumerating the ini file entries
 * $R5 = return value from ShellLink::GetShortCutTarget and
 *       ApplicationID::UninstallPinnedItem
 * $R6 = find handle and the long path to the Start Menu Programs directory
 *       (e.g. $SMPROGRAMS)
 * $R7 = path to the $QUICKLAUNCH\User Pinned directory and the return value
 *       from ReadINIStr for the relative path to the applications directory
 *       under the Start Menu Programs directory and the long path to this
 *       directory
 * $R8 = return filename from FindFirst / FindNext and the return value from
 *       ReadINIStr for enumerating shortcuts
 * $R9 = long path to the shortcuts log ini file
 */
!macro DeleteShortcuts

  !ifndef ${_MOZFUNC_UN}DeleteShortcuts
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !insertmacro ${_MOZFUNC_UN_TMP}GetParent
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}DeleteShortcuts "!insertmacro ${_MOZFUNC_UN}DeleteShortcutsCall"

    Function ${_MOZFUNC_UN}DeleteShortcuts
      Push $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4

      ${If} ${AtLeastWin7}
        ; Since shortcuts that are pinned can later be removed without removing
        ; the pinned shortcut unpin the pinned shortcuts for the application's
        ; main exe using the pinned shortcuts themselves.
        StrCpy $R7 "$QUICKLAUNCH\User Pinned"

        ${If} ${FileExists} "$R7\TaskBar"
          ; Delete TaskBar pinned shortcuts for the application's main exe
          FindFirst $R6 $R8 "$R7\TaskBar\*.lnk"
          ${Do}
            ${If} ${FileExists} "$R7\TaskBar\$R8"
              ShellLink::GetShortCutTarget "$R7\TaskBar\$R8"
              Pop $R5
              ${${_MOZFUNC_UN}GetLongPath} "$R5" $R5
              ${If} "$R5" == "$INSTDIR\${FileMainEXE}"
                ApplicationID::UninstallPinnedItem "$R7\TaskBar\$R8"
                Pop $R5
              ${EndIf}
            ${EndIf}
            ClearErrors
            FindNext $R6 $R8
            ${If} ${Errors}
              ${ExitDo}
            ${EndIf}
          ${Loop}
          FindClose $R6
        ${EndIf}

        ${If} ${FileExists} "$R7\StartMenu"
          ; Delete Start Menu pinned shortcuts for the application's main exe
          FindFirst $R6 $R8 "$R7\StartMenu\*.lnk"
          ${Do}
            ${If} ${FileExists} "$R7\StartMenu\$R8"
              ShellLink::GetShortCutTarget "$R7\StartMenu\$R8"
              Pop $R5
              ${${_MOZFUNC_UN}GetLongPath} "$R5" $R5
              ${If} "$R5" == "$INSTDIR\${FileMainEXE}"
                  ApplicationID::UninstallPinnedItem "$R7\StartMenu\$R8"
                  Pop $R5
              ${EndIf}
            ${EndIf}
            ClearErrors
            FindNext $R6 $R8
            ${If} ${Errors}
              ${ExitDo}
            ${EndIf}
          ${Loop}
          FindClose $R6
        ${EndIf}
      ${EndIf}

      ; Don't call ApplicationID::UninstallPinnedItem since pinned items for
      ; this application were removed above and removing them below will remove
      ; the association of side by side installations.
      ${${_MOZFUNC_UN}GetLongPath} "$INSTDIR\uninstall\${SHORTCUTS_LOG}" $R9
      ${If} ${FileExists} "$R9"
        ; Delete Start Menu shortcuts for this application
        StrCpy $R4 -1
        ${Do}
          IntOp $R4 $R4 + 1 ; Increment the counter
          ClearErrors
          ReadINIStr $R8 "$R9" "STARTMENU" "Shortcut$R4"
          ${If} ${Errors}
            ${ExitDo}
          ${EndIf}

          ${If} ${FileExists} "$SMPROGRAMS\$R8"
            ShellLink::GetShortCutTarget "$SMPROGRAMS\$R8"
            Pop $R5
            ${${_MOZFUNC_UN}GetLongPath} "$R5" $R5
            ${If} "$INSTDIR\${FileMainEXE}" == "$R5"
              Delete "$SMPROGRAMS\$R8"
            ${EndIf}
          ${EndIf}
        ${Loop}

        ; Delete Quick Launch shortcuts for this application
        StrCpy $R4 -1
        ${Do}
          IntOp $R4 $R4 + 1 ; Increment the counter
          ClearErrors
          ReadINIStr $R8 "$R9" "QUICKLAUNCH" "Shortcut$R4"
          ${If} ${Errors}
            ${ExitDo}
          ${EndIf}

          ${If} ${FileExists} "$QUICKLAUNCH\$R8"
            ShellLink::GetShortCutTarget "$QUICKLAUNCH\$R8"
            Pop $R5
            ${${_MOZFUNC_UN}GetLongPath} "$R5" $R5
            ${If} "$INSTDIR\${FileMainEXE}" == "$R5"
              Delete "$QUICKLAUNCH\$R8"
            ${EndIf}
          ${EndIf}
        ${Loop}

        ; Delete Desktop shortcuts for this application
        StrCpy $R4 -1
        ${Do}
          IntOp $R4 $R4 + 1 ; Increment the counter
          ClearErrors
          ReadINIStr $R8 "$R9" "DESKTOP" "Shortcut$R4"
          ${If} ${Errors}
            ${ExitDo}
          ${EndIf}

          ${If} ${FileExists} "$DESKTOP\$R8"
            ShellLink::GetShortCutTarget "$DESKTOP\$R8"
            Pop $R5
            ${${_MOZFUNC_UN}GetLongPath} "$R5" $R5
            ${If} "$INSTDIR\${FileMainEXE}" == "$R5"
              Delete "$DESKTOP\$R8"
            ${EndIf}
          ${EndIf}
        ${Loop}

        ${${_MOZFUNC_UN}GetLongPath} "$SMPROGRAMS" $R6

        ; Delete Start Menu Programs shortcuts for this application
        ClearErrors
        ReadINIStr $R7 "$R9" "SMPROGRAMS" "RelativePathToDir"
        ${${_MOZFUNC_UN}GetLongPath} "$R6\$R7" $R7
        ${Unless} "$R7" == ""
          StrCpy $R4 -1
          ${Do}
            IntOp $R4 $R4 + 1 ; Increment the counter
            ClearErrors
            ReadINIStr $R8 "$R9" "SMPROGRAMS" "Shortcut$R4"
            ${If} ${Errors}
              ${ExitDo}
            ${EndIf}

            ${If} ${FileExists} "$R7\$R8"
              ShellLink::GetShortCutTarget "$R7\$R8"
              Pop $R5
              ${${_MOZFUNC_UN}GetLongPath} "$R5" $R5
              ${If} "$INSTDIR\${FileMainEXE}" == "$R5"
                Delete "$R7\$R8"
              ${EndIf}
            ${EndIf}
          ${Loop}

          ; Delete Start Menu Programs directories for this application
          ${Do}
            ClearErrors
            ${If} "$R6" == "$R7"
              ${ExitDo}
            ${EndIf}
            RmDir "$R7"
            ${If} ${Errors}
              ${ExitDo}
            ${EndIf}
            ${${_MOZFUNC_UN}GetParent} "$R7" $R7
          ${Loop}
        ${EndUnless}
      ${EndIf}

      ClearErrors

      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Pop $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro DeleteShortcutsCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call DeleteShortcuts
  !verbose pop
!macroend

!macro un.DeleteShortcutsCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.DeleteShortcuts
  !verbose pop
!macroend

!macro un.DeleteShortcuts
  !ifndef un.DeleteShortcuts
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro DeleteShortcuts

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend


################################################################################
# Macros for parsing and updating the uninstall.log

/**
 * Updates the uninstall.log with new files added by software update.
 *
 * When modifying this macro be aware that LineFind uses all registers except
 * $R0-$R3 and TextCompareNoDetails uses all registers except $R0-$R9 so be
 * cautious. Callers of this macro are not affected.
 */
!macro UpdateUninstallLog

  !ifndef UpdateUninstallLog
    !insertmacro FileJoin
    !insertmacro LineFind
    !insertmacro TextCompareNoDetails
    !insertmacro TrimNewLines

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define UpdateUninstallLog "!insertmacro UpdateUninstallLogCall"

    Function UpdateUninstallLog
      Push $R3
      Push $R2
      Push $R1
      Push $R0

      ClearErrors

      GetFullPathName $R3 "$INSTDIR\uninstall"
      ${If} ${FileExists} "$R3\uninstall.update"
        ${LineFind} "$R3\uninstall.update" "" "1:-1" "CleanupUpdateLog"

        GetTempFileName $R2 "$R3"
        FileOpen $R1 "$R2" w
        ${TextCompareNoDetails} "$R3\uninstall.update" "$R3\uninstall.log" "SlowDiff" "CreateUpdateDiff"
        FileClose $R1

        IfErrors +2 0
        ${FileJoin} "$R3\uninstall.log" "$R2" "$R3\uninstall.log"

        ${DeleteFile} "$R2"
      ${EndIf}

      ClearErrors

      Pop $R0
      Pop $R1
      Pop $R2
      Pop $R3
    FunctionEnd

    ; This callback MUST use labels vs. relative line numbers.
    Function CleanupUpdateLog
      StrCpy $R2 "$R9" 12
      StrCmp "$R2" "EXECUTE ADD " +1 skip
      StrCpy $R9 "$R9" "" 12

      Push $R6
      Push $R5
      Push $R4
      StrCpy $R4 ""         ; Initialize to an empty string.
      StrCpy $R6 -1         ; Set the counter to -1 so it will start at 0.

      loop:
      IntOp $R6 $R6 + 1     ; Increment the counter.
      StrCpy $R5 $R9 1 $R6  ; Starting from the counter copy the next char.
      StrCmp $R5 "" copy    ; Are there no more chars?
      StrCmp $R5 "/" +1 +2  ; Is the char a /?
      StrCpy $R5 "\"        ; Replace the char with a \.

      StrCpy $R4 "$R4$R5"
      GoTo loop

      copy:
      StrCpy $R9 "File: \$R4"
      Pop $R6
      Pop $R5
      Pop $R4
      GoTo end

      skip:
      StrCpy $0 "SkipWrite"

      end:
      Push $0
    FunctionEnd

    Function CreateUpdateDiff
      ${TrimNewLines} "$9" $9
      ${If} $9 != ""
        FileWrite $R1 "$9$\r$\n"
      ${EndIf}

      Push 0
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro UpdateUninstallLogCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call UpdateUninstallLog
  !verbose pop
!macroend

/**
 * Copies files from a source directory to a destination directory with logging
 * to the uninstall.log. If any destination files are in use a reboot will be
 * necessary to complete the installation and the reboot flag (see IfRebootFlag
 * in the NSIS documentation).
 *
 * @param   _PATH_TO_SOURCE
 *          Source path to copy the files from. This must not end with a \.
 *
 * @param   _PATH_TO_DESTINATION
 *          Destination path to copy the files to. This must not end with a \.
 *
 * @param   _PREFIX_ERROR_CREATEDIR
 *          Prefix for the directory creation error message. The directory path
 *          will be inserted below this string.
 *
 * @param   _SUFFIX_ERROR_CREATEDIR
 *          Suffix for the directory creation error message. The directory path
 *          will be inserted above this string.
 *
 * $0  = destination file's parent directory used in the create_dir label
 * $R0 = copied value from $R6 (e.g. _PATH_TO_SOURCE)
 * $R1 = copied value from $R7 (e.g. _PATH_TO_DESTINATION)
 * $R2 = string length of the path to source
 * $R3 = relative path from the path to source
 * $R4 = copied value from $R8 (e.g. _PREFIX_ERROR_CREATEDIR)
 * $R5 = copied value from $R9 (e.g. _SUFFIX_ERROR_CREATEDIR)
 * note: the LocateNoDetails macro uses these registers so we copy the values
 *       to other registers.
 * $R6 = initially _PATH_TO_SOURCE and then set to "size" ($R6="" if directory,
 *       $R6="0" if file with /S=)"path\name" in callback
 * $R7 = initially _PATH_TO_DESTINATION and then set to "name" in callback
 * $R8 = initially _PREFIX_ERROR_CREATEDIR and then set to "path" in callback
 * $R9 = initially _SUFFIX_ERROR_CREATEDIR and then set to "path\name" in
 *       callback
 */
!macro CopyFilesFromDir

  !ifndef CopyFilesFromDir
    !insertmacro LocateNoDetails
    !insertmacro OnEndCommon
    !insertmacro WordReplace

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define CopyFilesFromDir "!insertmacro CopyFilesFromDirCall"

    Function CopyFilesFromDir
      Exch $R9
      Exch 1
      Exch $R8
      Exch 2
      Exch $R7
      Exch 3
      Exch $R6
      Push $R5
      Push $R4
      Push $R3
      Push $R2
      Push $R1
      Push $R0
      Push $0

      StrCpy $R0 "$R6"
      StrCpy $R1 "$R7"
      StrCpy $R4 "$R8"
      StrCpy $R5 "$R9"

      StrLen $R2 "$R0"

      ${LocateNoDetails} "$R0" "/L=FD" "CopyFileCallback"

      Pop $0
      Pop $R0
      Pop $R1
      Pop $R2
      Pop $R3
      Pop $R4
      Pop $R5
      Exch $R6
      Exch 3
      Exch $R7
      Exch 2
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    Function CopyFileCallback
      StrCpy $R3 $R8 "" $R2 ; $R3 always begins with a \.

      retry:
      ClearErrors
      StrCmp $R6 "" +1 copy_file
      IfFileExists "$R1$R3\$R7" end +1
      StrCpy $0 "$R1$R3\$R7"

      create_dir:
      ClearErrors
      CreateDirectory "$0"
      IfFileExists "$0" +1 err_create_dir  ; protect against looping.
      ${LogMsg} "Created Directory: $0"
      StrCmp $R6 "" end copy_file

      err_create_dir:
      ${LogMsg} "** ERROR Creating Directory: $0 **"
      MessageBox MB_RETRYCANCEL|MB_ICONQUESTION "$R4$\r$\n$\r$\n$0$\r$\n$\r$\n$R5" IDRETRY retry
      ${OnEndCommon}
      Quit

      copy_file:
      StrCpy $0 "$R1$R3"
      StrCmp "$0" "$INSTDIR" +2 +1
      IfFileExists "$0" +1 create_dir

      ClearErrors
      ${DeleteFile} "$R1$R3\$R7"
      IfErrors +1 dest_clear
      ClearErrors
      Rename "$R1$R3\$R7" "$R1$R3\$R7.moz-delete"
      IfErrors +1 reboot_delete

      ; file will replace destination file on reboot
      Rename "$R9" "$R9.moz-upgrade"
      CopyFiles /SILENT "$R9.moz-upgrade" "$R1$R3"
      Rename /REBOOTOK "$R1$R3\$R7.moz-upgrade" "$R1$R3\$R7"
      ${LogMsg} "Copied File: $R1$R3\$R7.moz-upgrade"
      ${LogMsg} "Delayed Install File (Reboot Required): $R1$R3\$R7"
      GoTo log_uninstall

      ; file will be deleted on reboot
      reboot_delete:
      CopyFiles /SILENT $R9 "$R1$R3"
      Delete /REBOOTOK "$R1$R3\$R7.moz-delete"
      ${LogMsg} "Installed File: $R1$R3\$R7"
      ${LogMsg} "Delayed Delete File (Reboot Required): $R1$R3\$R7.moz-delete"
      GoTo log_uninstall

      ; destination file doesn't exist - coast is clear
      dest_clear:
      CopyFiles /SILENT $R9 "$R1$R3"
      ${LogMsg} "Installed File: $R1$R3\$R7"

      log_uninstall:
      ; If the file is installed into the installation directory remove the
      ; installation directory's path from the file path when writing to the
      ; uninstall.log so it will be a relative path. This allows the same
      ; helper.exe to be used with zip builds if we supply an uninstall.log.
      ${WordReplace} "$R1$R3\$R7" "$INSTDIR" "" "+" $R3
      ${LogUninstall} "File: $R3"

      end:
      Push 0
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro CopyFilesFromDirCall _PATH_TO_SOURCE _PATH_TO_DESTINATION \
                            _PREFIX_ERROR_CREATEDIR _SUFFIX_ERROR_CREATEDIR
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_PATH_TO_SOURCE}"
  Push "${_PATH_TO_DESTINATION}"
  Push "${_PREFIX_ERROR_CREATEDIR}"
  Push "${_SUFFIX_ERROR_CREATEDIR}"
  Call CopyFilesFromDir
  !verbose pop
!macroend

/**
 * Parses the uninstall.log on install to first remove a previous installation's
 * files and then their directories if empty prior to installing.
 *
 * When modifying this macro be aware that LineFind uses all registers except
 * $R0-$R3 so be cautious. Callers of this macro are not affected.
 */
!macro OnInstallUninstall

  !ifndef OnInstallUninstall
    !insertmacro GetParent
    !insertmacro LineFind
    !insertmacro TrimNewLines

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define OnInstallUninstall "!insertmacro OnInstallUninstallCall"

    Function OnInstallUninstall
      Push $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4
      Push $R3
      Push $R2
      Push $R1
      Push $R0
      Push $TmpVal

      IfFileExists "$INSTDIR\uninstall\uninstall.log" +1 end

      ${LogHeader} "Removing Previous Installation"

      ; Copy the uninstall log file to a temporary file
      GetTempFileName $TmpVal
      CopyFiles /SILENT /FILESONLY "$INSTDIR\uninstall\uninstall.log" "$TmpVal"

      ; Delete files
      ${LineFind} "$TmpVal" "/NUL" "1:-1" "RemoveFilesCallback"

      ; Remove empty directories
      ${LineFind} "$TmpVal" "/NUL" "1:-1" "RemoveDirsCallback"

      ; Delete the temporary uninstall log file
      Delete /REBOOTOK "$TmpVal"

      ; Delete the uninstall log file
      Delete "$INSTDIR\uninstall\uninstall.log"

      end:
      ClearErrors

      Pop $TmpVal
      Pop $R0
      Pop $R1
      Pop $R2
      Pop $R3
      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Pop $R9
    FunctionEnd

    Function RemoveFilesCallback
      ${TrimNewLines} "$R9" $R9
      StrCpy $R1 "$R9" 5       ; Copy the first five chars

      StrCmp "$R1" "File:" +1 end
      StrCpy $R9 "$R9" "" 6    ; Copy string starting after the 6th char
      StrCpy $R0 "$R9" 1       ; Copy the first char

      StrCmp "$R0" "\" +1 end  ; If this isn't a relative path goto end
      StrCmp "$R9" "\install.log" end +1 ; Skip the install.log
      StrCmp "$R9" "\MapiProxy_InUse.dll" end +1 ; Skip the MapiProxy_InUse.dll
      StrCmp "$R9" "\mozMapi32_InUse.dll" end +1 ; Skip the mozMapi32_InUse.dll

      StrCpy $R1 "$INSTDIR$R9" ; Copy the install dir path and suffix it with the string
      IfFileExists "$R1" +1 end

      ClearErrors
      Delete "$R1"
      ${Unless} ${Errors}
        ${LogMsg} "Deleted File: $R1"
        Goto end
      ${EndUnless}

      ClearErrors
      Rename "$R1" "$R1.moz-delete"
      ${Unless} ${Errors}
        Delete /REBOOTOK "$R1.moz-delete"
        ${LogMsg} "Delayed Delete File (Reboot Required): $R1.moz-delete"
        GoTo end
      ${EndUnless}

      ; Check if the file exists in the source. If it does the new file will
      ; replace the existing file when the system is rebooted. If it doesn't
      ; the file will be deleted when the system is rebooted.
      ${Unless} ${FileExists} "$EXEDIR\core$R9"
      ${AndUnless}  ${FileExists} "$EXEDIR\optional$R9"
        Delete /REBOOTOK "$R1"
        ${LogMsg} "Delayed Delete File (Reboot Required): $R1"
      ${EndUnless}

      end:
      ClearErrors

      Push 0
    FunctionEnd

    ; Using locate will leave file handles open to some of the directories
    ; which will prevent the deletion of these directories. This parses the
    ; uninstall.log and uses the file entries to find / remove empty
    ; directories.
    Function RemoveDirsCallback
      ${TrimNewLines} "$R9" $R9
      StrCpy $R0 "$R9" 5          ; Copy the first five chars
      StrCmp "$R0" "File:" +1 end

      StrCpy $R9 "$R9" "" 6       ; Copy string starting after the 6th char
      StrCpy $R0 "$R9" 1          ; Copy the first char

      StrCpy $R1 "$INSTDIR$R9"    ; Copy the install dir path and suffix it with the string
      StrCmp "$R0" "\" loop end   ; If this isn't a relative path goto end

      loop:
      ${GetParent} "$R1" $R1         ; Get the parent directory for the path
      StrCmp "$R1" "$INSTDIR" end +1 ; If the directory is the install dir goto end

      IfFileExists "$R1" +1 loop  ; Only try to remove the dir if it exists
      ClearErrors
      RmDir "$R1"     ; Remove the dir
      IfErrors end +1  ; If we fail there is no use trying to remove its parent dir
      ${LogMsg} "Deleted Directory: $R1"
      GoTo loop

      end:
      ClearErrors

      Push 0
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro OnInstallUninstallCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call OnInstallUninstall
  !verbose pop
!macroend

/**
 * Parses the uninstall.log for the stub installer on install to first remove a
 * previous installation's files prior to installing.
 *
 * When modifying this macro be aware that LineFind uses all registers except
 * $R0-$R3 so be cautious. Callers of this macro are not affected.
 */
!macro OnStubInstallUninstall

  !ifndef OnStubInstallUninstall
    !insertmacro GetParent
    !insertmacro LineFind
    !insertmacro TrimNewLines

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define OnStubInstallUninstall "!insertmacro OnStubInstallUninstallCall"

    Function OnStubInstallUninstall
      Push $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4
      Push $R3
      Push $R2
      Push $R1
      Push $R0
      Push $TmpVal

      IfFileExists "$INSTDIR\uninstall\uninstall.log" +1 end

      ; Copy the uninstall log file to a temporary file
      GetTempFileName $TmpVal
      CopyFiles /SILENT /FILESONLY "$INSTDIR\uninstall\uninstall.log" "$TmpVal"

      CreateDirectory "$INSTDIR\${TO_BE_DELETED}"

      ; Delete files
      ${LineFind} "$TmpVal" "/NUL" "1:-1" "StubRemoveFilesCallback"

      ; Delete the temporary uninstall log file
      Delete /REBOOTOK "$TmpVal"

      RmDir /r /REBOOTOK "$INSTDIR\${TO_BE_DELETED}"

      end:
      ClearErrors

      Pop $TmpVal
      Pop $R0
      Pop $R1
      Pop $R2
      Pop $R3
      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Pop $R9
    FunctionEnd

    Function StubRemoveFilesCallback
      ${TrimNewLines} "$R9" $R9
      StrCpy $R1 "$R9" 5       ; Copy the first five chars

      StrCmp "$R1" "File:" +1 end
      StrCpy $R9 "$R9" "" 6    ; Copy string starting after the 6th char
      StrCpy $R0 "$R9" 1       ; Copy the first char

      StrCmp "$R0" "\" +1 end  ; If this isn't a relative path goto end
      StrCmp "$R9" "\MapiProxy_InUse.dll" end +1 ; Skip the MapiProxy_InUse.dll
      StrCmp "$R9" "\mozMapi32_InUse.dll" end +1 ; Skip the mozMapi32_InUse.dll

      StrCpy $R1 "$INSTDIR$R9" ; Copy the install dir path and suffix it with the string
      IfFileExists "$R1" +1 end

      ClearErrors
      Delete "$R1"
      ${Unless} ${Errors}
        Goto end
      ${EndUnless}

      GetTempFileName $R2 "$INSTDIR\${TO_BE_DELETED}"
      Delete "$R2"
      ClearErrors
      Rename "$R1" "$R2"
      ${If} ${Errors}
        Delete /REBOOTOK "$R1"
      ${EndUnless}

      end:
      ClearErrors

      Push 0
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro OnStubInstallUninstallCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call OnStubInstallUninstall
  !verbose pop
!macroend

/**
 * Parses the uninstall.log to unregister dll's, remove files, and remove
 * empty directories for this installation.
 *
 * When modifying this macro be aware that LineFind uses all registers except
 * $R0-$R3 so be cautious. Callers of this macro are not affected.
 */
!macro un.ParseUninstallLog

  !ifndef un.ParseUninstallLog
    !insertmacro un.GetParent
    !insertmacro un.LineFind
    !insertmacro un.TrimNewLines

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define un.ParseUninstallLog "!insertmacro un.ParseUninstallLogCall"

    Function un.ParseUninstallLog
      Push $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4
      Push $R3
      Push $R2
      Push $R1
      Push $R0
      Push $TmpVal

      IfFileExists "$INSTDIR\uninstall\uninstall.log" +1 end

      ; Copy the uninstall log file to a temporary file
      GetTempFileName $TmpVal
      CopyFiles /SILENT /FILESONLY "$INSTDIR\uninstall\uninstall.log" "$TmpVal"

      ; Unregister DLL's
      ${un.LineFind} "$TmpVal" "/NUL" "1:-1" "un.UnRegDLLsCallback"

      ; Delete files
      ${un.LineFind} "$TmpVal" "/NUL" "1:-1" "un.RemoveFilesCallback"

      ; Remove empty directories
      ${un.LineFind} "$TmpVal" "/NUL" "1:-1" "un.RemoveDirsCallback"

      ; Delete the temporary uninstall log file
      Delete /REBOOTOK "$TmpVal"

      end:

      Pop $TmpVal
      Pop $R0
      Pop $R1
      Pop $R2
      Pop $R3
      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Pop $R9
    FunctionEnd

    Function un.RemoveFilesCallback
      ${un.TrimNewLines} "$R9" $R9
      StrCpy $R1 "$R9" 5

      StrCmp "$R1" "File:" +1 end
      StrCpy $R9 "$R9" "" 6
      StrCpy $R0 "$R9" 1

      StrCpy $R1 "$INSTDIR$R9"
      StrCmp "$R0" "\" +2 +1
      StrCpy $R1 "$R9"

      IfFileExists "$R1" +1 end
      Delete "$R1"
      IfErrors +1 end
      ClearErrors
      Rename "$R1" "$R1.moz-delete"
      IfErrors +1 +3
      Delete /REBOOTOK "$R1"
      GoTo end

      Delete /REBOOTOK "$R1.moz-delete"

      end:
      ClearErrors

      Push 0
    FunctionEnd

    Function un.UnRegDLLsCallback
      ${un.TrimNewLines} "$R9" $R9
      StrCpy $R1 "$R9" 7

      StrCmp $R1 "DLLReg:" +1 end
      StrCpy $R9 "$R9" "" 8
      StrCpy $R0 "$R9" 1

      StrCpy $R1 "$INSTDIR$R9"
      StrCmp $R0 "\" +2 +1
      StrCpy $R1 "$R9"

      ${UnregisterDLL} $R1

      end:
      ClearErrors

      Push 0
    FunctionEnd

    ; Using locate will leave file handles open to some of the directories
    ; which will prevent the deletion of these directories. This parses the
    ; uninstall.log and uses the file entries to find / remove empty
    ; directories.
    Function un.RemoveDirsCallback
      ${un.TrimNewLines} "$R9" $R9
      StrCpy $R0 "$R9" 5          ; Copy the first five chars
      StrCmp "$R0" "File:" +1 end

      StrCpy $R9 "$R9" "" 6       ; Copy string starting after the 6th char
      StrCpy $R0 "$R9" 1          ; Copy the first char

      StrCpy $R1 "$INSTDIR$R9"    ; Copy the install dir path and suffix it with the string
      StrCmp "$R0" "\" loop       ; If this is a relative path goto the loop
      StrCpy $R1 "$R9"            ; Already a full path so copy the string

      loop:
      ${un.GetParent} "$R1" $R1   ; Get the parent directory for the path
      StrCmp "$R1" "$INSTDIR" end ; If the directory is the install dir goto end

      ; We only try to remove empty directories but the Desktop, StartMenu, and
      ; QuickLaunch directories can be empty so guard against removing them.
      SetShellVarContext all          ; Set context to all users
      StrCmp "$R1" "$DESKTOP" end     ; All users desktop
      StrCmp "$R1" "$STARTMENU" end   ; All users start menu

      SetShellVarContext current      ; Set context to all users
      StrCmp "$R1" "$DESKTOP" end     ; Current user desktop
      StrCmp "$R1" "$STARTMENU" end   ; Current user start menu
      StrCmp "$R1" "$QUICKLAUNCH" end ; Current user quick launch

      IfFileExists "$R1" +1 +3  ; Only try to remove the dir if it exists
      ClearErrors
      RmDir "$R1"    ; Remove the dir
      IfErrors end   ; If we fail there is no use trying to remove its parent dir

      StrCmp "$R0" "\" loop end  ; Only loop when the path is relative to the install dir

      end:
      ClearErrors

      Push 0
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro un.ParseUninstallLogCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.ParseUninstallLog
  !verbose pop
!macroend

/**
 * Finds a valid Start Menu shortcut in the uninstall log and returns the
 * relative path from the Start Menu's Programs directory to the shortcut's
 * directory.
 *
 * When modifying this macro be aware that LineFind uses all registers except
 * $R0-$R3 so be cautious. Callers of this macro are not affected.
 *
 * @return  _REL_PATH_TO_DIR
 *          The relative path to the application's Start Menu directory from the
 *          Start Menu's Programs directory.
 */
!macro FindSMProgramsDir

  !ifndef FindSMProgramsDir
    !insertmacro GetParent
    !insertmacro LineFind
    !insertmacro TrimNewLines

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define FindSMProgramsDir "!insertmacro FindSMProgramsDirCall"

    Function FindSMProgramsDir
      Exch $R3
      Push $R2
      Push $R1
      Push $R0

      StrCpy $R3 ""
      ${If} ${FileExists} "$INSTDIR\uninstall\uninstall.log"
        ${LineFind} "$INSTDIR\uninstall\uninstall.log" "/NUL" "1:-1" "FindSMProgramsDirRelPath"
      ${EndIf}
      ClearErrors

      Pop $R0
      Pop $R1
      Pop $R2
      Exch $R3
    FunctionEnd

    ; This callback MUST use labels vs. relative line numbers.
    Function FindSMProgramsDirRelPath
      Push 0
      ${TrimNewLines} "$R9" $R9
      StrCpy $R4 "$R9" 5

      StrCmp "$R4" "File:" +1 end_FindSMProgramsDirRelPath
      StrCpy $R9 "$R9" "" 6
      StrCpy $R4 "$R9" 1

      StrCmp "$R4" "\" end_FindSMProgramsDirRelPath +1

      SetShellVarContext all
      ${GetLongPath} "$SMPROGRAMS" $R4
      StrLen $R2 "$R4"
      StrCpy $R1 "$R9" $R2
      StrCmp "$R1" "$R4" +1 end_FindSMProgramsDirRelPath
      IfFileExists "$R9" +1 end_FindSMProgramsDirRelPath
      ShellLink::GetShortCutTarget "$R9"
      Pop $R0
      StrCmp "$INSTDIR\${FileMainEXE}" "$R0" +1 end_FindSMProgramsDirRelPath
      ${GetParent} "$R9" $R3
      IntOp $R2 $R2 + 1
      StrCpy $R3 "$R3" "" $R2

      Pop $R4             ; Remove the previously pushed 0 from the stack and
      push "StopLineFind" ; push StopLineFind to stop finding more lines.

      end_FindSMProgramsDirRelPath:
      ClearErrors

    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro FindSMProgramsDirCall _REL_PATH_TO_DIR
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call FindSMProgramsDir
  Pop ${_REL_PATH_TO_DIR}
  !verbose pop
!macroend


################################################################################
# Macros for custom branding

/**
 * Sets BrandFullName and / or BrandShortName to values provided in the specified
 * ini file and defaults to BrandShortName and BrandFullName as defined in
 * branding.nsi when the associated ini file entry is not specified.
 *
 * ini file format:
 * [Branding]
 * BrandFullName=Custom Full Name
 * BrandShortName=Custom Short Name
 *
 * @param   _PATH_TO_INI
 *          Path to the ini file.
 *
 * $R6 = return value from ReadINIStr
 * $R7 = stores BrandShortName
 * $R8 = stores BrandFullName
 * $R9 = _PATH_TO_INI
 */
!macro SetBrandNameVars

  !ifndef ${_MOZFUNC_UN}SetBrandNameVars
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}WordReplace
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    ; Prevent declaring vars twice when the SetBrandNameVars macro is
    ; inserted into both the installer and uninstaller.
    !ifndef SetBrandNameVars
      Var BrandFullName
      Var BrandFullNameDA
      Var BrandShortName
    !endif

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}SetBrandNameVars "!insertmacro ${_MOZFUNC_UN}SetBrandNameVarsCall"

    Function ${_MOZFUNC_UN}SetBrandNameVars
      Exch $R9
      Push $R8
      Push $R7
      Push $R6

      StrCpy $R8 "${BrandFullName}"
      StrCpy $R7 "${BrandShortName}"

      IfFileExists "$R9" +1 finish

      ClearErrors
      ReadINIStr $R6 $R9 "Branding" "BrandFullName"
      IfErrors +2 +1
      StrCpy $R8 "$R6"

      ClearErrors
      ReadINIStr $R6 $R9 "Branding" "BrandShortName"
      IfErrors +2 +1
      StrCpy $R7 "$R6"

      finish:
      StrCpy $BrandFullName "$R8"
      ${${_MOZFUNC_UN}WordReplace} "$R8" "&" "&&" "+" $R8
      StrCpy $BrandFullNameDA "$R8"
      StrCpy $BrandShortName "$R7"

      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro SetBrandNameVarsCall _PATH_TO_INI
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_PATH_TO_INI}"
  Call SetBrandNameVars
  !verbose pop
!macroend

!macro un.SetBrandNameVarsCall _PATH_TO_INI
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_PATH_TO_INI}"
  Call un.SetBrandNameVars
  !verbose pop
!macroend

!macro un.SetBrandNameVars
  !ifndef un.SetBrandNameVars
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro SetBrandNameVars

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Replaces the wizard's header image with the one specified.
 *
 * @param   _PATH_TO_IMAGE
 *          Fully qualified path to the bitmap to use for the header image.
 *
 * $R8 = hwnd for the control returned from GetDlgItem.
 * $R9 = _PATH_TO_IMAGE
 */
!macro ChangeMUIHeaderImage

  !ifndef ${_MOZFUNC_UN}ChangeMUIHeaderImage
    Var hHeaderBitmap

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}ChangeMUIHeaderImage "!insertmacro ${_MOZFUNC_UN}ChangeMUIHeaderImageCall"

    Function ${_MOZFUNC_UN}ChangeMUIHeaderImage
      Exch $R9
      Push $R8

      GetDlgItem $R8 $HWNDPARENT 1046
      System::Call 'user32::LoadImageW(i 0, w "$R9", i 0, i 0, i 0, i 0x0010|0x2000) i.s'
      Pop $hHeaderBitmap
      SendMessage $R8 ${STM_SETIMAGE} 0 $hHeaderBitmap
      ; There is no way to specify a show function for a custom page so hide
      ; and then show the control to force the bitmap to redraw.
      ShowWindow $R8 ${SW_HIDE}
      ShowWindow $R8 ${SW_SHOW}

      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro ChangeMUIHeaderImageCall _PATH_TO_IMAGE
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_PATH_TO_IMAGE}"
  Call ChangeMUIHeaderImage
  !verbose pop
!macroend

!macro un.ChangeMUIHeaderImageCall _PATH_TO_IMAGE
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_PATH_TO_IMAGE}"
  Call un.ChangeMUIHeaderImage
  !verbose pop
!macroend

!macro un.ChangeMUIHeaderImage
  !ifndef un.ChangeMUIHeaderImage
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro ChangeMUIHeaderImage

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend


################################################################################
# User interface callback helper defines and macros

/* Install type defines */
!ifndef INSTALLTYPE_BASIC
  !define INSTALLTYPE_BASIC     1
!endif

!ifndef INSTALLTYPE_CUSTOM
  !define INSTALLTYPE_CUSTOM    2
!endif

/**
 * Checks whether to display the current page (e.g. if not performing a custom
 * install don't display the custom pages).
 */
!macro CheckCustomCommon

  !ifndef CheckCustomCommon
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define CheckCustomCommon "!insertmacro CheckCustomCommonCall"

    Function CheckCustomCommon

      ; Abort if not a custom install
      IntCmp $InstallType ${INSTALLTYPE_CUSTOM} +2 +1 +1
      Abort

    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro CheckCustomCommonCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call CheckCustomCommon
  !verbose pop
!macroend

/**
 * Unloads dll's and releases references when the installer and uninstaller
 * exit.
 */
!macro OnEndCommon

  !ifndef ${_MOZFUNC_UN}OnEndCommon
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}UnloadUAC
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}OnEndCommon "!insertmacro ${_MOZFUNC_UN}OnEndCommonCall"

    Function ${_MOZFUNC_UN}OnEndCommon

      ${${_MOZFUNC_UN}UnloadUAC}
      StrCmp $hHeaderBitmap "" +3 +1
      System::Call "gdi32::DeleteObject(i s)" $hHeaderBitmap
      StrCpy $hHeaderBitmap ""

      System::Free 0

    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro OnEndCommonCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call OnEndCommon
  !verbose pop
!macroend

!macro un.OnEndCommonCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.OnEndCommon
  !verbose pop
!macroend

!macro un.OnEndCommon
  !ifndef un.OnEndCommon
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro OnEndCommon

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Called from the installer's .onInit function not to be confused with the
 * uninstaller's .onInit or the uninstaller's un.onInit functions.
 *
 * @param   _WARN_UNSUPPORTED_MSG
 *          Message displayed when the Windows version is not supported.
 *
 * $R5 = return value from the GetSize macro
 * $R6 = general string values, return value from GetTempFileName, return
 *       value from the GetSize macro
 * $R7 = full path to the configuration ini file
 * $R8 = used for OS Version and Service Pack detection and the return value
 *       from the GetParameters macro
 * $R9 = _WARN_UNSUPPORTED_MSG
 */
!macro InstallOnInitCommon

  !ifndef InstallOnInitCommon
    !insertmacro ElevateUAC
    !insertmacro GetOptions
    !insertmacro GetParameters
    !insertmacro GetSize

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define InstallOnInitCommon "!insertmacro InstallOnInitCommonCall"

    Function InstallOnInitCommon
      Exch $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5

      !ifdef HAVE_64BIT_OS
        ${Unless} ${RunningX64}
        ${OrUnless} ${AtLeastWinVista}
          MessageBox MB_OK|MB_ICONSTOP "$R9" IDOK
          ; Nothing initialized so no need to call OnEndCommon
          Quit
        ${EndUnless}

        SetRegView 64
      !else
        StrCpy $R8 "0"
        ${If} ${AtMostWin2000}
          StrCpy $R8 "1"
        ${EndIf}

        ${If} ${IsWinXP}
        ${AndIf} ${AtMostServicePack} 1
          StrCpy $R8 "1"
        ${EndIf}

        ${If} $R8 == "1"
          ; XXX-rstrong - some systems failed the AtLeastWin2000 test that we
          ; used to use for an unknown reason and likely fail the AtMostWin2000
          ; and possibly the IsWinXP test as well. To work around this also
          ; check if the Windows NT registry Key exists and if it does if the
          ; first char in CurrentVersion is equal to 3 (Windows NT 3.5 and
          ; 3.5.1), to 4 (Windows NT 4) or 5 (Windows 2000 and Windows XP).
          StrCpy $R8 ""
          ClearErrors
          ReadRegStr $R8 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentVersion"
          StrCpy $R8 "$R8" 1
          ${If} ${Errors}
          ${OrIf} "$R8" == "3"
          ${OrIf} "$R8" == "4"
          ${OrIf} "$R8" == "5"
            MessageBox MB_OK|MB_ICONSTOP "$R9" IDOK
            ; Nothing initialized so no need to call OnEndCommon
            Quit
          ${EndIf}
        ${EndUnless}
      !endif

      ${GetParameters} $R8

      ; Require elevation if the user can elevate
      ${ElevateUAC}

      ${If} $R8 != ""
        ; Default install type
        StrCpy $InstallType ${INSTALLTYPE_BASIC}

        ${Unless} ${Silent}
          ; Manually check for /S in the command line due to Bug 506867
          ClearErrors
          ${GetOptions} "$R8" "/S" $R7
          ${Unless} ${Errors}
            SetSilent silent
          ${Else}
            ; Support for the deprecated -ms command line argument. The new command
            ; line arguments are not supported when -ms is used.
            ClearErrors
            ${GetOptions} "$R8" "-ms" $R7
            ${Unless} ${Errors}
              SetSilent silent
            ${EndUnless}
          ${EndUnless}
        ${EndUnless}

        ; Support for specifying an installation configuration file.
        ClearErrors
        ${GetOptions} "$R8" "/INI=" $R7
        ${Unless} ${Errors}
          ; The configuration file must also exist
          ${If} ${FileExists} "$R7"
            SetSilent silent
            ReadINIStr $R8 $R7 "Install" "InstallDirectoryName"
            ${If} $R8 != ""
              !ifdef HAVE_64BIT_OS
                StrCpy $INSTDIR "$PROGRAMFILES64\$R8"
              !else
                StrCpy $INSTDIR "$PROGRAMFILES32\$R8"
              !endif
            ${Else}
              ReadINIStr $R8 $R7 "Install" "InstallDirectoryPath"
              ${If} $R8 != ""
                StrCpy $INSTDIR "$R8"
              ${EndIf}
            ${EndIf}

            ; Quit if we are unable to create the installation directory or we are
            ; unable to write to a file in the installation directory.
            ClearErrors
            ${If} ${FileExists} "$INSTDIR"
              GetTempFileName $R6 "$INSTDIR"
              FileOpen $R5 "$R6" w
              FileWrite $R5 "Write Access Test"
              FileClose $R5
              Delete $R6
              ${If} ${Errors}
                ; Nothing initialized so no need to call OnEndCommon
                Quit
              ${EndIf}
            ${Else}
              CreateDirectory "$INSTDIR"
              ${If} ${Errors}
                ; Nothing initialized so no need to call OnEndCommon
                Quit
              ${EndIf}
            ${EndIf}

            ReadINIStr $R8 $R7 "Install" "QuickLaunchShortcut"
            ${If} $R8 == "false"
              StrCpy $AddQuickLaunchSC "0"
            ${Else}
              StrCpy $AddQuickLaunchSC "1"
            ${EndIf}

            ReadINIStr $R8 $R7 "Install" "DesktopShortcut"
            ${If} $R8 == "false"
              StrCpy $AddDesktopSC "0"
            ${Else}
              StrCpy $AddDesktopSC "1"
            ${EndIf}

            ReadINIStr $R8 $R7 "Install" "StartMenuShortcuts"
            ${If} $R8 == "false"
              StrCpy $AddStartMenuSC "0"
            ${Else}
              StrCpy $AddStartMenuSC "1"
            ${EndIf}

            ReadINIStr $R8 $R7 "Install" "MaintenanceService"
            ${If} $R8 == "false"
              StrCpy $InstallMaintenanceService "0"
            ${EndIf}

            !ifndef NO_STARTMENU_DIR
              ReadINIStr $R8 $R7 "Install" "StartMenuDirectoryName"
              ${If} $R8 != ""
                StrCpy $StartMenuDir "$R8"
              ${EndIf}
            !endif
          ${EndIf}
        ${EndUnless}
      ${EndIf}
      ClearErrors

      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro InstallOnInitCommonCall _WARN_UNSUPPORTED_MSG
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_WARN_UNSUPPORTED_MSG}"
  Call InstallOnInitCommon
  !verbose pop
!macroend

/**
 * Called from the uninstaller's .onInit function not to be confused with the
 * installer's .onInit or the uninstaller's un.onInit functions.
 */
!macro UninstallOnInitCommon

  !ifndef UninstallOnInitCommon
    !insertmacro ElevateUAC
    !insertmacro GetLongPath
    !insertmacro GetOptions
    !insertmacro GetParameters
    !insertmacro GetParent
    !insertmacro UnloadUAC
    !insertmacro UpdateShortcutAppModelIDs
    !insertmacro UpdateUninstallLog

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define UninstallOnInitCommon "!insertmacro UninstallOnInitCommonCall"

    Function UninstallOnInitCommon
      ; Prevents breaking apps that don't use SetBrandNameVars
      !ifdef SetBrandNameVars
        ${SetBrandNameVars} "$EXEDIR\distribution\setup.ini"
      !endif

      ; Prevent launching the application when a reboot is required and this
      ; executable is the main application executable
      IfFileExists "$EXEDIR\${FileMainEXE}.moz-upgrade" +1 +4
      MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(WARN_RESTART_REQUIRED_UPGRADE)" IDNO +2
      Reboot
      Quit ; Nothing initialized so no need to call OnEndCommon

      ${GetParent} "$EXEDIR" $INSTDIR
      ${GetLongPath} "$INSTDIR" $INSTDIR
      IfFileExists "$INSTDIR\${FileMainEXE}" +2 +1
      Quit ; Nothing initialized so no need to call OnEndCommon

      ; Prevents breaking apps that don't use SetBrandNameVars
      !ifdef SetBrandNameVars
        ${SetBrandNameVars} "$INSTDIR\distribution\setup.ini"
      !endif

      ; Application update uses a directory named tobedeleted in the $INSTDIR to
      ; delete files on OS reboot when they are in use. Try to delete this
      ; directory if it exists.
      ${If} ${FileExists} "$INSTDIR\tobedeleted"
        RmDir /r "$INSTDIR\tobedeleted"
      ${EndIf}

      ; Prevent all operations (e.g. set as default, postupdate, etc.) when a
      ; reboot is required and the executable launched is helper.exe
      IfFileExists "$INSTDIR\${FileMainEXE}.moz-upgrade" +1 +4
      MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(WARN_RESTART_REQUIRED_UPGRADE)" IDNO +2
      Reboot
      Quit ; Nothing initialized so no need to call OnEndCommon

      !ifdef HAVE_64BIT_OS
        SetRegView 64
      !endif

      ${GetParameters} $R0

      StrCmp "$R0" "" continue +1

      ; Update this user's shortcuts with the latest app user model id.
      ClearErrors
      ${GetOptions} "$R0" "/UpdateShortcutAppUserModelIds" $R2
      IfErrors hideshortcuts +1
      StrCpy $R2 ""
!ifmacrodef InitHashAppModelId
      ${If} "$AppUserModelID" != ""
        ${UpdateShortcutAppModelIDs}  "$INSTDIR\${FileMainEXE}" "$AppUserModelID" $R2
      ${EndIf}
!endif
      StrCmp "$R2" "false" +1 finish ; true indicates that shortcuts have been updated
      Quit ; Nothing initialized so no need to call OnEndCommon

      ; Require elevation if the user can elevate
      hideshortcuts:
      ClearErrors
      ${GetOptions} "$R0" "/HideShortcuts" $R2
      IfErrors showshortcuts +1
!ifndef NONADMIN_ELEVATE
      ${ElevateUAC}
!endif
      ${HideShortcuts}
      GoTo finish

      ; Require elevation if the user can elevate
      showshortcuts:
      ClearErrors
      ${GetOptions} "$R0" "/ShowShortcuts" $R2
      IfErrors defaultappuser +1
!ifndef NONADMIN_ELEVATE
      ${ElevateUAC}
!endif
      ${ShowShortcuts}
      GoTo finish

      ; Require elevation if the the StartMenuInternet registry keys require
      ; updating and the user can elevate
      defaultappuser:
      ClearErrors
      ${GetOptions} "$R0" "/SetAsDefaultAppUser" $R2
      IfErrors defaultappglobal +1
      ${SetAsDefaultAppUser}
      GoTo finish

      ; Require elevation if the user can elevate
      defaultappglobal:
      ClearErrors
      ${GetOptions} "$R0" "/SetAsDefaultAppGlobal" $R2
      IfErrors postupdate +1
      ${ElevateUAC}
      ${SetAsDefaultAppGlobal}
      GoTo finish

      ; Do not attempt to elevate. The application launching this executable is
      ; responsible for elevation if it is required.
      postupdate:
      ${WordReplace} "$R0" "$\"" "" "+" $R0
      ClearErrors
      ${GetOptions} "$R0" "/PostUpdate" $R2
      IfErrors continue +1
      ; If the uninstall.log does not exist don't perform post update
      ; operations. This prevents updating the registry for zip builds.
      IfFileExists "$EXEDIR\uninstall.log" +2 +1
      Quit ; Nothing initialized so no need to call OnEndCommon
      ${PostUpdate}
      ClearErrors
      ${GetOptions} "$R0" "/UninstallLog=" $R2
      IfErrors updateuninstalllog +1
      StrCmp "$R2" "" finish +1
      GetFullPathName $R3 "$R2"
      IfFileExists "$R3" +1 finish
      Delete "$INSTDIR\uninstall\*wizard*"
      Delete "$INSTDIR\uninstall\uninstall.log"
      CopyFiles /SILENT /FILESONLY "$R3" "$INSTDIR\uninstall\"
      ${GetParent} "$R3" $R4
      Delete "$R3"
      RmDir "$R4"
      GoTo finish

      ; Do not attempt to elevate. The application launching this executable is
      ; responsible for elevation if it is required.
      updateuninstalllog:
      ${UpdateUninstallLog}

      finish:
      ${UnloadUAC}
      System::Call "shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)"
      Quit ; Nothing initialized so no need to call OnEndCommon

      continue:

      ; If the uninstall.log does not exist don't perform uninstall
      ; operations. This prevents running the uninstaller for zip builds.
      IfFileExists "$INSTDIR\uninstall\uninstall.log" +2 +1
      Quit ; Nothing initialized so no need to call OnEndCommon

      ; Require elevation if the user can elevate
      ${ElevateUAC}

      ; If we made it this far then this installer is being used as an uninstaller.
      WriteUninstaller "$EXEDIR\uninstaller.exe"

      ${Unless} ${Silent}
        ; Manually check for /S in the command line due to Bug 506867
        ClearErrors
        ${GetOptions} "$R0" "/S" $R2
        ${Unless} ${Errors}
          SetSilent silent
        ${Else}
          ; Support for the deprecated -ms command line argument.
          ClearErrors
          ${GetOptions} "$R0" "-ms" $R2
          ${Unless} ${Errors}
            SetSilent silent
          ${EndUnless}
        ${EndUnless}
      ${EndUnless}

      ${If} ${Silent}
        StrCpy $R1 "$\"$EXEDIR\uninstaller.exe$\" /S"
      ${Else}
        StrCpy $R1 "$\"$EXEDIR\uninstaller.exe$\""
      ${EndIf}

      ; When the uninstaller is launched it copies itself to the temp directory
      ; so it won't be in use so it can delete itself.
      ExecWait $R1
      ${DeleteFile} "$EXEDIR\uninstaller.exe"
      ${UnloadUAC}
      SetErrorLevel 0
      Quit ; Nothing initialized so no need to call OnEndCommon

    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro UninstallOnInitCommonCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call UninstallOnInitCommon
  !verbose pop
!macroend

/**
 * Called from the uninstaller's un.onInit function not to be confused with the
 * installer's .onInit or the uninstaller's .onInit functions.
 */
!macro un.UninstallUnOnInitCommon

  !ifndef un.UninstallUnOnInitCommon
    !insertmacro un.GetLongPath
    !insertmacro un.GetParent
    !insertmacro un.SetBrandNameVars

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define un.UninstallUnOnInitCommon "!insertmacro un.UninstallUnOnInitCommonCall"

    Function un.UninstallUnOnInitCommon
      ${un.GetParent} "$INSTDIR" $INSTDIR
      ${un.GetLongPath} "$INSTDIR" $INSTDIR
      ${Unless} ${FileExists} "$INSTDIR\${FileMainEXE}"
        Abort
      ${EndUnless}

      !ifdef HAVE_64BIT_OS
        SetRegView 64
      !endif

      ; Prevents breaking apps that don't use SetBrandNameVars
      !ifdef un.SetBrandNameVars
        ${un.SetBrandNameVars} "$INSTDIR\distribution\setup.ini"
      !endif

      ; Initialize $hHeaderBitmap to prevent redundant changing of the bitmap if
      ; the user clicks the back button
      StrCpy $hHeaderBitmap ""
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro un.UninstallUnOnInitCommonCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.UninstallUnOnInitCommon
  !verbose pop
!macroend

/**
 * Called from the MUI leaveOptions function to set the value of $INSTDIR.
 */
!macro LeaveOptionsCommon

  !ifndef LeaveOptionsCommon
    !insertmacro CanWriteToInstallDir
    !insertmacro GetLongPath

!ifndef NO_INSTDIR_FROM_REG
    !insertmacro GetSingleInstallPath
!endif

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define LeaveOptionsCommon "!insertmacro LeaveOptionsCommonCall"

    Function LeaveOptionsCommon
      Push $R9

!ifndef NO_INSTDIR_FROM_REG
      SetShellVarContext all      ; Set SHCTX to HKLM
      ${GetSingleInstallPath} "Software\Mozilla\${BrandFullNameInternal}" $R9

      StrCmp "$R9" "false" +1 finish_get_install_dir

      SetShellVarContext current  ; Set SHCTX to HKCU
      ${GetSingleInstallPath} "Software\Mozilla\${BrandFullNameInternal}" $R9

      finish_get_install_dir:
      StrCmp "$R9" "false" +2 +1
      StrCpy $INSTDIR "$R9"
!endif

      ; If the user doesn't have write access to the installation directory set
      ; the installation directory to a subdirectory of the All Users application
      ; directory and if the user can't write to that location set the installation
      ; directory to a subdirectory of the users local application directory
      ; (e.g. non-roaming).
      ${CanWriteToInstallDir} $R9
      StrCmp "$R9" "false" +1 finish_check_install_dir

      SetShellVarContext all      ; Set SHCTX to All Users
      StrCpy $INSTDIR "$APPDATA\${BrandFullName}\"
      ${CanWriteToInstallDir} $R9
      StrCmp "$R9" "false" +2 +1
      StrCpy $INSTDIR "$LOCALAPPDATA\${BrandFullName}\"

      finish_check_install_dir:
      IfFileExists "$INSTDIR" +3 +1
      Pop $R9
      Return

      ; Always display the long path if the path already exists.
      ${GetLongPath} "$INSTDIR" $INSTDIR

      ; The call to GetLongPath returns a long path without a trailing
      ; back-slash. Append a \ to the path to prevent the directory
      ; name from being appended when using the NSIS create new folder.
      ; http://www.nullsoft.com/free/nsis/makensis.htm#InstallDir
      StrCpy $INSTDIR "$INSTDIR\"

      Pop $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro LeaveOptionsCommonCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call LeaveOptionsCommon
  !verbose pop
!macroend

/**
 * Called from the MUI preDirectory function to verify there is enough disk
 * space for the installation and the installation directory is writable.
 *
 * $R9 = returned value from CheckDiskSpace and CanWriteToInstallDir macros
 */
!macro PreDirectoryCommon

  !ifndef PreDirectoryCommon
    !insertmacro CanWriteToInstallDir
    !insertmacro CheckDiskSpace

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define PreDirectoryCommon "!insertmacro PreDirectoryCommonCall"

    Function PreDirectoryCommon
      Push $R9

      IntCmp $InstallType ${INSTALLTYPE_CUSTOM} end +1 +1
      ${CanWriteToInstallDir} $R9
      StrCmp "$R9" "false" end +1
      ${CheckDiskSpace} $R9
      StrCmp "$R9" "false" end +1
      Abort

      end:

      Pop $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro PreDirectoryCommonCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call PreDirectoryCommon
  !verbose pop
!macroend

/**
 * Called from the MUI leaveDirectory function
 *
 * @param   _WARN_DISK_SPACE
 *          Message displayed when there isn't enough disk space to perform the
 *          installation.
 * @param   _WARN_WRITE_ACCESS
 *          Message displayed when the installer does not have write access to
 *          $INSTDIR.
 *
 * $R7 = returned value from CheckDiskSpace and CanWriteToInstallDir macros
 * $R8 = _WARN_DISK_SPACE
 * $R9 = _WARN_WRITE_ACCESS
 */
!macro LeaveDirectoryCommon

  !ifndef LeaveDirectoryCommon
    !insertmacro CheckDiskSpace
    !insertmacro CanWriteToInstallDir

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define LeaveDirectoryCommon "!insertmacro LeaveDirectoryCommonCall"

    Function LeaveDirectoryCommon
      Exch $R9
      Exch 1
      Exch $R8
      Push $R7

      ${CanWriteToInstallDir} $R7
      ${If} $R7 == "false"
        MessageBox MB_OK|MB_ICONEXCLAMATION "$R9"
        Abort
      ${EndIf}

      ${CheckDiskSpace} $R7
      ${If} $R7 == "false"
        MessageBox MB_OK|MB_ICONEXCLAMATION "$R8"
        Abort
      ${EndIf}

      Pop $R7
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro LeaveDirectoryCommonCall _WARN_DISK_SPACE _WARN_WRITE_ACCESS
  !verbose push
  Push "${_WARN_DISK_SPACE}"
  Push "${_WARN_WRITE_ACCESS}"
  !verbose ${_MOZFUNC_VERBOSE}
  Call LeaveDirectoryCommon
  !verbose pop
!macroend


################################################################################
# Install Section common macros.

/**
 * Performs common cleanup operations prior to the actual installation.
 * This macro should be called first when installation starts.
 */
!macro InstallStartCleanupCommon

  !ifndef InstallStartCleanupCommon
    !insertmacro CleanVirtualStore
    !insertmacro EndUninstallLog
    !insertmacro OnInstallUninstall

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define InstallStartCleanupCommon "!insertmacro InstallStartCleanupCommonCall"

    Function InstallStartCleanupCommon

      ; Remove files not removed by parsing the uninstall.log
      Delete "$INSTDIR\install_wizard.log"
      Delete "$INSTDIR\install_status.log"

      RmDir /r "$INSTDIR\updates"
      Delete "$INSTDIR\updates.xml"
      Delete "$INSTDIR\active-update.xml"

      RmDir /r "$INSTDIR\distribution"

      ; Remove files from the uninstall directory.
      ${If} ${FileExists} "$INSTDIR\uninstall"
        Delete "$INSTDIR\uninstall\*wizard*"
        Delete "$INSTDIR\uninstall\uninstall.ini"
        Delete "$INSTDIR\uninstall\cleanup.log"
        Delete "$INSTDIR\uninstall\uninstall.update"
        ${OnInstallUninstall}
      ${EndIf}

      ; Since we write to the uninstall.log in this directory during the
      ; installation create the directory if it doesn't already exist.
      IfFileExists "$INSTDIR\uninstall" +2 +1
      CreateDirectory "$INSTDIR\uninstall"

      ; Application update uses a directory named tobedeleted in the $INSTDIR to
      ; delete files on OS reboot when they are in use. Try to delete this
      ; directory if it exists.
      ${If} ${FileExists} "$INSTDIR\tobedeleted"
        RmDir /r "$INSTDIR\tobedeleted"
      ${EndIf}

      ; Remove files that may be left behind by the application in the
      ; VirtualStore directory.
      ${CleanVirtualStore}
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro InstallStartCleanupCommonCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call InstallStartCleanupCommon
  !verbose pop
!macroend

/**
 * Performs common cleanup operations after the actual installation.
 * This macro should be called last during the installation.
 */
!macro InstallEndCleanupCommon

  !ifndef InstallEndCleanupCommon
    !insertmacro EndUninstallLog

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define InstallEndCleanupCommon "!insertmacro InstallEndCleanupCommonCall"

    Function InstallEndCleanupCommon

      ; Close the file handle to the uninstall.log
      ${EndUninstallLog}

    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro InstallEndCleanupCommonCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call InstallEndCleanupCommon
  !verbose pop
!macroend


################################################################################
# UAC Related Macros

/**
 * Provides UAC elevation support for Vista and above (requires the UAC plugin).
 *
 * $0 = return values from calls to the UAC plugin (always uses $0)
 * $R9 = return values from GetParameters and GetOptions macros
 */
!macro ElevateUAC

  !ifndef ${_MOZFUNC_UN}ElevateUAC
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetOptions
    !insertmacro ${_MOZFUNC_UN_TMP}GetParameters
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}ElevateUAC "!insertmacro ${_MOZFUNC_UN}ElevateUACCall"

    Function ${_MOZFUNC_UN}ElevateUAC
      Push $R9
      Push $0

!ifndef NONADMIN_ELEVATE
        ${If} ${AtLeastWinVista}
          UAC::IsAdmin
          ; If the user is not an admin already
          ${If} "$0" != "1"
            UAC::SupportsUAC
            ; If the system supports UAC
            ${If} "$0" == "1"
              UAC::GetElevationType
              ; If the user account has a split token
              ${If} "$0" == "3"
                UAC::RunElevated
                UAC::Unload
                ; Nothing besides UAC initialized so no need to call OnEndCommon
                Quit
              ${EndIf}
            ${EndIf}
          ${Else}
            ${GetParameters} $R9
            ${If} $R9 != ""
              ClearErrors
              ${GetOptions} "$R9" "/UAC:" $0
              ; If the command line contains /UAC then we need to initialize
              ; the UAC plugin to use UAC::ExecCodeSegment to execute code in
              ; the non-elevated context.
              ${Unless} ${Errors}
                UAC::RunElevated
              ${EndUnless}
            ${EndIf}
          ${EndIf}
        ${EndIf}
!else
      ${If} ${AtLeastWinVista}
        UAC::IsAdmin
        ; If the user is not an admin already
        ${If} "$0" != "1"
          UAC::SupportsUAC
          ; If the system supports UAC require that the user elevate
          ${If} "$0" == "1"
            UAC::GetElevationType
            ; If the user account has a split token
            ${If} "$0" == "3"
              UAC::RunElevated
              UAC::Unload
              ; Nothing besides UAC initialized so no need to call OnEndCommon
              Quit
            ${EndIf}
          ${Else}
            ; Check if UAC is enabled. If the user has turned UAC on or off
            ; without rebooting this value will be incorrect. This is an
            ; edgecase that we have to live with when trying to allow
            ; installing when the user doesn't have privileges such as a public
            ; computer while trying to also achieve UAC elevation. When this
            ; happens the user will be presented with the runas dialog if the
            ; value is 1 and won't be presented with the UAC dialog when the
            ; value is 0.
            ReadRegDWord $R9 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" "EnableLUA"
            ${If} "$R9" == "1"
              ; This will display the UAC version of the runas dialog which
              ; requires a password for an existing user account.
              UAC::RunElevated
              ${If} "$0" == "0" ; Was elevation successful
                UAC::Unload
                ; Nothing besides UAC initialized so no need to call OnEndCommon
                Quit
              ${EndIf}
              ; Unload UAC since the elevation request was not successful and
              ; install anyway.
              UAC::Unload
            ${EndIf}
          ${EndIf}
        ${Else}
          ClearErrors
          ${${_MOZFUNC_UN}GetParameters} $R9
          ${${_MOZFUNC_UN}GetOptions} "$R9" "/UAC:" $R9
          ; If the command line contains /UAC then we need to initialize the UAC
          ; plugin to use UAC::ExecCodeSegment to execute code in the
          ; non-elevated context.
          ${Unless} ${Errors}
            UAC::RunElevated
          ${EndUnless}
        ${EndIf}
      ${EndIf}
!endif

      ClearErrors

      Pop $0
      Pop $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro ElevateUACCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call ElevateUAC
  !verbose pop
!macroend

!macro un.ElevateUACCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.ElevateUAC
  !verbose pop
!macroend

!macro un.ElevateUAC
  !ifndef un.ElevateUAC
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro ElevateUAC

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Unloads the UAC plugin so the NSIS plugins can be removed when the installer
 * and uninstaller exit.
 *
 * $R9 = return values from GetParameters and GetOptions macros
 */
!macro UnloadUAC

  !ifndef ${_MOZFUNC_UN}UnloadUAC
    !define _MOZFUNC_UN_TMP_UnloadUAC ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP_UnloadUAC}GetOptions
    !insertmacro ${_MOZFUNC_UN_TMP_UnloadUAC}GetParameters
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP_UnloadUAC}
    !undef _MOZFUNC_UN_TMP_UnloadUAC

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}UnloadUAC "!insertmacro ${_MOZFUNC_UN}UnloadUACCall"

    Function ${_MOZFUNC_UN}UnloadUAC
      ${Unless} ${AtLeastWinVista}
        Return
      ${EndUnless}

      Push $R9

      ClearErrors
      ${${_MOZFUNC_UN}GetParameters} $R9
      ${${_MOZFUNC_UN}GetOptions} "$R9" "/UAC:" $R9
      ; If the command line contains /UAC then we need to unload the UAC plugin
      IfErrors +2 +1
      UAC::Unload

      ClearErrors

      Pop $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro UnloadUACCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call UnloadUAC
  !verbose pop
!macroend

!macro un.UnloadUACCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call un.UnloadUAC
  !verbose pop
!macroend

!macro un.UnloadUAC
  !ifndef un.UnloadUAC
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro UnloadUAC

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend


################################################################################
# Macros for uninstall.log and install.log logging
#
# Since these are used by other macros they should be inserted first. All of
# these macros can be easily inserted using the _LoggingCommon macro.

/**
 * Adds all logging macros in the correct order in one fell swoop as well as
 * the vars for the install.log and uninstall.log file handles.
 */
!macro _LoggingCommon
  Var /GLOBAL fhInstallLog
  Var /GLOBAL fhUninstallLog
  !insertmacro StartInstallLog
  !insertmacro EndInstallLog
  !insertmacro StartUninstallLog
  !insertmacro EndUninstallLog
!macroend
!define _LoggingCommon "!insertmacro _LoggingCommon"

/**
 * Creates a file named install.log in the install directory (e.g. $INSTDIR)
 * and adds the installation started message to the install.log for this
 * installation. This also adds the fhInstallLog and fhUninstallLog vars used
 * for logging.
 *
 * $fhInstallLog = filehandle for $INSTDIR\install.log
 *
 * @param   _APP_NAME
 *          Typically the BrandFullName
 * @param   _AB_CD
 *          The locale identifier
 * @param   _APP_VERSION
 *          The application version
 * @param   _GRE_VERSION
 *          The Gecko Runtime Engine version
 *
 * $R6 = _APP_NAME
 * $R7 = _AB_CD
 * $R8 = _APP_VERSION
 * $R9 = _GRE_VERSION
 */
!macro StartInstallLog

  !ifndef StartInstallLog
    !insertmacro GetTime

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define StartInstallLog "!insertmacro StartInstallLogCall"

    Function StartInstallLog
      Exch $R9
      Exch 1
      Exch $R8
      Exch 2
      Exch $R7
      Exch 3
      Exch $R6
      Push $R5
      Push $R4
      Push $R3
      Push $R2
      Push $R1
      Push $R0
      Push $9

      ${DeleteFile} "$INSTDIR\install.log"
      FileOpen $fhInstallLog "$INSTDIR\install.log" w
      FileWriteWord $fhInstallLog "65279"

      ${GetTime} "" "L" $9 $R0 $R1 $R2 $R3 $R4 $R5
      FileWriteUTF16LE $fhInstallLog "$R6 Installation Started: $R1-$R0-$9 $R3:$R4:$R5"
      ${WriteLogSeparator}

      ${LogHeader} "Installation Details"
      ${LogMsg} "Install Dir: $INSTDIR"
      ${LogMsg} "Locale     : $R7"
      ${LogMsg} "App Version: $R8"
      ${LogMsg} "GRE Version: $R9"

      ${If} ${IsWinXP}
        ${LogMsg} "OS Name    : Windows XP"
      ${ElseIf} ${IsWin2003}
        ${LogMsg} "OS Name    : Windows 2003"
      ${ElseIf} ${IsWinVista}
        ${LogMsg} "OS Name    : Windows Vista"
      ${ElseIf} ${IsWin7}
        ${LogMsg} "OS Name    : Windows 7"
      ${ElseIf} ${IsWin8}
        ${LogMsg} "OS Name    : Windows 8"
      ${ElseIf} ${AtLeastWin8}
        ${LogMsg} "OS Name    : Above Windows 8"
      ${Else}
        ${LogMsg} "OS Name    : Unable to detect"
      ${EndIf}

      !ifdef HAVE_64BIT_OS
        ${LogMsg} "Target CPU : x64"
      !else
        ${LogMsg} "Target CPU : x86"
      !endif

      Pop $9
      Pop $R0
      Pop $R1
      Pop $R2
      Pop $R3
      Pop $R4
      Pop $R5
      Exch $R6
      Exch 3
      Exch $R7
      Exch 2
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro StartInstallLogCall _APP_NAME _AB_CD _APP_VERSION _GRE_VERSION
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_APP_NAME}"
  Push "${_AB_CD}"
  Push "${_APP_VERSION}"
  Push "${_GRE_VERSION}"
  Call StartInstallLog
  !verbose pop
!macroend

/**
 * Writes the installation finished message to the install.log and closes the
 * file handles to the install.log and uninstall.log
 *
 * @param   _APP_NAME
 *
 * $R9 = _APP_NAME
 */
!macro EndInstallLog

  !ifndef EndInstallLog
    !insertmacro GetTime

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define EndInstallLog "!insertmacro EndInstallLogCall"

    Function EndInstallLog
      Exch $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5
      Push $R4
      Push $R3
      Push $R2

      ${WriteLogSeparator}
      ${GetTime} "" "L" $R2 $R3 $R4 $R5 $R6 $R7 $R8
      FileWriteUTF16LE $fhInstallLog "$R9 Installation Finished: $R4-$R3-$R2 $R6:$R7:$R8$\r$\n"
      FileClose $fhInstallLog

      Pop $R2
      Pop $R3
      Pop $R4
      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro EndInstallLogCall _APP_NAME
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_APP_NAME}"
  Call EndInstallLog
  !verbose pop
!macroend

/**
 * Opens the file handle to the uninstall.log.
 *
 * $fhUninstallLog = filehandle for $INSTDIR\uninstall\uninstall.log
 */
!macro StartUninstallLog

  !ifndef StartUninstallLog
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define StartUninstallLog "!insertmacro StartUninstallLogCall"

    Function StartUninstallLog
      FileOpen $fhUninstallLog "$INSTDIR\uninstall\uninstall.log" w
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro StartUninstallLogCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call StartUninstallLog
  !verbose pop
!macroend

/**
 * Closes the file handle to the uninstall.log.
 */
!macro EndUninstallLog

  !ifndef EndUninstallLog

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define EndUninstallLog "!insertmacro EndUninstallLogCall"

    Function EndUninstallLog
      FileClose $fhUninstallLog
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro EndUninstallLogCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call EndUninstallLog
  !verbose pop
!macroend

/**
 * Adds a section header to the human readable log.
 *
 * @param   _HEADER
 *          The header text to write to the log.
 */
!macro LogHeader _HEADER
  ${WriteLogSeparator}
  FileWriteUTF16LE $fhInstallLog "${_HEADER}"
  ${WriteLogSeparator}
!macroend
!define LogHeader "!insertmacro LogHeader"

/**
 * Adds a section message to the human readable log.
 *
 * @param   _MSG
 *          The message text to write to the log.
 */
!macro LogMsg _MSG
  FileWriteUTF16LE $fhInstallLog "  ${_MSG}$\r$\n"
!macroend
!define LogMsg "!insertmacro LogMsg"

/**
 * Adds an uninstall entry to the uninstall log.
 *
 * @param   _MSG
 *          The message text to write to the log.
 */
!macro LogUninstall _MSG
  FileWrite $fhUninstallLog "${_MSG}$\r$\n"
!macroend
!define LogUninstall "!insertmacro LogUninstall"

/**
 * Adds a section divider to the human readable log.
 */
!macro WriteLogSeparator
  FileWriteUTF16LE $fhInstallLog "$\r$\n----------------------------------------\
                                  ---------------------------------------$\r$\n"
!macroend
!define WriteLogSeparator "!insertmacro WriteLogSeparator"


################################################################################
# Macros for managing the shortcuts log ini file

/**
 * Adds the most commonly used shortcut logging macros for the installer in one
 * fell swoop.
 */
!macro _LoggingShortcutsCommon
  !insertmacro LogDesktopShortcut
  !insertmacro LogQuickLaunchShortcut
  !insertmacro LogSMProgramsShortcut
!macroend
!define _LoggingShortcutsCommon "!insertmacro _LoggingShortcutsCommon"

/**
 * Creates the shortcuts log ini file with a UTF-16LE BOM if it doesn't exist.
 */
!macro initShortcutsLog
  Push $R9

  IfFileExists "$INSTDIR\uninstall\${SHORTCUTS_LOG}" +4 +1
  FileOpen $R9 "$INSTDIR\uninstall\${SHORTCUTS_LOG}" w
  FileWriteWord $R9 "65279"
  FileClose $R9

  Pop $R9
!macroend
!define initShortcutsLog "!insertmacro initShortcutsLog"

/**
 * Adds shortcut entries to the shortcuts log ini file. This macro is primarily
 * a helper used by the LogDesktopShortcut, LogQuickLaunchShortcut, and
 * LogSMProgramsShortcut macros but it can be used by other code if desired. If
 * the value already exists the the value is not written to the file.
 *
 * @param   _SECTION_NAME
 *          The section name to write to in the shortcut log ini file
 * @param   _FILE_NAME
 *          The shortcut's file name
 *
 * $R6 = return value from ReadIniStr for the shortcut file name
 * $R7 = counter for supporting multiple shortcuts in the same location
 * $R8 = _SECTION_NAME
 * $R9 = _FILE_NAME
 */
!macro LogShortcut

  !ifndef LogShortcut
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define LogShortcut "!insertmacro LogShortcutCall"

    Function LogShortcut
      Exch $R9
      Exch 1
      Exch $R8
      Push $R7
      Push $R6

      ClearErrors

      !insertmacro initShortcutsLog

      StrCpy $R6 ""
      StrCpy $R7 -1

      StrCmp "$R6" "$R9" +5 +1 ; if the shortcut already exists don't add it
      IntOp $R7 $R7 + 1 ; increment the counter
      ReadIniStr $R6 "$INSTDIR\uninstall\${SHORTCUTS_LOG}" "$R8" "Shortcut$R7"
      IfErrors +1 -3
      WriteINIStr "$INSTDIR\uninstall\${SHORTCUTS_LOG}" "$R8" "Shortcut$R7" "$R9"

      ClearErrors

      Pop $R6
      Pop $R7
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro LogShortcutCall _SECTION_NAME _FILE_NAME
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_SECTION_NAME}"
  Push "${_FILE_NAME}"
  Call LogShortcut
  !verbose pop
!macroend

/**
 * Adds a Desktop shortcut entry to the shortcuts log ini file.
 *
 * @param   _FILE_NAME
 *          The shortcut file name (e.g. shortcut.lnk)
 */
!macro LogDesktopShortcut

  !ifndef LogDesktopShortcut
    !insertmacro LogShortcut

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define LogDesktopShortcut "!insertmacro LogDesktopShortcutCall"

    Function LogDesktopShortcut
      Call LogShortcut
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro LogDesktopShortcutCall _FILE_NAME
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "DESKTOP"
  Push "${_FILE_NAME}"
  Call LogDesktopShortcut
  !verbose pop
!macroend

/**
 * Adds a QuickLaunch shortcut entry to the shortcuts log ini file.
 *
 * @param   _FILE_NAME
 *          The shortcut file name (e.g. shortcut.lnk)
 */
!macro LogQuickLaunchShortcut

  !ifndef LogQuickLaunchShortcut
    !insertmacro LogShortcut

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define LogQuickLaunchShortcut "!insertmacro LogQuickLaunchShortcutCall"

    Function LogQuickLaunchShortcut
      Call LogShortcut
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro LogQuickLaunchShortcutCall _FILE_NAME
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "QUICKLAUNCH"
  Push "${_FILE_NAME}"
  Call LogQuickLaunchShortcut
  !verbose pop
!macroend

/**
 * Adds a Start Menu shortcut entry to the shortcuts log ini file.
 *
 * @param   _FILE_NAME
 *          The shortcut file name (e.g. shortcut.lnk)
 */
!macro LogStartMenuShortcut

  !ifndef LogStartMenuShortcut
    !insertmacro LogShortcut

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define LogStartMenuShortcut "!insertmacro LogStartMenuShortcutCall"

    Function LogStartMenuShortcut
      Call LogShortcut
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro LogStartMenuShortcutCall _FILE_NAME
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "STARTMENU"
  Push "${_FILE_NAME}"
  Call LogStartMenuShortcut
  !verbose pop
!macroend

/**
 * Adds a Start Menu Programs shortcut entry to the shortcuts log ini file.
 *
 * @param   _FILE_NAME
 *          The shortcut file name (e.g. shortcut.lnk)
 */
!macro LogSMProgramsShortcut

  !ifndef LogSMProgramsShortcut
    !insertmacro LogShortcut

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define LogSMProgramsShortcut "!insertmacro LogSMProgramsShortcutCall"

    Function LogSMProgramsShortcut
      Call LogShortcut
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro LogSMProgramsShortcutCall _FILE_NAME
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "SMPROGRAMS"
  Push "${_FILE_NAME}"
  Call LogSMProgramsShortcut
  !verbose pop
!macroend

/**
 * Adds the relative path from the Start Menu Programs directory for the
 * application's Start Menu directory if it is different from the existing value
 * to the shortcuts log ini file.
 *
 * @param   _REL_PATH_TO_DIR
 *          The relative path from the Start Menu Programs directory to the
 *          program's directory.
 *
 * $R9 = _REL_PATH_TO_DIR
 */
!macro LogSMProgramsDirRelPath _REL_PATH_TO_DIR
  Push $R9

  !insertmacro initShortcutsLog

  ReadINIStr $R9 "$INSTDIR\uninstall\${SHORTCUTS_LOG}" "SMPROGRAMS" "RelativePathToDir"
  StrCmp "$R9" "${_REL_PATH_TO_DIR}" +2 +1
  WriteINIStr "$INSTDIR\uninstall\${SHORTCUTS_LOG}" "SMPROGRAMS" "RelativePathToDir" "${_REL_PATH_TO_DIR}"

  Pop $R9
!macroend
!define LogSMProgramsDirRelPath "!insertmacro LogSMProgramsDirRelPath"

/**
 * Copies the value for the relative path from the Start Menu programs directory
 * (e.g. $SMPROGRAMS) to the Start Menu directory as it is stored in the
 * shortcuts log ini file to the variable specified in the first parameter.
 */
!macro GetSMProgramsDirRelPath _VAR
  ReadINIStr ${_VAR} "$INSTDIR\uninstall\${SHORTCUTS_LOG}" "SMPROGRAMS" \
             "RelativePathToDir"
!macroend
!define GetSMProgramsDirRelPath "!insertmacro GetSMProgramsDirRelPath"

/**
 * Copies the shortcuts log ini file path to the variable specified in the
 * first parameter.
 */
!macro GetShortcutsLogPath _VAR
  StrCpy ${_VAR} "$INSTDIR\uninstall\${SHORTCUTS_LOG}"
!macroend
!define GetShortcutsLogPath "!insertmacro GetShortcutsLogPath"

/**
 * Deletes the shortcuts log ini file.
 */
!macro DeleteShortcutsLogFile
  ${DeleteFile} "$INSTDIR\uninstall\${SHORTCUTS_LOG}"
!macroend
!define DeleteShortcutsLogFile "!insertmacro DeleteShortcutsLogFile"


################################################################################
# Macros for managing specific Windows version features

/**
 * Sets the permitted layered service provider (LSP) categories on Windows
 * Vista and above for the application. Consumers should call this after an
 * installation log section has completed since this macro will log the results
 * to the installation log along with a header.
 *
 * !IMPORTANT - When calling this macro from an uninstaller do not specify a
 *              parameter. The paramter is hardcoded with 0x00000000 to remove
 *              the LSP category for the application when performing an
 *              uninstall.
 *
 * @param   _LSP_CATEGORIES
 *          The permitted LSP categories for the application. When called by an
 *          uninstaller this will always be 0x00000000.
 *
 * $R5 = error code popped from the stack for the WSCSetApplicationCategory call
 * $R6 = return value from the WSCSetApplicationCategory call
 * $R7 = string length for the long path to the main application executable
 * $R8 = long path to the main application executable
 * $R9 = _LSP_CATEGORIES
 */
!macro SetAppLSPCategories

  !ifndef ${_MOZFUNC_UN}SetAppLSPCategories
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}SetAppLSPCategories "!insertmacro ${_MOZFUNC_UN}SetAppLSPCategoriesCall"

    Function ${_MOZFUNC_UN}SetAppLSPCategories
      ${Unless} ${AtLeastWinVista}
        Return
      ${EndUnless}

      Exch $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5

      ${${_MOZFUNC_UN}GetLongPath} "$INSTDIR\${FileMainEXE}" $R8
      StrLen $R7 "$R8"

      ; Remove existing categories by setting the permitted categories to
      ; 0x00000000 since new categories are ANDed with existing categories. If
      ; the param value stored in $R9 is 0x00000000 then skip the removal since
      ; the categories  will be removed by the second call to
      ; WSCSetApplicationCategory.
      StrCmp "$R9" "0x00000000" +2 +1
      System::Call "Ws2_32::WSCSetApplicationCategory(w R8, i R7, w n, i 0,\
                                                      i 0x00000000, i n, *i) i"

      ; Set the permitted LSP categories
      System::Call "Ws2_32::WSCSetApplicationCategory(w R8, i R7, w n, i 0,\
                                                      i R9, i n, *i .s) i.R6"
      Pop $R5

!ifndef NO_LOG
      ${LogHeader} "Setting Permitted LSP Categories"
      StrCmp "$R6" 0 +3 +1
      ${LogMsg} "** ERROR Setting LSP Categories: $R5 **"
      GoTo +2
      ${LogMsg} "Permitted LSP Categories: $R9"
!endif

      ClearErrors

      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro SetAppLSPCategoriesCall _LSP_CATEGORIES
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_LSP_CATEGORIES}"
  Call SetAppLSPCategories
  !verbose pop
!macroend

!macro un.SetAppLSPCategoriesCall
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "0x00000000"
  Call un.SetAppLSPCategories
  !verbose pop
!macroend

!macro un.SetAppLSPCategories
  !ifndef un.SetAppLSPCategories
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro SetAppLSPCategories

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

/**
 * Checks if any pinned TaskBar lnk files point to the executable's path passed
 * to the macro.
 *
 * @param   _EXE_PATH
 *          The executable path
 * @return  _RESULT
 *          false if no pinned shotcuts were found for this install location.
 *          true if pinned shotcuts were found for this install location.
 *
 * $R5 = stores whether a TaskBar lnk file has been found for the executable
 * $R6 = long path returned from GetShortCutTarget and GetLongPath
 * $R7 = file name returned from FindFirst and FindNext
 * $R8 = find handle for FindFirst and FindNext
 * $R9 = _EXE_PATH and _RESULT
 */
!macro IsPinnedToTaskBar

  !ifndef IsPinnedToTaskBar
    !insertmacro GetLongPath

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define IsPinnedToTaskBar "!insertmacro IsPinnedToTaskBarCall"

    Function IsPinnedToTaskBar
      Exch $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5

      StrCpy $R5 "false"

      ${If} ${AtLeastWin7}
      ${AndIf} ${FileExists} "$QUICKLAUNCH\User Pinned\TaskBar"
        FindFirst $R8 $R7 "$QUICKLAUNCH\User Pinned\TaskBar\*.lnk"
        ${Do}
          ${If} ${FileExists} "$QUICKLAUNCH\User Pinned\TaskBar\$R7"
            ShellLink::GetShortCutTarget "$QUICKLAUNCH\User Pinned\TaskBar\$R7"
            Pop $R6
            ${GetLongPath} "$R6" $R6
            ${If} "$R6" == "$R9"
              StrCpy $R5 "true"
              ${ExitDo}
            ${EndIf}
          ${EndIf}
          ClearErrors
          FindNext $R8 $R7
          ${If} ${Errors}
            ${ExitDo}
          ${EndIf}
        ${Loop}
        FindClose $R8
      ${EndIf}

      ClearErrors

      StrCpy $R9 $R5

      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro IsPinnedToTaskBarCall _EXE_PATH _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_EXE_PATH}"
  Call IsPinnedToTaskBar
  Pop ${_RESULT}
  !verbose pop
!macroend

/**
 * Checks if any pinned Start Menu lnk files point to the executable's path
 * passed to the macro.
 *
 * @param   _EXE_PATH
 *          The executable path
 * @return  _RESULT
 *          false if no pinned shotcuts were found for this install location.
 *          true if pinned shotcuts were found for this install location.
 *
 * $R5 = stores whether a Start Menu lnk file has been found for the executable
 * $R6 = long path returned from GetShortCutTarget and GetLongPath
 * $R7 = file name returned from FindFirst and FindNext
 * $R8 = find handle for FindFirst and FindNext
 * $R9 = _EXE_PATH and _RESULT
 */
!macro IsPinnedToStartMenu

  !ifndef IsPinnedToStartMenu
    !insertmacro GetLongPath

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define IsPinnedToStartMenu "!insertmacro IsPinnedToStartMenuCall"

    Function IsPinnedToStartMenu
      Exch $R9
      Push $R8
      Push $R7
      Push $R6
      Push $R5

      StrCpy $R5 "false"

      ${If} ${AtLeastWin7}
      ${AndIf} ${FileExists} "$QUICKLAUNCH\User Pinned\StartMenu"
        FindFirst $R8 $R7 "$QUICKLAUNCH\User Pinned\StartMenu\*.lnk"
        ${Do}
          ${If} ${FileExists} "$QUICKLAUNCH\User Pinned\StartMenu\$R7"
            ShellLink::GetShortCutTarget "$QUICKLAUNCH\User Pinned\StartMenu\$R7"
            Pop $R6
            ${GetLongPath} "$R6" $R6
            ${If} "$R6" == "$R9"
              StrCpy $R5 "true"
              ${ExitDo}
            ${EndIf}
          ${EndIf}
          ClearErrors
          FindNext $R8 $R7
          ${If} ${Errors}
            ${ExitDo}
          ${EndIf}
        ${Loop}
        FindClose $R8
      ${EndIf}

      ClearErrors

      StrCpy $R9 $R5

      Pop $R5
      Pop $R6
      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro IsPinnedToStartMenuCall _EXE_PATH _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_EXE_PATH}"
  Call IsPinnedToStartMenu
  Pop ${_RESULT}
  !verbose pop
!macroend

/**
 * Gets the number of pinned shortcut lnk files pinned to the Task Bar.
 *
 * @return  _RESULT
 *          number of pinned shortcut lnk files.
 *
 * $R7 = file name returned from FindFirst and FindNext
 * $R8 = find handle for FindFirst and FindNext
 * $R9 = _RESULT
 */
!macro PinnedToTaskBarLnkCount

  !ifndef PinnedToTaskBarLnkCount
    !insertmacro GetLongPath

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define PinnedToTaskBarLnkCount "!insertmacro PinnedToTaskBarLnkCountCall"

    Function PinnedToTaskBarLnkCount
      Push $R9
      Push $R8
      Push $R7

      StrCpy $R9 0

      ${If} ${AtLeastWin7}
      ${AndIf} ${FileExists} "$QUICKLAUNCH\User Pinned\TaskBar"
        FindFirst $R8 $R7 "$QUICKLAUNCH\User Pinned\TaskBar\*.lnk"
        ${Do}
          ${If} ${FileExists} "$QUICKLAUNCH\User Pinned\TaskBar\$R7"
            IntOp $R9 $R9 + 1
          ${EndIf}
          ClearErrors
          FindNext $R8 $R7
          ${If} ${Errors}
            ${ExitDo}
          ${EndIf}
        ${Loop}
        FindClose $R8
      ${EndIf}

      ClearErrors

      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro PinnedToTaskBarLnkCountCall _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call PinnedToTaskBarLnkCount
  Pop ${_RESULT}
  !verbose pop
!macroend

/**
 * Gets the number of pinned shortcut lnk files pinned to the Start Menu.
 *
 * @return  _RESULT
 *          number of pinned shortcut lnk files.
 *
 * $R7 = file name returned from FindFirst and FindNext
 * $R8 = find handle for FindFirst and FindNext
 * $R9 = _RESULT
 */
!macro PinnedToStartMenuLnkCount

  !ifndef PinnedToStartMenuLnkCount
    !insertmacro GetLongPath

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define PinnedToStartMenuLnkCount "!insertmacro PinnedToStartMenuLnkCountCall"

    Function PinnedToStartMenuLnkCount
      Push $R9
      Push $R8
      Push $R7

      StrCpy $R9 0

      ${If} ${AtLeastWin7}
      ${AndIf} ${FileExists} "$QUICKLAUNCH\User Pinned\StartMenu"
        FindFirst $R8 $R7 "$QUICKLAUNCH\User Pinned\StartMenu\*.lnk"
        ${Do}
          ${If} ${FileExists} "$QUICKLAUNCH\User Pinned\StartMenu\$R7"
            IntOp $R9 $R9 + 1
          ${EndIf}
          ClearErrors
          FindNext $R8 $R7
          ${If} ${Errors}
            ${ExitDo}
          ${EndIf}
        ${Loop}
        FindClose $R8
      ${EndIf}

      ClearErrors

      Pop $R7
      Pop $R8
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro PinnedToStartMenuLnkCountCall _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Call PinnedToStartMenuLnkCount
  Pop ${_RESULT}
  !verbose pop
!macroend

/**
 * Update Start Menu / TaskBar lnk files that point to the executable's path
 * passed to the macro and all other shortcuts installed by the application with
 * the current application user model ID. Requires ApplicationID.
 *
 * NOTE: this does not update Desktop shortcut application user model ID due to
 *       bug 633728.
 *
 * @param   _EXE_PATH
 *          The main application executable path
 * @param   _APP_ID
 *          The application user model ID for the current install
 * @return  _RESULT
 *          false if no pinned shotcuts were found for this install location.
 *          true if pinned shotcuts were found for this install location.
 */
!macro UpdateShortcutAppModelIDs

  !ifndef UpdateShortcutAppModelIDs
    !insertmacro GetLongPath

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define UpdateShortcutAppModelIDs "!insertmacro UpdateShortcutAppModelIDsCall"

    Function UpdateShortcutAppModelIDs
      ; stack: path, appid
      Exch $R9 ; stack: $R9, appid | $R9 = path
      Exch 1   ; stack: appid, $R9
      Exch $R8 ; stack: $R8, $R9   | $R8 = appid
      Push $R7 ; stack: $R7, $R8, $R9
      Push $R6
      Push $R5
      Push $R4
      Push $R3 ; stack: $R3, $R5, $R6, $R7, $R8, $R9
      Push $R2

      ; $R9 = main application executable path
      ; $R8 = appid
      ; $R7 = path to the application's start menu programs directory
      ; $R6 = path to the shortcut log ini file
      ; $R5 = shortcut filename
      ; $R4 = GetShortCutTarget result

      StrCpy $R3 "false"

      ${If} ${AtLeastWin7}
        ; installed shortcuts
        ${${_MOZFUNC_UN}GetLongPath} "$INSTDIR\uninstall\${SHORTCUTS_LOG}" $R6
        ${If} ${FileExists} "$R6"
          ; Update the Start Menu shortcuts' App ID for this application
          StrCpy $R2 -1
          ${Do}
            IntOp $R2 $R2 + 1 ; Increment the counter
            ClearErrors
            ReadINIStr $R5 "$R6" "STARTMENU" "Shortcut$R2"
            ${If} ${Errors}
              ${ExitDo}
            ${EndIf}

            ${If} ${FileExists} "$SMPROGRAMS\$R5"
              ShellLink::GetShortCutTarget "$SMPROGRAMS\$$R5"
              Pop $R4
              ${GetLongPath} "$R4" $R4
              ${If} "$R4" == "$R9" ; link path == install path
                ApplicationID::Set "$SMPROGRAMS\$R5" "$R8"
                Pop $R4
              ${EndIf}
            ${EndIf}
          ${Loop}

          ; Update the Quick Launch shortcuts' App ID for this application
          StrCpy $R2 -1
          ${Do}
            IntOp $R2 $R2 + 1 ; Increment the counter
            ClearErrors
            ReadINIStr $R5 "$R6" "QUICKLAUNCH" "Shortcut$R2"
            ${If} ${Errors}
              ${ExitDo}
            ${EndIf}

            ${If} ${FileExists} "$QUICKLAUNCH\$R5"
              ShellLink::GetShortCutTarget "$QUICKLAUNCH\$R5"
              Pop $R4
              ${GetLongPath} "$R4" $R4
              ${If} "$R4" == "$R9" ; link path == install path
                ApplicationID::Set "$QUICKLAUNCH\$R5" "$R8"
                Pop $R4
              ${EndIf}
            ${EndIf}
          ${Loop}

          ; Update the Start Menu Programs shortcuts' App ID for this application
          ClearErrors
          ReadINIStr $R7 "$R6" "SMPROGRAMS" "RelativePathToDir"
          ${Unless} ${Errors}
            ${${_MOZFUNC_UN}GetLongPath} "$SMPROGRAMS\$R7" $R7
            ${Unless} "$R7" == ""
              StrCpy $R2 -1
              ${Do}
                IntOp $R2 $R2 + 1 ; Increment the counter
                ClearErrors
                ReadINIStr $R5 "$R6" "SMPROGRAMS" "Shortcut$R2"
                ${If} ${Errors}
                  ${ExitDo}
                ${EndIf}

                ${If} ${FileExists} "$R7\$R5"
                  ShellLink::GetShortCutTarget "$R7\$R5"
                  Pop $R4
                  ${GetLongPath} "$R4" $R4
                  ${If} "$R4" == "$R9" ; link path == install path
                    ApplicationID::Set "$R7\$R5" "$R8"
                    Pop $R4
                  ${EndIf}
                ${EndIf}
              ${Loop}
            ${EndUnless}
          ${EndUnless}
        ${EndIf}

        StrCpy $R7 "$QUICKLAUNCH\User Pinned"
        StrCpy $R3 "false"

        ; $R9 = main application executable path
        ; $R8 = appid
        ; $R7 = user pinned path
        ; $R6 = find handle
        ; $R5 = found filename
        ; $R4 = GetShortCutTarget result

        ; TaskBar links
        FindFirst $R6 $R5 "$R7\TaskBar\*.lnk"
        ${Do}
          ${If} ${FileExists} "$R7\TaskBar\$R5"
            ShellLink::GetShortCutTarget "$R7\TaskBar\$R5"
            Pop $R4
            ${If} "$R4" == "$R9" ; link path == install path
              ApplicationID::Set "$R7\TaskBar\$R5" "$R8"
              Pop $R4 ; pop Set result off the stack
              StrCpy $R3 "true"
            ${EndIf}
          ${EndIf}
          ClearErrors
          FindNext $R6 $R5
          ${If} ${Errors}
            ${ExitDo}
          ${EndIf}
        ${Loop}
        FindClose $R6

        ; Start menu links
        FindFirst $R6 $R5 "$R7\StartMenu\*.lnk"
        ${Do}
          ${If} ${FileExists} "$R7\StartMenu\$R5"
            ShellLink::GetShortCutTarget "$R7\StartMenu\$R5"
            Pop $R4
            ${If} "$R4" == "$R9" ; link path == install path
              ApplicationID::Set "$R7\StartMenu\$R5" "$R8"
              Pop $R4 ; pop Set result off the stack
              StrCpy $R3 "true"
            ${EndIf}
          ${EndIf}
          ClearErrors
          FindNext $R6 $R5
          ${If} ${Errors}
            ${ExitDo}
          ${EndIf}
        ${Loop}
        FindClose $R6
      ${EndIf}

      ClearErrors

      StrCpy $R9 $R3

      Pop $R2
      Pop $R3  ; stack: $R4, $R5, $R6, $R7, $R8, $R9
      Pop $R4  ; stack: $R5, $R6, $R7, $R8, $R9
      Pop $R5  ; stack: $R6, $R7, $R8, $R9
      Pop $R6  ; stack: $R7, $R8, $R9
      Pop $R7  ; stack: $R8, $R9
      Exch $R8 ; stack: appid, $R9 | $R8 = old $R8
      Exch 1   ; stack: $R9, appid
      Exch $R9 ; stack: path, appid | $R9 = old $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro UpdateShortcutAppModelIDsCall _EXE_PATH _APP_ID _RESULT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_APP_ID}"
  Push "${_EXE_PATH}"
  Call UpdateShortcutAppModelIDs
  Pop ${_RESULT}
  !verbose pop
!macroend

!macro IsUserAdmin
  ; Copied from: http://nsis.sourceforge.net/IsUserAdmin
  Function IsUserAdmin
    Push $R0
    Push $R1
    Push $R2

    ClearErrors
    UserInfo::GetName
    IfErrors Win9x
    Pop $R1
    UserInfo::GetAccountType
    Pop $R2

    StrCmp $R2 "Admin" 0 Continue
    StrCpy $R0 "true"
    Goto Done

    Continue:

    StrCmp $R2 "" Win9x
    StrCpy $R0 "false"
    Goto Done

    Win9x:
    StrCpy $R0 "true"

    Done:
    Pop $R2
    Pop $R1
    Exch $R0
  FunctionEnd
!macroend

/**
 * Retrieve if present or generate and store a 64 bit hash of an install path
 * using the City Hash algorithm.  On return the resulting id is saved in the
 * $AppUserModelID variable declared by inserting this macro. InitHashAppModelId
 * will attempt to load from HKLM/_REG_PATH first, then HKCU/_REG_PATH. If found
 * in either it will return the hash it finds. If not found it will generate a
 * new hash and attempt to store the hash in HKLM/_REG_PATH, then HKCU/_REG_PATH.
 * Subsequent calls will then retreive the stored hash value. On any failure,
 * $AppUserModelID will be set to an empty string.
 *
 * Registry format: root/_REG_PATH/"_EXE_PATH" = "hash"
 *
 * @param   _EXE_PATH
 *          The main application executable path
 * @param   _REG_PATH
 *          The HKLM/HKCU agnostic registry path where the key hash should
 *          be stored. ex: "Software\Mozilla\Firefox\TaskBarIDs"
 * @result  (Var) $AppUserModelID contains the app model id.
 */
!macro InitHashAppModelId
  !ifndef ${_MOZFUNC_UN}InitHashAppModelId
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}GetLongPath
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !ifndef InitHashAppModelId
      Var AppUserModelID
    !endif

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}InitHashAppModelId "!insertmacro ${_MOZFUNC_UN}InitHashAppModelIdCall"

    Function ${_MOZFUNC_UN}InitHashAppModelId
      ; stack: apppath, regpath
      Exch $R9 ; stack: $R9, regpath | $R9 = apppath
      Exch 1   ; stack: regpath, $R9
      Exch $R8 ; stack: $R8, $R9   | $R8 = regpath
      Push $R7

      ${If} ${AtLeastWin7}
        ${${_MOZFUNC_UN}GetLongPath} "$R9" $R9
        ClearErrors
        ReadRegStr $R7 HKLM "$R8" "$R9"
        ${If} ${Errors}
          ClearErrors
          ReadRegStr $R7 HKCU "$R8" "$R9"
          ${If} ${Errors}
            ; If it doesn't exist, create a new one and store it
            CityHash::GetCityHash64 "$R9"
            Pop $AppUserModelID
            ${If} $AppUserModelID == "error"
              GoTo end
            ${EndIf}
            ClearErrors
            WriteRegStr HKLM "$R8" "$R9" "$AppUserModelID"
            ${If} ${Errors}
              ClearErrors
              WriteRegStr HKCU "$R8" "$R9" "$AppUserModelID"
              ${If} ${Errors}
                StrCpy $AppUserModelID "error"
              ${EndIf}
            ${EndIf}
          ${Else}
            StrCpy $AppUserModelID $R7
          ${EndIf}
        ${Else}
          StrCpy $AppUserModelID $R7
        ${EndIf}
      ${EndIf}

      end:
      ${If} "$AppUserModelID" == "error"
        StrCpy $AppUserModelID ""
      ${EndIf}

      ClearErrors
      Pop $R7
      Exch $R8
      Exch 1
      Exch $R9
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro InitHashAppModelIdCall _EXE_PATH _REG_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_REG_PATH}"
  Push "${_EXE_PATH}"
  Call InitHashAppModelId
  !verbose pop
!macroend

!macro un.InitHashAppModelIdCall _EXE_PATH _REG_PATH
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_REG_PATH}"
  Push "${_EXE_PATH}"
  Call un.InitHashAppModelId
  !verbose pop
!macroend

!macro un.InitHashAppModelId
  !ifndef un.InitHashAppModelId
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro InitHashAppModelId

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend


################################################################################
# Helpers for the new user interface

!define DT_WORDBREAK 0x0010
!define DT_SINGLELINE 0x0020
!define DT_NOCLIP 0x0100
!define DT_CALCRECT 0x0400
!define DT_EDITCONTROL 0x2000
!define DT_RTLREADING 0x00020000
!define DT_NOFULLWIDTHCHARBREAK 0x00080000

!define WS_EX_NOINHERITLAYOUT 0x00100000
!define WS_EX_LAYOUTRTL 0x00400000

!define PBS_MARQUEE 0x08

!define /math PBM_SETRANGE32 ${WM_USER} + 6

!define SHACF_FILESYSTEM 1

!define MOZ_LOADTRANSPARENT ${LR_LOADFROMFILE}|${LR_LOADTRANSPARENT}|${LR_LOADMAP3DCOLORS}

; Extend nsDialogs.nsh to support creating centered labels if it is already
; included
!ifmacrodef __NSD_DefineControl
!insertmacro __NSD_DefineControl LabelCenter
!define __NSD_LabelCenter_CLASS STATIC
!define __NSD_LabelCenter_STYLE ${DEFAULT_STYLES}|${SS_NOTIFY}|${SS_CENTER}
!define __NSD_LabelCenter_EXSTYLE ${WS_EX_TRANSPARENT}
!endif

/**
 * Modified version of the __NSD_SetStretchedImage macro from nsDialogs.nsh that
 * supports transparency. See nsDialogs documentation for additional info.
 */
!macro __SetStretchedTransparentImage CONTROL IMAGE HANDLE
  Push $0
  Push $1
  Push $2
  Push $R0

  StrCpy $R0 ${CONTROL} ; in case ${CONTROL} is $0
  StrCpy $1 ""
  StrCpy $2 ""

  System::Call '*(i, i, i, i) i.s'
  Pop $0

  ${If} $0 <> 0
    System::Call 'user32::GetClientRect(i R0, i r0)'
    System::Call '*$0(i, i, i .s, i .s)'
    System::Free $0
    Pop $1
    Pop $2
  ${EndIf}

  System::Call 'user32::LoadImageW(i 0, t s, i ${IMAGE_BITMAP}, i r1, i r2, \
                                   i ${MOZ_LOADTRANSPARENT}) i .s' "${IMAGE}"
  Pop $0
  SendMessage $R0 ${STM_SETIMAGE} ${IMAGE_BITMAP} $0

  SetCtlColors $R0 "" transparent
  ${NSD_AddExStyle} $R0 ${WS_EX_TRANSPARENT}|${WS_EX_TOPMOST}

  Pop $R0
  Pop $2
  Pop $1
  Exch $0
  Pop ${HANDLE}
!macroend
!define SetStretchedTransparentImage `!insertmacro __SetStretchedTransparentImage`

/**
 * Removes a single style from a control.
 *
 * _HANDLE the handle of the control
 * _STYLE  the style to remove
 */
!macro _RemoveStyle _HANDLE _STYLE
  Push $0

  System::Call 'user32::GetWindowLongW(i ${_HANDLE}, i ${GWL_STYLE}) i .r0'
  IntOp $0 $0 | ${_STYLE}
  IntOp $0 $0 - ${_STYLE}
  System::Call 'user32::SetWindowLongW(i ${_HANDLE}, i ${GWL_STYLE}, i r0)'

  Pop $0
!macroend
!define RemoveStyle "!insertmacro _RemoveStyle"

/**
 * Removes a single extended style from a control.
 *
 * _HANDLE  the handle of the control
 * _EXSTYLE the extended style to remove
 */
!macro _RemoveExStyle _HANDLE _EXSTYLE
  Push $0

  System::Call 'user32::GetWindowLongW(i ${_HANDLE}, i ${GWL_EXSTYLE}) i .r0'
  IntOp $0 $0 | ${_EXSTYLE}
  IntOp $0 $0 - ${_EXSTYLE}
  System::Call 'user32::SetWindowLongW(i ${_HANDLE}, i ${GWL_EXSTYLE}, i r0)'

  Pop $0
!macroend
!define RemoveExStyle "!insertmacro _RemoveExStyle"

/**
 * Gets the extent of the specified text in pixels for sizing a control.
 *
 * _TEXT       the text to get the text extent for
 * _FONT       the font to use when getting the text extent
 * _RES_WIDTH  return value - control width for the text
 * _RES_HEIGHT return value - control height for the text
 */
!macro GetTextExtentCall _TEXT _FONT _RES_WIDTH _RES_HEIGHT
  Push "${_TEXT}"
  Push "${_FONT}"
  ${CallArtificialFunction} GetTextExtent_
  Pop ${_RES_WIDTH}
  Pop ${_RES_HEIGHT}
!macroend

!define GetTextExtent "!insertmacro GetTextExtentCall"
!define un.GetTextExtent "!insertmacro GetTextExtentCall"

!macro GetTextExtent_
  Exch $0 ; font
  Exch 1
  Exch $1 ; text
  Push $2
  Push $3
  Push $4
  Push $5
  Push $6
  Push $7

  ; Reuse the existing NSIS control which is used for BrandingText instead of
  ; creating a new control.
  GetDlgItem $2 $HWNDPARENT 1028

  System::Call 'user32::GetDC(i r2) i .r3'
  System::Call 'gdi32::SelectObject(i r3, i r0)'

  StrLen $4 "$1"

  System::Call '*(i, i) i .r5'
  System::Call 'gdi32::GetTextExtentPoint32W(i r3, t$\"$1$\", i r4, i r5)'
  System::Call '*$5(i .r6, i .r7)'
  System::Free $5

  System::Call 'user32::ReleaseDC(i r2, i r3)'

  StrCpy $1 $7
  StrCpy $0 $6

  Pop $7
  Pop $6
  Pop $5
  Pop $4
  Pop $3
  Pop $2
  Exch $1 ; return height
  Exch 1
  Exch $0 ; return width
!macroend

/**
 * Gets the width and the height of a control in pixels.
 *
 * _HANDLE     the handle of the control
 * _RES_WIDTH  return value - control width for the text
 * _RES_HEIGHT return value - control height for the text
 */
!macro GetDlgItemWidthHeightCall _HANDLE _RES_WIDTH _RES_HEIGHT
  Push "${_HANDLE}"
  ${CallArtificialFunction} GetDlgItemWidthHeight_
  Pop ${_RES_WIDTH}
  Pop ${_RES_HEIGHT}
!macroend

!define GetDlgItemWidthHeight "!insertmacro GetDlgItemWidthHeightCall"
!define un.GetDlgItemWidthHeight "!insertmacro GetDlgItemWidthHeightCall"

!macro GetDlgItemWidthHeight_
  Exch $0 ; handle for the control
  Push $1
  Push $2

  System::Call '*(i, i, i, i) i .r2'
  ; The left and top values will always be 0 so the right and bottom values
  ; will be the width and height.
  System::Call 'user32::GetClientRect(i r0, i r2)'
  System::Call '*$2(i, i, i .r0, i .r1)'
  System::Free $2

  Pop $2
  Exch $1 ; return height
  Exch 1
  Exch $0 ; return width
!macroend

/**
 * Gets the number of pixels from the beginning of the dialog to the end of a
 * control in a RTL friendly manner.
 *
 * _HANDLE the handle of the control
 * _RES_PX return value - pixels from the beginning of the dialog to the end of
 *         the control
 */
!macro GetDlgItemEndPXCall _HANDLE _RES_PX
  Push "${_HANDLE}"
  ${CallArtificialFunction} GetDlgItemEndPX_
  Pop ${_RES_PX}
!macroend

!define GetDlgItemEndPX "!insertmacro GetDlgItemEndPXCall"
!define un.GetDlgItemEndPX "!insertmacro GetDlgItemEndPXCall"

!macro GetDlgItemEndPX_
  Exch $0 ; handle of the control
  Push $1
  Push $2

  ; #32770 is the dialog class
  FindWindow $1 "#32770" "" $HWNDPARENT
  System::Call '*(i, i, i, i) i .r2'
  System::Call 'user32::GetWindowRect(i r0, i r2)'
  System::Call 'user32::MapWindowPoints(i 0, i r1,i r2, i 2)'
  System::Call '*$2(i, i, i .r0, i)'
  System::Free $2

  Pop $2
  Pop $1
  Exch $0 ; pixels from the beginning of the dialog to the end of the control
!macroend

/**
 * Gets the width and height for sizing a control that has the specified text.
 * If the text has embedded newlines then the width and height will be
 * determined without trying to optimize the control's width and height. If the
 * text doesn't contain newlines the control's height and width will be
 * dynamically determined using a minimum of 3 lines (incrementing the
 * number of lines if necessary) for the height and the maximum width specified.
 *
 * _TEXT       the text
 * _FONT       the font to use when getting the width and height
 * _MAX_WIDTH  the maximum width for the control
 * _RES_WIDTH  return value - control width for the text
 * _RES_HEIGHT return value - control height for the text
 */
!macro GetTextWidthHeight

  !ifndef ${_MOZFUNC_UN}GetTextWidthHeight
    !define _MOZFUNC_UN_TMP ${_MOZFUNC_UN}
    !insertmacro ${_MOZFUNC_UN_TMP}WordFind
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN ${_MOZFUNC_UN_TMP}
    !undef _MOZFUNC_UN_TMP

    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !define ${_MOZFUNC_UN}GetTextWidthHeight "!insertmacro ${_MOZFUNC_UN}GetTextWidthHeightCall"

    Function ${_MOZFUNC_UN}GetTextWidthHeight
      Exch $0  ; maximum width use to calculate the control's width and height
      Exch 1
      Exch $1  ; font
      Exch 2
      Exch $2  ; text
      Push $3
      Push $4
      Push $5
      Push $6
      Push $7
      Push $8
      Push $9
      Push $R0
      Push $R1
      Push $R2

      StrCpy $R2 "${DT_NOCLIP}|${DT_CALCRECT}"
      !ifdef ${AB_CD}_rtl
        StrCpy $R2 "$R2|${DT_RTLREADING}"
      !endif

      ; Reuse the existing NSIS control which is used for BrandingText instead
      ; of creating a new control.
      GetDlgItem $3 $HWNDPARENT 1028

      System::Call 'user32::GetDC(i r3) i .r4'
      System::Call 'gdi32::SelectObject(i r4, i r1)'

      StrLen $5 "$2" ; text length
      System::Call '*(i, i, i, i) i .r6'

      ClearErrors
      ${${_MOZFUNC_UN}WordFind} "$2" "$\n" "E#" $R0
      ${If} ${Errors}
        ; When there aren't newlines in the text calculate the size of the
        ; rectangle needed for the text with a minimum of three lines of text.
        ClearErrors
        System::Call 'user32::DrawTextW(i r4, t $\"$2$\", i r5, i r6, \
                                        i $R2|${DT_SINGLELINE})'
        System::Call '*$6(i, i, i .r8, i .r7)'
        System::Free $6

        ; Get the approximate number height needed to display the text starting
        ; with a minimum of 3 lines of text.
        StrCpy $9 $8
        StrCpy $R1 2 ; set the number of lines initially to 2
        ${While} $9 > $0
          IntOp $R1 $R1 + 1 ; increment the number of lines
          IntOp $9 $8 / $R1
        ${EndWhile}
        IntOp $7 $7 * $R1

        StrCpy $R0 $9
        ${Do}
          IntOp $R0 $R0 + 20
          System::Call '*(i, i, i R0, i r7) i .r6'
          System::Call 'user32::DrawTextW(i r4, t $\"$2$\", i r5, i r6, \
                                          i $R2|${DT_WORDBREAK}|${DT_NOFULLWIDTHCHARBREAK}) i .R1'
          System::Call '*$6(i, i, i .r8, i .r9)'
          System::Free $6
        ${LoopUntil} $7 >= $R1
      ${Else}
        ; When there are newlines in the text just return the size of the
        ; rectangle for the text.
        System::Call 'user32::DrawTextW(i r4, t $\"$2$\", i r5, i r6, i $R2)'
        System::Call '*$6(i, i, i .r8, i .r9)'
        System::Free $6
      ${EndIf}

      ; Reselect the original DC
      System::Call 'gdi32::SelectObject(i r4, i r1)'
      System::Call 'user32::ReleaseDC(i r3, i r4)'

      StrCpy $1 $9
      StrCpy $0 $8

      Pop $R2
      Pop $R1
      Pop $R0
      Pop $9
      Pop $8
      Pop $7
      Pop $6
      Pop $5
      Pop $4
      Pop $3
      Exch $2
      Exch 2
      Exch $1 ; return height
      Exch 1
      Exch $0 ; return width
    FunctionEnd

    !verbose pop
  !endif
!macroend

!macro GetTextWidthHeightCall _TEXT _FONT _MAX_WIDTH _RES_WIDTH _RES_HEIGHT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_TEXT}"
  Push "${_FONT}"
  Push "${_MAX_WIDTH}"
  Call GetTextWidthHeight
  Pop ${_RES_WIDTH}
  Pop ${_RES_HEIGHT}
  !verbose pop
!macroend

!macro un.GetTextWidthHeightCall _TEXT _FONT _MAX_WIDTH _RES_WIDTH _RES_HEIGHT
  !verbose push
  !verbose ${_MOZFUNC_VERBOSE}
  Push "${_TEXT}"
  Push "${_FONT}"
  Push "${_MAX_WIDTH}"
  Call un.GetTextWidthHeight
  Pop ${_RES_WIDTH}
  Pop ${_RES_HEIGHT}
  !verbose pop
!macroend

!macro un.GetTextWidthHeight
  !ifndef un.GetTextWidthHeight
    !verbose push
    !verbose ${_MOZFUNC_VERBOSE}
    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN "un."

    !insertmacro GetTextWidthHeight

    !undef _MOZFUNC_UN
    !define _MOZFUNC_UN
    !verbose pop
  !endif
!macroend

!ifdef MOZ_METRO
; Removes the CEH registration if it's set to our installation directory.
; If it's set to some other installation directory, then it should be removed
; by that installation. 
!macro RemoveDEHRegistrationIfMatchingCall un
  Function ${un}RemoveDEHRegistrationIfMatchingCall
    ; Move the old $R0 on the stack and set it to DEH ID
    Exch $R0
    ; Backup the old values of R8 and R7 on the stack
    Push $R8
    Push $R7

    ; Conditionally remove the DEH as long as we are the default (HKCU)
    ReadRegStr $R8 HKCU "Software\Classes\CLSID\$R0\LocalServer32" ""
    ${${un}GetLongPath} "$INSTDIR" $R7
    StrCmp "$R8" "" next +1
    IfFileExists "$R8" +1 clearHKCU
    ${${un}GetParent} "$R8" $R8
    ${${un}GetLongPath} "$R8" $R8
    StrCmp "$R7" "$R8" clearHKCU next
    clearHKCU:
    DeleteRegKey HKCU "Software\Classes\CLSID\$R0"
    next:

    ; Conditionally remove the DEH as long as we are the default (HKLM)
    ReadRegStr $R8 HKLM "Software\Classes\CLSID\$R0\LocalServer32" ""
    ${${un}GetLongPath} "$INSTDIR" $R7
    StrCmp "$R8" "" done +1
    IfFileExists "$R8" +1 clearHKLM
    ${${un}GetParent} "$R8" $R8
    ${${un}GetLongPath} "$R8" $R8
    StrCmp "$R7" "$R8" clearHKLM done
    clearHKLM:
    DeleteRegKey HKLM "Software\Classes\CLSID\$R0"
    done:

    ; Always remove the AppUserModelID keys for this installation
    DeleteRegKey HKCU "Software\Classes\$AppUserModelID"
    DeleteRegKey HKLM "Software\Classes\$AppUserModelID"

    ; Restore the stack back to its original state
    Pop $R7
    Pop $R8
    Pop $R0
  FunctionEnd
!macroend

!macro RemoveDEHRegistrationIfMatching
  !insertmacro RemoveDEHRegistrationIfMatchingCall ""
!macroend

!macro un.RemoveDEHRegistrationIfMatching
  !insertmacro RemoveDEHRegistrationIfMatchingCall "un."
!macroend

!macro CleanupMetroBrowserHandlerValues un DELEGATE_EXECUTE_HANDLER_ID
  Push ${DELEGATE_EXECUTE_HANDLER_ID}
  Call ${un}RemoveDEHRegistrationIfMatchingCall
!macroend
!define CleanupMetroBrowserHandlerValues '!insertmacro CleanupMetroBrowserHandlerValues ""'
!define un.CleanupMetroBrowserHandlerValues '!insertmacro CleanupMetroBrowserHandlerValues "un."'

!macro AddMetroBrowserHandlerValues DELEGATE_EXECUTE_HANDLER_ID \
                                    DELEGATE_EXECUTE_HANDLER_PATH \
                                    APP_USER_MODEL_ID \
                                    PROTOCOL_ACTIVATION_ID \
                                    FILE_ACTIVATION_ID
  ; Win8 doesn't use conventional progid command data to launch anymore.
  ; Instead it uses a delegate execute handler which is a light weight COM
  ; server for choosing the metro or desktop browser to launch depending
  ; on the current environment (metro/desktop) it was activated in.
  WriteRegStr SHCTX "Software\Classes\${APP_USER_MODEL_ID}" "" ""
  WriteRegStr SHCTX "Software\Classes\${APP_USER_MODEL_ID}\.exe" "" ""
  WriteRegStr SHCTX "Software\Classes\${APP_USER_MODEL_ID}\.exe\shell" "" "open"
  WriteRegStr SHCTX "Software\Classes\${APP_USER_MODEL_ID}\.exe\shell\open" "CommandId" "open"
  WriteRegStr SHCTX "Software\Classes\${APP_USER_MODEL_ID}\.exe\shell\open\command" "" "$2"
  WriteRegStr SHCTX "Software\Classes\${APP_USER_MODEL_ID}\.exe\shell\open\command" "DelegateExecute" "${DELEGATE_EXECUTE_HANDLER_ID}"

  ; Augment the url handler registrations with additional data needed for Metro
  WriteRegStr SHCTX "Software\Classes\${PROTOCOL_ACTIVATION_ID}" "AppUserModelID" "${APP_USER_MODEL_ID}"
  WriteRegStr SHCTX "Software\Classes\${PROTOCOL_ACTIVATION_ID}\Application" "AppUserModelID" "${APP_USER_MODEL_ID}"
  WriteRegStr SHCTX "Software\Classes\${PROTOCOL_ACTIVATION_ID}\Application" "ApplicationName" "$BrandShortName"
  WriteRegStr SHCTX "Software\Classes\${PROTOCOL_ACTIVATION_ID}\Application" "ApplicationIcon" "$INSTDIR\${FileMainEXE},0"
  WriteRegStr SHCTX "Software\Classes\${PROTOCOL_ACTIVATION_ID}\Application" "ApplicationCompany" "${CompanyName}"
  WriteRegStr SHCTX "Software\Classes\${PROTOCOL_ACTIVATION_ID}\Application" "ApplicationDescription" "$(REG_APP_DESC)"
  WriteRegStr SHCTX "Software\Classes\${PROTOCOL_ACTIVATION_ID}\shell" "" "open"
  WriteRegStr SHCTX "Software\Classes\${PROTOCOL_ACTIVATION_ID}\shell\open" "CommandId" "open"
  WriteRegStr SHCTX "Software\Classes\${PROTOCOL_ACTIVATION_ID}\shell\open\command" "DelegateExecute" "${DELEGATE_EXECUTE_HANDLER_ID}"

  ; Augment the file handler registrations with additional data needed for Metro
  WriteRegStr SHCTX "Software\Classes\${FILE_ACTIVATION_ID}" "AppUserModelID" "${APP_USER_MODEL_ID}"
  WriteRegStr SHCTX "Software\Classes\${FILE_ACTIVATION_ID}\shell" "" "open"
  WriteRegStr SHCTX "Software\Classes\${FILE_ACTIVATION_ID}\shell\open" "CommandId" "open"
  WriteRegStr SHCTX "Software\Classes\${FILE_ACTIVATION_ID}\shell\open\command" "DelegateExecute" "${DELEGATE_EXECUTE_HANDLER_ID}"

  ; Win8 Metro delegate execute handler registration
  WriteRegStr SHCTX "Software\Classes\CLSID\${DELEGATE_EXECUTE_HANDLER_ID}" "" "$BrandShortName CommandExecuteHandler"
  WriteRegStr SHCTX "Software\Classes\CLSID\${DELEGATE_EXECUTE_HANDLER_ID}" "AppId" "${DELEGATE_EXECUTE_HANDLER_ID}"
  WriteRegStr SHCTX "Software\Classes\CLSID\${DELEGATE_EXECUTE_HANDLER_ID}\LocalServer32" "" "${DELEGATE_EXECUTE_HANDLER_PATH}"
!macroend
!define AddMetroBrowserHandlerValues "!insertmacro AddMetroBrowserHandlerValues"
!endif ;end MOZ_METRO

