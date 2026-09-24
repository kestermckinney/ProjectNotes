# Native report-plugin removal manifest

Status: applied. The three bundled report plugins below are deleted, together
with their exclusive option forms; the Widgets frontend no longer exists, so
its scope question is settled.

| Bundled file | Native replacement | Removed |
| --- | --- | --- |
| `plugins/exportnotes_plugin.py` | Send Meeting Notes and Meeting Notes Report native preparation/builders | Yes, with `plugins/forms/dialogExportNotesOptions.ui`. |
| `plugins/exportstatusreport_plugin.py` | Native Status Report builder and review route | Yes, with `plugins/forms/dialogExportStatusReportOptions.ui`. |
| `plugins/exporttrackeritems_plugin.py` | Native Tracker Items Report builder and review route | Yes, with `plugins/forms/dialogExportTrackerOptions.ui`. |

The legacy Meeting and Email Types settings were removed in the same pass, now
that email templates, meeting templates and Schedule Meeting are native: the
settings dialogs and their two forms are gone, as is the dynamic Send Email /
Schedule Meeting menu population in `base_plugin.py`. Saved `MeetingEmailTypes`
settings rows are deliberately left in place for import.

Still intentionally **not** removed: shared plugin helpers (including
`dialogExportLocation.ui`, used by the template plugins), user plugins,
unrelated report plugins, and user data/settings. Upgrade cleanup
must identify the shipped bundled files exactly and never recursively remove a
plugin directory.

## Upgrade cleanup

Every bundled plugin, thread, form and helper that any release shipped and a
later one dropped is listed by exact name in three places, one per packaging
route. Keep them identical:

- `packaging/windows/remove_retired_plugins.nsh`: run by the Windows installer
  before it copies the new payload, and by the uninstaller.
- `packaging/RemoveRetiredPlugins.cmake.in`: an `install(SCRIPT)` for the
  Linux/Flatpak install tree.
- `packaging/macos/projectnotes-scripts/preinstall`: run by the macOS pkg.

The list covers the report plugins above, the Python File Finder, and the
older email, meeting, contact and settings plugins retired in 4.0 and earlier.
Cached bytecode for those scripts goes with them. User plugins are never
touched, because no plugin folder is removed recursively.
