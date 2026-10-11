# Installing on Linux (Flatpak)

Project Notes is distributed for Linux as a [Flatpak](https://flatpak.org). It
runs on any distribution with Flatpak installed (Fedora, Ubuntu, Debian, Arch,
openSUSE, and others).

## Prerequisites

Project Notes uses the KDE runtime hosted on Flathub:

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

If you do not have Flatpak yet, see [flatpak.org/setup](https://flatpak.org/setup/).

The working update server is currently hosted in the ProjectNotes Pages site.
A separate `projectnotes-flatpak` GitLab Pages project is planned; installation
commands below continue to use the live server until that deployment is ready.

## Option 1 — Add the Project Notes repository (recommended)

Register the GitLab-hosted update server:

```bash
flatpak remote-add --if-not-exists projectnotes \
  https://kestermckinney.gitlab.io/ProjectNotes/flatpak/projectnotes.flatpakrepo
flatpak install projectnotes com.projectnotespro.ProjectNotes
```

Launch it from your application menu, or run:

```bash
flatpak run com.projectnotespro.ProjectNotes
```

To update later:

```bash
flatpak update
```

## Moving an existing installation to GitLab

The former GitHub-hosted update server is unavailable. If you already have the
`projectnotes` remote, import the new GitLab signing key and change its URL.
Download the public key first:

```bash
curl --fail --location --output projectnotes.gpg https://kestermckinney.gitlab.io/ProjectNotes/flatpak/projectnotes.gpg
gpg --show-keys --with-fingerprint projectnotes.gpg
```

Verify that the primary fingerprint is `E0C07E21F0E41F83CBDCCB04F82B4E4D2183B011`, then migrate:

```bash
flatpak remote-modify --gpg-verify --gpg-import=projectnotes.gpg --url=https://kestermckinney.gitlab.io/ProjectNotes/flatpak/ projectnotes
flatpak update com.projectnotespro.ProjectNotes
```

Use `flatpak remotes --show-details` to check your remotes. If you originally
installed system-wide, add `--system` to the migration commands; if you installed
for your user, add `--user`. If you used another name for the old remote, substitute
that name for `projectnotes`. This preserves your installed app and its data.

## Option 2 — Single-file install

Download `ProjectNotes.flatpak` from the
[latest GitLab release](https://gitlab.com/kestermckinney/ProjectNotes/-/releases/permalink/latest), then run:

```bash
flatpak install ProjectNotes.flatpak
```

A current bundle includes the GitLab repository descriptor for subsequent
updates. If an existing remote still points to the former host, update its URL
using the migration instructions above.

Your database and settings live in your home folder and are not removed by
uninstalling the app.

## Verifying the download

The public signing key is embedded in `projectnotes.flatpakrepo`. Flatpak
verifies signed installs and updates automatically. The GitLab signing key has primary fingerprint
`E0C07E21F0E41F83CBDCCB04F82B4E4D2183B011`; it differs from the former repository key.
