#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only
#
# build_appstore.sh
#
# Builds, signs, packages and (optionally) uploads the Mac App Store flavor of
# Project Notes (cmake -DPN_APP_STORE=ON). It ships in the same App Store
# Connect record as the iOS app ("Project Notes Pro"), so it uses that app's
# App ID, com.projectnotespro.projectnotesmobile, under Apple Team 2PY624KHXH.
# The direct download (GitHub, build_installer.sh) uses the same bundle ID.
#
# Usage:
#   ./build_appstore.sh [BUILD_NUMBER] [options]
#
# Options:
#   --build-number <N>   CFBundleVersion (build number). App Store Connect rejects
#                        an upload that reuses a build number, so this must be
#                        unique and increasing. May also be given as the first
#                        bare argument. Defaults to a UTC timestamp.
#   --api-key <KEYID>    App Store Connect API Key ID    (enables upload)
#   --api-issuer <ID>    App Store Connect API Issuer ID (enables upload)
#   --validate-only      With an API key, validate the package but do not upload.
#   --skip-build         Package the existing build in PN_BUILD_DIR as is.
#   --no-user-plugins    Build with PN_APP_STORE_USER_PLUGINS=OFF (only if App
#                        Review objects to user-installed plugins).
#   -h, --help           Show this help.
#
# The API key/issuer can also come from ASC_API_KEY_ID / ASC_API_ISSUER. Without
# them the script stops after building the .pkg.
#
# Configuration (override via environment variables):
#   QT_MACOS                 Qt macOS kit (contains bin/qt-cmake)
#   VCPKG_ROOT               vcpkg checkout used for static Hunspell and OpenSSL
#   PN_BUILD_DIR             CMake build directory for the App Store flavor
#   TEAM_ID                  10-character Apple Team ID
#   APP_IDENTITY             App signing identity. Default: the first
#                            "Apple Distribution" or "3rd Party Mac Developer
#                            Application" identity for TEAM_ID in the keychain
#   INSTALLER_IDENTITY       Package signing identity. Default: the first
#                            "3rd Party Mac Developer Installer" or "Mac
#                            Installer Distribution" identity for TEAM_ID
#   PN_PROVISIONING_PROFILE  Mac App Store provisioning profile. Default: the
#                            newest matching one Xcode has downloaded
#
# Prerequisites: see packaging/macos/AppStoreReleasing.md.

set -euo pipefail

usage() { sed -n '/^# Usage:/,/^$/p' "$0" | sed 's/^# \{0,1\}//'; }

# ── Configuration ─────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# shellcheck source=build_common.sh
source "${SCRIPT_DIR}/build_common.sh"

PN_BUILD_DIR="${PN_BUILD_DIR:-${PROJECT_ROOT}/build/appstore}"
TEAM_ID="${TEAM_ID:-2PY624KHXH}"
APP_IDENTITY="${APP_IDENTITY:-}"
INSTALLER_IDENTITY="${INSTALLER_IDENTITY:-}"
PN_PROVISIONING_PROFILE="${PN_PROVISIONING_PROFILE:-}"
ASC_API_KEY_ID="${ASC_API_KEY_ID:-}"
ASC_API_ISSUER="${ASC_API_ISSUER:-}"

# Must match PN_MACOS_BUNDLE_ID in the top-level CMakeLists.txt.
BUNDLE_ID="com.projectnotespro.projectnotesmobile"
PN_APP="${PN_BUILD_DIR}/ProjectNotesDesktop/Project Notes.app"
BASE_ENTITLEMENTS="${SCRIPT_DIR}/ProjectNotes-AppStore.entitlements"
STAGING_DIR="${SCRIPT_DIR}/staging-appstore"
OUTPUT_DIR="${SCRIPT_DIR}/output-appstore"

# ── Argument parsing ──────────────────────────────────────────────────────────
BUILD_NUMBER=""
DO_BUILD=true
VALIDATE_ONLY=false
USER_PLUGINS=ON
while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-number)    BUILD_NUMBER="$2"; shift 2 ;;
        --api-key)         ASC_API_KEY_ID="$2"; shift 2 ;;
        --api-issuer)      ASC_API_ISSUER="$2"; shift 2 ;;
        --validate-only)   VALIDATE_ONLY=true; shift ;;
        --skip-build)      DO_BUILD=false; shift ;;
        --no-user-plugins) USER_PLUGINS=OFF; shift ;;
        -h|--help)         usage; exit 0 ;;
        -*)                echo "ERROR: unknown option: $1" >&2; usage; exit 2 ;;
        *)
            if [[ -z "${BUILD_NUMBER}" ]]; then
                BUILD_NUMBER="$1"; shift
            else
                echo "ERROR: unexpected argument: $1" >&2; usage; exit 2
            fi
            ;;
    esac
done
BUILD_NUMBER="${BUILD_NUMBER:-$(date -u +%Y%m%d%H%M)}"
[[ "${BUILD_NUMBER}" =~ ^[0-9]+(\.[0-9]+){0,2}$ ]] || {
    echo "ERROR: build number must be one to three dot-separated integers: ${BUILD_NUMBER}" >&2; exit 2; }

PKG="${OUTPUT_DIR}/ProjectNotes-${PN_VERSION}-${BUILD_NUMBER}-AppStore.pkg"

# First keychain identity (policy $1) whose name matches the regex $2.
find_identity() {
    security find-identity -v -p "$1" 2>/dev/null | \
        sed -n 's/^.*"\(.*\)"$/\1/p' | grep -E -m 1 "$2" || true
}

# Newest unexpired Mac App Store profile for TEAM_ID.BUNDLE_ID. Development
# profiles list devices and Developer ID profiles cover all devices; an App
# Store profile does neither.
find_profile() {
    local want="${TEAM_ID}.${BUNDLE_ID}" now best="" best_created="" tmp p dir created
    now="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    tmp="$(mktemp)"
    for dir in "$HOME/Library/Developer/Xcode/UserData/Provisioning Profiles" \
               "$HOME/Library/MobileDevice/Provisioning Profiles"; do
        [[ -d "${dir}" ]] || continue
        for p in "${dir}"/*.provisionprofile; do
            [[ -f "${p}" ]] || continue
            security cms -D -i "${p}" > "${tmp}" 2>/dev/null || continue
            [[ "$(/usr/libexec/PlistBuddy -c 'Print :Entitlements:com.apple.application-identifier' "${tmp}" 2>/dev/null)" == "${want}" ]] || continue
            /usr/libexec/PlistBuddy -c 'Print :Platform' "${tmp}" 2>/dev/null | grep -q OSX || continue
            /usr/libexec/PlistBuddy -c 'Print :ProvisionedDevices' "${tmp}" >/dev/null 2>&1 && continue
            [[ "$(/usr/libexec/PlistBuddy -c 'Print :ProvisionsAllDevices' "${tmp}" 2>/dev/null)" == "true" ]] && continue
            [[ "$(plutil -extract ExpirationDate raw -o - "${tmp}")" > "${now}" ]] || continue
            created="$(plutil -extract CreationDate raw -o - "${tmp}")"
            if [[ -z "${best}" || "${created}" > "${best_created}" ]]; then
                best="${p}"; best_created="${created}"
            fi
        done
    done
    rm -f "${tmp}"
    echo "${best}"
}

# ── Sanity checks ─────────────────────────────────────────────────────────────
if ${DO_BUILD}; then
    pn_require_tools
fi

APP_IDENTITY="${APP_IDENTITY:-$(find_identity codesigning "^(Apple Distribution|3rd Party Mac Developer Application): .*\(${TEAM_ID}\)$")}"
INSTALLER_IDENTITY="${INSTALLER_IDENTITY:-$(find_identity basic "^(3rd Party Mac Developer Installer|Mac Installer Distribution): .*\(${TEAM_ID}\)$")}"
[[ -n "${APP_IDENTITY}" ]] || die "No Apple Distribution certificate for team ${TEAM_ID} in the keychain.
Create one in Xcode > Settings > Accounts > Manage Certificates, or set APP_IDENTITY."
[[ -n "${INSTALLER_IDENTITY}" ]] || die "No Mac Installer Distribution certificate for team ${TEAM_ID} in the keychain.
Create one in Xcode > Settings > Accounts > Manage Certificates, or set INSTALLER_IDENTITY."

PN_PROVISIONING_PROFILE="${PN_PROVISIONING_PROFILE:-$(find_profile)}"
[[ -n "${PN_PROVISIONING_PROFILE}" && -f "${PN_PROVISIONING_PROFILE}" ]] || die "No Mac App Store provisioning profile for ${TEAM_ID}.${BUNDLE_ID}.
Download it in Xcode > Settings > Accounts > Download Manual Profiles, or set PN_PROVISIONING_PROFILE."

echo "==> Project Notes Mac App Store build"
echo "    Version      : ${PN_VERSION} (${BUILD_NUMBER})"
echo "    Bundle ID    : ${BUNDLE_ID}"
echo "    Runs on      : macOS ${MIN_MACOS}+, ${PN_ARCHS[*]}"
echo "    Build dir    : ${PN_BUILD_DIR}"
echo "    App identity : ${APP_IDENTITY}"
echo "    Pkg identity : ${INSTALLER_IDENTITY}"
echo "    Profile      : ${PN_PROVISIONING_PROFILE}"
echo

# ── Steps 1-3: static Hunspell and OpenSSL, configure, build ──────────────────
if ${DO_BUILD}; then
    log "==> [1/6] Building Hunspell and OpenSSL with vcpkg..."
    pn_build_static_deps
    log "==> [2/6] Configuring and [3/6] building..."
    pn_configure_and_build "${PN_BUILD_DIR}" \
        -DPN_APP_STORE=ON \
        -DPN_APP_STORE_USER_PLUGINS="${USER_PLUGINS}" \
        -DPN_MACOS_BUNDLE_ID="${BUNDLE_ID}"
else
    log "==> [1-3/6] Skipped (--skip-build)"
fi

[[ -d "${PN_APP}" ]] || die "App bundle not found: ${PN_APP}"
# A standard build contains the Thunderbird helper, which the App Store flavor
# leaves out. Refuse a bundle built without the flavor.
[[ ! -e "${PN_APP}/Contents/MacOS/ProjectNotesThunderbirdContacts" ]] || \
    die "${PN_APP} is a standard build (it contains the Thunderbird helper). Configure with -DPN_APP_STORE=ON."
BUILT_ID="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "${PN_APP}/Contents/Info.plist")"
[[ "${BUILT_ID}" == "${BUNDLE_ID}" ]] || \
    die "${PN_APP} has bundle ID ${BUILT_ID}, expected ${BUNDLE_ID}. Rebuild without --skip-build."

# ── Step 4: stage and check the bundle ────────────────────────────────────────
log "==> [4/6] Staging..."
rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}" "${OUTPUT_DIR}"
STAGED="${STAGING_DIR}/Project Notes.app"
ditto "${PN_APP}" "${STAGED}"
PLIST="${STAGED}/Contents/Info.plist"

pn_prune_bundle "${STAGED}"

# App Store Connect needs a unique build number per upload, and a marketing
# version that grows with each release (the CMake template uses major.minor).
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion ${BUILD_NUMBER}" "${PLIST}"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString ${PN_VERSION}" "${PLIST}"
pn_check_bundle "${STAGED}"

cp "${PN_PROVISIONING_PROFILE}" "${STAGED}/Contents/embedded.provisionprofile"

# App Store Connect rejects any file carrying com.apple.quarantine (ITMS-91109).
# A profile downloaded through a browser has it, and cp keeps it.
xattr -r -d com.apple.quarantine "${STAGED}" 2>/dev/null || true
if xattr -r "${STAGED}" 2>/dev/null | grep -q "com.apple.quarantine"; then
    die "Could not remove com.apple.quarantine from ${STAGED}."
fi

# ── Step 5: sign and package ──────────────────────────────────────────────────
log "==> [5/6] Signing..."
# TestFlight and the App Store check that the signature carries the same
# application identifier as the embedded profile. Xcode adds these two keys
# itself; a build without Xcode must add them.
ENTITLEMENTS="${STAGING_DIR}/ProjectNotes-AppStore.entitlements"
cp "${BASE_ENTITLEMENTS}" "${ENTITLEMENTS}"
/usr/libexec/PlistBuddy -c "Add :com.apple.application-identifier string ${TEAM_ID}.${BUNDLE_ID}" "${ENTITLEMENTS}"
/usr/libexec/PlistBuddy -c "Add :com.apple.developer.team-identifier string ${TEAM_ID}" "${ENTITLEMENTS}"

# Every nested Mach-O first (deepest path first), then frameworks and plug-in
# bundles, then the app with the sandbox entitlements. Only the main executable
# carries entitlements; Qt's frameworks and plug-ins are libraries.
macho_files "${STAGED}/Contents" | grep -v "/Contents/MacOS/" | \
    while IFS= read -r item; do printf '%d\t%s\n' "${#item}" "${item}"; done | sort -rn | cut -f2- | \
    while IFS= read -r item; do
        codesign --force --sign "${APP_IDENTITY}" --timestamp "${item}" 2>&1 | \
            grep -v "replacing existing signature" || true
    done
find "${STAGED}/Contents" \( -name "*.framework" -o -name "*.bundle" \) -prune -print | \
    while IFS= read -r item; do printf '%d\t%s\n' "${#item}" "${item}"; done | sort -rn | cut -f2- | \
    while IFS= read -r item; do
        codesign --force --sign "${APP_IDENTITY}" --timestamp "${item}" 2>&1 | \
            grep -v "replacing existing signature" || true
    done
codesign --force --sign "${APP_IDENTITY}" --timestamp \
    --entitlements "${ENTITLEMENTS}" "${STAGED}"
codesign --verify --deep --strict --verbose=2 "${STAGED}"
codesign -d --entitlements - --xml "${STAGED}" 2>/dev/null | grep -q "com.apple.security.app-sandbox" || \
    die "The signed app does not carry the App Sandbox entitlement."
# App Review rejects these (Guideline 2.4.5(i)); see AppStoreReviewNotes.md.
if codesign -d --entitlements - --xml "${STAGED}" 2>/dev/null | grep -q -E "apple-events|temporary-exception"; then
    die "The signed app carries an Apple Events or temporary-exception entitlement, which App Review rejects."
fi

log "Building ${PKG}..."
rm -f "${PKG}"
productbuild --component "${STAGED}" /Applications \
    --sign "${INSTALLER_IDENTITY}" \
    "${PKG}"
pkgutil --check-signature "${PKG}" >/dev/null || die "The package signature does not verify."
log "Built: ${PKG}"

# ── Step 6: upload (optional) ─────────────────────────────────────────────────
log "==> [6/6] Upload..."
if [[ -n "${ASC_API_KEY_ID}" && -n "${ASC_API_ISSUER}" ]]; then
    UPLOAD_ARGS=(--api-key "${ASC_API_KEY_ID}" --api-issuer "${ASC_API_ISSUER}")
    ${VALIDATE_ONLY} && UPLOAD_ARGS+=(--validate-only)
    "${SCRIPT_DIR}/upload_appstore.sh" "${UPLOAD_ARGS[@]}" "${PKG}"
else
    cat <<EOF
    Skipping upload (ASC_API_KEY_ID / ASC_API_ISSUER not set).
    Upload the package with either:
      - Transporter.app: drag in ${PKG}
      - packaging/macos/upload_appstore.sh --api-key <KEYID> --api-issuer <ISSUER> "${PKG}"
EOF
fi

echo
echo "Done."
