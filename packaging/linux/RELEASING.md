# Releasing the Linux Flatpak (maintainer guide)

The Flatpak is **self-hosted**, not on Flathub. A signed ostree repository is
published to a dedicated GitHub Pages repo and a one-file bundle is attached to
each GitHub Release. The `.github/workflows/flatpak-release.yml` workflow does
the build/sign/publish on every `v*` tag; the steps below cover the one-time
setup and the per-release routine.

## One-time setup

### 1. Create the dedicated Pages repo

Create a new **public** GitHub repo `kestermckinney/projectnotes-flatpak`. It
holds nothing but the published ostree repo (the workflow force-pushes a
`gh-pages` branch into it). In its **Settings → Pages**, set the source to
*Deploy from a branch* → `gh-pages` / `/ (root)`.

> Why a separate repo? The product site already lives at
> `kestermckinney.github.io/ProjectNotes`. Keeping the repo separate means a
> release can never overwrite that site.

### 2. Generate the signing key

```bash
gpg --quick-gen-key "ProjectNotes Flatpak Signing" default default never
# Note the key ID printed (the long hex fingerprint).

# Public half -> goes into projectnotes.flatpakrepo (single line, no wrapping):
gpg --export "ProjectNotes Flatpak Signing" | base64 --wrap=0

# Private half -> goes into the GitHub secret below (keep this safe!):
gpg --armor --export-secret-keys "ProjectNotes Flatpak Signing"
```

Paste the base64 public key as the `GPGKey=` value in
`packaging/linux/projectnotes.flatpakrepo` and commit it.

### 3. Add repository secrets

In **ProjectNotes → Settings → Secrets and variables → Actions**:

| Secret | Value |
| --- | --- |
| `FLATPAK_GPG_PRIVATE_KEY` | The full ASCII-armored private key block from step 2. |
| `FLATPAK_GPG_KEY_ID` | The signing key's fingerprint/ID (or its uid string). |
| `FLATPAK_PAGES_DEPLOY_TOKEN` | A Personal Access Token with `repo` scope that can push to `projectnotes-flatpak`. |

## Per-release routine

1. Make sure all app changes are committed **and pushed** to the public remote —
   the manifest builds from pinned commits, so unpushed work won't be included.
2. Bump both `commit:` fields in `com.projectnotespro.ProjectNotes.yml`
   (ProjectNotes and SqliteSyncPro) to the new commits.
3. Add a `<release version="X.Y.Z" date="…"/>` entry to
   `com.projectnotespro.ProjectNotes.metainfo.xml`.
4. Commit and push those packaging changes.
5. Tag and push:
   ```bash
   git tag vX.Y.Z
   git push origin vX.Y.Z
   ```

The workflow then builds in the KDE 6.9 container, signs the repo and bundle,
publishes the ostree repo to Pages (preserving prior versions), and creates the
GitHub Release with the bundle attached.

## Local testing (optional)

`build-flatpak.sh` honors the same knobs as CI, so you can dry-run a signed
build locally:

```bash
cd packaging/linux
GPG_KEY_ID="ProjectNotes Flatpak Signing" \
RUNTIME_REPO_URL="https://kestermckinney.github.io/projectnotes-flatpak/projectnotes.flatpakrepo" \
  ./build-flatpak.sh build && ./build-flatpak.sh repo && ./build-flatpak.sh bundle
```

Omit the env vars for a plain unsigned local build (`./build-flatpak.sh build`
then `install`/`run`).

## First-release sanity check

After the first tag, on a clean machine (or a throwaway container):

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak remote-add --if-not-exists projectnotes \
  https://kestermckinney.github.io/projectnotes-flatpak/projectnotes.flatpakrepo
flatpak install projectnotes com.projectnotespro.ProjectNotes
```

If the install verifies the signature and launches, the pipeline is healthy.
