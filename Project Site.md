# Project Site

- [GitHub Pages](https://kestermckinney.github.io/ProjectNotes/)
- [GitHub repository](https://github.com/kestermckinney/ProjectNotes)
- [README](README.md)
- [Contributing](CONTRIBUTING.md)
- [System requirements](Systemrequirements.md)
- [Documentation](https://www.projectnotespro.com/docs)

The public website sources are in `site/`. `.github/workflows/pages.yml`
deploys them to GitHub Pages when `site/` changes on `main`. Online
documentation is hosted at `https://www.projectnotespro.com/docs` by
PN-AccountManager; `.github/workflows/docs-deploy.yml` triggers its rebuild.

The Flatpak repository has its own GitHub project,
`kestermckinney/projectnotes-flatpak`, served at
`https://kestermckinney.github.io/projectnotes-flatpak/`, so a website
deployment can never remove the app's update server.

GitLab (`gitlab.com/kestermckinney/ProjectNotes`) stays public while installed
6.4.0 through 6.5.1 builds, which check only GitLab, update. Its Pages site
still serves the GitLab Flatpak repository at
`https://kestermckinney.gitlab.io/ProjectNotes/flatpak/`. Leave both up until
those users have moved.
