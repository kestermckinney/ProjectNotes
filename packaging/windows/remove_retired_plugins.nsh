; Every bundled Python plugin, thread, form and helper, the embedded Python
; runtime, and the Qt WebEngine files that earlier Project Notes releases
; shipped. File copies merge into $INSTDIR, so
; without this an upgrade leaves the retired scripts in place and their menus
; come back. Included by the install section (before the new payload is copied)
; and by the uninstaller.
;
; Names are listed one by one on purpose: the plugins folder can also hold the
; user's own plugins, so it is never removed recursively. Keep this list in step
; with packaging/RemoveRetiredPlugins.cmake.in and
; packaging/macos/projectnotes-scripts/preinstall.

; ── Retired plugins ─────────────────────────────────────────────────────────────
Delete "$INSTDIR\plugins\archivenotes_plugin.py"
Delete "$INSTDIR\plugins\cci_connectdrives_plugin.py"
Delete "$INSTDIR\plugins\cci_ifs_projects_import_plugin.py"
Delete "$INSTDIR\plugins\cci_ifs_projects_statusreport_plugin.py"
Delete "$INSTDIR\plugins\cci_scheduleoneonone_plugin.py"
Delete "$INSTDIR\plugins\cci_scheduleoneononeattendee_plugin.py"
Delete "$INSTDIR\plugins\cci_scheduleoneononeteam_plugin.py"
Delete "$INSTDIR\plugins\contactexport_plugin.py"
Delete "$INSTDIR\plugins\contactimport_plugin.py"
Delete "$INSTDIR\threads\icloudsync_thread.py"
Delete "$INSTDIR\threads\__pycache__\icloudsync_thread.*.pyc"
Delete "$INSTDIR\plugins\copy_email_member_plugin.py"
Delete "$INSTDIR\plugins\copy_email_plugin.py"
Delete "$INSTDIR\plugins\copy_filepath_plugin.py"
Delete "$INSTDIR\plugins\editor_plugin.py"
Delete "$INSTDIR\plugins\email_as_attachment_plugin.py"
Delete "$INSTDIR\plugins\emailarchive_plugin.py"
Delete "$INSTDIR\plugins\exportnotes_plugin.py"
Delete "$INSTDIR\plugins\exportstatusreport_plugin.py"
Delete "$INSTDIR\plugins\exporttrackeritems_plugin.py"
Delete "$INSTDIR\plugins\exporttrakeritems_plugin.py"
Delete "$INSTDIR\plugins\findprojectemailperson_plugin.py"
Delete "$INSTDIR\plugins\findprojectmailperson_plugin.py"
Delete "$INSTDIR\plugins\globalsettings_plugin.py"
Delete "$INSTDIR\plugins\ifs_cloud_projectsstatusreportfixed_plugin.py"
Delete "$INSTDIR\plugins\ifs_cloud_settings_plugin.py"
Delete "$INSTDIR\plugins\ifs_project_report_plugin.py"
Delete "$INSTDIR\plugins\outlooksync_plugin.py"
Delete "$INSTDIR\plugins\paul_cleanpc_plugin.py"
Delete "$INSTDIR\plugins\schedulecustomerkickoffinvite_plugin.py"
Delete "$INSTDIR\plugins\schedulecustomerlessonslearnedinvite_plugin.py"
Delete "$INSTDIR\plugins\schedulecustomerstatusinvite_plugin.py"
Delete "$INSTDIR\plugins\scheduleinternalkickoffinvite_plugin.py"
Delete "$INSTDIR\plugins\scheduleinternallessonslearnedinvite_plugin.py"
Delete "$INSTDIR\plugins\scheduleinternalstatusinvite_plugin.py"
Delete "$INSTDIR\plugins\sendmeetingnotes_plugin.py"
Delete "$INSTDIR\plugins\sendprojectemail_plugin.py"
Delete "$INSTDIR\plugins\sendprojectemailtoattendee_plugin.py"
Delete "$INSTDIR\plugins\sendprojectemailtoperson_plugin.py"
Delete "$INSTDIR\plugins\sendprojectemailtoteam_plugin.py"
Delete "$INSTDIR\plugins\tester_plugin.py"
Delete "$INSTDIR\plugins\tester_tools.py"
Delete "$INSTDIR\plugins\trackkerreport_plugin.py"

; ── Retired forms and helpers ──────────────────────────────────────────────────
Delete "$INSTDIR\plugins\forms\dialogClassification.ui"
Delete "$INSTDIR\plugins\forms\dialogExportNotesOptions.ui"
Delete "$INSTDIR\plugins\forms\dialogExportStatusReportOptions.ui"
Delete "$INSTDIR\plugins\forms\dialogExportTrackerOptions.ui"
Delete "$INSTDIR\plugins\forms\dialogFileFinder.ui"
Delete "$INSTDIR\plugins\forms\dialogMeetingEmailTemplate.ui"
Delete "$INSTDIR\plugins\forms\dialogMeetingEmailTypes.ui"
Delete "$INSTDIR\plugins\forms\dialogStatusRptOptions.ui"
Delete "$INSTDIR\plugins\forms\dialogTrackerOptions.ui"
Delete "$INSTDIR\plugins\forms\dialogTrackerRptOptions.ui"
Delete "$INSTDIR\plugins\includes\dialogDuplicateFilesFound.ui"
Delete "$INSTDIR\plugins\includes\dialogNotesArchiveOptions.ui"
Delete "$INSTDIR\plugins\includes\dialogStatusRptOptions.ui"
Delete "$INSTDIR\plugins\includes\dialogTrackerRptOptions.ui"

; ── Replaced by JavaScript plugins ──────────────────────────────────────────────
Delete "$INSTDIR\plugins\base_plugin.py"
Delete "$INSTDIR\plugins\base_plugin_settings.py"
Delete "$INSTDIR\plugins\excelkill_plugin.py"
Delete "$INSTDIR\plugins\ifs_ssrs_generate_plugin.py"
Delete "$INSTDIR\plugins\ifscloud_plugin_settings.py"
Delete "$INSTDIR\plugins\myshortcuts_plugin.py"
Delete "$INSTDIR\plugins\newchangeorder_plugin.py"
Delete "$INSTDIR\plugins\newmsproject_plugin.py"
Delete "$INSTDIR\plugins\newpcrregistry_plugin.py"
Delete "$INSTDIR\plugins\newpowerpoint_plugin.py"
Delete "$INSTDIR\plugins\newriskregister_plugin.py"
Delete "$INSTDIR\plugins\open_msproject_plugin.py"
Delete "$INSTDIR\plugins\projectnotes.py"
Delete "$INSTDIR\plugins\team_member_quick_add_plugin.py"
Delete "$INSTDIR\plugins\wordkill_plugin.py"
Delete "$INSTDIR\plugins\includes\collaboration_tools.py"
Delete "$INSTDIR\plugins\includes\common.py"
Delete "$INSTDIR\plugins\includes\excel_tools.py"
Delete "$INSTDIR\plugins\includes\graphapi_tools.py"
Delete "$INSTDIR\plugins\includes\icloud_tools.py"
Delete "$INSTDIR\plugins\includes\ifs_tools.py"
Delete "$INSTDIR\plugins\includes\noteformatter.py"
Delete "$INSTDIR\plugins\includes\outlook_tools.py"
Delete "$INSTDIR\plugins\includes\settings_dialog.py"
Delete "$INSTDIR\plugins\includes\word_tools.py"
Delete "$INSTDIR\threads\ifssync_thread.py"
Delete "$INSTDIR\threads\outlooksync_thread.py"
Delete "$INSTDIR\plugins\forms\dialogDuplicateFilesFound.ui"
Delete "$INSTDIR\plugins\forms\dialogEditor.ui"
Delete "$INSTDIR\plugins\forms\dialogExportLocation.ui"
Delete "$INSTDIR\plugins\forms\dialogIFSCloud.ui"
Delete "$INSTDIR\plugins\forms\dialogMyShortcuts.ui"
Delete "$INSTDIR\plugins\forms\dialogOutlookIntegrationOptions.ui"
Delete "$INSTDIR\plugins\forms\dialogSSRSOptions.ui"
Delete "$INSTDIR\plugins\forms\dialogSettingsMigrator.ui"
Delete "$INSTDIR\plugins\forms\dialogTeamMemberQuickAdd.ui"

; ── Shipped by early releases ──────────────────────────────────────────────────
Delete "$INSTDIR\plugins\includes\graphapitools.py"
Delete "$INSTDIR\plugins\includes\dialogClassification.ui"
Delete "$INSTDIR\plugins\includes\dialogEditor.ui"
Delete "$INSTDIR\plugins\includes\dialogFileFinder.ui"
Delete "$INSTDIR\plugins\includes\dialogIFSCloud.ui"
Delete "$INSTDIR\plugins\includes\dialogMeetingTemplate.ui"
Delete "$INSTDIR\plugins\includes\dialogMeetingTypes.ui"
Delete "$INSTDIR\plugins\includes\dialogMyShortcuts.ui"
Delete "$INSTDIR\plugins\includes\dialogOutlookIntegrationOptions.ui"
Delete "$INSTDIR\plugins\examplemeeting.xml"
Delete "$INSTDIR\plugins\exampleproject.xml"
Delete "$INSTDIR\plugins\people.xml"
Delete "$INSTDIR\plugins\plugintest.db"
Delete "$INSTDIR\plugins\TestExcel.xlsx"

; ── Retired Office Tools templates (replaced by the configurable templates) ────
Delete "$INSTDIR\plugins\office-tools\templates\Lessons Learned Template.xlsx"
Delete "$INSTDIR\plugins\office-tools\templates\Meeting Template.xlsx"
Delete "$INSTDIR\plugins\office-tools\templates\Risk Register Template.xlsx"

; ── Retired threads ────────────────────────────────────────────────────────────
Delete "$INSTDIR\threads\filefinder_thread.py"
Delete "$INSTDIR\threads\tester_thread_1.py"

; ── Cached bytecode ────────────────────────────────────────────────────────────
; Python rebuilds these on the next start, so clearing them all is safe and
; also covers bytecode left behind by any retired script above.
; The folders hold nothing but generated bytecode, so they go whole, including
; any .opt-N.pyc or other file Python left in them. The bundled plugin folders
; are named one by one because plugins\ also holds the user's own plugins.
RMDir /r "$INSTDIR\__pycache__"
RMDir /r "$INSTDIR\threads\__pycache__"
RMDir /r "$INSTDIR\plugins\__pycache__"
RMDir /r "$INSTDIR\plugins\includes\__pycache__"
RMDir /r "$INSTDIR\plugins\base\__pycache__"
RMDir /r "$INSTDIR\plugins\myshortcuts\__pycache__"
RMDir /r "$INSTDIR\plugins\office-tools\__pycache__"
RMDir /r "$INSTDIR\plugins\archive-mail\__pycache__"
RMDir /r "$INSTDIR\plugins\outlook-integration\__pycache__"
RMDir /r "$INSTDIR\plugins\team-member-quick-add\__pycache__"

; ── Emptied folders (RMDir only removes a folder that is already empty) ────────
RMDir "$INSTDIR\plugins\includes"
RMDir "$INSTDIR\plugins\forms"
; The old shared templates folder goes whole, including any files added to it:
; document templates now come from each plugin, and the folder's former role
; is being replaced.
RMDir /r "$INSTDIR\plugins\templates"
RMDir "$INSTDIR\threads"

; ── Embedded Python runtime (earlier releases) ─────────────────────────────────
; Installed by this installer only, so the site-packages folder can go whole.
RMDir /r "$INSTDIR\site-packages"
Delete "$INSTDIR\python.cat"
Delete "$INSTDIR\python.exe"
Delete "$INSTDIR\python3.dll"
Delete "$INSTDIR\python313.dll"
Delete "$INSTDIR\python313.zip"
Delete "$INSTDIR\python313._pth"
Delete "$INSTDIR\pythonw.exe"
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
Delete "$INSTDIR\libcrypto-3.dll"
Delete "$INSTDIR\libffi-8.dll"
Delete "$INSTDIR\libssl-3.dll"
Delete "$INSTDIR\sqlite3.dll"

; ── ICU copied from the build machine (6.4.x and earlier) ──────────────────────
; Those installers shipped the build machine's System32 icuuc.dll, which is an
; ARM64 build. Qt6Core.dll imports icuuc.dll and the loader looks in $INSTDIR
; before System32, so a leftover copy stops the x64 app at startup with
; 0xc0000142. Windows provides the right icuuc.dll itself.
Delete "$INSTDIR\icuuc.dll"

; ── Qt WebEngine (earlier releases printed reports with it) ────────────────────
Delete "$INSTDIR\Qt6Positioning.dll"
Delete "$INSTDIR\Qt6WebChannel.dll"
Delete "$INSTDIR\Qt6WebChannelQuick.dll"
Delete "$INSTDIR\Qt6WebEngineCore.dll"
Delete "$INSTDIR\Qt6WebEngineQuick.dll"
Delete "$INSTDIR\QtWebEngineProcess.exe"
RMDir /r "$INSTDIR\resources"
RMDir /r "$INSTDIR\translations"
RMDir /r "$INSTDIR\qtwebengine_locales"
RMDir /r "$INSTDIR\position"
