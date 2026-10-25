# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only
#
# build_common.sh
#
# Shared by build_installer.sh (direct download) and build_appstore.sh (Mac App
# Store). Source it; don't run it. It builds the app so it runs on every Mac
# with macOS MIN_MACOS or later, Apple silicon and Intel:
#
#   - Hunspell and OpenSSL come from vcpkg as static universal libraries built
#     for MIN_MACOS (vcpkg-triplets/). Homebrew's copies target the build
#     machine's macOS and are arm64 only.
#   - The deployment target is passed explicitly. Qt Creator passes an empty
#     one, which makes the compiler target the build machine's macOS.
#   - pn_check_bundle refuses a bundle with a binary that would not load on
#     such a Mac.
#
# Callers set SCRIPT_DIR and PROJECT_ROOT first, and may override QT_MACOS and
# VCPKG_ROOT through the environment.

# Keep in step with the triplets, distribution.xml and CMAKE_OSX_DEPLOYMENT_TARGET
# in the top-level CMakeLists.txt.
MIN_MACOS="13.0"
PN_ARCHS=(arm64 x86_64)

QT_MACOS="${QT_MACOS:-$HOME/Qt/6.11.1/macos}"
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/Documents/Projects/vcpkg}"
PN_DEPS_ROOT="${PROJECT_ROOT}/build/macos-deps"
PN_DEPS="${PN_DEPS_ROOT}/universal"

PN_VERSION="$(sed -n 's/.*project(ProjectNotes VERSION \([0-9.]*\).*/\1/p' "${PROJECT_ROOT}/CMakeLists.txt")"

log() { echo "[$(date '+%H:%M:%S')] $*"; }
die() { echo "ERROR: $*" >&2; exit 1; }

# True when version $1 is newer than version $2.
version_gt() {
    [[ "$1" != "$2" && "$(printf '%s\n%s\n' "$1" "$2" | sort -V | tail -n 1)" == "$1" ]]
}

# Every Mach-O file under $1, one path per line.
macho_files() {
    find "$1" -type f -print0 | xargs -0 file 2>/dev/null | \
        grep "Mach-O" | grep -v "(for architecture" | sed -E 's/:[[:space:]]+Mach-O.*$//'
}

pn_require_tools() {
    [[ -n "${PN_VERSION}" ]] || die "Could not read the version from CMakeLists.txt."
    [[ -x "${QT_MACOS}/bin/qt-cmake" ]] || die "qt-cmake not found in ${QT_MACOS}/bin
Set QT_MACOS to your Qt macOS kit, e.g. QT_MACOS=\$HOME/Qt/6.11.1/macos"
    [[ -x "${VCPKG_ROOT}/vcpkg" ]] || die "vcpkg not found at ${VCPKG_ROOT}/vcpkg. Set VCPKG_ROOT."
}

# Builds static Hunspell and OpenSSL for each architecture and merges them into
# universal libraries under PN_DEPS.
pn_build_static_deps() {
    local arm="${PN_DEPS_ROOT}/pn-arm64-osx" x64="${PN_DEPS_ROOT}/pn-x64-osx" lib
    # --classic: the repository's vcpkg.json describes the Windows build.
    "${VCPKG_ROOT}/vcpkg" install --classic \
        hunspell:pn-arm64-osx openssl:pn-arm64-osx \
        hunspell:pn-x64-osx openssl:pn-x64-osx \
        --overlay-triplets "${SCRIPT_DIR}/vcpkg-triplets" \
        --x-install-root "${PN_DEPS_ROOT}"

    # The headers serve both architectures, so they must not differ.
    diff -r -q "${arm}/include" "${x64}/include" >/dev/null || \
        die "vcpkg's arm64 and x86_64 headers differ; the libraries cannot share one include folder."
    rm -rf "${PN_DEPS}"
    mkdir -p "${PN_DEPS}/lib"
    cp -R "${arm}/include" "${PN_DEPS}/include"
    for lib in "${arm}"/lib/*.a; do
        lipo -create "${lib}" "${x64}/lib/$(basename "${lib}")" \
            -output "${PN_DEPS}/lib/$(basename "${lib}")"
    done
}

# Configures $1 as a universal Release build and builds the app. Further
# arguments are passed to CMake.
pn_configure_and_build() {
    local build_dir="$1" hunspell_lib generator=()
    shift
    hunspell_lib="$(find "${PN_DEPS}/lib" -maxdepth 1 -name 'libhunspell*.a' | head -n 1)"
    [[ -n "${hunspell_lib}" ]] || die "No static Hunspell library in ${PN_DEPS}/lib"
    command -v ninja >/dev/null && generator=(-G Ninja)
    # Libraries are passed by path: find_library would otherwise prefer
    # Homebrew's dylibs, and a build directory configured before keeps the
    # paths it found then.
    "${QT_MACOS}/bin/qt-cmake" -S "${PROJECT_ROOT}" -B "${build_dir}" \
        ${generator[@]+"${generator[@]}"} \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${MIN_MACOS}" \
        "-DCMAKE_OSX_ARCHITECTURES=$(IFS=';'; echo "${PN_ARCHS[*]}")" \
        -DHunspell_INCLUDE_DIRS="${PN_DEPS}/include" \
        -DHUNSPELL_LIBRARIES="${hunspell_lib}" \
        -DOPENSSL_ROOT_DIR="${PN_DEPS}" \
        -DOPENSSL_USE_STATIC_LIBS=TRUE \
        -DOPENSSL_INCLUDE_DIR="${PN_DEPS}/include" \
        -DOPENSSL_CRYPTO_LIBRARY="${PN_DEPS}/lib/libcrypto.a" \
        -DOPENSSL_SSL_LIBRARY="${PN_DEPS}/lib/libssl.a" \
        "$@"
    cmake --build "${build_dir}" --target ProjectNotes --parallel
}

# Removes what a release bundle must not carry.
pn_prune_bundle() {
    local app="$1"
    # Windows-only plugins never belong in a Mac bundle.
    rm -rf "${app}/Contents/Resources/plugins/office-tools"
    # Build-system artifacts cannot be signed and are not needed at runtime.
    find "${app}/Contents" \( -name "*.a" -o -name "*.o" -o -name "*.pc" -o -name "*.prl" \) -delete 2>/dev/null || true
    # The app only uses SQLite. macdeployqt copies every SQL driver, and the
    # ODBC one drags in Homebrew's libiodbc.
    find "${app}/Contents/PlugIns/sqldrivers" -type f ! -name "libqsqlite.dylib" -delete 2>/dev/null || true
    rm -f "${app}/Contents/Frameworks/"libiodbc*.dylib
}

# Fails when a binary in the bundle lacks one of PN_ARCHS, needs a macOS newer
# than MIN_MACOS, or loads a library from outside the bundle.
pn_check_bundle() {
    local app="$1" bin rel arch archs minos plist_min problems=0
    log "Checking architectures, minimum macOS and library paths..."
    plist_min="$(/usr/libexec/PlistBuddy -c 'Print :LSMinimumSystemVersion' "${app}/Contents/Info.plist" 2>/dev/null || true)"
    [[ "${plist_min}" == "${MIN_MACOS}" ]] || \
        die "Info.plist LSMinimumSystemVersion is '${plist_min}', expected ${MIN_MACOS}."
    while IFS= read -r bin; do
        rel="${bin#"${app}/"}"
        archs="$(lipo -archs "${bin}" 2>/dev/null || true)"
        for arch in "${PN_ARCHS[@]}"; do
            if ! grep -qw "${arch}" <<< "${archs}"; then
                echo "  no ${arch} slice: ${rel}"; problems=$((problems + 1))
                continue
            fi
            minos="$(vtool -arch "${arch}" -show-build "${bin}" 2>/dev/null | awk '/minos/ { print $2; exit }')"
            if [[ -n "${minos}" ]] && version_gt "${minos}" "${MIN_MACOS}"; then
                echo "  ${arch} needs macOS ${minos}: ${rel}"; problems=$((problems + 1))
            fi
        done
        if otool -L "${bin}" 2>/dev/null | tail -n +2 | grep -qE "/opt/homebrew/|/usr/local/"; then
            echo "  links outside the bundle: ${rel}"; problems=$((problems + 1))
        fi
    done < <(macho_files "${app}/Contents")
    [[ ${problems} -eq 0 ]] || die "${problems} problem(s) in the bundle (listed above)."
}
