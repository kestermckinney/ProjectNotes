# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only
#
# vcpkg triplet for the macOS release builds (build_installer.sh and
# build_appstore.sh, through build_common.sh). The x86_64 half of the universal
# static Hunspell and OpenSSL.
#
# Homebrew's libraries target the macOS they were built on and are not
# universal, so linking them would stop the app from launching on older Macs.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES x86_64)
# Keep in step with MIN_MACOS in build_common.sh.
set(VCPKG_OSX_DEPLOYMENT_TARGET 13.0)
# Only the release libraries are linked.
set(VCPKG_BUILD_TYPE release)
