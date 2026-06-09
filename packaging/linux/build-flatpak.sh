#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/flatpak-build"
REPO_DIR="$SCRIPT_DIR/flatpak-repo"
BUNDLE_FILE="$SCRIPT_DIR/ProjectNotes.flatpak"
APP_ID="com.projectnotespro.ProjectNotes"
MANIFEST="$SCRIPT_DIR/${APP_ID}.yml"
RUNTIME_VERSION="6.9"
# Official Flathub linter, shipped as part of the org.flatpak.Builder flatpak.
LINTER_APP="org.flatpak.Builder"
METAINFO_FILE="$SCRIPT_DIR/${APP_ID}.metainfo.xml"
DESKTOP_FILE="$SCRIPT_DIR/${APP_ID}.desktop"

usage() {
    echo "Usage: $0 [setup|build|lint|run|install|repo|bundle|regen-pip|clean]"
    echo ""
    echo "  NOTE: the manifest now uses pinned git sources (Flathub-compatible),"
    echo "        so build/repo clone ProjectNotes + SqliteSyncPro from GitHub at"
    echo "        the pinned commits. A new release means: commit, push, then bump"
    echo "        the 'commit:' fields in the manifest before building."
    echo ""
    echo "  setup      Install flatpak-builder, KDE runtime/SDK, and lint tooling"
    echo "  build      Build the Flatpak (runs setup check first)"
    echo "  lint       Run Flathub-style validation (manifest, metainfo, desktop)"
    echo "             Add --strict (or STRICT=1) to fail on any issue."
    echo "  run        Test-run the built Flatpak"
    echo "  install    Install the Flatpak for the current user"
    echo "  repo       Export to a local Flatpak repository (lint runs first)"
    echo "  bundle     Create a standalone .flatpak bundle file"
    echo "  regen-pip  Regenerate Python dependency sources"
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
    echo "    (clones ProjectNotes + SqliteSyncPro from GitHub at pinned commits;"
    echo "     ensure those commits are pushed to origin first)"
    flatpak-builder --force-clean "$BUILD_DIR" "$MANIFEST"
    echo "==> Build complete. Output in $BUILD_DIR"
}

cmd_run() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Error: Build directory not found. Run '$0 build' first."
        exit 1
    fi
    echo "==> Running $APP_ID..."
    flatpak-builder --run "$BUILD_DIR" "$MANIFEST" projectnotes
}

cmd_install() {
    echo "==> Installing Flatpak for current user..."
    flatpak-builder --user --install --force-clean "$BUILD_DIR" "$MANIFEST"
    echo "==> Installed. Run with: flatpak run $APP_ID"
}

cmd_repo() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Error: Build directory not found. Run '$0 build' first."
        exit 1
    fi
    # Gate the publishable artifact on the Flathub-style lint. Honors STRICT=1.
    cmd_lint
    echo "==> Exporting to local repository at $REPO_DIR..."
    flatpak-builder --repo="$REPO_DIR" --force-clean "$BUILD_DIR" "$MANIFEST"
    echo "==> Repository created at $REPO_DIR"
}

cmd_bundle() {
    if [ ! -d "$REPO_DIR" ]; then
        echo "Error: Repository not found. Run '$0 repo' first."
        exit 1
    fi
    echo "==> Creating bundle at $BUNDLE_FILE..."
    flatpak build-bundle "$REPO_DIR" "$BUNDLE_FILE" "$APP_ID"
    echo "==> Bundle created: $BUNDLE_FILE"
    echo "    Install with: flatpak install $BUNDLE_FILE"
}

cmd_regen_pip() {
    echo "==> Regenerating Python dependency sources..."
    python3 -m flatpak_pip_generator requests msal requests_oauthlib urllib3 \
        --yaml --runtime="org.kde.Sdk//${RUNTIME_VERSION}" --output "$SCRIPT_DIR/python-deps"
    echo "    NOTE: Check python-deps.yaml for .tar.gz entries and replace with .whl URLs from PyPI"
    echo "==> Updated $SCRIPT_DIR/python-deps.yaml"
    echo ""
    echo "    PyQt6 / PyQt6-WebEngine sources are hand-pinned in python-pyqt6.yaml"
    echo "    and python-pyqt6-webengine.yaml (built from source, not generated here)."
    echo "    Versions must track the runtime Qt (currently Qt 6.9.x -> PyQt6 6.9.x)."
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
    run)       cmd_run ;;
    install)   cmd_install ;;
    repo)      cmd_repo ;;
    bundle)    cmd_bundle ;;
    regen-pip) cmd_regen_pip ;;
    clean)     cmd_clean ;;
    *)         usage ;;
esac
