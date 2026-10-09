# Releasing the Linux Flatpak (maintainer guide)

Flatpak publishing will move to the dedicated GitLab Pages project
`kestermckinney/projectnotes-flatpak`, with repository root [https://kestermckinney.gitlab.io/projectnotes-flatpak/](https://kestermckinney.gitlab.io/projectnotes-flatpak/).
The checked-in `projectnotes.flatpakrepo` targets that future deployment.

The working update server is currently [https://kestermckinney.gitlab.io/ProjectNotes/flatpak/](https://kestermckinney.gitlab.io/ProjectNotes/flatpak/).
Keep user installation instructions pointing there until the dedicated project
has been created, deployed, and verified. Bundles and the public descriptor are
available on [GitLab releases](https://gitlab.com/kestermckinney/ProjectNotes/-/releases).
Do not publish the changed descriptor before its target is available.

Release automation is maintained separately from this migration. The retained
`.github/workflows/flatpak-release.yml` is legacy configuration and cannot run
on GitLab. Do not follow its GitHub account, token, or Pages setup instructions.

## Source and version preparation

1. Commit and push the application and SqliteSyncPro changes to GitLab. The
   Flatpak builder fetches remote source, not your working tree.
2. Add the release version and date to
   `com.projectnotespro.ProjectNotes.metainfo.xml` and verify the CMake version.
3. Build and verify the intended version before tagging. `PN_REF=main` builds
   the current default branch; `PN_REF=<tag|branch|sha>` selects another ref.
   SqliteSyncPro follows its default-branch tip unless `PN_REF=pinned` is used.
4. Commit and push any metadata changes, then tag the intended release commit:

   ```bash
   git tag vX.Y.Z
   git push origin vX.Y.Z
   ```

`build-flatpak.sh` resolves the selected commits into a git-ignored generated
manifest. Direct manifest builds and `PN_REF=pinned` use the committed pins;
ensure those commits are available from their GitLab repositories.

## Signing and local verification

Use the currently published GitLab signing key, with primary fingerprint
`E0C07E21F0E41F83CBDCCB04F82B4E4D2183B011`. It differs from the former GitHub repository
key; existing clients must import the new public key during migration.
The public half is committed as `GPGKey=` in `projectnotes.flatpakrepo`; the
private half belongs in the maintainer's local keyring or a protected GitLab
CI/CD file variable named `FLATPAK_GPG_PRIVATE_KEY`. Release automation also
uses `FLATPAK_GPG_KEY_ID` and, when required, `FLATPAK_GPG_PASSPHRASE`.
Do not commit private keys or passphrases.

From `packaging/linux`, build and export a signed repository and bundle:

```bash
export PN_REF=vX.Y.Z
export GPG_KEY_ID="E0C07E21F0E41F83CBDCCB04F82B4E4D2183B011"
export APP_REPO_URL="https://kestermckinney.gitlab.io/projectnotes-flatpak/"
export RUNTIME_REPO_URL="https://flathub.org/repo/flathub.flatpakrepo"
./build-flatpak.sh build
./build-flatpak.sh repo
./build-flatpak.sh bundle
```

Set `GPG_HOMEDIR` if the signing key is in a separate keyring. Omit the signing
variables for unsigned local testing. Use `./build-flatpak.sh run` to launch the
built app and `./build-flatpak.sh lint --strict` to validate its metadata.

## Publication requirements

Create `kestermckinney/projectnotes-flatpak` as a public GitLab project and
configure GitLab Pages to serve the signed repository from its root. Disable
**Use unique domain** to retain the planned address, or update the descriptor
and all public installation links to the actual Pages URL. The separate
publisher is release-pipeline work and is not implemented here.

The publisher must preserve existing OSTree objects/update history and the
current GitLab signing key, expose the repository at the dedicated Pages root, and copy the
repository descriptor and icon alongside it. Its public descriptor must use:

- `Url=https://kestermckinney.gitlab.io/projectnotes-flatpak/`
- `Icon=https://kestermckinney.gitlab.io/projectnotes-flatpak/icon.png`

Publish `ProjectNotes.flatpak` and `projectnotes.flatpakrepo` as durable GitLab
release asset links. Keep any existing release description when adding Linux
installation instructions. Release publication is not implemented by the
static website sources in `site/`.

## Verify the dedicated deployment

After the dedicated project is deployed, verify it on a clean machine. Only
then update README and Linux installation instructions to use the new address:

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak remote-add --if-not-exists projectnotes \
  https://kestermckinney.gitlab.io/projectnotes-flatpak/projectnotes.flatpakrepo
flatpak install projectnotes com.projectnotespro.ProjectNotes
```

Check that installation verifies the signature and the app reports the intended
version. Also test an existing installation after moving its remote:

```bash
curl --fail --location --output projectnotes.gpg https://kestermckinney.gitlab.io/projectnotes-flatpak/projectnotes.gpg
# Verify the primary fingerprint above before importing the public key.
gpg --show-keys --with-fingerprint projectnotes.gpg
flatpak remote-modify --gpg-verify --gpg-import=projectnotes.gpg --url=https://kestermckinney.gitlab.io/projectnotes-flatpak/ projectnotes
flatpak update com.projectnotespro.ProjectNotes
```

Retain the current `https://kestermckinney.gitlab.io/ProjectNotes/flatpak/` deployment during the move so existing clients
can continue updating. Existing GitLab clients need only a URL change if they
already trust the published GitLab signing key. GitHub-era clients also need
the public-key import above. Re-export bundles with the dedicated repository
URL before attaching them to releases; do not simply relabel old bundles.
