# Project Notes 6.5.1

This release lets a project keep its files in Microsoft Teams.

- A project's primary project folder can be its Microsoft Teams channel folder. File Finder lists it as **Teams Project Folder** and uses it when there is no local folder, or always with the new **Use the Microsoft Teams folder as the project folder when both are found** setting. **Set as Primary Project Folder** on a Teams folder location chooses it by hand.
- Documents made from MS Office Templates, reports saved with the new **Save to Project Folder** button, Archive Mail, and IFS Cloud SSRS reports all save into a Teams project folder through Microsoft 365.
- The Office 365 app registration now needs `Files.ReadWrite.All` in place of `Files.Read.All`. After an administrator grants it, sign in again under **Settings > Office 365 Integration**.
- Adds `projectFiles` to the plugin API for reading and writing the project folder, local or Microsoft Teams.
- Removes the leftover ARM64 `icuuc.dll` that upgrades on Windows could leave behind.

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
