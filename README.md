# ProjectNotes

[![License](https://img.shields.io/badge/License-GPL%203.0-blue?style=for-the-badge)](LICENSE)

[Project site](https://kestermckinney.gitlab.io/ProjectNotes/) · [GitLab repository](https://gitlab.com/kestermckinney/ProjectNotes) · [Latest release](https://gitlab.com/kestermckinney/ProjectNotes/-/releases/permalink/latest)

![ProjectNotes demo](docs/img/demo.gif)

ProjectNotes is a project manager's workspace for notes, action items, project metrics, reporting, and workflow automation. It is designed for people who manage multiple projects and need one place to capture decisions, track follow-up, and keep project communication moving.

## Explore

- [Project site](https://kestermckinney.gitlab.io/ProjectNotes/)
- [Documentation](https://www.projectnotespro.com/docs)
- [Planning and issue board](https://gitlab.com/kestermckinney/ProjectNotes/-/boards)
- [Report a bug](https://gitlab.com/kestermckinney/ProjectNotes/-/issues/new?issuable_template=Bug)
- [Request an enhancement](https://gitlab.com/kestermckinney/ProjectNotes/-/issues/new?issuable_template=Enhancement)
- [Ask a usage question](https://gitlab.com/kestermckinney/ProjectNotes/-/issues/new?issuable_template=Question)
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
  https://kestermckinney.gitlab.io/ProjectNotes/flatpak/projectnotes.flatpakrepo
flatpak install projectnotes com.projectnotespro.ProjectNotes
```

Prefer a single file? Download `ProjectNotes.flatpak` from the
[latest GitLab release](https://gitlab.com/kestermckinney/ProjectNotes/-/releases/permalink/latest).
For an existing installation using the former GitHub-hosted remote, see the
migration instructions below.
Full instructions: [Installing on Linux](docs/Introduction/InstallLinux.md).

## Updating An Existing Installation

Older desktop builds check the former GitHub repository, which is unavailable.
Download and install a new Windows or macOS build from [GitLab releases](https://gitlab.com/kestermckinney/ProjectNotes/-/releases)
when one is available. Builds containing the GitLab updater will check GitLab for
subsequent updates.

For Linux installations using the former update server, keep the existing remote
and import the GitLab signing key, change its URL, then update:

```bash
curl --fail --location --output projectnotes.gpg https://kestermckinney.gitlab.io/ProjectNotes/flatpak/projectnotes.gpg
flatpak remote-modify --gpg-verify --gpg-import=projectnotes.gpg --url=https://kestermckinney.gitlab.io/ProjectNotes/flatpak/ projectnotes
flatpak update com.projectnotespro.ProjectNotes
```

See [Installing on Linux](docs/Introduction/InstallLinux.md) for the key fingerprint
and instructions for user-specific installations.

## Activity And Requests

- [Open issues](https://gitlab.com/kestermckinney/ProjectNotes/-/issues?state=opened)
- [Open bugs](https://gitlab.com/kestermckinney/ProjectNotes/-/issues?state=opened&search=%5BBug%5D)
- [Open enhancements](https://gitlab.com/kestermckinney/ProjectNotes/-/issues?state=opened&search=%5BEnhancement%5D)
- [Usage questions](https://gitlab.com/kestermckinney/ProjectNotes/-/issues?state=opened&search=%5BQuestion%5D)
- [Recent commits](https://gitlab.com/kestermckinney/ProjectNotes/-/commits/main)
- [Releases and downloads](https://gitlab.com/kestermckinney/ProjectNotes/-/releases)

## Contributing

Contributions are welcome in several forms:

- Bug reports with clear reproduction steps
- Enhancement requests with a real project-management use case
- Documentation fixes and clarification
- Focused merge requests with tests or screenshots when relevant

Please start with [CONTRIBUTING.md](CONTRIBUTING.md) before opening a merge request.

## Building From Source

For build and setup details, see:

- [How to Contribute](https://gitlab.com/kestermckinney/ProjectNotes/-/blob/main/CONTRIBUTING.md)
- [System Requirements](Systemrequirements.md)
- [Developer documentation](docs/index.md)

## Related projects

- [SqliteSyncPro](https://gitlab.com/kestermckinney/SqliteSyncPro) — the Qt library used for ProjectNotes synchronization.
- [ScheduleIO](https://gitlab.com/kestermckinney/ScheduleIO) — a Qt library for reading, writing, and calculating Microsoft Project schedules.

## License

Copyright (c) Paul McKinney.

Licensed under the [GPL v3.0](LICENSE).
