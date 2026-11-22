#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only
#
# write_asc_key.sh
#
# For the release workflow. Saves the App Store Connect API key where altool
# looks for it (~/.appstoreconnect/private_keys/AuthKey_<KEYID>.p8) and writes
# the JSON key file fastlane reads to $RUNNER_TEMP/asc_api_key.json.
#
# Environment: ASC_API_KEY_ID, ASC_API_ISSUER_ID, ASC_API_KEY_P8 (the .p8 text).

set -euo pipefail

: "${ASC_API_KEY_ID:?}" "${ASC_API_ISSUER_ID:?}" "${ASC_API_KEY_P8:?}" "${RUNNER_TEMP:?}"

dir="$HOME/.appstoreconnect/private_keys"
mkdir -p "${dir}"
printf '%s\n' "${ASC_API_KEY_P8}" > "${dir}/AuthKey_${ASC_API_KEY_ID}.p8"
chmod 600 "${dir}/AuthKey_${ASC_API_KEY_ID}.p8"

python3 - "${RUNNER_TEMP}/asc_api_key.json" <<'PY'
import json, os, sys
with open(sys.argv[1], "w") as f:
    json.dump({"key_id": os.environ["ASC_API_KEY_ID"],
               "issuer_id": os.environ["ASC_API_ISSUER_ID"],
               "key": os.environ["ASC_API_KEY_P8"],
               "in_house": False}, f)
PY
chmod 600 "${RUNNER_TEMP}/asc_api_key.json"
