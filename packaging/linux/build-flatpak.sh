#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/flatpak-build"
REPO_DIR="$SCRIPT_DIR/flatpak-repo"
MANIFEST="$SCRIPT_DIR/com.kestermckinney.projectnotes.yml"
BUNDLE_FILE="$SCRIPT_DIR/ProjectNotes.flatpak"
APP_ID="com.kestermckinney.projectnotes"
RUNTIME_VERSION="6.9"

usage() {
    echo "Usage: $0 [setup|build|run|install|repo|bundle|regen-pip|clean]"
    echo ""
    echo "  setup      Install flatpak-builder and KDE runtime/SDK"
    echo "  build      Build the Flatpak (runs setup check first)"
    echo "  run        Test-run the built Flatpak"
    echo "  install    Install the Flatpak for the current user"
    echo "  repo       Export to a local Flatpak repository"
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

    echo "==> Setup complete."
}

cmd_build() {
    echo "==> Building Flatpak..."
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
    run)       cmd_run ;;
    install)   cmd_install ;;
    repo)      cmd_repo ;;
    bundle)    cmd_bundle ;;
    regen-pip) cmd_regen_pip ;;
    clean)     cmd_clean ;;
    *)         usage ;;
esac
