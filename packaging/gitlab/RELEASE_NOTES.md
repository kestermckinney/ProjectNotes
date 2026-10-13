# Project Notes 3.4.0

This release expands contact management and communication workflows, improves keyboard access and navigation, and completes the move to the Qt Quick desktop interface.

### New features

- **Contact synchronization:** Sync contacts with macOS Contacts, Microsoft Outlook, and Thunderbird. Review proposed imports and exports, resolve duplicate matches, and inspect conflicts before applying changes.
- **Meeting and email integrations:** Create project and location email drafts, prepare meeting invitations with iCalendar, Thunderbird, Microsoft 365, or native Apple Mail and Calendar, and use reusable meeting templates.
- **Reports and audiences:** Preview reports before sending, save recipient audiences, select recipients with clearer review steps, and include location attachments in project communications.
- **Navigation and preferences:** Use keyboard shortcuts and improved focus behavior across lists, folders, the navigation rail, forms, and preferences. Settings are organized into clearer sections.
- **Contact and company records:** The interface and data layer now use Contact and Company terminology consistently, with updated filtering, search, details, and mobile screens.
- **Windows integrations:** The Windows package includes Outlook and Thunderbird contact helpers alongside the main application and Remote Host installer.

### Fixes and improvements

- Fixed keyboard navigation and selection in combo boxes, forms, lists, and dialogs.
- Improved contact import matching, duplicate-aware exports, and contact-sync formatting.
- Fixed meeting attendee flows, email recipient selection, and project/location attachment handling.
- Removed the retired Widgets interface and obsolete bundled meeting/email plugins. Existing databases are cleaned of retired settings while current QML preferences and project data are preserved.
- Improved update handling and release downloads through GitLab.

Linux builds are distributed through a signed Flatpak repository on GitLab Pages. Windows and macOS installers are distributed as release assets.

### Install on Linux

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak remote-add --if-not-exists projectnotes https://kestermckinney.gitlab.io/ProjectNotes/flatpak/projectnotes.flatpakrepo
flatpak install projectnotes com.projectnotespro.ProjectNotes
```

Alternatively, download `ProjectNotes.flatpak` from this release and run `flatpak install ProjectNotes.flatpak`. The bundle includes the application repository URL and signing key for future updates.

Enable automatic Flatpak updates in your desktop software manager, or run:

```bash
flatpak update com.projectnotespro.ProjectNotes
```

If you previously installed from the GitHub repository, migrate your remote before updating:

```bash
curl --fail --location --output projectnotes.gpg https://kestermckinney.gitlab.io/ProjectNotes/flatpak/projectnotes.gpg
flatpak remote-modify --gpg-import=projectnotes.gpg --url=https://kestermckinney.gitlab.io/ProjectNotes/flatpak/ projectnotes
flatpak update com.projectnotespro.ProjectNotes
```

Existing desktop builds that still check GitHub require a one-time manual installation of this version to switch their update checks to GitLab. Windows and macOS update downloads require their matching installers to be attached to the release.
