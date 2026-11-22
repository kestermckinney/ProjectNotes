# Project Notes 6.5.2

This release changes how the macOS version works with Calendar and how the Mac App Store version copies data from the downloaded version. Both changes remove permissions the Mac App Store does not allow.

### macOS

- **MacOS Email and Calendar** saves scheduled meetings to your default calendar for new events, then brings Calendar forward and tells you which calendar and date it used. macOS now asks only for calendar access, not for permission to control Calendar. Open the event in Calendar, paste the copied attendees and click **Send**.
- The Mac App Store version asks you to choose the downloaded version's data folder before copying your data. The Open dialog starts in that folder.
- The downloaded version keeps a copy of its settings in its data folder, so the Mac App Store version can bring them over along with your data.

### Updates (Windows and macOS)

- **Check for Updates** looks for releases on GitHub as well as GitLab and offers the newest one it finds. If both have the same version, it uses GitHub. This prepares for moving the project to GitHub.

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

Desktop builds from 6.4.0 through 6.5.1 check only GitLab, and builds older than 6.4.0 check only GitHub. Windows and macOS update downloads require their matching installers to be attached to the release.
