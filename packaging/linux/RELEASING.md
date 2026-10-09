# Linux releases on GitLab

The GitLab pipeline builds a signed OSTree repository, publishes it at
`https://kestermckinney.gitlab.io/ProjectNotes/flatpak/`, and uploads durable
`ProjectNotes.flatpak` and `projectnotes.flatpakrepo` assets to the generic package
registry. The release links point to those packages, rather than expiring CI artifacts.

## One-time setup

1. Make release metadata and packages publicly readable. The updater uses the
   public GitLab API and never embeds a private access token in the application.
2. Enable GitLab Pages with public access. Disable **Use unique domain** in Pages
   settings so the stable URL above serves the repository without login or redirects.
   The `public/` output here contains only the distribution repository. If this
   project also hosts a website, combine its output with this job before deployment.
3. Protect `v*` tags. Add protected CI/CD variables:

   | Variable | Type | Contents |
   | --- | --- | --- |
   | `FLATPAK_GPG_PRIVATE_KEY` | File | ASCII-armored existing repository signing key |
   | `FLATPAK_GPG_KEY_ID` | Variable | Signing-key fingerprint |
   | `FLATPAK_GPG_PASSPHRASE` | Masked variable | Passphrase, if the key uses one |

   Reuse the key matching `packaging/linux/projectnotes.flatpakrepo`. Keep its
   private half out of Git. The build checks the exported public key before signing.
   Existing GitHub Actions secrets cannot be read back; import the original key
   from its backup. Do not silently generate a new key for existing installations.
4. Use a runner that supports Flatpak/bubblewrap sandbox builds with the KDE 6.9
   container. A self-managed Docker runner may require privileged mode.
   For private SqliteSyncPro sources, authorize this project in SqliteSyncPro’s
   CI job-token allowlist and configure a Git credential helper using the job
   token. This project has protected `GIT_CONFIG_COUNT`, `GIT_CONFIG_KEY_0`,
   `GIT_CONFIG_VALUE_0`, and `GIT_TERMINAL_PROMPT` variables for that purpose.

## Prepare 6.4.0

Update the desktop version and installer versions, review
`packaging/gitlab/RELEASE_NOTES.md`, commit and push, then create `v6.4.0` at the
reviewed commit. The pipeline verifies that the tag matches the desktop version.
The mobile application has its own release cycle and version.

GitLab has no GitHub-style draft release flag. Keep the release notes in Git, or
create an **upcoming release** with a future `released_at` date. The application
ignores upcoming releases. Build artifacts are available for review from the
`flatpak-release` job. The `prepare-release` job uploads assets and creates an upcoming release dated
2099-01-01 to keep it pending explicit publication. The manual `publish-release`
job turns
an upcoming release into a current release (preserving its notes), and deploys
Pages. Do not run it until the release is ready to become available to users.

This pipeline currently builds Linux only. Attach a
`ProjectNotes-6.4.0-Windows-x64-Setup.exe` and a macOS `.zip` asset with `macOS` in
its link name before publishing if Windows/macOS should receive this version.
The updater reads GitLab `assets.links` and uses `direct_asset_url` or `url`.

## Local signed build

```bash
export PN_REF=v6.4.0
export GPG_KEY_ID=YOUR_EXISTING_SIGNING_KEY_FINGERPRINT
export APP_REPO_URL=https://kestermckinney.gitlab.io/ProjectNotes/flatpak/
export RUNTIME_REPO_URL=https://flathub.org/repo/flathub.flatpakrepo
./packaging/linux/build-flatpak.sh repo
./packaging/linux/build-flatpak.sh bundle
```

Run `build` first if the build directory does not exist. Source builds fetch from
GitLab at pinned commits; unpushed edits are not included. `APP_REPO_URL` embeds
the application's update remote. `RUNTIME_REPO_URL` refers only to dependencies.
Signed bundles also embed the public signing key.

## Updates and migration

Users add the descriptor with `flatpak remote-add --if-not-exists projectnotes
https://kestermckinney.gitlab.io/ProjectNotes/flatpak/projectnotes.flatpakrepo`.
Desktop software managers can install updates automatically when enabled;
command-line users run `flatpak update`.

For the existing GitHub remote, run:

```bash
curl --fail --location --output projectnotes.gpg https://kestermckinney.gitlab.io/ProjectNotes/flatpak/projectnotes.gpg
flatpak remote-modify --gpg-import=projectnotes.gpg --url=https://kestermckinney.gitlab.io/ProjectNotes/flatpak/ projectnotes
flatpak update com.projectnotespro.ProjectNotes
```

The GitLab repository uses a new signing key. Existing installations must import
`projectnotes.gpg` using the migration command above.

Keep the existing app ID and `master` branch to preserve installed refs. Every
repository build includes a complete delta from an empty installation. This
pipeline starts a fresh repository, so clients can update without old server
objects but will download a full delta. A future incremental implementation can
restore the previous repository before export to reduce update bandwidth.

## Deploy a prepared repository while keeping the release upcoming

A maintainer can upload a signed repository as `repository.tar.gz` to the same
generic package/version as the bundle, containing `index.html` and `flatpak/`.
Trigger an API pipeline on main with `REPOSITORY_PACKAGE_VERSION=v6.4.0` and
`REPOSITORY_PACKAGE_SHA256` set to the archive’s SHA-256 digest. The
`deploy-prepared-repository` job verifies the archive checksum and publishes
Pages while leaving the release’s upcoming date unchanged. Future tag pipelines
use the normal signed build, preparation, and manual publication workflow.
