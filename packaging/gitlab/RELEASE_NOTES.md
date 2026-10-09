# Project Notes 6.4.0

Project Notes now uses GitLab for source code, release downloads, and desktop update checks.

- Replaces the legacy desktop interface with the Qt Quick desktop application.
- Adds built-in report previews, saved report audiences, email templates, and report delivery.
- Adds meeting scheduling with iCalendar, Thunderbird, and Microsoft 365, plus native Apple Mail/Calendar and Windows Outlook integration.
- Improves project navigation, note editing, context menus, and dialog interactions.
- Distributes Linux builds through a signed Flatpak repository on GitLab Pages, supporting updates through desktop software managers and `flatpak update`.

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
flatpak remote-modify --url=https://kestermckinney.gitlab.io/ProjectNotes/flatpak/ projectnotes
flatpak update com.projectnotespro.ProjectNotes
```

Existing desktop builds that still check GitHub require a one-time manual installation of this version to switch their update checks to GitLab. Windows and macOS update downloads require their matching installers to be attached to the release.
