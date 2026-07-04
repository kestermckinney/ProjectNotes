#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# build_installer.sh
# Builds a signed, notarized macOS installer (.pkg wrapped in .dmg) containing
# ProjectNotes.app, plus a self-contained ProjectNotes-<ver>-macOS.zip consumed
# by the in-app auto-updater (UpdateManager on macOS).
#
# Usage:
#   ./build_installer.sh [options]
#
# Options:
#   --sign        Code-sign both app bundles (requires SIGN_IDENTITY)
#   --notarize    Submit the final .pkg for Apple notarization (implies --sign)
#   --dmg         Wrap the signed .pkg in a distributable .dmg
#   --all         Equivalent to --sign --notarize --dmg
#
# Configuration (override via environment variables or edit the defaults below):
#   SIGN_IDENTITY            Developer ID Application certificate CN
#   INSTALLER_IDENTITY       Developer ID Installer certificate CN
#   TEAM_ID                  10-character Apple Team ID
#   APPLE_ID                 Apple ID email for notarytool
#   NOTARIZE_KEYCHAIN_PROFILE Keychain profile name created with:
#                              xcrun notarytool store-credentials <profile>
#   PN_BUILD_DIR             Path to the CMake Release build dir for ProjectNotes
#
# Prerequisites:
#   - Xcode Command Line Tools (codesign, pkgbuild, productbuild, hdiutil, xcrun)
#   - Both apps must be built in Release before running this script.

set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────
# Distribution requires the Developer ID *Application* cert (for the .app and its
# nested binaries) and the Developer ID *Installer* cert (for the .pkg wrapper),
# both under Apple Team 2PY624KHXH. Do NOT use an "Apple Development" identity —
# Apple rejects it during notarization. Override any of these via the environment.
TEAM_ID="${TEAM_ID:-2PY624KHXH}"
SIGN_IDENTITY="${SIGN_IDENTITY:-Developer ID Application: Paul McKinney (2PY624KHXH)}"
INSTALLER_IDENTITY="${INSTALLER_IDENTITY:-Developer ID Installer: Paul McKinney (2PY624KHXH)}"
APPLE_ID="${APPLE_ID:-paul.mckinney@me.com}"
NOTARIZE_KEYCHAIN_PROFILE="${NOTARIZE_KEYCHAIN_PROFILE:-ProjectNotes-Notarize}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# Adjust these to match your Qt Creator build directory names
PN_BUILD_DIR="${PN_BUILD_DIR:-${PROJECT_ROOT}/build/Qt_6_10_2_for_macOS-Release}"

PN_APP="${PN_BUILD_DIR}/Project Notes.app"

PN_VERSION="5.2.3"
INSTALLER_VERSION="${PN_VERSION}"

STAGING_DIR="${SCRIPT_DIR}/staging"
OUTPUT_DIR="${SCRIPT_DIR}/output"
RESOURCES_DIR="${SCRIPT_DIR}/resources"

# ── Argument parsing ───────────────────────────────────────────────────────────

DO_SIGN=false
DO_NOTARIZE=false
DO_DMG=false

for arg in "$@"; do
    case "$arg" in
        --sign)      DO_SIGN=true ;;
        --notarize)  DO_SIGN=true; DO_NOTARIZE=true ;;
        --dmg)       DO_DMG=true ;;
        --all)       DO_SIGN=true; DO_NOTARIZE=true; DO_DMG=true ;;
        *)           echo "Unknown option: $arg"; exit 1 ;;
    esac
done

# ── Helpers ────────────────────────────────────────────────────────────────────

log()  { echo "[$(date '+%H:%M:%S')] $*"; }
die()  { echo "ERROR: $*" >&2; exit 1; }

require_app() {
    [[ -d "$1" ]] || die "App bundle not found: $1
Build the project in Release first, or set PN_BUILD_DIR / SA_BUILD_DIR."
}

# Deep-sign an app bundle with hardened runtime.
# All nested dylibs/frameworks/extensions must be signed before the outer bundle.
sign_bundle() {
    local bundle="$1"
    local entitlements="${2:-}"

    log "Signing: ${bundle}"

    # Strip build-system artifacts from nested frameworks.  Static archives
    # (.a), object files (.o), pkg-config files (.pc), and shell config scripts
    # (.sh) cannot be code-signed and cause notarization / codesign --strict to
    # reject the bundle (e.g. Python's config-*/python.o).  These files are
    # development-only and not needed at runtime.
    find "${bundle}/Contents" \
        \( -name "*.a" -o -name "*.o" -o -name "*.pc" \
        -o -name "tclConfig.sh" -o -name "tkConfig.sh" \
        -o -name "tclooConfig.sh" -o -name "tkooConfig.sh" \) \
        -delete 2>/dev/null || true

    # ── Pass 1: sign every Mach-O binary directly, deepest path first ──────────
    # We must sign the actual binaries, not just their enclosing bundles. The
    # PyQt5 Qt5 frameworks ship from PyPI ad-hoc signed AND with a non-standard
    # layout (no Versions/Current symlink), so signing the .framework bundle
    # re-seals its CodeResources but leaves the inner Mach-O ad-hoc — which is
    # exactly what notarization rejects ("not signed with a valid Developer ID
    # certificate" / "no secure timestamp" / "hardened runtime not enabled").
    # Signing the versioned binary directly fixes that; the bundle is re-sealed
    # over it in Pass 2.
    #
    # Detect Mach-O files by content (not extension) so nothing is missed:
    # loadable modules (.so/.dylib), framework main binaries (no extension), and
    # bare executables such as Python.framework's bin/python3.13. A single
    # batched `file` scan over the bundle is cheap. printf+cut (not awk) sorts by
    # path length without splitting on spaces, so deepest paths sign first.
    # `file` column-pads filenames with spaces before the description, so match
    # "Mach-O" anywhere and strip the ":<spaces>Mach-O…" suffix to recover the
    # path. Drop the universal-binary "(for architecture …)" sub-lines so each
    # binary is signed once.
    find "${bundle}/Contents" -type f -print0 2>/dev/null | \
    xargs -0 file 2>/dev/null | \
    grep "Mach-O" | grep -v "(for architecture" | \
    sed -E 's/:[[:space:]]+Mach-O.*$//' | \
    while IFS= read -r item; do printf '%d\t%s\n' "${#item}" "$item"; done | \
    sort -rn | cut -f2- | \
    while IFS= read -r item; do
        codesign --force --sign "${SIGN_IDENTITY}" \
            --options runtime \
            --timestamp \
            "${item}" 2>&1 | grep -v "replacing existing signature" || true
    done

    # ── Pass 2: sign nested bundles depth-first to (re)seal their contents ─────
    # Deepest path first ensures an inner bundle is sealed before the bundle that
    # encloses it (e.g. QtWebEngineProcess.app before QtWebEngineCore.framework,
    # and all nested frameworks before Python.framework). Frameworks get their
    # CodeResources re-sealed over the Developer ID binaries signed in Pass 1;
    # .app helpers also get their own Mach-O executable signed here. Entitlements
    # are applied only to .app bundles (executables); they are meaningless on a
    # plain framework.
    find "${bundle}/Contents" \
         \( -name "*.framework" -o -name "*.app" \) 2>/dev/null | \
    while IFS= read -r item; do printf '%d\t%s\n' "${#item}" "$item"; done | \
    sort -rn | cut -f2- | \
    while IFS= read -r item; do
        if [[ "${item}" == *.app && -n "${entitlements}" ]]; then
            codesign --force --sign "${SIGN_IDENTITY}" \
                --options runtime \
                --timestamp \
                --entitlements "${entitlements}" \
                "${item}" 2>&1 | grep -v "replacing existing signature" || true
        else
            codesign --force --sign "${SIGN_IDENTITY}" \
                --options runtime \
                --timestamp \
                "${item}" 2>&1 | grep -v "replacing existing signature" || true
        fi
    done

    # Sign the bundle itself
    local extra_args=()
    [[ -n "${entitlements}" ]] && extra_args+=(--entitlements "${entitlements}")

    codesign --force --verify --verbose \
        --sign "${SIGN_IDENTITY}" \
        --options runtime \
        --timestamp \
        ${extra_args[@]+"${extra_args[@]}"} \
        "${bundle}"

    codesign --verify --deep --strict --verbose=2 "${bundle}"
    log "Signed successfully: $(basename "${bundle}")"
}

# ── Step 1: Validate inputs ────────────────────────────────────────────────────

log "=== Project Notes macOS Installer Build ==="
log "Project Notes: ${PN_APP}"

require_app "${PN_APP}"

# ── Step 2: Prepare staging area ──────────────────────────────────────────────

log "Preparing staging area..."
# Clear immutable flags then delete. Fall back to find -delete if rm -rf
# is blocked by memory-mapped files (e.g. Python.framework dylibs in use).
for _dir in "${STAGING_DIR}" "${OUTPUT_DIR}"; do
    if [[ -d "${_dir}" ]]; then
        chflags -R nouchg "${_dir}" 2>/dev/null || true
        chmod -R u+w  "${_dir}" 2>/dev/null || true
        rm -rf "${_dir}" 2>/dev/null || \
            find "${_dir}" -depth -exec rm -rf {} + 2>/dev/null || true
        [[ -d "${_dir}" ]] && die "Could not remove ${_dir} — close any apps using it and retry."
    fi
done
mkdir -p \
    "${STAGING_DIR}/projectnotes" \
    "${OUTPUT_DIR}"

# Copy app bundle into staging root.  Use ditto (not cp -R) so that bundle
# structure, resource forks, and extended attributes are preserved correctly
# without creating spurious ._* AppleDouble files that break code signing.
log "Copying app bundle..."
ditto "${PN_APP}" "${STAGING_DIR}/projectnotes/Project Notes.app"

PN_STAGED="${STAGING_DIR}/projectnotes/Project Notes.app"
PN_RESOURCES="${PN_STAGED}/Contents/Resources"

# Second copy for the in-app auto-updater asset.  Signed/notarized/zipped in
# Step 8.
UPDATER_APP="${STAGING_DIR}/updater/Project Notes.app"
mkdir -p "${STAGING_DIR}/updater"
ditto "${PN_APP}" "${UPDATER_APP}"

# ── IFS plugin removal ──────────────────────────────────────────────────────────
# The IFS Cloud plugins are excluded from the macOS distribution. Shipping them
# as a separate optional installer component is incompatible with a notarized,
# signed app: the component's payload is an unsigned partial .app (which fails
# notarization) and installing it into the already-signed bundle would
# invalidate the Developer ID signature on the user's machine. Strip the files
# from both the installer bundle and the auto-updater bundle so neither carries
# them.

IFS_PLUGIN_FILES=(
    "plugins/ifs_ssrs_generate_plugin.py"
    "plugins/ifscloud_plugin_settings.py"
    "plugins/includes/ifs_tools.py"
    "threads/ifssync_thread.py"
)

log "Removing IFS plugin files from macOS bundles..."
for rel in "${IFS_PLUGIN_FILES[@]}"; do
    for base in "${PN_RESOURCES}" "${UPDATER_APP}/Contents/Resources"; do
        rm -f "${base}/${rel}"
        # Also drop any compiled bytecode so the module cannot be imported.
        mod="$(basename "${rel}" .py)"
        rm -f "$(dirname "${base}/${rel}")/__pycache__/${mod}".*.pyc
    done
done

# ── Step 3: Code signing ──────────────────────────────────────────────────────

if [[ "${DO_SIGN}" == true ]]; then
    log "=== Code Signing ==="
    sign_bundle "${PN_STAGED}" "${SCRIPT_DIR}/ProjectNotes.entitlements"
    sign_bundle "${UPDATER_APP}" "${SCRIPT_DIR}/ProjectNotes.entitlements"
else
    log "Skipping code signing (pass --sign to enable)"
fi

# ── Step 4: Build component packages ──────────────────────────────────────────

log "=== Building Component Packages ==="

pkgbuild \
    --root "${STAGING_DIR}/projectnotes" \
    --component-plist "${SCRIPT_DIR}/ProjectNotes-component.plist" \
    --identifier "com.projectnotespro.ProjectNotes" \
    --version "${PN_VERSION}" \
    --install-location "/Applications" \
    "${OUTPUT_DIR}/ProjectNotes.pkg"
log "Built: ProjectNotes.pkg"

# ── Step 5: Build distribution package ────────────────────────────────────────

log "=== Building Distribution Package ==="

DIST_PKG="${OUTPUT_DIR}/ProjectNotes-${INSTALLER_VERSION}-macOS.pkg"

DIST_XML_TMP="${OUTPUT_DIR}/distribution.xml"
sed "s/@VERSION@/${INSTALLER_VERSION}/g" "${SCRIPT_DIR}/distribution.xml" > "${DIST_XML_TMP}"

INSTALLER_SIGN_ARGS=()
if [[ "${DO_SIGN}" == true ]]; then
    INSTALLER_SIGN_ARGS+=(--sign "${INSTALLER_IDENTITY}")
fi

productbuild \
    --distribution "${DIST_XML_TMP}" \
    --package-path "${OUTPUT_DIR}" \
    --resources "${RESOURCES_DIR}" \
    ${INSTALLER_SIGN_ARGS[@]+"${INSTALLER_SIGN_ARGS[@]}"} \
    "${DIST_PKG}"

log "Built: ${DIST_PKG}"

# ── Step 6: Notarization ──────────────────────────────────────────────────────

if [[ "${DO_NOTARIZE}" == true ]]; then
    log "=== Notarizing ==="
    log "Submitting ${DIST_PKG} to Apple notarization service..."

    xcrun notarytool submit "${DIST_PKG}" \
        --apple-id   "${APPLE_ID}" \
        --team-id    "${TEAM_ID}" \
        --keychain-profile "${NOTARIZE_KEYCHAIN_PROFILE}" \
        --wait

    log "Stapling notarization ticket..."
    xcrun stapler staple "${DIST_PKG}"
    xcrun stapler validate "${DIST_PKG}"
    log "Notarization complete."
else
    log "Skipping notarization (pass --notarize to enable)"
fi

# ── Step 7: DMG wrapping ──────────────────────────────────────────────────────

if [[ "${DO_DMG}" == true ]]; then
    log "=== Creating DMG ==="

    DMG_STAGING="${STAGING_DIR}/dmg"
    mkdir -p "${DMG_STAGING}"
    cp "${DIST_PKG}" "${DMG_STAGING}/"

    DMG_PATH="${OUTPUT_DIR}/ProjectNotes-${INSTALLER_VERSION}-macOS.dmg"

    hdiutil create \
        -volname "ProjectNotes ${PN_VERSION}" \
        -srcfolder "${DMG_STAGING}" \
        -ov \
        -format UDZO \
        "${DMG_PATH}"

    if [[ "${DO_SIGN}" == true ]]; then
        codesign --sign "${SIGN_IDENTITY}" --timestamp "${DMG_PATH}"
    fi

    if [[ "${DO_NOTARIZE}" == true ]]; then
        log "Notarizing DMG..."
        xcrun notarytool submit "${DMG_PATH}" \
            --apple-id   "${APPLE_ID}" \
            --team-id    "${TEAM_ID}" \
            --keychain-profile "${NOTARIZE_KEYCHAIN_PROFILE}" \
            --wait
        xcrun stapler staple "${DMG_PATH}"
        log "DMG notarized."
    fi

    log "Built: ${DMG_PATH}"
fi

# ── Step 8: Auto-updater zip ───────────────────────────────────────────────────
# A zip of the self-contained .app consumed by the in-app updater
# (UpdateManager::launchInstaller on macOS). selectPlatformAsset matches a
# release asset whose name contains "macOS" and ends in ".zip".

UPDATE_ZIP="${OUTPUT_DIR}/ProjectNotes-${INSTALLER_VERSION}-macOS.zip"

log "=== Building auto-updater zip ==="
# --keepParent so the archive expands to "Project Notes.app", not its contents.
ditto -c -k --keepParent "${UPDATER_APP}" "${UPDATE_ZIP}"

if [[ "${DO_NOTARIZE}" == true ]]; then
    log "Notarizing updater app..."
    xcrun notarytool submit "${UPDATE_ZIP}" \
        --apple-id   "${APPLE_ID}" \
        --team-id    "${TEAM_ID}" \
        --keychain-profile "${NOTARIZE_KEYCHAIN_PROFILE}" \
        --wait

    # Staple the ticket onto the .app, then re-zip so the shipped archive carries it.
    xcrun stapler staple "${UPDATER_APP}"
    xcrun stapler validate "${UPDATER_APP}"
    rm -f "${UPDATE_ZIP}"
    ditto -c -k --keepParent "${UPDATER_APP}" "${UPDATE_ZIP}"
    log "Updater app notarized and stapled."
else
    log "Skipping updater notarization (pass --notarize to enable)"
fi

log "Built: ${UPDATE_ZIP}"

# ── Done ──────────────────────────────────────────────────────────────────────

log "=== Build complete ==="
log "Output directory: ${OUTPUT_DIR}"
ls -lh "${OUTPUT_DIR}"
