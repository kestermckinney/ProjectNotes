# Project Notes 6.5.0

Plugins are now written in JavaScript and run inside Project Notes. Python is no longer needed or bundled.

- Rewrites the bundled plugins (My Shortcuts, Archive Mail, Office Tools, Team Member Quick Add) and the IFS Cloud plugin in JavaScript. Their settings carry over from the Python versions on first start.
- Adds plugin installation from `.pnplugin` packages, with a review of what each plugin can do before it runs.
- Adds plugin settings backup under **Settings > Plugins**. Export settings to a file and import them on another computer. Files from the old Settings Migrator import too.
- Moves your own plugins to the application data folder, so upgrades leave them alone.
- Renders report PDFs without Qt WebEngine, which makes the download much smaller.
- Adds native macOS Contacts support and renames People and Clients to Contacts and Companies throughout.
- Reorganizes the Settings page and moves Appearance into Preferences.
- Stores project folders in their own synced table. A project is now in one folder at a time, and existing folders move over on the first start; a project that was in several folders keeps the first one.
- Click a folder's name in the sidebar to show only its projects. The arrow at the end of the header collapses the folder, and collapsed folders are now remembered per computer.
- Renames folders from Settings or from a new right-click menu on folder headers, which also edits the icon and color.
- Adds a Folder filter to the project list on desktop and mobile, and exposes `folder_id` on projects to plugins.
- The Windows and macOS installers remove the embedded Python runtime, its packages, and the retired Python plugins left behind by earlier versions.

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

Desktop builds older than 6.4.0 still check GitHub for updates and need a one-time manual installation of this version to switch to GitLab. Windows and macOS update downloads require their matching installers to be attached to the release.
