# Project Notes for Linux

This is the Flatpak repository for [Project Notes](https://www.projectnotespro.com).
It serves installs and automatic updates. The application source and releases are
in [kestermckinney/ProjectNotes](https://github.com/kestermckinney/ProjectNotes).

## Install

Project Notes uses the KDE runtime from Flathub. Add Flathub, then this repository:

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak remote-add --if-not-exists projectnotes \
  https://kestermckinney.github.io/projectnotes-flatpak/projectnotes.flatpakrepo
flatpak install projectnotes com.projectnotespro.ProjectNotes
```

Start it from your application menu, or run `flatpak run com.projectnotespro.ProjectNotes`.

No Flatpak yet? See [flatpak.org/setup](https://flatpak.org/setup/).

### Single-file install

Download `ProjectNotes.flatpak` from the
[latest release](https://github.com/kestermckinney/ProjectNotes/releases/latest) and run:

```bash
flatpak install ProjectNotes.flatpak
```

The bundle adds this repository, so later updates arrive the same way.

## Update

Turn on automatic updates in your software manager, or run:

```bash
flatpak update com.projectnotespro.ProjectNotes
```

## Moving from GitLab

Installs added from `kestermckinney.gitlab.io` stopped getting updates when the
project moved back to GitHub. This repository is signed with a different key, so
import it along with the new address:

```bash
curl --fail --location https://kestermckinney.github.io/projectnotes-flatpak/projectnotes.flatpakrepo \
  | sed -n 's/^GPGKey=//p' | base64 --decode > projectnotes-github.gpg
gpg --show-keys --with-fingerprint projectnotes-github.gpg
```

Check that the primary fingerprint is `FEC8AE6825C826AFD564C424BA3027E5F2617C22`, then:

```bash
flatpak remote-modify --gpg-verify --gpg-import=projectnotes-github.gpg \
  --url=https://kestermckinney.github.io/projectnotes-flatpak/ projectnotes
flatpak update com.projectnotespro.ProjectNotes
```

Add `--user` or `--system` to match how you installed. Your data and settings stay
where they are.

Full instructions: [Installing on Linux](https://www.projectnotespro.com/docs/Introduction/InstallLinux).
