#!/usr/bin/env bash
# Copyright (C) 2026 Paul McKinney
# sync_plugins.sh
#
# Syncs only changed Python plugins and threads into the built app bundle.
# Uses rsync --checksum so only files whose content actually changed are copied.
# Run this instead of a full build when only .py files have been edited.
#
# Usage:
#   ./sync_plugins.sh                    # auto-detects Debug then Release bundle
#   ./sync_plugins.sh --release          # prefer Release bundle
#   ./sync_plugins.sh /path/to/My.app    # explicit bundle path

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ── Locate the app bundle ──────────────────────────────────────────────────────

BUNDLE=""
PREFER_RELEASE=false

for arg in "$@"; do
    case "$arg" in
        --release) PREFER_RELEASE=true ;;
        *.app)     BUNDLE="$arg" ;;
        *)         echo "Unknown option: $arg"; exit 1 ;;
    esac
done

if [[ -z "$BUNDLE" ]]; then
    if $PREFER_RELEASE; then
        SEARCH_ORDER=(
            "${SCRIPT_DIR}/build/Qt_6_10_2_for_macOS-Release/ProjectNotes.app"
            "${SCRIPT_DIR}/build/Qt_6_10_2_for_macOS-Debug/ProjectNotes.app"
        )
    else
        SEARCH_ORDER=(
            "${SCRIPT_DIR}/build/Qt_6_10_2_for_macOS-Debug/ProjectNotes.app"
            "${SCRIPT_DIR}/build/Qt_6_10_2_for_macOS-Release/ProjectNotes.app"
        )
    fi

    for candidate in "${SEARCH_ORDER[@]}"; do
        if [[ -d "$candidate" ]]; then
            BUNDLE="$candidate"
            break
        fi
    done
fi

if [[ -z "$BUNDLE" || ! -d "$BUNDLE" ]]; then
    echo "ERROR: Could not find ProjectNotes.app."
    echo "  Pass the bundle path explicitly: $0 /path/to/ProjectNotes.app"
    echo "  Or build the project at least once first."
    exit 1
fi

RESOURCES="${BUNDLE}/Contents/Resources"

if [[ ! -d "$RESOURCES" ]]; then
    echo "ERROR: ${BUNDLE} does not look like a valid app bundle (no Contents/Resources)."
    exit 1
fi

echo "Bundle : ${BUNDLE}"
echo "Syncing plugins and threads (changed files only)..."

# --checksum compares file content, not just timestamp, so IDE-touch side-effects
# don't cause unnecessary copies. -v lists each file that is actually updated.
rsync -rv --checksum \
    "${SCRIPT_DIR}/plugins/" \
    "${RESOURCES}/plugins/"

rsync -rv --checksum \
    "${SCRIPT_DIR}/threads/" \
    "${RESOURCES}/threads/"

echo "Done."
