# ProjectNotes

[![License](https://img.shields.io/badge/License-GPL%203.0-blue?style=for-the-badge)](LICENSE)

[Project site](https://kestermckinney.github.io/ProjectNotes/) · [GitHub repository](https://github.com/kestermckinney/ProjectNotes) · [Latest release](https://github.com/kestermckinney/ProjectNotes/releases/latest)

![ProjectNotes demo](docs/img/demo.gif)

ProjectNotes is a project manager's workspace for notes, action items, project metrics, reporting, and workflow automation. It is designed for people who manage multiple projects and need one place to capture decisions, track follow-up, and keep project communication moving.

## Explore

- [Project site](https://kestermckinney.github.io/ProjectNotes/)
- [Documentation](https://www.projectnotespro.com/docs)
- [Report a bug](https://github.com/kestermckinney/ProjectNotes/issues/new?template=bug_report.yml)
- [Request an enhancement](https://github.com/kestermckinney/ProjectNotes/issues/new?template=enhancement_request.yml)
- [Ask a usage question](https://github.com/kestermckinney/ProjectNotes/issues/new?template=question.yml)
- [Contributing guide](CONTRIBUTING.md)

## What ProjectNotes Does

- Organizes meeting notes, action items, risks, issues, and change requests in one application.
- Helps project managers maintain project health visibility across multiple teams.
- Supports workflow automation with JavaScript plugins that add menus, dialogs, settings and background tasks.
- Exports project information to PDF and Excel-oriented outputs.
- Runs on Windows, macOS, and Linux, with an iPhone and iPad companion app.
- Backs up and synchronizes your data across all your devices with a [Project Notes Pro](https://www.projectnotespro.com) subscription.

## Install On Linux

Project Notes ships as a self-hosted Flatpak with automatic updates:

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak remote-add --if-not-exists projectnotes \
  https://kestermckinney.github.io/projectnotes-flatpak/projectnotes.flatpakrepo
flatpak install projectnotes com.projectnotespro.ProjectNotes
```

Prefer a single file? Download `ProjectNotes.flatpak` from the
[latest GitHub release](https://github.com/kestermckinney/ProjectNotes/releases/latest).
Full instructions: [Installing on Linux](docs/Introduction/InstallLinux.md).

## Updating An Existing Installation

The project moved from GitLab back to GitHub with version 6.5.2. Windows and
macOS builds from 6.4.0 through 6.5.1 check only GitLab, and 6.5.2 is published
there too, so they update to it and then check GitHub from then on. If a build
does not offer an update, download the latest one from
[GitHub releases](https://github.com/kestermckinney/ProjectNotes/releases).

A Linux installation added from `kestermckinney.gitlab.io` has to be pointed at
the GitHub repository, which uses a different signing key. Project Notes
reminds you at startup. The steps and key fingerprint are in
[Installing on Linux](docs/Introduction/InstallLinux.md#moving-an-existing-installation-to-github).

## Activity And Requests

- [Open issues](https://github.com/kestermckinney/ProjectNotes/issues)
- [Open bugs](https://github.com/kestermckinney/ProjectNotes/issues?q=is%3Aissue%20is%3Aopen%20%5BBug%5D)
- [Open enhancements](https://github.com/kestermckinney/ProjectNotes/issues?q=is%3Aissue%20is%3Aopen%20%5BEnhancement%5D)
- [Usage questions](https://github.com/kestermckinney/ProjectNotes/issues?q=is%3Aissue%20is%3Aopen%20%5BQuestion%5D)
- [Recent commits](https://github.com/kestermckinney/ProjectNotes/commits/main)
- [Releases and downloads](https://github.com/kestermckinney/ProjectNotes/releases)

## Contributing

Contributions are welcome in several forms:

- Bug reports with clear reproduction steps
- Enhancement requests with a real project-management use case
- Documentation fixes and clarification
- Focused pull requests with tests or screenshots when relevant

Please start with [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

## Building From Source

For build and setup details, see:

- [How to Contribute](https://github.com/kestermckinney/ProjectNotes/blob/main/CONTRIBUTING.md)
- [System Requirements](Systemrequirements.md)
- [Developer documentation](docs/index.md)

## Related projects

- [SqliteSyncPro](https://github.com/kestermckinney/SqliteSyncPro) — the Qt library used for ProjectNotes synchronization.
- [ScheduleIO](https://github.com/kestermckinney/ScheduleIO) — a Qt library for reading, writing, and calculating Microsoft Project schedules.

## License

Copyright (c) Paul McKinney.

Licensed under the [GPL v3.0](LICENSE).
