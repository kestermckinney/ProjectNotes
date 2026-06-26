#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# build_appstore.sh
# Builds, archives, exports, and (optionally) uploads ProjectNotesMobile to
# App Store Connect. Targets the registered App ID
# com.projectnotespro.projectnotesmobile under Apple Team 2PY624KHXH.
#
# Signing is pinned in ProjectNotesMobile/CMakeLists.txt
# (CODE_SIGN_STYLE=Automatic, DEVELOPMENT_TEAM, PRODUCT_BUNDLE_IDENTIFIER), so
# the first run with -allowProvisioningUpdates creates the Apple Distribution
# certificate and the App Store provisioning profile automatically.
#
# Usage:
#   ./build_appstore.sh [BUILD_NUMBER] [options]
#
# Options:
#   --build-number <N>   CFBundleVersion (build number). App Store Connect rejects
#                        re-uploads that reuse a build number, so this must be
#                        unique and increasing per upload. May also be given as the
#                        first bare positional arg. Defaults to a UTC timestamp.
#   --api-key <KEYID>    App Store Connect API Key ID    (enables auto-upload)
#   --api-issuer <ID>    App Store Connect API Issuer ID (enables auto-upload)
#   -h, --help           Show this help.
#
# The API key/issuer can be passed with the flags above OR via the env vars
# ASC_API_KEY_ID / ASC_API_ISSUER. Resolution order: flag > env var > unset
# (unset = skip upload, just produce the .ipa).
#
# Configuration (override via environment variables):
#   QT_IOS           Path to the Qt iOS kit (contains bin/qt-cmake)
#   TEAM_ID          10-character Apple Team ID
#   SCHEME           Xcode scheme to archive
#   ASC_API_KEY_ID   App Store Connect API Key ID   (same as --api-key)
#   ASC_API_ISSUER   App Store Connect API Issuer ID (same as --api-issuer)
#
# Prerequisites:
#   - Xcode + command line tools (xcodebuild, xcrun)
#   - Qt for iOS installed at $QT_IOS
#   - For auto-upload: an App Store Connect API key (.p8) saved in
#     ~/.appstoreconnect/private_keys/AuthKey_<KEYID>.p8

set -euo pipefail

usage() { sed -n '13,40p' "$0" | sed 's/^# \{0,1\}//'; }

# ── Configuration ─────────────────────────────────────────────────────────────
QT_IOS="${QT_IOS:-$HOME/Qt/6.10.2/ios}"
TEAM_ID="${TEAM_ID:-2PY624KHXH}"
SCHEME="${SCHEME:-ProjectNotesMobile}"
BUNDLE_ID="com.projectnotespro.projectnotesmobile"

# API key/issuer default to the env vars; --api-key / --api-issuer override below.
ASC_API_KEY_ID="${ASC_API_KEY_ID:-}"
ASC_API_ISSUER="${ASC_API_ISSUER:-}"

# ── Argument parsing ──────────────────────────────────────────────────────────
BUILD_NUMBER=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-number) BUILD_NUMBER="$2"; shift 2 ;;
        --api-key)      ASC_API_KEY_ID="$2"; shift 2 ;;
        --api-issuer)   ASC_API_ISSUER="$2"; shift 2 ;;
        -h|--help)      usage; exit 0 ;;
        -*)             echo "ERROR: unknown option: $1" >&2; usage; exit 2 ;;
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

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

SRC_DIR="${PROJECT_ROOT}/ProjectNotesMobile"
BUILD_DIR="${SCRIPT_DIR}/build"
XCODEPROJ="${BUILD_DIR}/ProjectNotesMobile.xcodeproj"
ARCHIVE="${BUILD_DIR}/ProjectNotesMobile.xcarchive"
EXPORT_DIR="${BUILD_DIR}/export"
EXPORT_OPTIONS="${SCRIPT_DIR}/ExportOptions.plist"

QT_CMAKE="${QT_IOS}/bin/qt-cmake"

# ── Sanity checks ─────────────────────────────────────────────────────────────
if [[ ! -x "${QT_CMAKE}" ]]; then
    echo "ERROR: qt-cmake not found at ${QT_CMAKE}" >&2
    echo "       Set QT_IOS to your Qt iOS kit, e.g. QT_IOS=\$HOME/Qt/6.10.2/ios" >&2
    exit 1
fi

echo "==> ProjectNotesMobile App Store build"
echo "    Bundle ID    : ${BUNDLE_ID}"
echo "    Team         : ${TEAM_ID}"
echo "    Build number : ${BUILD_NUMBER}"
echo "    Qt iOS kit   : ${QT_IOS}"
echo

# ── Step 1: configure (Xcode generator, Release) ──────────────────────────────
echo "==> [1/4] Configuring Xcode project..."
"${QT_CMAKE}" -S "${SRC_DIR}" -B "${BUILD_DIR}" -G Xcode -DCMAKE_BUILD_TYPE=Release

# ── Step 2: archive (auto-creates distribution cert + App Store profile) ───────
echo "==> [2/4] Archiving..."
xcodebuild \
    -project "${XCODEPROJ}" \
    -scheme "${SCHEME}" \
    -configuration Release \
    -destination 'generic/platform=iOS' \
    -archivePath "${ARCHIVE}" \
    CURRENT_PROJECT_VERSION="${BUILD_NUMBER}" \
    -allowProvisioningUpdates \
    archive

# ── Step 3: export signed .ipa ────────────────────────────────────────────────
echo "==> [3/4] Exporting signed .ipa..."
rm -rf "${EXPORT_DIR}"
xcodebuild \
    -exportArchive \
    -archivePath "${ARCHIVE}" \
    -exportPath "${EXPORT_DIR}" \
    -exportOptionsPlist "${EXPORT_OPTIONS}" \
    -allowProvisioningUpdates

IPA="$(/usr/bin/find "${EXPORT_DIR}" -maxdepth 1 -name '*.ipa' | head -n 1)"
if [[ -z "${IPA}" ]]; then
    echo "ERROR: export produced no .ipa in ${EXPORT_DIR}" >&2
    exit 1
fi
echo "    Exported: ${IPA}"

# ── Step 4: upload (optional) ─────────────────────────────────────────────────
echo "==> [4/4] Upload..."
if [[ -n "${ASC_API_KEY_ID:-}" && -n "${ASC_API_ISSUER:-}" ]]; then
    echo "    Uploading via App Store Connect API key ${ASC_API_KEY_ID}..."
    xcrun altool --upload-app \
        -f "${IPA}" \
        -t ios \
        --apiKey "${ASC_API_KEY_ID}" \
        --apiIssuer "${ASC_API_ISSUER}"
    echo "    Upload complete. Check App Store Connect → TestFlight / Activity for processing."
else
    cat <<EOF
    Skipping auto-upload (ASC_API_KEY_ID / ASC_API_ISSUER not set).
    Upload the build with either:
      • Transporter.app  — drag in: ${IPA}
      • Xcode Organizer  — Window → Organizer → Distribute App
      • Re-run with API key:
          ASC_API_KEY_ID=XXXXXXXXXX ASC_API_ISSUER=xxxx-... ./build_appstore.sh ${BUILD_NUMBER}
        (requires ~/.appstoreconnect/private_keys/AuthKey_<ASC_API_KEY_ID>.p8)

    To validate before publishing (no upload):
      xcrun altool --validate-app -f "${IPA}" -t ios --apiKey <KEYID> --apiIssuer <ISSUER>
EOF
fi

echo
echo "Done."
