# Project Site

- [GitLab Pages](https://kestermckinney.gitlab.io/ProjectNotes/)
- [GitLab repository](https://gitlab.com/kestermckinney/ProjectNotes)
- [README](README.md)
- [Contributing](CONTRIBUTING.md)
- [System requirements](Systemrequirements.md)
- [Documentation](https://projectnotes.readthedocs.io)

The public website sources are in `site/`. The GitLab Pages deployment also
hosts the signed Flatpak repository under `flatpak/`. Any website deployment
must preserve that directory and its existing OSTree objects/signatures.
Publishing only `site/` would remove the app's update server.

The obsolete GitHub Pages website workflow has been removed. GitLab release
and Pages publication configuration is maintained separately and is not changed
by this content migration. Read the Docs is already connected to GitLab.

Future Flatpak hosting is the dedicated GitLab project
`kestermckinney/projectnotes-flatpak` at `https://kestermckinney.gitlab.io/projectnotes-flatpak/`. The checked-in descriptor
targets that planned site. Keep the current `flatpak/` deployment available
until the new host is verified and installed clients have migrated.
