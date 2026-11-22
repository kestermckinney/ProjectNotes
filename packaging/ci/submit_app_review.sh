#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only
#
# submit_app_review.sh
#
# For the release workflow, after the build has been uploaded. Uses fastlane
# deliver to create (or reuse) the App Store version, set its "What's New"
# text from packaging/ci/STORE_WHATS_NEW.txt, wait until App Store Connect has
# processed the uploaded build, attach it and submit the version for App
# Review. The version is released by hand once approved.
#
# Usage:
#   submit_app_review.sh <ios|osx> <uploaded .ipa or .pkg> <version>
#
# Environment: RUNNER_TEMP (holding asc_api_key.json from write_asc_key.sh),
# BUNDLE_ID, and optionally STORE_LOCALE (default en-US).

set -euo pipefail

platform="${1:?platform (ios or osx)}"
package="${2:?the uploaded .ipa or .pkg}"
version="${3:?version}"
locale="${STORE_LOCALE:-en-US}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
key_json="${RUNNER_TEMP:?}/asc_api_key.json"
[[ -f "${key_json}" ]] || { echo "ERROR: ${key_json} not found; run write_asc_key.sh first." >&2; exit 1; }
[[ -f "${package}" ]] || { echo "ERROR: ${package} not found." >&2; exit 1; }

case "${platform}" in
    ios) package_arg=(--ipa "${package}") ;;
    osx) package_arg=(--pkg "${package}") ;;
    *)   echo "ERROR: platform must be ios or osx" >&2; exit 2 ;;
esac

command -v fastlane >/dev/null || brew install fastlane

# deliver uploads only the metadata files present, so this changes nothing but
# the release notes.
metadata="${RUNNER_TEMP}/deliver-metadata-${platform}"
rm -rf "${metadata}"
mkdir -p "${metadata}/${locale}"
cp "${SCRIPT_DIR}/STORE_WHATS_NEW.txt" "${metadata}/${locale}/release_notes.txt"

# Without --build_number, deliver reads the build number from the package and
# waits for App Store Connect to finish processing that build before it
# submits.
fastlane deliver \
    --api_key_path "${key_json}" \
    --app_identifier "${BUNDLE_ID:?}" \
    --platform "${platform}" \
    --app_version "${version}" \
    "${package_arg[@]}" \
    --skip_binary_upload true \
    --skip_screenshots true \
    --metadata_path "${metadata}" \
    --submit_for_review true \
    --automatic_release false \
    --run_precheck_before_submit false \
    --force true \
    --submission_information '{"export_compliance_uses_encryption": false, "add_id_info_uses_idfa": false}'
