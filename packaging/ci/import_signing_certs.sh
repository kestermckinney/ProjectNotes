#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only
#
# import_signing_certs.sh
#
# For the release workflow. Creates a temporary keychain, imports one .p12 that
# holds the signing identities a job needs (certificate and private key for
# each), and puts the keychain on the search list so codesign, productbuild and
# xcodebuild find them.
#
# Environment:
#   CERTS_P12        The .p12, base64 encoded
#   CERTS_PASSWORD   Its export password
#
# Export the .p12 from Keychain Access: select every certificate the job needs
# (with its private key), then File > Export Items. See packaging/ci/RELEASING.md.

set -euo pipefail

: "${CERTS_P12:?CERTS_P12 is not set}"
: "${CERTS_PASSWORD:?CERTS_PASSWORD is not set}"
: "${RUNNER_TEMP:?RUNNER_TEMP is not set; this script is for GitHub Actions}"

keychain="${RUNNER_TEMP}/pn-signing.keychain-db"
keychain_password="$(uuidgen)"
p12="${RUNNER_TEMP}/pn-signing.p12"

security create-keychain -p "${keychain_password}" "${keychain}"
# Stay unlocked for six hours, longer than any job runs.
security set-keychain-settings -lut 21600 "${keychain}"
security unlock-keychain -p "${keychain_password}" "${keychain}"

printf '%s' "${CERTS_P12}" | base64 --decode > "${p12}"
security import "${p12}" -k "${keychain}" -P "${CERTS_PASSWORD}" -f pkcs12 \
    -T /usr/bin/codesign -T /usr/bin/productbuild -T /usr/bin/pkgbuild \
    -T /usr/bin/security -T /usr/bin/xcodebuild
rm -f "${p12}"

# Apple's intermediates, in case the runner image lacks a current one.
for cer in AppleWWDRCAG3.cer DeveloperIDG2CA.cer; do
    if curl -fsSL "https://www.apple.com/certificateauthority/${cer}" -o "${RUNNER_TEMP}/${cer}"; then
        security import "${RUNNER_TEMP}/${cer}" -k "${keychain}" >/dev/null 2>&1 || true
    fi
done

# Let the signing tools use the keys without a password prompt.
security set-key-partition-list -S apple-tool:,apple:,codesign: -s \
    -k "${keychain_password}" "${keychain}" >/dev/null

# Search the new keychain first, then the ones already listed.
existing=()
while IFS= read -r line; do
    line="${line#"${line%%[![:space:]]*}"}"
    existing+=("${line//\"/}")
done < <(security list-keychains -d user)
security list-keychains -d user -s "${keychain}" ${existing[@]+"${existing[@]}"}

echo "Signing identities:"
security find-identity -v "${keychain}"
