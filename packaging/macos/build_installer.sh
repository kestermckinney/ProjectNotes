#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# build_installer.sh
# Builds a signed, notarized macOS installer (.pkg wrapped in .dmg) containing
# ProjectNotes.app and ProjectNotesRemoteHost.app.
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
#   SA_BUILD_DIR             Path to the CMake Release build dir for ProjectNotesRemoteHost
#
# Prerequisites:
#   - Xcode Command Line Tools (codesign, pkgbuild, productbuild, hdiutil, xcrun)
#   - Both apps must be built in Release before running this script.

set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────
TEAM_ID="UWCKDYP67Y"
SIGN_IDENTITY="11477C487BBFAF86C840CFF198A5F7007BD4927D"
SIGN_IDENTITY="${SIGN_IDENTITY:-Developer ID Application: Paul McKinney (UWCKDYP67Y)}"
INSTALLER_IDENTITY="${INSTALLER_IDENTITY:-Developer ID Installer: Paul McKinney (2PY624KHXH)}"
TEAM_ID="${TEAM_ID:-UWCKDYP67Y}"
APPLE_ID="${APPLE_ID:-paul.mckinney@me.com}"
NOTARIZE_KEYCHAIN_PROFILE="${NOTARIZE_KEYCHAIN_PROFILE:-ProjectNotes-Notarize}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
SQLADMIN_ROOT="$(cd "${PROJECT_ROOT}/../SqliteSyncPro" && pwd)"

# Adjust these to match your Qt Creator build directory names
PN_BUILD_DIR="${PN_BUILD_DIR:-${PROJECT_ROOT}/build/Qt_6_10_2_for_macOS-Release}"
SA_BUILD_DIR="${SA_BUILD_DIR:-${SQLADMIN_ROOT}/build/Qt_6_10_2_for_macOS-Release/admin}"

PN_APP="${PN_BUILD_DIR}/Project Notes.app"
SA_APP="${SA_BUILD_DIR}/Project Notes Remote Host.app"

PN_VERSION="5.0.0"
SA_VERSION="0.1.0"
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
    # (.a), pkg-config files (.pc), and shell config scripts (.sh) cannot be
    # code-signed and cause codesign --strict to reject the bundle.  These
    # files are development-only and not needed at runtime.
    find "${bundle}/Contents" \
        \( -name "*.a" -o -name "*.pc" \
        -o -name "tclConfig.sh" -o -name "tkConfig.sh" \
        -o -name "tclooConfig.sh" -o -name "tkooConfig.sh" \) \
        -delete 2>/dev/null || true

    # Sign all executable code depth-first across the entire bundle.
    # Use printf+cut (not awk) to sort by path length without splitting on spaces.
    # --options runtime must match the outer bundle; --timestamp is required for
    # Developer ID signatures.
    find "${bundle}/Contents" \
         \( -name "*.so" -o -name "*.dylib" \) 2>/dev/null | \
    while IFS= read -r item; do printf '%d\t%s\n' "${#item}" "$item"; done | \
    sort -rn | cut -f2- | \
    while IFS= read -r item; do
        codesign --force --verify --verbose \
            --sign "${SIGN_IDENTITY}" \
            --options runtime \
            --timestamp \
            "${item}" 2>&1 | grep -v "replacing existing signature" || true
    done

    # Sign nested frameworks depth-first (longest path first ensures inner
    # versioned binaries are signed before their enclosing bundle).
    find "${bundle}/Contents" \
         -name "*.framework" 2>/dev/null | \
    while IFS= read -r item; do printf '%d\t%s\n' "${#item}" "$item"; done | \
    sort -rn | cut -f2- | \
    while IFS= read -r item; do
        codesign --force --verify --verbose \
            --sign "${SIGN_IDENTITY}" \
            --options runtime \
            --timestamp \
            "${item}" 2>&1 | grep -v "replacing existing signature" || true
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
log "ProjectNotesRemoteHost: ${SA_APP}"

require_app "${PN_APP}"
require_app "${SA_APP}"

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
    "${STAGING_DIR}/sqladmin" \
    "${OUTPUT_DIR}"

# Copy app bundles into staging roots.  Use ditto (not cp -R) so that bundle
# structure, resource forks, and extended attributes are preserved correctly
# without creating spurious ._* AppleDouble files that break code signing.
log "Copying app bundles..."
ditto "${PN_APP}" "${STAGING_DIR}/projectnotes/Project Notes.app"
ditto "${SA_APP}" "${STAGING_DIR}/sqladmin/Project Notes Remote Host.app"

PN_STAGED="${STAGING_DIR}/projectnotes/Project Notes.app"
SA_STAGED="${STAGING_DIR}/sqladmin/Project Notes Remote Host.app"

# ── Step 3: Code signing ──────────────────────────────────────────────────────

if [[ "${DO_SIGN}" == true ]]; then
    log "=== Code Signing ==="
    sign_bundle "${PN_STAGED}" "${SCRIPT_DIR}/ProjectNotes.entitlements"
    sign_bundle "${SA_STAGED}"
else
    log "Skipping code signing (pass --sign to enable)"
fi

# ── Step 4: Build component packages ──────────────────────────────────────────

log "=== Building Component Packages ==="

pkgbuild \
    --root "${STAGING_DIR}/projectnotes" \
    --component-plist "${SCRIPT_DIR}/ProjectNotes-component.plist" \
    --identifier "com.kestermckinney.projectnotes" \
    --version "${PN_VERSION}" \
    --install-location "/Applications" \
    "${OUTPUT_DIR}/ProjectNotes.pkg"
log "Built: ProjectNotes.pkg"

pkgbuild \
    --root "${STAGING_DIR}/sqladmin" \
    --component-plist "${SCRIPT_DIR}/ProjectNotesRemoteHost-component.plist" \
    --identifier "com.sqlitesyncpro.sqladmin" \
    --version "${SA_VERSION}" \
    --install-location "/Applications" \
    "${OUTPUT_DIR}/ProjectNotesRemoteHost.pkg"
log "Built: ProjectNotesRemoteHost.pkg"

# ── Step 5: Build distribution package ────────────────────────────────────────

log "=== Building Distribution Package ==="

DIST_PKG="${OUTPUT_DIR}/ProjectNotes-${INSTALLER_VERSION}-macOS.pkg"

INSTALLER_SIGN_ARGS=()
if [[ "${DO_SIGN}" == true ]]; then
    INSTALLER_SIGN_ARGS+=(--sign "${INSTALLER_IDENTITY}")
fi

productbuild \
    --distribution "${SCRIPT_DIR}/distribution.xml" \
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

# ── Done ──────────────────────────────────────────────────────────────────────

log "=== Build complete ==="
log "Output directory: ${OUTPUT_DIR}"
ls -lh "${OUTPUT_DIR}"
