; Script generated by the HM NIS Edit Script Wizard.
Unicode True

RequestExecutionLevel user

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Project Notes"
!define PRODUCT_VERSION "3.1.0"
!define PRODUCT_PUBLISHER "Paul McKinney"
!define PRODUCT_WEB_SITE "https://github.com/kestermckinney/ProjectNotes"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\ProjectNotes.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI 1.67 compatible ------
;!include "MUI.nsh"
!define MULTIUSER_USE_PROGRAMFILES64
!define MULTIUSER_INSTALLMODE_INSTDIR "${PRODUCT_NAME}"  ; suggested name of directory to install (under $PROGRAMFILES or $LOCALAPPDATA)
!define MULTIUSER_INSTALLMODE_INSTALL_REGISTRY_KEY "${PRODUCT_NAME}"  ; registry key for INSTALL info, placed under [HKLM|HKCU]\Software  (can be ${APP_NAME} or some {GUID})
!define MULTIUSER_INSTALLMODE_UNINSTALL_REGISTRY_KEY "${PRODUCT_NAME}"  ; registry key for UNINSTALL info, placed under [HKLM|HKCU]\Software\Microsoft\Windows\CurrentVersion\Uninstall  (can be ${APP_NAME} or some {GUID})
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "UninstallString"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME "InstallLocation"
;!define MULTIUSER_INSTALLMODE_DISPLAYNAME "${APP_NAME} ${VERSION} ${PRODUCT_EDITION}" ; this is optional... name that will be displayed in add/remove programs (default is ${APP_NAME} ${VERSION})
!define MULTIUSER_INSTALLMODE_ALLOW_ELEVATION   ; allow requesting for elevation... if false, radiobutton will be disabled and user will have to restart installer with elevated permissions
!define MULTIUSER_INSTALLMODE_DEFAULT_CURRENTUSER  ; only available if MULTIUSER_INSTALLMODE_ALLOW_ELEVATION


!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!include MultiUser.nsh
!include MUI2.nsh

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
!insertmacro MULTIUSER_PAGE_INSTALLMODE
; License page
!insertmacro MUI_PAGE_LICENSE "license.rtf"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\ProjectNotes.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

!system 'cscript build_file_list.vbs'

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "ProjectNotes-Setup64.exe"
InstallDir "$PROGRAMFILES64\Project Notes"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer

  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\ProjectNotes.exe"

  CreateDirectory "$SMPROGRAMS\Project Notes"
  CreateShortCut  "$SMPROGRAMS\Project Notes\Project Notes.lnk" "$INSTDIR\ProjectNotes.exe"
  CreateShortCut  "$DESKTOP\Project Notes.lnk" "$INSTDIR\ProjectNotes.exe"

  ; Project Notes Needed Libraries
  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\ProjectNotes.exe"

  ; include files that were built
  !include "install_files.nsh"

  ; Help files
  SetOutPath "$INSTDIR\docs"
  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\docs\Project Notes.qch"
  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\docs\Project Notes.qhc"
  
  ; Project Notes Base Plugins

  SetOutPath "$INSTDIR\plugins"
  File "..\..\plugins\archivenotes_plugin.py"
  File "..\..\plugins\archivenotes_plugin.py"
  ;cci_connectdrives_plugin.py
  ;cci_ifs_projects_import_plugin.py
  ;cci_ifs_projects_statusreport_plugin.py
  ;cci_scheduleoneonone_plugin.py
  File "..\..\plugins\contactexport_plugin.py"
  File "..\..\plugins\contactimport_plugin.py"
  File "..\..\plugins\editor_plugin.py"
  File "..\..\plugins\emailarchive_plugin.py"
  File "..\..\plugins\excelkill_plugin.py"
  File "..\..\plugins\globalsettings_plugin.py"
  File "..\..\plugins\newchangeorder_plugin.py"
  File "..\..\plugins\newmsproject_plugin.py"
  File "..\..\plugins\newpowerpoint_plugin.py"
  File "..\..\plugins\newriskregister_plugin.py"
  ; paul_cleanpc_plugin.py
  File "..\..\plugins\schedulecustomerkickoffinvite_plugin.py"
  File "..\..\plugins\schedulecustomerlessonslearnedinvite_plugin.py"
  File "..\..\plugins\schedulecustomerstatusinvite_plugin.py"
  File "..\..\plugins\scheduleinternalkickoffinvite_plugin.py"
  File "..\..\plugins\scheduleinternallessonslearnedinvite_plugin.py"
  File "..\..\plugins\scheduleinternalstatusinvite_plugin.py"
  File "..\..\plugins\sendmeetingnotes_plugin.py"
  File "..\..\plugins\sendprojectemailtoattendee_plugin.py"
  File "..\..\plugins\sendprojectemailtoperson_plugin.py"
  File "..\..\plugins\sendprojectemailtoteam_plugin.py"
  File "..\..\plugins\trackkerreport_plugin.py"
  
  SetOutPath "$INSTDIR\plugins\includes"
  File "..\..\plugins\includes\common.py"
  File "..\..\plugins\includes\excel_tools.py"
  File "..\..\plugins\includes\dialogNotesArchiveOptions.ui"
  ;File "..\..\plugins\includes\dialogStatusRptOptions.ui"
  File "..\..\plugins\includes\dialogTrackerRptOptions.ui"


  SetOutPath "$INSTDIR\plugins\templates"
  ;File "..\..\plugins\templates\Customer Kick Off Template.ppt"
  ;File "..\..\plugins\templates\Customer MES Kick Off Template.ppt"
  ;File "..\..\plugins\templates\Full DeltaV Schedule Template.mpp"
  ;File "..\..\plugins\templates\Full MES Schedule Template.mpp"
  ;File "..\..\plugins\templates\Internal Kick Off Template.ppt"
  File "..\..\plugins\templates\Lessons Learned Template.xlsx"
  ;File "..\..\plugins\templates\Lilly B132 Schedule Template.mpp"
  ;File "..\..\plugins\templates\Lilly B314 Schedule Template.mpp"
  ;File "..\..\plugins\templates\Lilly IFS B314 Schedule Template.mpp"
  File "..\..\plugins\templates\Meeting Template.xlsx"
  ;File "..\..\plugins\templates\Single Task Template.mpp"
  ;File "..\..\plugins\templates\Status Report Template.xlsx"
  File "..\..\plugins\templates\Tracker Items Template.xlsx"
  File "..\..\plugins\templates\Risk Register Template.xlsx"
  ;File "..\..\plugins\templates\PCR Template.docx"
  
  SetOutPath "$INSTDIR\dictionary"
  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\dictionary\index.ini"
  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\dictionary\es_ANY.dic"
  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\dictionary\es_ANY.aff"
  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\dictionary\en_US.dic"
  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\dictionary\en_US.aff"
  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\dictionary\en_GB.dic"
  File "..\..\..\build-ProjectNotes-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release\dictionary\en_GB.aff"

  ; not needed anymore EnVar::AddValue "Path" "$INSTDIR"
SectionEnd

Section -AdditionalIcons
  SetOutPath $INSTDIR
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\Project Notes\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\Project Notes\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\ProjectNotes.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$\"$INSTDIR\uninst.exe$\""
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\ProjectNotes.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "Project Notes was successfully removed from your computer."
FunctionEnd

Function .onInit
  !insertmacro MULTIUSER_INIT
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove Project Notes and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  ; SetShellVarContext all ; only use when RequestExecutionLevel admin

  ; not needed anymore EnVar::DeleteValue "Path" "$INSTDIR"

  ; Help files
  Delete "$INSTDIR\docs\Project Notes.qch"
  Delete "$INSTDIR\docs\Project Notes.qhc"

  Delete "$INSTDIR\docs\.Project Notes\*.*"
  RMDir "$INSTDIR\docs\.Project Notes"
  RMDir "$INSTDIR\docs\"

  !include "remove_files.nsh"

  ; remove base itms and dictionaris
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\dictionary\en_GB.aff"
  Delete "$INSTDIR\dictionary\en_GB.dic"
  Delete "$INSTDIR\dictionary\en_US.aff"
  Delete "$INSTDIR\dictionary\en_US.dic"
  Delete "$INSTDIR\dictionary\es_ANY.aff"
  Delete "$INSTDIR\dictionary\es_ANY.dic"
  Delete "$INSTDIR\dictionary\index.ini"
  RMDir "$INSTDIR\dictionary"

  ; remove all items in the plugins folder
  Delete "$INSTDIR\plugins\archivenotes_plugin.py"
  Delete "$INSTDIR\plugins\archivenotes_plugin.py"
  ;cci_connectdrives_plugin.py
  ;cci_ifs_projects_import_plugin.py
  ;cci_ifs_projects_statusreport_plugin.py
  ;cci_scheduleoneonone_plugin.py
  Delete "$INSTDIR\plugins\contactexport_plugin.py"
  Delete "$INSTDIR\plugins\contactimport_plugin.py"
  Delete "$INSTDIR\plugins\editor_plugin.py"
  Delete "$INSTDIR\plugins\emailarchive_plugin.py"
  Delete "$INSTDIR\plugins\excelkill_plugin.py"
  Delete "$INSTDIR\plugins\globalsettings_plugin.py"
  Delete "$INSTDIR\plugins\newchangeorder_plugin.py"
  Delete "$INSTDIR\plugins\newmsproject_plugin.py"
  Delete "$INSTDIR\plugins\newpowerpoint_plugin.py"
  Delete "$INSTDIR\plugins\newriskregister_plugin.py"

  ; paul_cleanpc_plugin.py
  Delete "$INSTDIR\plugins\schedulecustomerkickoffinvite_plugin.py"
  Delete "$INSTDIR\plugins\schedulecustomerlessonslearnedinvite_plugin.py"
  Delete "$INSTDIR\plugins\schedulecustomerstatusinvite_plugin.py"
  Delete "$INSTDIR\plugins\scheduleinternalkickoffinvite_plugin.py"
  Delete "$INSTDIR\plugins\scheduleinternallessonslearnedinvite_plugin.py"
  Delete "$INSTDIR\plugins\scheduleinternalstatusinvite_plugin.py"
  Delete "$INSTDIR\plugins\sendmeetingnotes_plugin.py"
  Delete "$INSTDIR\plugins\sendprojectemailtoattendee_plugin.py"
  Delete "$INSTDIR\plugins\sendprojectemailtoperson_plugin.py"  
  Delete "$INSTDIR\plugins\sendprojectemailtoteam_plugin.py"
  Delete "$INSTDIR\plugins\trackkerreport_plugin.py"
  
  Delete "$INSTDIR\plugins\includes\common.py"
  Delete "$INSTDIR\plugins\includes\excel_tools.py"
  Delete "$INSTDIR\plugins\includes\dialogNotesArchiveOptions.ui"
  ;Delete "$INSTDIR\plugins\includes\dialogStatusRptOptions.ui"
  Delete "$INSTDIR\plugins\includes\dialogTrackerRptOptions.ui"

  Delete "$INSTDIR\plugins\includes\__pycache__\*.*"
  RMDir "$INSTDIR\plugins\includes\__pycache__"
  RMDir "$INSTDIR\plugins\includes"

  ;Delete "$INSTDIR\plugins\templates\Customer Kick Off Template.ppt"
  ;Delete "$INSTDIR\plugins\templates\Customer MES Kick Off Template.ppt"
  ;Delete "$INSTDIR\plugins\templates\Full DeltaV Schedule Template.mpp"
  ;Delete "$INSTDIR\plugins\templates\Full MES Schedule Template.mpp"
  ;Delete "$INSTDIR\plugins\templates\Internal Kick Off Template.ppt"
  Delete "$INSTDIR\plugins\templates\Lessons Learned Template.xlsx"
  ;Delete "$INSTDIR\plugins\templates\Lilly B132 Schedule Template.mpp"
  ;Delete "$INSTDIR\plugins\templates\Lilly B314 Schedule Template.mpp"
  ;Delete "$INSTDIR\plugins\templates\Lilly IFS B314 Schedule Template.mpp"
  Delete "$INSTDIR\plugins\templates\Meeting Template.xlsx"
  ;Delete "$INSTDIR\plugins\templates\Single Task Template.mpp"
  ;Delete "$INSTDIR\plugins\templates\Status Report Template.xlsx"
  Delete "$INSTDIR\plugins\templates\Tracker Items Template.xlsx"
  Delete "$INSTDIR\plugins\templates\Risk Register Template.xlsx"
  Delete "$INSTDIR\plugins\templates\PCR Template.docx"

  RMDir "$INSTDIR\plugins\templates"

  Delete "$INSTDIR\plugins\__pycache__\*.*"
  RMDir "$INSTDIR\plugins\__pycache__"
  RMDIR "$INSTDIR\plugins"

  Delete "$INSTDIR\ProjectNotes.exe"

  Delete "$SMPROGRAMS\Project Notes\Uninstall.lnk"
  Delete "$SMPROGRAMS\Project Notes\Website.lnk"
  Delete "$DESKTOP\Project Notes.lnk"
  Delete "$SMPROGRAMS\Project Notes\Project Notes.lnk"

  RMDir "$SMPROGRAMS\Project Notes"
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
