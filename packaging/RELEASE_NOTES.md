# Project Notes 6.5.2

This release changes how the macOS version works with Calendar and how the Mac App Store version copies data from the downloaded version. Both changes remove permissions the Mac App Store does not allow.

### macOS

- **MacOS Email and Calendar** saves scheduled meetings to your default calendar for new events, then brings Calendar forward and tells you which calendar and date it used. macOS now asks only for calendar access, not for permission to control Calendar. Open the event in Calendar, paste the copied attendees and click **Send**.
- The Mac App Store version asks you to choose the downloaded version's data folder before copying your data. The Open dialog starts in that folder.
- The downloaded version keeps a copy of its settings in its data folder, so the Mac App Store version can bring them over along with your data.

### Project Notes is back on GitHub

The project now lives at https://github.com/kestermckinney/ProjectNotes. This release is published on GitHub and on GitLab. GitLab stays up for a while so installed copies can reach it, but later releases appear only on GitHub.

- **Check for Updates** (Windows and macOS) looks for releases on GitHub as well as GitLab and offers the newest one it finds. If both have the same version, it uses GitHub. Builds 6.4.0 through 6.5.1 check only GitLab, so install this release to keep receiving updates.
- On Linux, Project Notes tells you at startup when your Flatpak installation still uses the GitLab repository, and links to the steps for moving it.

### Install on Linux

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak remote-add --if-not-exists projectnotes https://kestermckinney.github.io/projectnotes-flatpak/projectnotes.flatpakrepo
flatpak install projectnotes com.projectnotespro.ProjectNotes
```

Alternatively, download `ProjectNotes.flatpak` from the GitHub release and run `flatpak install ProjectNotes.flatpak`. The bundle includes the repository URL and signing key for future updates.

Enable automatic Flatpak updates in your desktop software manager, or run:

```bash
flatpak update com.projectnotespro.ProjectNotes
```

### Moving a Linux installation from GitLab

The GitHub repository is signed with a different key. Extract it from the repository file and check it:

```bash
curl --fail --location https://kestermckinney.github.io/projectnotes-flatpak/projectnotes.flatpakrepo \
  | sed -n 's/^GPGKey=//p' | base64 --decode > projectnotes-github.gpg
gpg --show-keys --with-fingerprint projectnotes-github.gpg
```

The primary fingerprint must be `FEC8AE6825C826AFD564C424BA3027E5F2617C22`. Then move the remote and update:

```bash
flatpak remote-modify --gpg-verify --gpg-import=projectnotes-github.gpg \
  --url=https://kestermckinney.github.io/projectnotes-flatpak/ projectnotes
flatpak update com.projectnotespro.ProjectNotes
```

Add `--user` or `--system` to match how you installed. Full instructions: https://www.projectnotespro.com/docs/Introduction/InstallLinux

Windows and macOS update downloads require their installers to be attached to the release on each host.
