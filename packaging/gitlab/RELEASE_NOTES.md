# Project Notes 6.4.2

This maintenance release fixes the Windows in-app update path.

### Fixes

- The Windows updater now finds the desktop installer when GitLab gives its release link a descriptive label instead of a filename. It also skips the separate Remote Host installer.

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
