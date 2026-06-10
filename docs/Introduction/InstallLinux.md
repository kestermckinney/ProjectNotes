# Installing on Linux (Flatpak)

Project Notes is distributed for Linux as a [Flatpak](https://flatpak.org). It
runs on any distribution with Flatpak installed (Fedora, Ubuntu, Debian, Arch,
openSUSE, and others).

You can install it two ways. The first is recommended because it keeps the app
up to date automatically.

## Prerequisites

Project Notes uses the KDE runtime, which is hosted on Flathub, so make sure the
Flathub remote is present (it already is on most distributions):

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

If you do not have Flatpak itself yet, see
[flatpak.org/setup](https://flatpak.org/setup/) for your distribution.

## Option 1 — Add the Project Notes repository (recommended)

This registers our update server, so new releases arrive through the normal
`flatpak update` mechanism.

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

## Option 2 — Single-file install

If you prefer a one-off download (for example on an offline machine), grab
`ProjectNotes.flatpak` from the
[latest release](https://github.com/kestermckinney/ProjectNotes/releases/latest)
and install it:

```bash
flatpak install ProjectNotes.flatpak
```

This bundle also registers the update remote, so you can still run
`flatpak update` afterward to get future releases.

## Uninstalling

```bash
flatpak uninstall com.projectnotespro.ProjectNotes
flatpak remote-delete projectnotes        # optional: remove the update remote
```

Your data (the Project Notes database and settings) lives in your home folder
and is **not** removed by uninstalling the app.

## Verifying the download

Releases are signed with our GPG key. The repository's public key is embedded in
the `projectnotes.flatpakrepo` file, so Flatpak verifies signatures on every
install and update automatically — no manual step required.
