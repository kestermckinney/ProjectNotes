; Project Notes NSIS installer script
; Run "cmake --build . --target deploy" before compiling this script.
; All files are sourced from the CMake deploy staging directory.
Unicode True

RequestExecutionLevel user

; ── Product defines ───────────────────────────────────────────────────────────
!define PRODUCT_NAME      "Project Notes"
!define PRODUCT_VERSION   "5.0.1"
!define PRODUCT_PUBLISHER "Paul McKinney"
!define PRODUCT_WEB_SITE  "https://github.com/kestermckinney/ProjectNotes"
!define PRODUCT_DIR_REGKEY  "Software\Microsoft\Windows\CurrentVersion\App Paths\Project Notes.exe"
!define PRODUCT_UNINST_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; ── Deploy directory (relative to this script's location) ─────────────────────
; Change BUILD_CONFIG if your Qt Creator kit name differs.
!define BUILD_CONFIG "Desktop_Qt_6_10_0_MSVC2022_64bit-Release"
!define BUILD_DIR    "..\..\build\${BUILD_CONFIG}"
!define DEPLOY_DIR   "${BUILD_DIR}\deploy"

; Run the cmake deploy target now so the staging directory exists before NSIS
; tries to reference any files inside it.
!system 'cmake --build "${BUILD_DIR}" --target deploy' = 0

; ── Multi-user install ────────────────────────────────────────────────────────
!define MULTIUSER_USE_PROGRAMFILES64
!define MULTIUSER_INSTALLMODE_INSTDIR                  "${PRODUCT_NAME}"
!define MULTIUSER_INSTALLMODE_INSTALL_REGISTRY_KEY     "${PRODUCT_NAME}"
!define MULTIUSER_INSTALLMODE_UNINSTALL_REGISTRY_KEY   "${PRODUCT_NAME}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "UninstallString"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME "InstallLocation"
!define MULTIUSER_INSTALLMODE_ALLOW_ELEVATION
!define MULTIUSER_INSTALLMODE_DEFAULT_CURRENTUSER
!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!include MultiUser.nsh
!include MUI2.nsh

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
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_LICENSE "license.rtf"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\Project Notes.exe"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ── Installer metadata ────────────────────────────────────────────────────────
Name    "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "ProjectNotes-${PRODUCT_VERSION}-Windows-x64-Setup.exe"
InstallDir          "$PROGRAMFILES64\Project Notes"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails   show
ShowUnInstDetails show

; ══════════════════════════════════════════════════════════════════════════════
Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  ; ── Executables ─────────────────────────────────────────────────────────────
  File "${DEPLOY_DIR}\LICENSE.txt"
  File "${DEPLOY_DIR}\Project Notes.exe"
  File "${DEPLOY_DIR}\Project Notes Remote Host.exe"

  CreateDirectory "$SMPROGRAMS\Project Notes"
  CreateShortCut  "$SMPROGRAMS\Project Notes\Project Notes.lnk"             "$INSTDIR\Project Notes.exe"
  CreateShortCut  "$SMPROGRAMS\Project Notes\Project Notes Remote Host.lnk" "$INSTDIR\Project Notes Remote Host.exe"
  CreateShortCut  "$DESKTOP\Project Notes.lnk"                              "$INSTDIR\Project Notes.exe"

  ; ── Qt libraries (populated by windeployqt during cmake deploy) ─────────────
  File "${DEPLOY_DIR}\D3Dcompiler_47.dll"
  File "${DEPLOY_DIR}\icuuc.dll"
  File "${DEPLOY_DIR}\opengl32sw.dll"
  File "${DEPLOY_DIR}\Qt6Charts.dll"
  File "${DEPLOY_DIR}\Qt6Core.dll"
  File "${DEPLOY_DIR}\Qt6Gui.dll"
  File "${DEPLOY_DIR}\Qt6Network.dll"
  File "${DEPLOY_DIR}\Qt6OpenGL.dll"
  File "${DEPLOY_DIR}\Qt6OpenGLWidgets.dll"
  File "${DEPLOY_DIR}\Qt6Pdf.dll"
  File "${DEPLOY_DIR}\Qt6Sql.dll"
  File "${DEPLOY_DIR}\Qt6Svg.dll"
  File "${DEPLOY_DIR}\Qt6Widgets.dll"
  File "${DEPLOY_DIR}\Qt6Xml.dll"

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
  File "${DEPLOY_DIR}\sqldrivers\qsqlpsql.dll"

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

  ; ── SqliteSyncPro runtime DLLs ───────────────────────────────────────────────
  File "${DEPLOY_DIR}\libpq.dll"

  ; ── Extra DLLs (OpenSSL non-x64 variants, SQLite, DirectX SC, libffi) ────────
  File "${DEPLOY_DIR}\dxcompiler.dll"
  File "${DEPLOY_DIR}\dxil.dll"
  File "${DEPLOY_DIR}\libcrypto-3.dll"
  File "${DEPLOY_DIR}\libffi-8.dll"
  File "${DEPLOY_DIR}\libssl-3.dll"
  File "${DEPLOY_DIR}\sqlite3.dll"

  ; ── Python 3.13 embeddable runtime ───────────────────────────────────────────
  ; These files come from the Python embeddable distribution copied by cmake deploy.
  ; Update this list when upgrading Python.
  File "${DEPLOY_DIR}\python.cat"
  File "${DEPLOY_DIR}\python.exe"
  File "${DEPLOY_DIR}\python3.dll"
  File "${DEPLOY_DIR}\python313.dll"
  File "${DEPLOY_DIR}\python313.zip"
  File "${DEPLOY_DIR}\python313._pth"
  File "${DEPLOY_DIR}\pythonw.exe"
  File "${DEPLOY_DIR}\vcruntime140.dll"
  File "${DEPLOY_DIR}\vcruntime140_1.dll"
  File "${DEPLOY_DIR}\_asyncio.pyd"
  File "${DEPLOY_DIR}\_bz2.pyd"
  File "${DEPLOY_DIR}\_ctypes.pyd"
  File "${DEPLOY_DIR}\_decimal.pyd"
  File "${DEPLOY_DIR}\_elementtree.pyd"
  File "${DEPLOY_DIR}\_hashlib.pyd"
  File "${DEPLOY_DIR}\_lzma.pyd"
  File "${DEPLOY_DIR}\_multiprocessing.pyd"
  File "${DEPLOY_DIR}\_overlapped.pyd"
  File "${DEPLOY_DIR}\_queue.pyd"
  File "${DEPLOY_DIR}\_socket.pyd"
  File "${DEPLOY_DIR}\_sqlite3.pyd"
  File "${DEPLOY_DIR}\_ssl.pyd"
  File "${DEPLOY_DIR}\_uuid.pyd"
  File "${DEPLOY_DIR}\_wmi.pyd"
  File "${DEPLOY_DIR}\_zoneinfo.pyd"
  File "${DEPLOY_DIR}\pyexpat.pyd"
  File "${DEPLOY_DIR}\select.pyd"
  File "${DEPLOY_DIR}\unicodedata.pyd"
  File "${DEPLOY_DIR}\winsound.pyd"

  ; ── MSVC runtime (sourced from the local Windows install) ────────────────────
  File "C:\Windows\System32\msvcp140.dll"
  File "C:\Windows\System32\msvcp140_1.dll"
  File "C:\Windows\System32\msvcp140_2.dll"

  ; ── site-packages (generated by collect_packages.py during cmake deploy) ─────
  !include "install_files.nsh"

  ; ── Project Notes plugins ────────────────────────────────────────────────────
  SetOutPath "$INSTDIR\plugins"
  File "${DEPLOY_DIR}\plugins\base_plugin.py"
  File "${DEPLOY_DIR}\plugins\base_plugin_settings.py"
  File "${DEPLOY_DIR}\plugins\excelkill_plugin.py"
  File "${DEPLOY_DIR}\plugins\exportnotes_plugin.py"
  File "${DEPLOY_DIR}\plugins\exporttrackeritems_plugin.py"
   File "${DEPLOY_DIR}\plugins\findprojectemailperson_plugin.py"
   File "${DEPLOY_DIR}\plugins\myshortcuts_plugin.py"
  File "${DEPLOY_DIR}\plugins\newchangeorder_plugin.py"
  File "${DEPLOY_DIR}\plugins\newmsproject_plugin.py"
  File "${DEPLOY_DIR}\plugins\newpcrregistry_plugin.py"
  File "${DEPLOY_DIR}\plugins\newpowerpoint_plugin.py"
  File "${DEPLOY_DIR}\plugins\newriskregister_plugin.py"
  File "${DEPLOY_DIR}\plugins\open_msproject_plugin.py"
  File "${DEPLOY_DIR}\plugins\trackkerreport_plugin.py"
  File "${DEPLOY_DIR}\plugins\wordkill_plugin.py"

  SetOutPath "$INSTDIR\plugins\includes"
  File "${DEPLOY_DIR}\plugins\includes\collaboration_tools.py"
  File "${DEPLOY_DIR}\plugins\includes\common.py"
  File "${DEPLOY_DIR}\plugins\includes\excel_tools.py"
   File "${DEPLOY_DIR}\plugins\includes\graphapi_tools.py"
   File "${DEPLOY_DIR}\plugins\includes\noteformatter.py"
  File "${DEPLOY_DIR}\plugins\includes\outlook_tools.py"
  File "${DEPLOY_DIR}\plugins\includes\word_tools.py"

  SetOutPath "$INSTDIR\plugins\forms"
  File "${DEPLOY_DIR}\plugins\forms\dialogClassification.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogDuplicateFilesFound.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogEditor.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogExportLocation.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogExportNotesOptions.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogExportTrackerOptions.ui"
   File "${DEPLOY_DIR}\plugins\forms\dialogFileFinder.ui"
   File "${DEPLOY_DIR}\plugins\forms\dialogMeetingEmailTemplate.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogMeetingEmailTypes.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogMyShortcuts.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogOutlookIntegrationOptions.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogSettingsMigrator.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogSSRSOptions.ui"
  File "${DEPLOY_DIR}\plugins\forms\dialogTrackerRptOptions.ui"

  SetOutPath "$INSTDIR\plugins\templates"
  File "${DEPLOY_DIR}\plugins\templates\Lessons Learned Template.xlsx"
  File "${DEPLOY_DIR}\plugins\templates\Risk Register Template.xlsx"
  File "${DEPLOY_DIR}\plugins\templates\Tracker Items Template.xlsx"

  ; ── Background threads ───────────────────────────────────────────────────────
  SetOutPath "$INSTDIR\threads"
   File "${DEPLOY_DIR}\threads\filefinder_thread.py"
   File "${DEPLOY_DIR}\threads\outlooksync_thread.py"

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

Section "Custom IFS Plugins" SEC_IFS
  SetOutPath "$INSTDIR\plugins"
  File "${DEPLOY_DIR}\plugins\ifscloud_plugin_settings.py"
  File "${DEPLOY_DIR}\plugins\ifs_ssrs_generate_plugin.py"

  SetOutPath "$INSTDIR\plugins\forms"
  File "${DEPLOY_DIR}\plugins\forms\dialogIFSCloud.ui"

  SetOutPath "$INSTDIR\plugins\includes"
  File "${DEPLOY_DIR}\plugins\includes\ifs_tools.py"

  SetOutPath "$INSTDIR\threads"
  File "${DEPLOY_DIR}\threads\ifssync_thread.py"
SectionEnd

Section -AdditionalIcons
  SetOutPath $INSTDIR
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\Project Notes\Website.lnk"   "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\Project Notes\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\Project Notes.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName"       "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"   "$\"$INSTDIR\uninst.exe$\""
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon"       "$INSTDIR\Project Notes.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion"    "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout"      "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher"         "${PRODUCT_PUBLISHER}"
SectionEnd

; ══════════════════════════════════════════════════════════════════════════════
Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "Project Notes was successfully removed from your computer."
FunctionEnd

Function .onInit
  !insertmacro MULTIUSER_INIT

  SectionGetFlags ${SEC_IFS} $0
  IntOp $0 $0 & 0xFFFFFFFE  ; clears SF_SELECTED (same as SECTION_OFF mask)
  SectionSetFlags ${SEC_IFS} $0

FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove Project Notes and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  ; ── site-packages (generated list) ──────────────────────────────────────────
  !include "remove_files.nsh"

  ; ── Dictionaries ─────────────────────────────────────────────────────────────
  Delete "$INSTDIR\dictionary\en_GB.aff"
  Delete "$INSTDIR\dictionary\en_GB.dic"
  Delete "$INSTDIR\dictionary\en_US.aff"
  Delete "$INSTDIR\dictionary\en_US.dic"
  Delete "$INSTDIR\dictionary\es_ANY.aff"
  Delete "$INSTDIR\dictionary\es_ANY.dic"
  Delete "$INSTDIR\dictionary\index.ini"
  RMDir  "$INSTDIR\dictionary"

  ; ── Plugins ──────────────────────────────────────────────────────────────────
  Delete "$INSTDIR\plugins\base_plugin.py"
  Delete "$INSTDIR\plugins\base_plugin_settings.py"
  Delete "$INSTDIR\plugins\excelkill_plugin.py"
  Delete "$INSTDIR\plugins\exportnotes_plugin.py"
  Delete "$INSTDIR\plugins\exporttrackeritems_plugin.py"
  Delete "$INSTDIR\plugins\findprojectemailperson_plugin.py"
  Delete "$INSTDIR\plugins\ifscloud_plugin_settings.py"
  Delete "$INSTDIR\plugins\ifs_ssrs_generate_plugin.py"
  Delete "$INSTDIR\plugins\myshortcuts_plugin.py"
  Delete "$INSTDIR\plugins\newchangeorder_plugin.py"
  Delete "$INSTDIR\plugins\newmsproject_plugin.py"
  Delete "$INSTDIR\plugins\newpcrregistry_plugin.py"
  Delete "$INSTDIR\plugins\newpowerpoint_plugin.py"
  Delete "$INSTDIR\plugins\newriskregister_plugin.py"
  Delete "$INSTDIR\plugins\open_msproject_plugin.py"
  Delete "$INSTDIR\plugins\trackkerreport_plugin.py"
  Delete "$INSTDIR\plugins\wordkill_plugin.py"

  Delete "$INSTDIR\plugins\includes\collaboration_tools.py"
  Delete "$INSTDIR\plugins\includes\common.py"
  Delete "$INSTDIR\plugins\includes\excel_tools.py"
  Delete "$INSTDIR\plugins\includes\graphapi_tools.py"
  Delete "$INSTDIR\plugins\includes\ifs_tools.py"
  Delete "$INSTDIR\plugins\includes\noteformatter.py"
  Delete "$INSTDIR\plugins\includes\outlook_tools.py"
  Delete "$INSTDIR\plugins\includes\word_tools.py"
  Delete "$INSTDIR\plugins\includes\__pycache__\*.*"
  RMDir  "$INSTDIR\plugins\includes\__pycache__"
  RMDir  "$INSTDIR\plugins\includes"

  Delete "$INSTDIR\plugins\forms\dialogClassification.ui"
  Delete "$INSTDIR\plugins\forms\dialogDuplicateFilesFound.ui"
  Delete "$INSTDIR\plugins\forms\dialogEditor.ui"
  Delete "$INSTDIR\plugins\forms\dialogExportLocation.ui"
  Delete "$INSTDIR\plugins\forms\dialogExportNotesOptions.ui"
  Delete "$INSTDIR\plugins\forms\dialogExportTrackerOptions.ui"
  Delete "$INSTDIR\plugins\forms\dialogFileFinder.ui"
  Delete "$INSTDIR\plugins\forms\dialogIFSCloud.ui"
  Delete "$INSTDIR\plugins\forms\dialogMeetingEmailTemplate.ui"
  Delete "$INSTDIR\plugins\forms\dialogMeetingEmailTypes.ui"
  Delete "$INSTDIR\plugins\forms\dialogMyShortcuts.ui"
  Delete "$INSTDIR\plugins\forms\dialogOutlookIntegrationOptions.ui"
  Delete "$INSTDIR\plugins\forms\dialogSettingsMigrator.ui"
  Delete "$INSTDIR\plugins\forms\dialogSSRSOptions.ui"
  Delete "$INSTDIR\plugins\forms\dialogTrackerRptOptions.ui"
  Delete "$INSTDIR\plugins\forms\__pycache__\*.*"
  RMDir  "$INSTDIR\plugins\forms\__pycache__"
  RMDir  "$INSTDIR\plugins\forms"

  Delete "$INSTDIR\plugins\templates\Lessons Learned Template.xlsx"
  Delete "$INSTDIR\plugins\templates\Risk Register Template.xlsx"
  Delete "$INSTDIR\plugins\templates\Tracker Items Template.xlsx"
  RMDir  "$INSTDIR\plugins\templates"

  Delete "$INSTDIR\plugins\__pycache__\*.*"
  RMDir  "$INSTDIR\plugins\__pycache__"
  RMDir  "$INSTDIR\plugins"

  ; ── Threads ──────────────────────────────────────────────────────────────────
  Delete "$INSTDIR\threads\filefinder_thread.py"
  Delete "$INSTDIR\threads\ifssync_thread.py"
  Delete "$INSTDIR\threads\outlooksync_thread.py"
  Delete "$INSTDIR\threads\__pycache__\*.*"
  RMDir  "$INSTDIR\threads\__pycache__"
  RMDir  "$INSTDIR\threads"

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
  Delete "$INSTDIR\sqldrivers\qsqlpsql.dll"
  RMDir  "$INSTDIR\sqldrivers"
  Delete "$INSTDIR\styles\qmodernwindowsstyle.dll"
  RMDir  "$INSTDIR\styles"
  Delete "$INSTDIR\tls\qcertonlybackend.dll"
  Delete "$INSTDIR\tls\qschannelbackend.dll"
  RMDir  "$INSTDIR\tls"

  ; ── Root files ───────────────────────────────────────────────────────────────
  Delete "$INSTDIR\Project Notes.exe"
  Delete "$INSTDIR\Project Notes Remote Host.exe"

  Delete "$INSTDIR\D3Dcompiler_47.dll"
  Delete "$INSTDIR\icuuc.dll"
  Delete "$INSTDIR\opengl32sw.dll"
  Delete "$INSTDIR\Qt6Charts.dll"
  Delete "$INSTDIR\Qt6Core.dll"
  Delete "$INSTDIR\Qt6Gui.dll"
  Delete "$INSTDIR\Qt6Network.dll"
  Delete "$INSTDIR\Qt6OpenGL.dll"
  Delete "$INSTDIR\Qt6OpenGLWidgets.dll"
  Delete "$INSTDIR\Qt6Pdf.dll"
  Delete "$INSTDIR\Qt6Sql.dll"
  Delete "$INSTDIR\Qt6Svg.dll"
  Delete "$INSTDIR\Qt6Widgets.dll"
  Delete "$INSTDIR\Qt6Xml.dll"

  Delete "$INSTDIR\hunspell-1.7-0.dll"
  Delete "$INSTDIR\libcrypto-3-x64.dll"
  Delete "$INSTDIR\legacy.dll"
  Delete "$INSTDIR\libssl-3-x64.dll"

  Delete "$INSTDIR\libpq.dll"

  Delete "$INSTDIR\dxcompiler.dll"
  Delete "$INSTDIR\dxil.dll"
  Delete "$INSTDIR\libcrypto-3.dll"
  Delete "$INSTDIR\libffi-8.dll"
  Delete "$INSTDIR\libssl-3.dll"
  Delete "$INSTDIR\sqlite3.dll"

  Delete "$INSTDIR\python.cat"
  Delete "$INSTDIR\python.exe"
  Delete "$INSTDIR\python3.dll"
  Delete "$INSTDIR\python313.dll"
  Delete "$INSTDIR\python313.zip"
  Delete "$INSTDIR\python313._pth"
  Delete "$INSTDIR\pythonw.exe"
  Delete "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\vcruntime140_1.dll"
  Delete "$INSTDIR\_asyncio.pyd"
  Delete "$INSTDIR\_bz2.pyd"
  Delete "$INSTDIR\_ctypes.pyd"
  Delete "$INSTDIR\_decimal.pyd"
  Delete "$INSTDIR\_elementtree.pyd"
  Delete "$INSTDIR\_hashlib.pyd"
  Delete "$INSTDIR\_lzma.pyd"
  Delete "$INSTDIR\_multiprocessing.pyd"
  Delete "$INSTDIR\_overlapped.pyd"
  Delete "$INSTDIR\_queue.pyd"
  Delete "$INSTDIR\_socket.pyd"
  Delete "$INSTDIR\_sqlite3.pyd"
  Delete "$INSTDIR\_ssl.pyd"
  Delete "$INSTDIR\_uuid.pyd"
  Delete "$INSTDIR\_wmi.pyd"
  Delete "$INSTDIR\_zoneinfo.pyd"
  Delete "$INSTDIR\pyexpat.pyd"
  Delete "$INSTDIR\select.pyd"
  Delete "$INSTDIR\unicodedata.pyd"
  Delete "$INSTDIR\winsound.pyd"

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
  Delete "$SMPROGRAMS\Project Notes\Project Notes Remote Host.lnk"
  RMDir  "$SMPROGRAMS\Project Notes"
  RMDir  "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
