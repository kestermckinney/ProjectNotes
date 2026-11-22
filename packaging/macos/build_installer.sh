#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# build_installer.sh
# Builds a signed, notarized macOS installer (.pkg wrapped in .dmg) containing
# the QML desktop frontend's "Project Notes.app" (ProjectNotesDesktop target),
# plus a self-contained ProjectNotes-<ver>-macOS.zip consumed by the in-app
# auto-updater (UpdateManager on macOS). These are the direct-download assets
# published on GitHub; the Mac App Store build comes from build_appstore.sh.
#
# The app is built here, as a universal (Apple silicon and Intel) Release build
# that runs on macOS 13 or later. build_common.sh explains how.
#
# Usage:
#   ./build_installer.sh [options]
#
# Options:
#   --sign        Code-sign both app bundles (requires SIGN_IDENTITY)
#   --notarize    Submit the final .pkg for Apple notarization (implies --sign)
#   --dmg         Wrap the signed .pkg in a distributable .dmg
#   --all         Equivalent to --sign --notarize --dmg
#   --skip-build  Package the existing build in PN_BUILD_DIR as is. It must
#                 still pass the macOS 13 / universal checks.
#
# Configuration (override via environment variables or edit the defaults below):
#   SIGN_IDENTITY            Developer ID Application certificate CN
#   INSTALLER_IDENTITY       Developer ID Installer certificate CN
#   TEAM_ID                  10-character Apple Team ID
#   APPLE_ID                 Apple ID email for notarytool
#   NOTARIZE_KEYCHAIN_PROFILE Keychain profile name created with:
#                              xcrun notarytool store-credentials <profile>
#   NOTARY_API_KEY_PATH      App Store Connect API key (.p8). When set, notarytool
#                            uses it with NOTARY_API_KEY_ID and NOTARY_API_ISSUER
#                            instead of the keychain profile (CI does this).
#   PN_BUILD_DIR             CMake build directory (default build/release)
#   QT_MACOS                 Qt macOS kit (contains bin/qt-cmake)
#   VCPKG_ROOT               vcpkg checkout used for static Hunspell and OpenSSL
#
# Prerequisites:
#   - Xcode Command Line Tools (codesign, pkgbuild, productbuild, hdiutil, xcrun)
#   - The Qt macOS kit and a vcpkg checkout (see build_common.sh)

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

# notarytool credentials: an API key when one is given, else the keychain profile.
if [[ -n "${NOTARY_API_KEY_PATH:-}" ]]; then
    NOTARY_AUTH=(--key "${NOTARY_API_KEY_PATH}" --key-id "${NOTARY_API_KEY_ID:?}" --issuer "${NOTARY_API_ISSUER:?}")
else
    NOTARY_AUTH=(--apple-id "${APPLE_ID}" --team-id "${TEAM_ID}" --keychain-profile "${NOTARIZE_KEYCHAIN_PROFILE}")
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# shellcheck source=build_common.sh
source "${SCRIPT_DIR}/build_common.sh"

# Not the Qt Creator build directory: Qt Creator leaves the deployment target
# empty, and the result only runs on the build machine's macOS.
PN_BUILD_DIR="${PN_BUILD_DIR:-${PROJECT_ROOT}/build/release}"

PN_APP="${PN_BUILD_DIR}/ProjectNotesDesktop/Project Notes.app"
# The iOS app's bundle ID, shared with the App Store build. Must match
# PN_MACOS_BUNDLE_ID in the top-level CMakeLists.txt.
BUNDLE_ID="com.projectnotespro.projectnotesmobile"

INSTALLER_VERSION="${PN_VERSION}"

STAGING_DIR="${SCRIPT_DIR}/staging"
OUTPUT_DIR="${SCRIPT_DIR}/output"
RESOURCES_DIR="${SCRIPT_DIR}/resources"

# ── Argument parsing ───────────────────────────────────────────────────────────

DO_SIGN=false
DO_NOTARIZE=false
DO_DMG=false
DO_BUILD=true

for arg in "$@"; do
    case "$arg" in
        --sign)      DO_SIGN=true ;;
        --notarize)  DO_SIGN=true; DO_NOTARIZE=true ;;
        --dmg)       DO_DMG=true ;;
        --all)       DO_SIGN=true; DO_NOTARIZE=true; DO_DMG=true ;;
        --skip-build) DO_BUILD=false ;;
        *)           echo "Unknown option: $arg"; exit 1 ;;
    esac
done

# ── Helpers ────────────────────────────────────────────────────────────────────

require_app() {
    [[ -d "$1" ]] || die "App bundle not found: $1
Run without --skip-build, or set PN_BUILD_DIR."
    # The App Store flavor is sandboxed and leaves out the updater and the
    # Thunderbird helper; it must not ship as the direct download.
    [[ -e "$1/Contents/MacOS/ProjectNotesThunderbirdContacts" ]] || \
        die "$1 is the Mac App Store flavor. Build without -DPN_APP_STORE."
    local id
    id="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$1/Contents/Info.plist")"
    [[ "${id}" == "${BUNDLE_ID}" ]] || \
        die "$1 has bundle ID ${id}, expected ${BUNDLE_ID}. Run without --skip-build."
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
    # reject the bundle.  These files are development-only and not needed at
    # runtime.
    find "${bundle}/Contents" \
        \( -name "*.a" -o -name "*.o" -o -name "*.pc" \
        -o -name "tclConfig.sh" -o -name "tkConfig.sh" \
        -o -name "tclooConfig.sh" -o -name "tkooConfig.sh" \) \
        -delete 2>/dev/null || true

    # ── Pass 1: sign every Mach-O binary directly, deepest path first ──────────
    # We must sign the actual binaries, not just their enclosing bundles: a
    # framework with a non-standard layout (no Versions/Current symlink) gets
    # its CodeResources re-sealed while the inner Mach-O stays ad-hoc, which
    # notarization rejects ("not signed with a valid Developer ID certificate" /
    # "no secure timestamp" / "hardened runtime not enabled"). Signing the
    # binary directly fixes that; the bundle is re-sealed over it in Pass 2.
    #
    # Detect Mach-O files by content (not extension) so nothing is missed:
    # loadable modules (.so/.dylib), framework main binaries (no extension), and
    # bare helper executables. A single
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
    # encloses it. Frameworks get their
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

# ── Step 1: Build ─────────────────────────────────────────────────────────────

log "=== Project Notes ${PN_VERSION} macOS Installer Build ==="
log "Project Notes: ${PN_APP}"

if [[ "${DO_BUILD}" == true ]]; then
    pn_require_tools
    log "=== Building Hunspell and OpenSSL with vcpkg ==="
    pn_build_static_deps
    log "=== Building Project Notes (macOS ${MIN_MACOS}+, ${PN_ARCHS[*]}) ==="
    # PN_APP_STORE=OFF in case the build directory was ever configured with it.
    pn_configure_and_build "${PN_BUILD_DIR}" -DPN_APP_STORE=OFF -DPN_MACOS_BUNDLE_ID="${BUNDLE_ID}"
else
    log "Skipping build (--skip-build)"
fi

require_app "${PN_APP}"

# ── Step 2: Prepare staging area ──────────────────────────────────────────────

log "Preparing staging area..."
# Clear immutable flags then delete. Fall back to find -delete if rm -rf
# is blocked by memory-mapped files (e.g. dylibs in use).
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
pn_prune_bundle "${PN_STAGED}"
pn_check_bundle "${PN_STAGED}"

# Second copy for the in-app auto-updater asset.  Signed/notarized/zipped in
# Step 8.
UPDATER_APP="${STAGING_DIR}/updater/Project Notes.app"
mkdir -p "${STAGING_DIR}/updater"
ditto "${PN_APP}" "${UPDATER_APP}"
pn_prune_bundle "${UPDATER_APP}"

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

# The package identifier matches the bundle ID. Releases before 6.5.0 used
# com.projectnotespro.ProjectNotes for both, so macOS records this as a new
# package rather than an upgrade; the preinstall script removes the old app.
# distribution.xml names the same identifier.
pkgbuild \
    --root "${STAGING_DIR}/projectnotes" \
    --scripts "${SCRIPT_DIR}/projectnotes-scripts" \
    --component-plist "${SCRIPT_DIR}/ProjectNotes-component.plist" \
    --identifier "${BUNDLE_ID}" \
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
        "${NOTARY_AUTH[@]}" \
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
            "${NOTARY_AUTH[@]}" \
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
        "${NOTARY_AUTH[@]}" \
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
