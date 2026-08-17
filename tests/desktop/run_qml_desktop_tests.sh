#!/usr/bin/env bash
# Configure, build, and run the QML desktop test suite headless.
#
# Builds in a dedicated build dir (build-qmltests) with the shipping QML desktop
# target OFF so the test target is the sole owner of the ProjectNotesDesktop QML
# module. Uses the offscreen platform so it needs no display, and an isolated
# QStandardPaths test profile so it never touches your real database.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$ROOT/build-qmltests"

cmake -S "$ROOT" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_QML_DESKTOP_TESTS=ON \
    -DBUILD_QML_DESKTOP=OFF

# nproc is Linux-only; macOS has sysctl instead.
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

cmake --build "$BUILD" --target tst_qmldesktop -j"$JOBS"

export QT_QPA_PLATFORM=offscreen
# Keep plugin/python noise out of the test's own stdout stream if desired.
"$BUILD/tests/desktop/tst_qmldesktop" "$@"
