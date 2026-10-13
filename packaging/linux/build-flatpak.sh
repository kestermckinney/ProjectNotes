#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/flatpak-build"
# REPO_DIR / BUNDLE_FILE are overridable via env so CI can export into a clone of
# the already-published ostree repo (preserving update history / deltas).
REPO_DIR="${REPO_DIR:-$SCRIPT_DIR/flatpak-repo}"
BUNDLE_FILE="${BUNDLE_FILE:-$SCRIPT_DIR/ProjectNotes.flatpak}"
APP_ID="com.projectnotespro.ProjectNotes"
MANIFEST="$SCRIPT_DIR/${APP_ID}.yml"
RUNTIME_VERSION="6.9"

# ── Optional GPG signing / self-hosting knobs (all opt-in via env) ───────────
# GPG_KEY_ID       If set, `repo` and `bundle` are signed with this key.
# GPG_HOMEDIR      Optional GnuPG home holding that key (used by CI's ephemeral
#                  keyring); passed through to flatpak as --gpg-homedir.
# APP_REPO_URL     Application update repository embedded via --repo-url.
# RUNTIME_REPO_URL Runtime dependency remote (.flatpakrepo), normally Flathub.
GPG_KEY_ID="${GPG_KEY_ID:-}"
GPG_HOMEDIR="${GPG_HOMEDIR:-}"
APP_REPO_URL="${APP_REPO_URL:-}"
RUNTIME_REPO_URL="${RUNTIME_REPO_URL:-https://flathub.org/repo/flathub.flatpakrepo}"

# DISABLE_ROFILES_FUSE  Set to 1 in containerized CI where rofiles-fuse can't
#                       spawn (FUSE unavailable). Harmless to leave unset locally.
DISABLE_ROFILES_FUSE="${DISABLE_ROFILES_FUSE:-0}"

# Shared flatpak-builder flags. rofiles-fuse mounts the build cache read-only
# mid-build via FUSE, which fails inside CI containers; let CI opt out.
BUILDER_FLAGS=()
[ "$DISABLE_ROFILES_FUSE" = "1" ] && BUILDER_FLAGS+=("--disable-rofiles-fuse")

# USE_BUILDER_FLATPAK  auto (default): build with the org.flatpak.Builder flatpak
#                      when it's installed, else the host flatpak-builder. 1/0
#                      force one or the other. The host flatpak 1.18.2 on
#                      Fedora 44 fails `build-init --base` with
#                      "lsetxattr(security.selinux): Operation not supported":
#                      it copies each BaseApp file's label via /proc/self/fd with
#                      lgetxattr, so it reads the /proc symlink's own label and
#                      can't write it back. The Builder flatpak's copy is unaffected.
USE_BUILDER_FLATPAK="${USE_BUILDER_FLATPAK:-auto}"
FLATPAK_BUILDER=(flatpak-builder)
if [ "$USE_BUILDER_FLATPAK" = "1" ] || \
   { [ "$USE_BUILDER_FLATPAK" = "auto" ] && flatpak info org.flatpak.Builder &>/dev/null; }; then
    FLATPAK_BUILDER=(flatpak run --command=flatpak-builder org.flatpak.Builder)
    # The Builder sandbox has no /dev/fuse, so rofiles-fuse can't mount.
    [ "$DISABLE_ROFILES_FUSE" = "1" ] || BUILDER_FLAGS+=("--disable-rofiles-fuse")
fi

# ── Source pins ──────────────────────────────────────────────────────────────
# PN_REF  Which ProjectNotes source build/install/repo compile:
#           latest  (default) the newest v* tag on GitLab
#           pinned  the `commit:` values committed in the manifest, untouched
#           <tag|branch|sha>  that ref, e.g. v6.1.0, main, or a full commit SHA
#         Unless pinned, SqliteSyncPro follows its default-branch tip, matching
#         the release workflow. The resolved pins go into a generated copy of
#         the manifest; the committed manifest is never edited.
PN_REF="${PN_REF:-latest}"
PN_GIT_URL="https://gitlab.com/kestermckinney/ProjectNotes.git"
SS_GIT_URL="https://gitlab.com/kestermckinney/SqliteSyncPro.git"
# Must sit next to the real manifest so its relative module includes resolve.
RESOLVED_MANIFEST="$SCRIPT_DIR/.${APP_ID}.resolved.yml"

# Print the commit SHA that <ref> names on remote <url>. Annotated tags are
# peeled to the commit they point at.
remote_sha() {
    local url="$1" ref="$2" out
    if [[ "$ref" =~ ^[0-9a-f]{40}$ ]]; then
        echo "$ref"
        return
    fi
    out="$(git ls-remote "$url" "refs/tags/$ref^{}" "refs/tags/$ref" "refs/heads/$ref" "$ref")"
    # Prefer the peeled tag line, then whatever matched first.
    { grep -F "refs/tags/$ref^{}" <<<"$out" || head -n1 <<<"$out"; } | awk 'NR==1 {print $1}'
}

# Set MANIFEST to the manifest build/install/repo should use, per PN_REF.
resolve_manifest() {
    [ "$PN_REF" = "pinned" ] && { echo "==> Using committed manifest pins (PN_REF=pinned)"; return; }

    local pn_ref="$PN_REF" pn_sha ss_sha
    if [ "$pn_ref" = "latest" ]; then
        pn_ref="$(git ls-remote --tags "$PN_GIT_URL" 'v*' | awk '{print $2}' \
            | sed -e 's#^refs/tags/##' -e 's#\^{}$##' | sort -uV | tail -n1)"
        [ -n "$pn_ref" ] || { echo "Error: no v* tags found on $PN_GIT_URL"; exit 1; }
    fi
    pn_sha="$(remote_sha "$PN_GIT_URL" "$pn_ref")"
    ss_sha="$(remote_sha "$SS_GIT_URL" HEAD)"
    [[ "$pn_sha" =~ ^[0-9a-f]{40}$ ]] || { echo "Error: could not resolve ProjectNotes ref '$pn_ref'"; exit 1; }
    [[ "$ss_sha" =~ ^[0-9a-f]{40}$ ]] || { echo "Error: could not resolve SqliteSyncPro HEAD"; exit 1; }

    # Same rewrite as the release workflow's "Pin sources" step.
    awk -v pn="$pn_sha" -v ss="$ss_sha" '
        /url: .*kestermckinney\/ProjectNotes\.git/  { inpn = 1 }
        inpn && /^[[:space:]]*commit:/ { sub(/commit:.*/, "commit: " pn); inpn = 0 }
        /url: .*kestermckinney\/SqliteSyncPro\.git/ { inss = 1 }
        inss && /^[[:space:]]*commit:/ { sub(/commit:.*/, "commit: " ss); inss = 0 }
        { print }
    ' "$MANIFEST" > "$RESOLVED_MANIFEST"
    MANIFEST="$RESOLVED_MANIFEST"
    echo "==> ProjectNotes  -> $pn_ref ($pn_sha)"
    echo "==> SqliteSyncPro -> HEAD ($ss_sha)"
}

# Build the shared --gpg-sign/--gpg-homedir argument list into the named array.
# Usage: gpg_sign_args MYARRAY ; then expand "${MYARRAY[@]}".
gpg_sign_args() {
    local -n _out="$1"
    _out=()
    if [ -n "$GPG_KEY_ID" ]; then
        _out+=("--gpg-sign=$GPG_KEY_ID")
        [ -n "$GPG_HOMEDIR" ] && _out+=("--gpg-homedir=$GPG_HOMEDIR")
    fi
}
# Official Flathub linter, shipped as part of the org.flatpak.Builder flatpak.
LINTER_APP="org.flatpak.Builder"
METAINFO_FILE="$SCRIPT_DIR/${APP_ID}.metainfo.xml"
DESKTOP_FILE="$SCRIPT_DIR/${APP_ID}.desktop"

usage() {
    echo "Usage: $0 [setup|build|lint|verify-webengine|run|install|repo|bundle|clean]"
    echo ""
    echo "  NOTE: build/install/repo clone ProjectNotes + SqliteSyncPro from GitLab."
    echo "        By default they build the newest v* tag (with SqliteSyncPro HEAD)."
    echo "        Override with PN_REF=<tag|branch|sha>, or PN_REF=pinned to use the"
    echo "        manifest's committed 'commit:' values. Push before building."
    echo ""
    echo "  setup      Install flatpak-builder, KDE runtime/SDK, and lint tooling"
    echo "  build      Build the Flatpak (runs setup check first)"
    echo "  lint       Run Flathub-style validation (manifest, metainfo, desktop)"
    echo "             Add --strict (or STRICT=1) to fail on any issue."
    echo "  verify-webengine  Verify native Qt WebEngine files in a completed build"
    echo "  run        Test-run the built Flatpak"
    echo "  install    Install the Flatpak for the current user"
    echo "  repo       Export to a local Flatpak repository (lint runs first)"
    echo "  bundle     Create a standalone .flatpak bundle file"
    echo "  clean      Remove build and repo directories"
    exit 1
}

cmd_setup() {
    echo "==> Checking for flatpak-builder..."
    if ! command -v flatpak-builder &>/dev/null; then
        echo "    Installing flatpak-builder..."
        sudo dnf install -y flatpak-builder
    else
        echo "    flatpak-builder is installed."
    fi

    echo "==> Ensuring Flathub remote exists..."
    flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

    echo "==> Installing KDE runtime and SDK ${RUNTIME_VERSION}..."
    flatpak install -y --noninteractive flathub org.kde.Platform//${RUNTIME_VERSION} org.kde.Sdk//${RUNTIME_VERSION} || true

    echo "==> Installing Qt WebEngine BaseApp ${RUNTIME_VERSION} (PDF-export plugins)..."
    flatpak install -y --noninteractive flathub io.qt.qtwebengine.BaseApp//${RUNTIME_VERSION} || true

    echo "==> Installing Flathub lint tooling (${LINTER_APP} + appstream + desktop-file-utils)..."
    flatpak install -y --noninteractive flathub "$LINTER_APP" || true
    if ! command -v appstreamcli &>/dev/null || ! command -v desktop-file-validate &>/dev/null; then
        sudo dnf install -y appstream desktop-file-utils || true
    fi

    echo "==> Setup complete."
}

# Flathub-style validation gate. Mirrors the checks Flathub CI runs:
#   1. flatpak-builder-lint on the manifest (finish-args / permissions / structure)
#   2. appstreamcli validate on the AppStream metainfo
#   3. desktop-file-validate on the .desktop entry
# Non-fatal by default (reports issues but continues). Pass --strict, or set
# STRICT=1, to make any failure abort. Missing tools are skipped with a hint
# rather than treated as failures, so this never blocks on a bare machine.
cmd_lint() {
    local strict=0
    [ "${1:-}" = "--strict" ] && strict=1
    [ "${STRICT:-0}" = "1" ] && strict=1

    local failures=0
    echo "==> Linting (Flathub-style validation)..."

    # 1. Manifest + finish-args lint via the official Flathub linter.
    if flatpak info "$LINTER_APP" &>/dev/null; then
        echo "--> flatpak-builder-lint: manifest"
        if ! flatpak run --command=flatpak-builder-lint "$LINTER_APP" manifest "$MANIFEST"; then
            echo "    FAIL: manifest lint"
            failures=$((failures + 1))
        fi
    else
        echo "--> SKIP manifest lint (${LINTER_APP} not installed; run '$0 setup')"
    fi

    # 2. AppStream metainfo validation.
    if command -v appstreamcli &>/dev/null; then
        echo "--> appstreamcli validate: metainfo"
        if ! appstreamcli validate --no-net "$METAINFO_FILE"; then
            echo "    FAIL: metainfo validation"
            failures=$((failures + 1))
        fi
    else
        echo "--> SKIP metainfo validation (appstreamcli not found; dnf install appstream)"
    fi

    # 3. Desktop entry validation.
    if command -v desktop-file-validate &>/dev/null; then
        echo "--> desktop-file-validate"
        if ! desktop-file-validate "$DESKTOP_FILE"; then
            echo "    FAIL: desktop file validation"
            failures=$((failures + 1))
        fi
    else
        echo "--> SKIP desktop validation (desktop-file-validate not found; dnf install desktop-file-utils)"
    fi

    if [ "$failures" -gt 0 ]; then
        echo "==> Lint finished with ${failures} failing check(s)."
        if [ "$strict" = "1" ]; then
            echo "    --strict set: aborting."
            return 1
        fi
        echo "    (non-fatal; pass --strict or set STRICT=1 to enforce)"
    else
        echo "==> Lint passed."
    fi
    return 0
}

cmd_build() {
    echo "==> Building Flatpak..."
    echo "    (clones ProjectNotes + SqliteSyncPro from GitLab at pinned commits;"
    echo "     ensure those commits are pushed to origin first)"
    resolve_manifest
    "${FLATPAK_BUILDER[@]}" "${BUILDER_FLAGS[@]}" --force-clean "$BUILD_DIR" "$MANIFEST"
    cmd_verify_webengine
    echo "==> Build complete. Output in $BUILD_DIR"
}

# The QML executable uses Qt WebEngine directly. Keep this check inside the
# Flatpak build root so a retained PyQt wheel cannot satisfy it accidentally.
cmd_verify_webengine() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Error: Build directory not found. Run '$0 build' first."
        return 1
    fi
    echo "==> Verifying native Qt WebEngine payload..."
    flatpak build "$BUILD_DIR" sh -c '
        set -eu
        test -x /app/lib/libexec/QtWebEngineProcess
        test -f /app/resources/qtwebengine_resources.pak
        test -f /app/resources/qtwebengine_resources_100p.pak
        test -f /app/resources/v8_context_snapshot.bin
        test -f /app/translations/qtwebengine_locales/en-US.pak
    '
    echo "==> Native Qt WebEngine payload verified."
}

cmd_run() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Error: Build directory not found. Run '$0 build' first."
        exit 1
    fi
    echo "==> Running $APP_ID..."
    # flatpak-builder --run mounts the Wayland socket but, unlike `flatpak run`,
    # leaves WAYLAND_DISPLAY unset; fallback-x11 also hides DISPLAY, so Qt
    # would fall back to xcb and abort. Pass the host's value through.
    if [ -n "${WAYLAND_DISPLAY:-}" ]; then
        flatpak-builder --run "$BUILD_DIR" "$MANIFEST" \
            env WAYLAND_DISPLAY="$WAYLAND_DISPLAY" projectnotes
    else
        flatpak-builder --run "$BUILD_DIR" "$MANIFEST" projectnotes
    fi
}

cmd_install() {
    echo "==> Installing Flatpak for current user..."
    resolve_manifest
    "${FLATPAK_BUILDER[@]}" "${BUILDER_FLAGS[@]}" --user --install --force-clean "$BUILD_DIR" "$MANIFEST"
    echo "==> Installed. Run with: flatpak run $APP_ID"
}

cmd_repo() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Error: Build directory not found. Run '$0 build' first."
        exit 1
    fi
    # Gate the publishable artifact on the Flathub-style lint. Honors STRICT=1.
    cmd_lint
    resolve_manifest
    local sign_args
    gpg_sign_args sign_args
    if [ -n "$GPG_KEY_ID" ]; then
        echo "==> Exporting to repository at $REPO_DIR (signed with $GPG_KEY_ID)..."
    else
        echo "==> Exporting to repository at $REPO_DIR (unsigned; set GPG_KEY_ID to sign)..."
    fi
    "${FLATPAK_BUILDER[@]}" "${BUILDER_FLAGS[@]}" --repo="$REPO_DIR" "${sign_args[@]}" --force-clean "$BUILD_DIR" "$MANIFEST"
    echo "==> Repository created at $REPO_DIR"
}

cmd_bundle() {
    if [ ! -d "$REPO_DIR" ]; then
        echo "Error: Repository not found. Run '$0 repo' first."
        exit 1
    fi
    # OSTree commits are signed at export; build-bundle --gpg-sign is for OCI.
    local extra=()
    # Embed the update remote so installing the bundle also wires up `flatpak update`.
    [ -n "$APP_REPO_URL" ] && extra+=("--repo-url=$APP_REPO_URL")
    [ -n "$GPG_KEY_ID" ] && {
        local key_file
        key_file="$(mktemp)"
        gpg ${GPG_HOMEDIR:+--homedir "$GPG_HOMEDIR"} --export "$GPG_KEY_ID" > "$key_file"
        extra+=("--gpg-keys=$key_file")
    }
    [ -n "$RUNTIME_REPO_URL" ] && extra+=("--runtime-repo=$RUNTIME_REPO_URL")
    echo "==> Creating bundle at $BUNDLE_FILE..."
    flatpak build-bundle "${extra[@]}" "$REPO_DIR" "$BUNDLE_FILE" "$APP_ID"
    [ -z "${key_file:-}" ] || rm -f "$key_file"
    echo "==> Bundle created: $BUNDLE_FILE"
    echo "    Install with: flatpak install $BUNDLE_FILE"
}

cmd_clean() {
    echo "==> Cleaning build artifacts..."
    rm -rf "$BUILD_DIR" "$REPO_DIR"
    rm -f "$BUNDLE_FILE"
    echo "==> Clean complete."
}

case "${1:-}" in
    setup)     cmd_setup ;;
    build)     cmd_build ;;
    lint)      cmd_lint "${2:-}" ;;
    verify-webengine) cmd_verify_webengine ;;
    run)       cmd_run ;;
    install)   cmd_install ;;
    repo)      cmd_repo ;;
    bundle)    cmd_bundle ;;
    clean)     cmd_clean ;;
    *)         usage ;;
esac
