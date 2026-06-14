; Project Notes Remote Host NSIS installer script
; Run "cmake --build . --target deploy" before compiling this script.
; All files are sourced from the CMake deploy staging directory.
Unicode True

RequestExecutionLevel user

; ── Product defines ───────────────────────────────────────────────────────────
!define PRODUCT_NAME      "Project Notes Remote Host"
!define PRODUCT_VERSION   "5.2.1"
!define PRODUCT_PUBLISHER "Paul McKinney"
!define PRODUCT_WEB_SITE  "https://github.com/kestermckinney/ProjectNotes"
!define PRODUCT_DIR_REGKEY  "Software\Microsoft\Windows\CurrentVersion\App Paths\Project Notes Remote Host.exe"
!define PRODUCT_UNINST_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKCU"

; ── Deploy directory (relative to this script's location) ─────────────────────
!define BUILD_CONFIG "Desktop_Qt_6_10_0_MSVC2022_64bit-Release"
!define BUILD_DIR    "..\..\build\${BUILD_CONFIG}"
!define DEPLOY_DIR   "${BUILD_DIR}\deploy"

!system 'cmake --build "${BUILD_DIR}" --target deploy' = 0

!include MUI2.nsh

; ── MUI settings ──────────────────────────────────────────────────────────────
!define MUI_ABORTWARNING
!define MUI_ICON   "installer_icon.ico"
!define MUI_UNICON "installer_icon.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP          "installer_sidebar.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP        "installer_sidebar.bmp"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP                "installer_header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP              "installer_header.bmp"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.rtf"
; No directory page: installs per-user only, under
; $LOCALAPPDATA\Project Notes Remote Host. The location is enforced in .onInit.
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\Project Notes Remote Host.exe"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ── Installer metadata ────────────────────────────────────────────────────────
Name    "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "ProjectNotesRemoteHost-${PRODUCT_VERSION}-Windows-x64-Setup.exe"
InstallDir          "$LOCALAPPDATA\Project Notes Remote Host"
InstallDirRegKey HKCU "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails   show
ShowUnInstDetails show

; ══════════════════════════════════════════════════════════════════════════════
Function .onInit
  ; Enforce a per-user install location. If a stale registry value or a silent
  ; /D= override points outside the user's profile, reset to the default under
  ; $LOCALAPPDATA so the component is never installed system-wide.
  StrLen $1 "$LOCALAPPDATA"
  StrCpy $2 "$INSTDIR" $1
  StrCmp $2 "$LOCALAPPDATA" +2 0
    StrCpy $INSTDIR "$LOCALAPPDATA\Project Notes Remote Host"
FunctionEnd

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  ; ── Executables ─────────────────────────────────────────────────────────────
  File "${DEPLOY_DIR}\LICENSE.txt"
  File "${DEPLOY_DIR}\Project Notes Remote Host.exe"

  CreateDirectory "$SMPROGRAMS\Project Notes Remote Host"
  CreateShortCut  "$SMPROGRAMS\Project Notes Remote Host\Project Notes Remote Host.lnk" "$INSTDIR\Project Notes Remote Host.exe"
  CreateShortCut  "$DESKTOP\Project Notes Remote Host.lnk"                              "$INSTDIR\Project Notes Remote Host.exe"

  ; ── Qt libraries ─────────────────────────────────────────────────────────────
  File "${DEPLOY_DIR}\D3Dcompiler_47.dll"
  File "${DEPLOY_DIR}\icuuc.dll"
  File "${DEPLOY_DIR}\opengl32sw.dll"
  File "${DEPLOY_DIR}\Qt6Core.dll"
  File "${DEPLOY_DIR}\Qt6Gui.dll"
  File "${DEPLOY_DIR}\Qt6Network.dll"
  File "${DEPLOY_DIR}\Qt6Sql.dll"
  File "${DEPLOY_DIR}\Qt6Widgets.dll"
  File "${DEPLOY_DIR}\Qt6Xml.dll"

  SetOutPath "$INSTDIR\platforms"
  File "${DEPLOY_DIR}\platforms\qwindows.dll"

  SetOutPath "$INSTDIR\sqldrivers"
  File "${DEPLOY_DIR}\sqldrivers\qsqlite.dll"
  File "${DEPLOY_DIR}\sqldrivers\qsqlpsql.dll"

  SetOutPath "$INSTDIR\styles"
  File "${DEPLOY_DIR}\styles\qmodernwindowsstyle.dll"

  SetOutPath "$INSTDIR\tls"
  File "${DEPLOY_DIR}\tls\qcertonlybackend.dll"
  File "${DEPLOY_DIR}\tls\qschannelbackend.dll"

  SetOutPath "$INSTDIR"

  ; ── Postgres client library ───────────────────────────────────────────────────
  File "${DEPLOY_DIR}\libpq.dll"

  ; ── OpenSSL ───────────────────────────────────────────────────────────────────
  File "${DEPLOY_DIR}\libcrypto-3-x64.dll"
  File "${DEPLOY_DIR}\legacy.dll"
  File "${DEPLOY_DIR}\libssl-3-x64.dll"
  File "${DEPLOY_DIR}\libcrypto-3.dll"
  File "${DEPLOY_DIR}\libssl-3.dll"

  ; ── MSVC runtime ─────────────────────────────────────────────────────────────
  File "${DEPLOY_DIR}\vcruntime140.dll"
  File "${DEPLOY_DIR}\vcruntime140_1.dll"
  File "C:\Windows\System32\msvcp140.dll"
  File "C:\Windows\System32\msvcp140_1.dll"
  File "C:\Windows\System32\msvcp140_2.dll"

SectionEnd

Section -AdditionalIcons
  SetOutPath $INSTDIR
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\Project Notes Remote Host\Website.lnk"   "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\Project Notes Remote Host\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKCU "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\Project Notes Remote Host.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName"          "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"      "$\"$INSTDIR\uninst.exe$\""
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "QuietUninstallString" "$\"$INSTDIR\uninst.exe$\" /S"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon"          "$INSTDIR\Project Notes Remote Host.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion"       "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout"         "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher"            "${PRODUCT_PUBLISHER}"
SectionEnd

; ══════════════════════════════════════════════════════════════════════════════
Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "Project Notes Remote Host was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove Project Notes Remote Host and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  ; ── Qt plugins ───────────────────────────────────────────────────────────────
  Delete "$INSTDIR\platforms\qwindows.dll"
  RMDir  "$INSTDIR\platforms"
  Delete "$INSTDIR\sqldrivers\qsqlite.dll"
  Delete "$INSTDIR\sqldrivers\qsqlpsql.dll"
  RMDir  "$INSTDIR\sqldrivers"
  Delete "$INSTDIR\styles\qmodernwindowsstyle.dll"
  RMDir  "$INSTDIR\styles"
  Delete "$INSTDIR\tls\qcertonlybackend.dll"
  Delete "$INSTDIR\tls\qschannelbackend.dll"
  RMDir  "$INSTDIR\tls"

  ; ── Root files ───────────────────────────────────────────────────────────────
  Delete "$INSTDIR\Project Notes Remote Host.exe"

  Delete "$INSTDIR\D3Dcompiler_47.dll"
  Delete "$INSTDIR\icuuc.dll"
  Delete "$INSTDIR\opengl32sw.dll"
  Delete "$INSTDIR\Qt6Core.dll"
  Delete "$INSTDIR\Qt6Gui.dll"
  Delete "$INSTDIR\Qt6Network.dll"
  Delete "$INSTDIR\Qt6Sql.dll"
  Delete "$INSTDIR\Qt6Widgets.dll"
  Delete "$INSTDIR\Qt6Xml.dll"

  Delete "$INSTDIR\libpq.dll"

  Delete "$INSTDIR\libcrypto-3-x64.dll"
  Delete "$INSTDIR\legacy.dll"
  Delete "$INSTDIR\libssl-3-x64.dll"
  Delete "$INSTDIR\libcrypto-3.dll"
  Delete "$INSTDIR\libssl-3.dll"

  Delete "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\vcruntime140_1.dll"
  Delete "$INSTDIR\msvcp140.dll"
  Delete "$INSTDIR\msvcp140_1.dll"
  Delete "$INSTDIR\msvcp140_2.dll"

  Delete "$INSTDIR\LICENSE.txt"
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"

  Delete "$SMPROGRAMS\Project Notes Remote Host\Uninstall.lnk"
  Delete "$SMPROGRAMS\Project Notes Remote Host\Website.lnk"
  Delete "$DESKTOP\Project Notes Remote Host.lnk"
  Delete "$SMPROGRAMS\Project Notes Remote Host\Project Notes Remote Host.lnk"
  RMDir  "$SMPROGRAMS\Project Notes Remote Host"
  RMDir  "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKCU "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
