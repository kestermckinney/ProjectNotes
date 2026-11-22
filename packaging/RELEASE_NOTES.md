# Project Notes 6.5.3

This release collects everything since 6.5.0. Releases 6.5.1 and 6.5.2 were announced only on GitLab, so their changes are listed here too. It is also the first release published only on GitHub.

### Microsoft Teams project folders

- A project's primary folder can now be its Microsoft Teams channel folder. Reports, archived email and other files Project Notes saves to the project folder are written to Teams through Microsoft Graph.
- File Finder lists the Teams channel folder as **Teams Project Folder** next to the local **Project Folder**, and either can be made primary. The local folder wins unless **Use the Microsoft Teams folder as the project folder when both are found** is on. The location menu can also set a Teams folder by hand.
- Saving into Teams needs the Files.ReadWrite.All permission. If your organization has not approved it, turn off **Save files into Microsoft Teams folders** in Settings and sign in again. Teams folders stay readable, and files are saved to local folders instead.
- Email archiving uses the local project folder whenever one is available.
- Signing in under **Settings > Office 365 Integration** now asks for every permission File Finder's Teams search needs. If your organization lets you approve apps yourself, File Finder no longer needs an administrator to approve the Teams permissions first. Turned on **Include Office 365 locations** after signing in? Sign in again.
- **Office 365 Integration** in the user guide now has a checklist for administrators that covers every Microsoft 365 permission Project Notes uses.

### macOS

- **MacOS Email and Calendar** saves scheduled meetings to your default calendar, then brings Calendar forward and tells you which calendar and date it used. macOS asks only for calendar access, not for permission to control Calendar. Open the event in Calendar, paste the copied attendees and click **Send**.
- The Mac App Store version asks you to choose the downloaded version's data folder before copying your data. The Open dialog starts in that folder.
- The downloaded version keeps a copy of its settings in its data folder, so the Mac App Store version can bring them over along with your data.

### Windows

- Upgrades from 6.4.x no longer fail to start with error 0xc0000142. The installer now removes a leftover `icuuc.dll` that 6.4.x installers put in the program folder.
- Emails composed through Outlook (Classic) keep the signature Outlook adds to new messages, including plugin emails such as IFS reports. Project Notes content goes above it.

### Updates

- **Check for Updates** (Windows and macOS) looks for releases on GitHub as well as GitLab and offers the newest it finds. Builds 6.4.0 through 6.5.1 check only GitLab: they are offered 6.5.2 there first, and after that update they find this release on GitHub.
- A new preference, **Check for updates automatically at startup**, turns off the quiet check at launch. **Check for Updates** still works by hand. Store versions hide the option because the store handles updates.
- On Linux, Project Notes tells you at startup when your Flatpak installation still uses the GitLab repository, and links to the steps for moving it.

### Fixes

- **Add Team Member** lists every contact, even when the Contacts list has a search or filter applied. This applies to desktop and mobile.
- Fixed crashes at launch caused by plugins starting at the same moment.
- The search clear buttons work on every page.
- Checkboxes in report forms respond to clicks again. A hidden scrollbar was covering them.
- Temporary files made by plugins are removed when Project Notes closes.
- Help links open the user guide on the Project Notes website.

### Install on Linux

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak remote-add --if-not-exists projectnotes https://kestermckinney.github.io/projectnotes-flatpak/projectnotes.flatpakrepo
flatpak install projectnotes com.projectnotespro.ProjectNotes
```

Alternatively, download `ProjectNotes.flatpak` from this release and run `flatpak install ProjectNotes.flatpak`. The bundle includes the repository URL and signing key for future updates.

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
