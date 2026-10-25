#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only
#
# upload_appstore.sh
#
# Validates and uploads a Mac App Store package built by build_appstore.sh to
# App Store Connect. build_appstore.sh calls this when it is given an API key;
# run it on its own to retry an upload without rebuilding.
#
# Usage:
#   ./upload_appstore.sh [options] [PKG]
#
# Arguments:
#   PKG                  The .pkg to upload. Defaults to the newest
#                        output-appstore/ProjectNotes-*-AppStore.pkg.
#
# Options:
#   --api-key <KEYID>    App Store Connect API Key ID    (or ASC_API_KEY_ID)
#   --api-issuer <ID>    App Store Connect API Issuer ID (or ASC_API_ISSUER)
#   --validate-only      Run App Store Connect's checks without uploading.
#   -h, --help           Show this help.
#
# The key's .p8 file must be saved as AuthKey_<KEYID>.p8 in one of the folders
# altool searches: ~/.appstoreconnect/private_keys, ~/.private_keys,
# ~/private_keys or ./private_keys.

set -euo pipefail

usage() { sed -n '11,27p' "$0" | sed 's/^# \{0,1\}//'; }

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/output-appstore"

ASC_API_KEY_ID="${ASC_API_KEY_ID:-}"
ASC_API_ISSUER="${ASC_API_ISSUER:-}"

PKG=""
VALIDATE_ONLY=false
while [[ $# -gt 0 ]]; do
    case "$1" in
        --api-key)       ASC_API_KEY_ID="$2"; shift 2 ;;
        --api-issuer)    ASC_API_ISSUER="$2"; shift 2 ;;
        --validate-only) VALIDATE_ONLY=true; shift ;;
        -h|--help)       usage; exit 0 ;;
        -*)              echo "ERROR: unknown option: $1" >&2; usage; exit 2 ;;
        *)
            if [[ -z "${PKG}" ]]; then
                PKG="$1"; shift
            else
                echo "ERROR: unexpected argument: $1" >&2; usage; exit 2
            fi
            ;;
    esac
done

die() { echo "ERROR: $*" >&2; exit 1; }

if [[ -z "${PKG}" ]]; then
    PKG="$(ls -t "${OUTPUT_DIR}"/ProjectNotes-*-AppStore.pkg 2>/dev/null | head -n 1 || true)"
    [[ -n "${PKG}" ]] || die "No package in ${OUTPUT_DIR}. Run build_appstore.sh first, or name the .pkg."
fi
[[ -f "${PKG}" ]] || die "Package not found: ${PKG}"
[[ -n "${ASC_API_KEY_ID}" && -n "${ASC_API_ISSUER}" ]] || \
    die "Set --api-key and --api-issuer (or ASC_API_KEY_ID and ASC_API_ISSUER)."

KEY_FILE=""
for dir in "$HOME/.appstoreconnect/private_keys" "$HOME/.private_keys" "$HOME/private_keys" "./private_keys"; do
    if [[ -f "${dir}/AuthKey_${ASC_API_KEY_ID}.p8" ]]; then
        KEY_FILE="${dir}/AuthKey_${ASC_API_KEY_ID}.p8"; break
    fi
done
[[ -n "${KEY_FILE}" ]] || die "AuthKey_${ASC_API_KEY_ID}.p8 not found. Save it in ~/.appstoreconnect/private_keys/."

# Catch a package signed for direct download before App Store Connect does.
pkgutil --check-signature "${PKG}" | grep -qE "3rd Party Mac Developer Installer|Mac Installer Distribution" || \
    die "${PKG} is not signed with a Mac Installer Distribution certificate."

AUTH=(--apiKey "${ASC_API_KEY_ID}" --apiIssuer "${ASC_API_ISSUER}")

echo "==> Validating $(basename "${PKG}")..."
xcrun altool --validate-app -f "${PKG}" -t macos "${AUTH[@]}"

if ${VALIDATE_ONLY}; then
    echo "==> Validation passed. Not uploading (--validate-only)."
    exit 0
fi

echo "==> Uploading..."
xcrun altool --upload-app -f "${PKG}" -t macos "${AUTH[@]}"
echo "==> Uploaded. The build appears in App Store Connect > TestFlight once processing finishes."
