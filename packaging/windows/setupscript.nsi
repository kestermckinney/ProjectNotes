; Project Notes NSIS installer script
; Run "cmake --build . --target deploy" before compiling this script.
; All files are sourced from the CMake deploy staging directory.
Unicode True

RequestExecutionLevel user

; ── Product defines ───────────────────────────────────────────────────────────
!define PRODUCT_NAME      "Project Notes"
!define PRODUCT_VERSION   "6.5.0"
!define PRODUCT_PUBLISHER "Paul McKinney"
!define PRODUCT_WEB_SITE  "https://www.projectnotespro.com"
!define PRODUCT_DIR_REGKEY  "Software\Microsoft\Windows\CurrentVersion\App Paths\Project Notes.exe"
!define PRODUCT_UNINST_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKCU"

; ── Deploy directory (relative to this script's location) ─────────────────────
; Change BUILD_CONFIG if your Qt Creator kit name differs.
!define BUILD_CONFIG "Desktop_Qt_6_10_0_MSVC2022_64bit-Release"
!define BUILD_DIR    "..\..\build\${BUILD_CONFIG}"
!define DEPLOY_DIR   "${BUILD_DIR}\deploy"

; Run the cmake deploy target now so the staging directory exists before NSIS
; tries to reference any files inside it.
!ifndef PN_DEPLOY_STAGED
  !system 'cmake --build "${BUILD_DIR}" --target deploy' = 0
!endif

!include MUI2.nsh
!include FileFunc.nsh   ; GetParameters / GetOptions for unattended-update flags
!include Sections.nsh   ; SF_SELECTED

; Set to "1" (via the /relaunch command-line flag) when the in-app auto-updater
; runs this installer; it tells .onInstSuccess to relaunch the app afterward.
Var RelaunchFlag

; ── MUI settings ──────────────────────────────────────────────────────────────
!define MUI_ABORTWARNING
!define MUI_ICON   "installer_icon.ico"
!define MUI_UNICON "installer_icon.ico"

; Sidebar bitmap shown on Welcome and Finish pages (164x314 px)
!define MUI_WELCOMEFINISHPAGE_BITMAP          "installer_sidebar.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP        "installer_sidebar.bmp"

; Header bitmap shown on all other pages (150x57 px)
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP                "installer_header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP              "installer_header.bmp"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.rtf"
; No directory page: Project Notes installs per-user only, under
; $LOCALAPPDATA\Programs\Project Notes. The location is enforced in .onInit so
; it can never be redirected to a system-wide path.
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\Project Notes.exe"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ── Component descriptions ─────────────────────────────────────────────────────
LangString DESC_SEC01   ${LANG_ENGLISH} "Install Project Notes, the standard plugins, and spell-check dictionaries."

; ── Installer metadata ────────────────────────────────────────────────────────
Name    "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "ProjectNotes-${PRODUCT_VERSION}-Windows-x64-Setup.exe"
InstallDir          "$LOCALAPPDATA\Programs\Project Notes"
InstallDirRegKey HKCU "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails   show
ShowUnInstDetails show

; ══════════════════════════════════════════════════════════════════════════════
Section "MainSection" SEC01
  ; ── Unattended update: wait for a running instance to exit ───────────────────
  ; The in-app updater launches this installer silently while Project Notes is
  ; still running. Windows keeps the running executable locked, so wait until it
  ; can be opened for writing before overwriting any files. Bounded to ~30s so a
  ; stuck process can't hang the install forever. Skipped on a fresh install
  ; (no existing exe) and for normal interactive runs.
  IfSilent 0 waitdone
  StrCpy $0 0
  waitloop:
    IfFileExists "$INSTDIR\Project Notes.exe" 0 waitdone
    ClearErrors
    FileOpen $1 "$INSTDIR\Project Notes.exe" a
    IfErrors 0 unlocked
      IntOp $0 $0 + 1
      IntCmp $0 60 waitdone 0 waitdone   ; give up after ~30s and proceed
      Sleep 500
      Goto waitloop
  unlocked:
    FileClose $1
  waitdone:

  ; Remove the Python plugins and runtime earlier releases shipped before
  ; copying the current payload.
  !include "remove_retired_plugins.nsh"

  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  ; ── Executables ─────────────────────────────────────────────────────────────
  File "${DEPLOY_DIR}\LICENSE.txt"
  File "${DEPLOY_DIR}\Project Notes.exe"
  File "${DEPLOY_DIR}\ProjectNotesThunderbirdContacts.exe"
  File "${DEPLOY_DIR}\ProjectNotesOutlookContacts.exe"
  File "${DEPLOY_DIR}\projectnotes-contacts.xpi"

  CreateDirectory "$SMPROGRAMS\Project Notes"
  CreateShortCut  "$SMPROGRAMS\Project Notes\Project Notes.lnk"             "$INSTDIR\Project Notes.exe"
  CreateShortCut  "$DESKTOP\Project Notes.lnk"                              "$INSTDIR\Project Notes.exe"

  ; ── Qt libraries (populated by windeployqt during cmake deploy) ─────────────
  ; Include every Qt library staged by windeployqt. Keeping a hand-maintained
  ; list here omitted Qt6Positioning.dll even though it was in deploy/.
  File "${DEPLOY_DIR}\D3Dcompiler_47.dll"
  File "${DEPLOY_DIR}\icuuc.dll"
  File "${DEPLOY_DIR}\opengl32sw.dll"
  File "${DEPLOY_DIR}\Qt6*.dll"

  SetOutPath "$INSTDIR"

  ; QML modules (QtQuick, QtQuick.Controls incl. all built-in styles, Layouts,
  ; Dialogs, etc.) — recursive include since windeployqt's --qmldir scan
  ; produces a deep, version-sensitive tree (~1300 files) not worth hand-listing.
  SetOutPath "$INSTDIR\qml"
  File /r "${DEPLOY_DIR}\qml\*.*"

  SetOutPath "$INSTDIR\generic"
  File "${DEPLOY_DIR}\generic\qtuiotouchplugin.dll"

  SetOutPath "$INSTDIR\iconengines"
  File "${DEPLOY_DIR}\iconengines\qsvgicon.dll"

  SetOutPath "$INSTDIR\imageformats"
  File "${DEPLOY_DIR}\imageformats\qgif.dll"
  File "${DEPLOY_DIR}\imageformats\qico.dll"
  File "${DEPLOY_DIR}\imageformats\qjpeg.dll"
  File "${DEPLOY_DIR}\imageformats\qpdf.dll"
  File "${DEPLOY_DIR}\imageformats\qsvg.dll"

  SetOutPath "$INSTDIR\networkinformation"
  File "${DEPLOY_DIR}\networkinformation\qnetworklistmanager.dll"

  SetOutPath "$INSTDIR\platforms"
  File "${DEPLOY_DIR}\platforms\qwindows.dll"

  SetOutPath "$INSTDIR\sqldrivers"
  File "${DEPLOY_DIR}\sqldrivers\qsqlibase.dll"
  File "${DEPLOY_DIR}\sqldrivers\qsqlite.dll"
  File "${DEPLOY_DIR}\sqldrivers\qsqlmimer.dll"
  File "${DEPLOY_DIR}\sqldrivers\qsqloci.dll"
  File "${DEPLOY_DIR}\sqldrivers\qsqlodbc.dll"

  SetOutPath "$INSTDIR\styles"
  File "${DEPLOY_DIR}\styles\qmodernwindowsstyle.dll"

  SetOutPath "$INSTDIR\tls"
  File "${DEPLOY_DIR}\tls\qcertonlybackend.dll"
  File "${DEPLOY_DIR}\tls\qschannelbackend.dll"

  SetOutPath "$INSTDIR"

  ; ── ProjectNotes vcpkg DLLs ──────────────────────────────────────────────────
  File "${DEPLOY_DIR}\hunspell-1.7-0.dll"
  File "${DEPLOY_DIR}\libcrypto-3-x64.dll"
  File "${DEPLOY_DIR}\legacy.dll"
  File "${DEPLOY_DIR}\libssl-3-x64.dll"

  ; ── DirectX Shader Compiler (Qt Quick RHI) ───────────────────────────────────
  File "${DEPLOY_DIR}\dxcompiler.dll"
  File "${DEPLOY_DIR}\dxil.dll"

  ; ── MSVC runtime (sourced from the local Windows install) ────────────────────
  File "C:\Windows\System32\vcruntime140.dll"
  File "C:\Windows\System32\vcruntime140_1.dll"
  File "C:\Windows\System32\msvcp140.dll"
  File "C:\Windows\System32\msvcp140_1.dll"
  File "C:\Windows\System32\msvcp140_2.dll"

  ; ── Bundled JavaScript plugins ───────────────────────────────────────────────
  SetOutPath "$INSTDIR\plugins"
  File /r "${DEPLOY_DIR}\plugins\*.*"

  ; ── Spell-check dictionaries ─────────────────────────────────────────────────
  SetOutPath "$INSTDIR\dictionary"
  File "${DEPLOY_DIR}\dictionary\index.ini"
  File "${DEPLOY_DIR}\dictionary\en_GB.aff"
  File "${DEPLOY_DIR}\dictionary\en_GB.dic"
  File "${DEPLOY_DIR}\dictionary\en_US.aff"
  File "${DEPLOY_DIR}\dictionary\en_US.dic"
  File "${DEPLOY_DIR}\dictionary\es_ANY.aff"
   File "${DEPLOY_DIR}\dictionary\es_ANY.dic"

SectionEnd

Section -AdditionalIcons
  SetOutPath $INSTDIR
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\Project Notes\Website.lnk"   "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\Project Notes\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKCU "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\Project Notes.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName"       "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"   "$\"$INSTDIR\uninst.exe$\""
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon"       "$INSTDIR\Project Notes.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion"    "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout"      "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher"         "${PRODUCT_PUBLISHER}"
SectionEnd

; ── Component description callbacks ───────────────────────────────────────────
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01}   $(DESC_SEC01)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ══════════════════════════════════════════════════════════════════════════════
Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "Project Notes was successfully removed from your computer."
FunctionEnd

Function .onInit
  ; Enforce a per-user install location. If a stale registry value or a silent
  ; /D= override points outside the user's profile, reset to the default under
  ; $LOCALAPPDATA\Programs so the app is never installed system-wide.
  StrLen $1 "$LOCALAPPDATA\Programs"
  StrCpy $2 "$INSTDIR" $1
  StrCmp $2 "$LOCALAPPDATA\Programs" +2 0
    StrCpy $INSTDIR "$LOCALAPPDATA\Programs\Project Notes"

  ; Detect the auto-updater's /relaunch flag so .onInstSuccess can reopen the app.
  ${GetParameters} $R0
  ClearErrors
  ${GetOptions} $R0 "/relaunch" $R1
  IfErrors +2 0
    StrCpy $RelaunchFlag "1"
FunctionEnd

Function .onInstSuccess
  ; Only the in-app updater passes /relaunch; relaunch the app after a silent
  ; install (the finish-page "run" option does not fire in silent mode). Normal
  ; interactive installs leave $RelaunchFlag empty and use the finish page.
  StrCmp $RelaunchFlag "1" 0 +2
    Exec '"$INSTDIR\Project Notes.exe"'
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove Project Notes and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  ; ── Dictionaries ─────────────────────────────────────────────────────────────
  Delete "$INSTDIR\dictionary\en_GB.aff"
  Delete "$INSTDIR\dictionary\en_GB.dic"
  Delete "$INSTDIR\dictionary\en_US.aff"
  Delete "$INSTDIR\dictionary\en_US.dic"
  Delete "$INSTDIR\dictionary\es_ANY.aff"
  Delete "$INSTDIR\dictionary\es_ANY.dic"
  Delete "$INSTDIR\dictionary\index.ini"
  RMDir  "$INSTDIR\dictionary"

  ; ── Retired plugins (shared with the install section) ────────────────────────
  !include "remove_retired_plugins.nsh"

  ; ── Bundled plugins ──────────────────────────────────────────────────────────
  ; Only the folders this installer created; the user's own plugins live in
  ; their profile, not here.
  RMDir /r "$INSTDIR\plugins\base"
  RMDir /r "$INSTDIR\plugins\myshortcuts"
  RMDir /r "$INSTDIR\plugins\office-tools"
  RMDir /r "$INSTDIR\plugins\archive-mail"
  RMDir /r "$INSTDIR\plugins\outlook-integration"
  RMDir /r "$INSTDIR\plugins\team-member-quick-add"
  RMDir  "$INSTDIR\plugins"

  ; ── Qt plugins ───────────────────────────────────────────────────────────────
  Delete "$INSTDIR\generic\qtuiotouchplugin.dll"
  RMDir  "$INSTDIR\generic"
  Delete "$INSTDIR\iconengines\qsvgicon.dll"
  RMDir  "$INSTDIR\iconengines"
  Delete "$INSTDIR\imageformats\qgif.dll"
  Delete "$INSTDIR\imageformats\qico.dll"
  Delete "$INSTDIR\imageformats\qjpeg.dll"
  Delete "$INSTDIR\imageformats\qpdf.dll"
  Delete "$INSTDIR\imageformats\qsvg.dll"
  RMDir  "$INSTDIR\imageformats"
  Delete "$INSTDIR\networkinformation\qnetworklistmanager.dll"
  RMDir  "$INSTDIR\networkinformation"
  Delete "$INSTDIR\platforms\qwindows.dll"
  RMDir  "$INSTDIR\platforms"
  Delete "$INSTDIR\sqldrivers\qsqlibase.dll"
  Delete "$INSTDIR\sqldrivers\qsqlite.dll"
  Delete "$INSTDIR\sqldrivers\qsqlmimer.dll"
  Delete "$INSTDIR\sqldrivers\qsqloci.dll"
  Delete "$INSTDIR\sqldrivers\qsqlodbc.dll"
  RMDir  "$INSTDIR\sqldrivers"
  Delete "$INSTDIR\styles\qmodernwindowsstyle.dll"
  RMDir  "$INSTDIR\styles"
  Delete "$INSTDIR\tls\qcertonlybackend.dll"
  Delete "$INSTDIR\tls\qschannelbackend.dll"
  RMDir  "$INSTDIR\tls"

  ; ── Root files ───────────────────────────────────────────────────────────────
  Delete "$INSTDIR\Project Notes.exe"
  Delete "$INSTDIR\ProjectNotesThunderbirdContacts.exe"
  Delete "$INSTDIR\ProjectNotesOutlookContacts.exe"
  Delete "$INSTDIR\projectnotes-contacts.xpi"

  Delete "$INSTDIR\D3Dcompiler_47.dll"
  Delete "$INSTDIR\icuuc.dll"
  Delete "$INSTDIR\opengl32sw.dll"
  Delete "$INSTDIR\Qt6*.dll"

  RMDir /r "$INSTDIR\qml"

  Delete "$INSTDIR\hunspell-1.7-0.dll"
  Delete "$INSTDIR\libcrypto-3-x64.dll"
  Delete "$INSTDIR\legacy.dll"
  Delete "$INSTDIR\libssl-3-x64.dll"

  Delete "$INSTDIR\dxcompiler.dll"
  Delete "$INSTDIR\dxil.dll"
  Delete "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\vcruntime140_1.dll"
  Delete "$INSTDIR\msvcp140.dll"
  Delete "$INSTDIR\msvcp140_1.dll"
  Delete "$INSTDIR\msvcp140_2.dll"

  Delete "$INSTDIR\LICENSE.txt"
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"

  Delete "$SMPROGRAMS\Project Notes\Uninstall.lnk"
  Delete "$SMPROGRAMS\Project Notes\Website.lnk"
  Delete "$DESKTOP\Project Notes.lnk"
  Delete "$SMPROGRAMS\Project Notes\Project Notes.lnk"
  RMDir  "$SMPROGRAMS\Project Notes"
  RMDir  "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKCU "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
