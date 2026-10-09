#!/usr/bin/env bash
set -euo pipefail
: "${FLATPAK_GPG_PRIVATE_KEY:?Set the protected GitLab file variable containing the signing key}"
: "${FLATPAK_GPG_KEY_ID:?Set the signing key fingerprint}"
: "${CI_COMMIT_SHA:?}"
: "${CI_COMMIT_TAG:?}"
: "${FLATPAK_REPO_URL:?}"

# Signing material lives only in an ephemeral keyring, never in artifacts.
export GNUPGHOME
GNUPGHOME="$(mktemp -d)"
chmod 700 "$GNUPGHOME"
trap 'gpgconf --kill gpg-agent || true; rm -rf "$GNUPGHOME"' EXIT
printf '%s\n' allow-loopback-pinentry allow-preset-passphrase \
  'default-cache-ttl 21600' 'max-cache-ttl 21600' > "$GNUPGHOME/gpg-agent.conf"
gpg --batch --import "$FLATPAK_GPG_PRIVATE_KEY"
if [ -n "${FLATPAK_GPG_PASSPHRASE:-}" ]; then
  preset="$(gpgconf --list-dirs libexecdir)/gpg-preset-passphrase"
  while read -r grip; do
    printf '%s' "$FLATPAK_GPG_PASSPHRASE" | "$preset" --preset "$grip"
  done < <(gpg --batch --with-colons --with-keygrip --list-secret-keys "$FLATPAK_GPG_KEY_ID" | awk -F: '$1 == "grp" { print $10 }')
fi

# Reject accidental key rotation: installed users trust the published public key.
python3 - <<'PY'
import base64, configparser, os, subprocess
repo = configparser.ConfigParser(interpolation=None)
repo.read('packaging/linux/projectnotes.flatpakrepo')
expected = base64.b64decode(repo['Flatpak Repo']['GPGKey'])
actual = subprocess.check_output(['gpg', '--export', os.environ['FLATPAK_GPG_KEY_ID']])
if expected != actual:
    raise SystemExit('Signing key differs from projectnotes.flatpakrepo; review key migration before publishing')
PY

version="$(sed -n 's/.*project(ProjectNotes VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
[ "$CI_COMMIT_TAG" = "v$version" ] || { echo 'Tag does not match application version'; exit 1; }
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install -y --noninteractive flathub org.kde.Platform//6.9 org.kde.Sdk//6.9 io.qt.qtwebengine.BaseApp//6.9
mkdir -p /var/lib/dbus
dbus-uuidgen --ensure=/etc/machine-id
export GPG_KEY_ID="$FLATPAK_GPG_KEY_ID" GPG_HOMEDIR="$GNUPGHOME"
export APP_REPO_URL="$FLATPAK_REPO_URL" PN_REF="$CI_COMMIT_SHA"
# Export performs the build; do not compile everything twice.
# cmd_repo needs the build directory to exist before invoking flatpak-builder.
mkdir -p packaging/linux/flatpak-build
bash packaging/linux/build-flatpak.sh repo
bash packaging/linux/build-flatpak.sh verify-webengine
flatpak build-update-repo --gpg-sign="$GPG_KEY_ID" --gpg-homedir="$GPG_HOMEDIR" \
  --generate-static-deltas packaging/linux/flatpak-repo
bash packaging/linux/build-flatpak.sh bundle
mkdir -p public/flatpak
cp -a packaging/linux/flatpak-repo/. public/flatpak/
cp packaging/linux/projectnotes.flatpakrepo packaging/linux/projectnotes.gpg public/flatpak/
cp packaging/linux/icons/256x256/com.projectnotespro.ProjectNotes.png public/flatpak/icon.png
# Flatpak clients must not follow Pages' unique-domain redirects for every object.
printf '%s\n' 'Project Notes Linux update repository' > public/index.html
