; Every bundled plugin, thread, form and helper that an earlier Project Notes
; release shipped and a later one dropped. File copies merge into $INSTDIR, so
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
Delete "$INSTDIR\plugins\templates\Tracker Items Template.xlsx"

; ── Retired threads ────────────────────────────────────────────────────────────
Delete "$INSTDIR\threads\filefinder_thread.py"
Delete "$INSTDIR\threads\tester_thread_1.py"

; ── Cached bytecode ────────────────────────────────────────────────────────────
; Python rebuilds these on the next start, so clearing them all is safe and
; also covers bytecode left behind by any retired script above.
Delete "$INSTDIR\plugins\__pycache__\*.pyc"
Delete "$INSTDIR\plugins\__pycache__\*.pyo"
Delete "$INSTDIR\plugins\includes\__pycache__\*.pyc"
Delete "$INSTDIR\plugins\includes\__pycache__\*.pyo"
Delete "$INSTDIR\threads\__pycache__\*.pyc"
Delete "$INSTDIR\threads\__pycache__\*.pyo"
