# Linux releases

Releases are published on GitHub. `.github/workflows/flatpak-release.yml`
builds a signed OSTree repository when a `v*` tag is pushed, publishes it to
the `gh-pages` branch of `kestermckinney/projectnotes-flatpak` (served at
`https://kestermckinney.github.io/projectnotes-flatpak/`), and attaches
`ProjectNotes.flatpak` and `projectnotes.flatpakrepo` to the GitHub release.

GitLab stays public during the move. Builds 6.4.0 through 6.5.1 check only
GitLab for updates, so 6.5.2 is released on both hosts; after that, release on
GitHub only. The GitLab pipeline (`.gitlab-ci.yml`, `packaging/gitlab/`) still
builds the GitLab-signed repository at
`https://kestermckinney.gitlab.io/ProjectNotes/flatpak/`. Its descriptor and
public key are `packaging/gitlab/projectnotes.flatpakrepo` and
`packaging/gitlab/projectnotes.gpg`. Remove that pipeline when GitLab is retired.

## Pushing to both hosts

While GitLab is kept, the local `origin` remote fetches from GitHub and pushes
to GitHub and GitLab:

```bash
git remote set-url origin https://github.com/kestermckinney/ProjectNotes.git
git remote set-url --add --push origin https://github.com/kestermckinney/ProjectNotes.git
git remote set-url --add --push origin https://gitlab.com/kestermckinney/ProjectNotes.git
```

Do the same in `../SqliteSyncPro`. Both pipelines clone ProjectNotes and
SqliteSyncPro from GitHub at pinned commits, so push before tagging.

## One-time setup on GitHub

1. Repository secrets (Settings > Secrets and variables > Actions):

   | Secret | Contents |
   | --- | --- |
   | `FLATPAK_GPG_PRIVATE_KEY` | ASCII-armored private signing key |
   | `FLATPAK_GPG_KEY_ID` | Its fingerprint, `FEC8AE6825C826AFD564C424BA3027E5F2617C22` |
   | `FLATPAK_GPG_PASSPHRASE` | Its passphrase |
   | `FLATPAK_PAGES_DEPLOY_TOKEN` | Token that can push to `kestermckinney/projectnotes-flatpak` |
   | `VERCEL_DOCS_DEPLOY_HOOK` | Deploy hook for projectnotespro.com (used by `docs-deploy.yml`) |

   The key must match `packaging/linux/projectnotes.flatpakrepo`; the workflow
   refuses to sign with any other. Installed users trust that key, so never
   replace it with a new one.
2. Create the `linux-release` environment (Settings > Environments) and add
   yourself as a required reviewer. The publish job waits for that approval.
3. Leave GitHub Pages on `kestermckinney/projectnotes-flatpak` serving the
   `gh-pages` branch.

## Prepare a release

Update the desktop and installer versions, review `packaging/RELEASE_NOTES.md`,
commit and push, then tag `vX.Y.Z` at the reviewed commit and push the tag. The
workflow checks that the tag matches the version in `CMakeLists.txt`.

1. The `build` job builds and signs the repository, starting from the
   published one so users download a delta.
2. Review the run, then approve the `publish` job. It refuses to run when a
   newer release is already published, deploys the repository and attaches the
   bundle. If the release does not exist yet, it is created as a **draft** from
   `packaging/RELEASE_NOTES.md`.
3. Attach `ProjectNotes-X.Y.Z-Windows-x64-Setup.exe`,
   `ProjectNotesRemoteHost-X.Y.Z-Windows-x64-Setup.exe` and the macOS
   `ProjectNotes-X.Y.Z-macOS.zip`, then publish the draft. The desktop updater
   ignores drafts, so nobody is offered the release until then.

For 6.5.2, also run the GitLab pipeline: `prepare-release` creates an upcoming
release, then attach the Windows and macOS installers there and run the manual
`publish-release` job.

The updater reads the latest GitHub release (`assets`, `browser_download_url`)
and the latest GitLab release (`assets.links`), and offers whichever has the
higher version, GitHub on a tie. An older release left on either host is ignored.

## Local signed build

```bash
export PN_REF=v6.5.2
export GPG_KEY_ID=FEC8AE6825C826AFD564C424BA3027E5F2617C22
export APP_REPO_URL=https://kestermckinney.github.io/projectnotes-flatpak/
./packaging/linux/build-flatpak.sh build
./packaging/linux/build-flatpak.sh repo
./packaging/linux/build-flatpak.sh bundle
```

Sources are cloned from GitHub at pinned commits, so unpushed edits are not
included. `PN_GIT_URL` and `SS_GIT_URL` override the clone URLs. `APP_REPO_URL`
is the update remote embedded in the bundle, and `RUNTIME_REPO_URL` (Flathub by
default) is where dependencies come from. Signed bundles also embed the public key.

## Keys

| Repository | Primary fingerprint |
| --- | --- |
| GitHub, `kestermckinney.github.io/projectnotes-flatpak` | `FEC8AE6825C826AFD564C424BA3027E5F2617C22` |
| GitLab, `kestermckinney.gitlab.io/ProjectNotes/flatpak` | `E0C07E21F0E41F83CBDCCB04F82B4E4D2183B011` |

Users move an installation from GitLab to GitHub with the steps in
`docs/Introduction/InstallLinux.md`. Keep the app ID and the `master` branch so
installed refs keep updating. Private-key, passphrase and revocation backups are
in the ignored `.release-secrets/` directory and must never enter Git.

## Known validation limits

The Flathub linter flags the home-folder permission, which this self-hosted app
needs for user documents and plugin workflows. It also reports KDE 6.9 as end
of life; move to a newer runtime before a later Linux release.
