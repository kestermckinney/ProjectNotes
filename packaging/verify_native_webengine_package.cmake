# Verifies the native Qt WebEngine payload expected by a distributable package.
#
# Usage:
#   cmake -DPN_PLATFORM=windows|macos|flatpak -DPN_PACKAGE_ROOT=<root> -P
#       packaging/verify_native_webengine_package.cmake
#
# This deliberately checks only Qt's native deployment locations. A PyQt wheel
# under site-packages is not evidence that the C++ renderer can start.

if(NOT DEFINED PN_PLATFORM OR NOT DEFINED PN_PACKAGE_ROOT)
    message(FATAL_ERROR "PN_PLATFORM and PN_PACKAGE_ROOT are required")
endif()

get_filename_component(_root "${PN_PACKAGE_ROOT}" ABSOLUTE)
if(NOT IS_DIRECTORY "${_root}")
    message(FATAL_ERROR "Native WebEngine package root does not exist: ${_root}")
endif()

string(TOLOWER "${PN_PLATFORM}" _platform)
if(_platform STREQUAL "windows")
    set(_helper "${_root}/QtWebEngineProcess.exe")
    set(_resources "${_root}/resources")
    set(_locales "${_root}/translations/qtwebengine_locales")
elseif(_platform STREQUAL "macos")
    set(_framework "${_root}/Contents/Frameworks/QtWebEngineCore.framework")
    set(_helper "${_framework}/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess")
    set(_resources "${_framework}/Resources")
    set(_locales "${_framework}/Resources/qtwebengine_locales")
elseif(_platform STREQUAL "flatpak")
    set(_helper "${_root}/lib/libexec/QtWebEngineProcess")
    set(_resources "${_root}/resources")
    set(_locales "${_root}/translations/qtwebengine_locales")
else()
    message(FATAL_ERROR "Unsupported PN_PLATFORM: ${PN_PLATFORM}")
endif()

foreach(_path IN ITEMS
        "${_helper}"
        "${_resources}/qtwebengine_resources.pak"
        "${_resources}/qtwebengine_resources_100p.pak"
        "${_locales}/en-US.pak")
    if(NOT EXISTS "${_path}")
        message(FATAL_ERROR "Missing native Qt WebEngine deployment file: ${_path}")
    endif()
endforeach()

# macOS deploys an architecture-specific V8 snapshot (for example
# v8_context_snapshot.arm64.bin); Linux and Windows use the unqualified name.
if(_platform STREQUAL "macos")
    file(GLOB _snapshots "${_resources}/v8_context_snapshot.*.bin")
    if(NOT _snapshots)
        message(FATAL_ERROR "Missing native Qt WebEngine V8 snapshot under: ${_resources}")
    endif()
else()
    if(NOT EXISTS "${_resources}/v8_context_snapshot.bin")
        message(FATAL_ERROR "Missing native Qt WebEngine deployment file: ${_resources}/v8_context_snapshot.bin")
    endif()
endif()

message(STATUS "Verified native Qt WebEngine package payload (${_platform}): ${_root}")
