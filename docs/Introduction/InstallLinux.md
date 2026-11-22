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

## Option 1 — Add the Project Notes repository (recommended)

Register the GitHub-hosted update server:

```bash
flatpak remote-add --if-not-exists projectnotes \
  https://kestermckinney.github.io/projectnotes-flatpak/projectnotes.flatpakrepo
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

## Moving an existing installation to GitHub

Project Notes releases are published on GitHub again. If you installed from the
GitLab repository (`kestermckinney.gitlab.io`), your installation will not
receive new releases until you point it at GitHub. When a newer release is
available on GitHub, Project Notes tells you so at startup and links to this page.

The GitHub repository is signed with a different key than the GitLab one, so
import its key along with the new address. Extract the public key from the
repository file:

```bash
curl --fail --location https://kestermckinney.github.io/projectnotes-flatpak/projectnotes.flatpakrepo \
  | sed -n 's/^GPGKey=//p' | base64 --decode > projectnotes-github.gpg
gpg --show-keys --with-fingerprint projectnotes-github.gpg
```

Verify that the primary fingerprint is `FEC8AE6825C826AFD564C424BA3027E5F2617C22`, then migrate:

```bash
flatpak remote-modify --gpg-verify --gpg-import=projectnotes-github.gpg \
  --url=https://kestermckinney.github.io/projectnotes-flatpak/ projectnotes
flatpak update com.projectnotespro.ProjectNotes
```

Use `flatpak remotes --show-details` to check your remotes. If you originally
installed system-wide, add `--system` to the migration commands; if you installed
for your user, add `--user`. If you used another name for the remote, substitute
that name for `projectnotes`. Your installed app and its data stay as they are.

If your `projectnotes` remote already shows the `github.io` address, nothing
needs to change. Run `flatpak update`.

## Option 2 — Single-file install

Download `ProjectNotes.flatpak` from the
[latest GitHub release](https://github.com/kestermckinney/ProjectNotes/releases/latest), then run:

```bash
flatpak install ProjectNotes.flatpak
```

The bundle registers the GitHub repository for later updates. If an existing
remote still points to GitLab, change it using the migration steps above.

Your database and settings live in your home folder and are not removed by
uninstalling the app.

## Verifying the download

The public signing key is embedded in `projectnotes.flatpakrepo`. Flatpak
verifies signed installs and updates automatically. The GitHub signing key has
primary fingerprint `FEC8AE6825C826AFD564C424BA3027E5F2617C22`. The GitLab
repository used a different key, `E0C07E21F0E41F83CBDCCB04F82B4E4D2183B011`.
