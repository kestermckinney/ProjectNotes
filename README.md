# ProjectNotes

[![Project site](https://img.shields.io/badge/Project%20Site-Live-2ecc71?style=for-the-badge)](https://kestermckinney.github.io/ProjectNotes/)
[![License](https://img.shields.io/github/license/kestermckinney/ProjectNotes?style=for-the-badge)](LICENSE)
[![Latest release](https://img.shields.io/github/v/release/kestermckinney/ProjectNotes?style=for-the-badge)](https://github.com/kestermckinney/ProjectNotes/releases)
[![Downloads](https://img.shields.io/github/downloads/kestermckinney/ProjectNotes/total?style=for-the-badge)](https://github.com/kestermckinney/ProjectNotes/releases)
[![Stars](https://img.shields.io/github/stars/kestermckinney/ProjectNotes?style=for-the-badge)](https://github.com/kestermckinney/ProjectNotes/stargazers)
[![Forks](https://img.shields.io/github/forks/kestermckinney/ProjectNotes?style=for-the-badge)](https://github.com/kestermckinney/ProjectNotes/network/members)
[![Open issues](https://img.shields.io/github/issues/kestermckinney/ProjectNotes?style=for-the-badge)](https://github.com/kestermckinney/ProjectNotes/issues)
[![Last commit](https://img.shields.io/github/last-commit/kestermckinney/ProjectNotes?style=for-the-badge)](https://github.com/kestermckinney/ProjectNotes/commits/main)

ProjectNotes is a project manager's workspace for notes, action items, project metrics, reporting, and workflow automation. It is designed for people who manage multiple projects and need one place to capture decisions, track follow-up, and keep project communication moving.

## Explore

- [Project site](https://kestermckinney.github.io/ProjectNotes/)
- [Documentation](https://projectnotes.readthedocs.io)
- [Report a bug](https://github.com/kestermckinney/ProjectNotes/issues/new?template=bug_report.yml)
- [Request an enhancement](https://github.com/kestermckinney/ProjectNotes/issues/new?template=enhancement_request.yml)
- [Ask a usage question](https://github.com/kestermckinney/ProjectNotes/issues/new?template=question.yml)
- [Contributing guide](CONTRIBUTING.md)

## What ProjectNotes Does

- Organizes meeting notes, action items, risks, issues, and change requests in one application.
- Helps project managers maintain project health visibility across multiple teams.
- Supports workflow automation with Python plugins and background tasks.
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
[latest release](https://github.com/kestermckinney/ProjectNotes/releases/latest).
Full instructions: [Installing on Linux](docs/Introduction/InstallLinux.md).

## Activity And Request Snapshot

[![Open bugs](https://img.shields.io/github/issues-search?query=repo%3Akestermckinney%2FProjectNotes+is%3Aissue+is%3Aopen+%22%5BBug%5D%3A%22+in%3Atitle&label=open%20bugs&style=flat-square)](https://github.com/kestermckinney/ProjectNotes/issues?q=is%3Aissue%20is%3Aopen%20%22%5BBug%5D%3A%22%20in%3Atitle)
[![Open enhancements](https://img.shields.io/github/issues-search?query=repo%3Akestermckinney%2FProjectNotes+is%3Aissue+is%3Aopen+%22%5BEnhancement%5D%3A%22+in%3Atitle&label=open%20enhancements&style=flat-square)](https://github.com/kestermckinney/ProjectNotes/issues?q=is%3Aissue%20is%3Aopen%20%22%5BEnhancement%5D%3A%22%20in%3Atitle)
[![Open questions](https://img.shields.io/github/issues-search?query=repo%3Akestermckinney%2FProjectNotes+is%3Aissue+is%3Aopen+%22%5BQuestion%5D%3A%22+in%3Atitle&label=open%20questions&style=flat-square)](https://github.com/kestermckinney/ProjectNotes/issues?q=is%3Aissue%20is%3Aopen%20%22%5BQuestion%5D%3A%22%20in%3Atitle)

These badges update automatically from GitHub so visitors can quickly see activity, open requests, and whether the project is actively maintained.

## Contributing

Contributions are welcome in several forms:

- Bug reports with clear reproduction steps
- Enhancement requests with a real project-management use case
- Documentation fixes and clarification
- Focused pull requests with tests or screenshots when relevant

Please start with [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

## Building From Source

For build and setup details, see:

- [How to Contribute](https://github.com/kestermckinney/ProjectNotes/wiki/How-to-Contribute)
- [System Requirements](Systemrequirements.md)
- [Developer documentation](docs/index.md)

## License

Copyright (c) Paul McKinney.

Licensed under the [GPL v3.0](LICENSE).
