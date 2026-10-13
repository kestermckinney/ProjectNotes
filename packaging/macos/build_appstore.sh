#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only
#
# build_appstore.sh
#
# Signs the Mac App Store flavor of Project Notes and packs it for App Store
# Connect, optionally uploading it.
#
# Build the app first with the App Store flavor:
#   cmake -S . -B build/appstore -DCMAKE_BUILD_TYPE=Release -DPN_APP_STORE=ON
#   cmake --build build/appstore
#
# Usage:
#   PN_BUILD_DIR=build/appstore PN_PROVISIONING_PROFILE=~/ProjectNotes_Mac_AppStore.provisionprofile \
#       packaging/macos/build_appstore.sh
#
# Environment:
#   PN_BUILD_DIR             CMake build dir holding ProjectNotesDesktop/Project Notes.app
#   PN_PROVISIONING_PROFILE  Mac App Store provisioning profile for com.projectnotespro.ProjectNotes
#   APP_IDENTITY             default "3rd Party Mac Developer Application: Paul McKinney (2PY624KHXH)"
#   INSTALLER_IDENTITY       default "3rd Party Mac Developer Installer: Paul McKinney (2PY624KHXH)"
#   ASC_API_KEY_ID, ASC_API_ISSUER
#                            App Store Connect API key; when both are set the
#                            package is uploaded (the .p8 must be in
#                            ~/.appstoreconnect/private_keys/)
#
# Unlike build_installer.sh there are no installer scripts (the App Store does
# not run them) and no updater archive (the App Store updates the app).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

PN_BUILD_DIR="${PN_BUILD_DIR:-${PROJECT_ROOT}/build/appstore}"
PN_APP="${PN_BUILD_DIR}/ProjectNotesDesktop/Project Notes.app"
PN_PROVISIONING_PROFILE="${PN_PROVISIONING_PROFILE:-}"
APP_IDENTITY="${APP_IDENTITY:-3rd Party Mac Developer Application: Paul McKinney (2PY624KHXH)}"
INSTALLER_IDENTITY="${INSTALLER_IDENTITY:-3rd Party Mac Developer Installer: Paul McKinney (2PY624KHXH)}"
ENTITLEMENTS="${SCRIPT_DIR}/ProjectNotes-AppStore.entitlements"
PN_VERSION="$(sed -n 's/.*project(ProjectNotes VERSION \([0-9.]*\).*/\1/p' "${PROJECT_ROOT}/CMakeLists.txt")"

STAGING_DIR="${SCRIPT_DIR}/staging-appstore"
OUTPUT_DIR="${SCRIPT_DIR}/output"
PKG="${OUTPUT_DIR}/ProjectNotes-${PN_VERSION}-AppStore.pkg"

log() { echo "[$(date '+%H:%M:%S')] $*"; }
die() { echo "ERROR: $*" >&2; exit 1; }

[[ -d "${PN_APP}" ]] || die "App bundle not found: ${PN_APP}
Build with -DPN_APP_STORE=ON first, or set PN_BUILD_DIR."
[[ -n "${PN_PROVISIONING_PROFILE}" && -f "${PN_PROVISIONING_PROFILE}" ]] || \
    die "Set PN_PROVISIONING_PROFILE to the Mac App Store provisioning profile."

# A standard build contains the Thunderbird helper executable, which the App
# Store flavor leaves out. Refuse a bundle built without the flavor.
if [[ -e "${PN_APP}/Contents/MacOS/ProjectNotesThunderbirdContacts" ]]; then
    die "${PN_APP} looks like a standard build (it contains the Thunderbird helper). Rebuild with -DPN_APP_STORE=ON."
fi

log "=== Project Notes Mac App Store package ${PN_VERSION} ==="
rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}" "${OUTPUT_DIR}"
STAGED="${STAGING_DIR}/Project Notes.app"
ditto "${PN_APP}" "${STAGED}"

# Windows-only plugins never belong in a Mac bundle.
rm -rf "${STAGED}/Contents/Resources/plugins/office-tools"
# Build-system artifacts cannot be signed and are not needed at runtime.
find "${STAGED}/Contents" \( -name "*.a" -o -name "*.o" -o -name "*.pc" -o -name "*.prl" \) -delete 2>/dev/null || true

cp "${PN_PROVISIONING_PROFILE}" "${STAGED}/Contents/embedded.provisionprofile"

# ── Sign: every nested Mach-O first (deepest path first), then frameworks and
#    plug-in bundles, then the app with the sandbox entitlements. Only the main
#    executable carries entitlements; Qt's frameworks and plug-ins are libraries.
log "Signing nested code..."
find "${STAGED}/Contents" -type f -print0 | xargs -0 file 2>/dev/null | \
    grep "Mach-O" | grep -v "(for architecture" | sed -E 's/:[[:space:]]+Mach-O.*$//' | \
    grep -v "/Contents/MacOS/" | \
    while IFS= read -r item; do printf '%d\t%s\n' "${#item}" "$item"; done | sort -rn | cut -f2- | \
    while IFS= read -r item; do
        codesign --force --sign "${APP_IDENTITY}" --timestamp "${item}"
    done
find "${STAGED}/Contents" \( -name "*.framework" -o -name "*.bundle" \) -prune -print | \
    while IFS= read -r item; do printf '%d\t%s\n' "${#item}" "$item"; done | sort -rn | cut -f2- | \
    while IFS= read -r item; do
        codesign --force --sign "${APP_IDENTITY}" --timestamp "${item}"
    done

log "Signing the app with the sandbox entitlements..."
codesign --force --sign "${APP_IDENTITY}" --timestamp \
    --entitlements "${ENTITLEMENTS}" "${STAGED}"
codesign --verify --deep --strict --verbose=2 "${STAGED}"
codesign -d --entitlements - "${STAGED}" | grep -q "com.apple.security.app-sandbox" || \
    die "The signed app does not carry the App Sandbox entitlement."

# ── Package ───────────────────────────────────────────────────────────────────
log "Building ${PKG}..."
rm -f "${PKG}"
productbuild --component "${STAGED}" /Applications \
    --sign "${INSTALLER_IDENTITY}" \
    "${PKG}"
log "Built: ${PKG}"

# ── Upload (optional) ─────────────────────────────────────────────────────────
if [[ -n "${ASC_API_KEY_ID:-}" && -n "${ASC_API_ISSUER:-}" ]]; then
    log "Uploading to App Store Connect..."
    xcrun altool --upload-app -f "${PKG}" -t macos \
        --apiKey "${ASC_API_KEY_ID}" --apiIssuer "${ASC_API_ISSUER}"
    log "Uploaded. Check App Store Connect > TestFlight for processing."
else
    log "Set ASC_API_KEY_ID and ASC_API_ISSUER to upload, or upload ${PKG} with Transporter."
fi
