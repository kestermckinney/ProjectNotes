cmake_minimum_required(VERSION 3.16)

project(ProjectNotes VERSION 6.4.0 LANGUAGES CXX)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS deployment version")
endif()

# Optionally, set version components explicitly
set(APP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(APP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(APP_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Configure the version header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h
    @ONLY
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt::Widgets is still required even though the frontend is pure QML:
# ProjectNotesCore reports database errors through QMessageBox, so the app runs
# a QApplication.
# Charts is not used by this project's own code, but SqliteSyncProLib links
# Qt::Charts. Finding it here, at top-level scope, is what makes the imported
# target visible to the consumers that walk SqliteSyncProLib's dependencies.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Sql Xml Network Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Sql Xml Network Charts)

# CorePrivate exposes QtCore's private QZipWriter (qzipwriter_p.h), used by the
# "Send Logs to Support" log bundler.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS CorePrivate)
if(UNIX AND NOT APPLE AND NOT ANDROID)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS DBus)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENV{QT_LOGGING_RULES} "qt.*.debug=true")
endif()

# Find the Hunspell library
find_path(Hunspell_INCLUDE_DIRS NAMES hunspell/hunspell.hxx)
find_library(HUNSPELL_LIBRARIES NAMES hunspell hunspell-1.7)

message(STATUS "Hunspell includes: ${Hunspell_INCLUDE_DIRS}")
message(STATUS "Hunspell library: ${HUNSPELL_LIBRARIES}")

# Log bundling for "Send Logs to Support" uses Qt's built-in QZipWriter
# (QtCore private API, header QtCore/private/qzipwriter_p.h), so no external zip
# library is required. It is provided by the Qt6::CorePrivate target, linked
# into the executable below. (The vcpkg quazip port was avoided because it pulls
# in a full vcpkg Qt rebuild, and a source build of QuaZip needs the Qt5Compat
# module which is not part of this Qt install.)


if(APPLE AND NOT OPENSSL_ROOT_DIR)
    if(EXISTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libcrypto.a")
        set(OPENSSL_ROOT_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
    else()
        execute_process(
            COMMAND brew --prefix openssl
            OUTPUT_VARIABLE OPENSSL_ROOT_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()
endif()

# Location of the SqliteSyncPro source. Defaults to a sibling checkout
# (normal local/dev build); the Flatpak build overrides this with
# -DSQLITESYNCPRO_SRC_DIR=SqliteSyncPro/src, where the git source is fetched
# into a subdirectory of the build root.
set(SQLITESYNCPRO_SRC_DIR "../SqliteSyncPro/src" CACHE STRING "Path to SqliteSyncPro/src")
add_subdirectory(${SQLITESYNCPRO_SRC_DIR} SqliteSyncPro)
add_subdirectory(ProjectNotesCore)
# Shared integration contracts also provide File Finder's existing OAuth
# implementation. Email/rendering remain desktop-only below.
add_subdirectory(ProjectNotesIntegrations)
add_subdirectory(ProjectNotesFileFinder EXCLUDE_FROM_ALL)
if(NOT ANDROID AND NOT IOS)
    add_subdirectory(ProjectNotesPlugins)
endif()
if(NOT ANDROID AND NOT IOS)
    add_subdirectory(ProjectNotesEmail)
    add_subdirectory(ProjectNotesEmailRendering)
    add_subdirectory(ProjectNotesMeetings)
    add_subdirectory(ProjectNotesContacts)
endif()

# ── QML desktop frontend ─────────────────────────────────────────────────────
# The ProjectNotesDesktop target is the desktop app; it reuses ProjectNotesCore.
# ON by default on desktop platforms.
option(BUILD_QML_DESKTOP "Build the new QML (Qt Quick) desktop frontend" ON)

# Mac App Store flavor of the desktop app: App Sandbox (security-scoped
# bookmarks for folders the user picks), no in-app updater, no programs
# started by plugins, no Thunderbird. Packaged by packaging/macos/build_appstore.sh.
option(PN_APP_STORE "Build the Mac App Store flavor of the desktop app" OFF)
# Lets App Review's objection to user plugins be met without a code change.
option(PN_APP_STORE_USER_PLUGINS "Let the App Store flavor load plugins from the user's plugins folder" ON)
if(PN_APP_STORE AND NOT APPLE)
    message(FATAL_ERROR "PN_APP_STORE is for macOS builds only.")
endif()
if(BUILD_QML_DESKTOP AND NOT ANDROID AND NOT IOS)
    add_subdirectory(ProjectNotesDesktop)
endif()

# Automated tests for the QML desktop frontend. OFF by default. Configure a
# dedicated build dir with -DBUILD_QML_DESKTOP_TESTS=ON -DBUILD_QML_DESKTOP=OFF
# so tests/desktop is the only target declaring the ProjectNotesDesktop QML
# module (a second declaration of the same URI would clash).
option(BUILD_QML_DESKTOP_TESTS "Build the QML desktop test suite" OFF)
if(BUILD_QML_DESKTOP_TESTS AND NOT ANDROID AND NOT IOS)
    enable_testing()
    add_subdirectory(tests/desktop)
endif()

# Needed by the Linux asset and freedesktop install rules further down.
include(GNUInstallDirs)


# ── Install rules (Linux/Flatpak) ────────────────────────────────────────────
# These stage runtime assets and freedesktop metadata for the *installed* tree.
if(NOT APPLE)
    # CMake directory installs merge into an existing prefix. Remove every
    # Python plugin, thread and form earlier releases shipped, so an upgrade
    # leaves none of them behind.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/RemoveRetiredPlugins.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/RemoveRetiredPlugins.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/RemoveRetiredPlugins.cmake")

    # Install runtime assets alongside the executable so packaged builds
    # (e.g. Flatpak installs to /app/bin) can find them via appResourcesPath().
    # Windows-only plugins (Office automation) are left out.
    install(DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/plugins"
            "${CMAKE_CURRENT_SOURCE_DIR}/dictionary"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        PATTERN "office-tools" EXCLUDE
    )

    # Freedesktop integration (Linux/Flatpak): desktop entry, AppStream metadata,
    # and hicolor icons so the app appears in menus and Flatpak can export it.
    if(NOT WIN32)
        set(_linux_pkg "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux")

        # Install the QML desktop binary itself — what Linux/Flatpak ships.
        # Guarded by TARGET so a -DBUILD_QML_DESKTOP=OFF configure (e.g.
        # tests/desktop's dedicated build) doesn't fail to generate.
        if(TARGET ProjectNotes)
            install(TARGETS ProjectNotes
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()

        install(FILES "${_linux_pkg}/com.projectnotespro.ProjectNotes.desktop"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
        )
        install(FILES "${_linux_pkg}/com.projectnotespro.ProjectNotes.metainfo.xml"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
        )
        foreach(_size 64 128 256 512)
            install(FILES "${_linux_pkg}/icons/${_size}x${_size}/com.projectnotespro.ProjectNotes.png"
                DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}x${_size}/apps"
            )
        endforeach()
    endif()
endif()

# ── Windows installer staging target ──────────────────────────────────────────
if(WIN32)
    set(SQLITESYNCPRO_BUILD_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../SqliteSyncPro/build/Desktop_Qt_6_10_0_MSVC2022_64bit-Release"
        CACHE PATH "SqliteSyncPro release build directory")

    # Locate windeployqt from the Qt installation used to build
    get_target_property(_qmake_loc Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qmake_loc}" DIRECTORY)

    # Locate dxcompiler.dll/dxil.dll from the Windows SDK (needed by Qt Quick's RHI)
    file(GLOB _dxc_candidates
        "C:/Program Files (x86)/Windows Kits/10/bin/*/x64/dxcompiler.dll"
    )
    if(_dxc_candidates)
        list(SORT _dxc_candidates ORDER DESCENDING)
        list(GET _dxc_candidates 0 _dxcompiler_dll)
        get_filename_component(_dxc_dir "${_dxcompiler_dll}" DIRECTORY)
        set(_dxil_dll "${_dxc_dir}/dxil.dll")
        message(STATUS "Found dxcompiler.dll: ${_dxcompiler_dll}")
    else()
        message(WARNING "dxcompiler.dll not found in Windows SDK — Qt Quick may not render at runtime")
    endif()

    set(DEPLOY_DIR  "${CMAKE_BINARY_DIR}/deploy")
    set(NSIS_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows")

    # Only meaningful when the desktop target actually exists — stages its
    # binary via $<TARGET_FILE:ProjectNotes>. Guarded so a
    # -DBUILD_QML_DESKTOP=OFF configure (e.g. tests/desktop's dedicated
    # "-DBUILD_QML_DESKTOP=OFF -DBUILD_QML_DESKTOP_TESTS=ON" build) doesn't
    # fail to generate over a target that was never built.
    if(TARGET ProjectNotes)
    add_custom_target(deploy
        COMMENT "Staging Windows deployment to ${DEPLOY_DIR}"

        # 1. Clean slate
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory   "${DEPLOY_DIR}"

        # 2. Application executables
        # The QML target builds as "Project Notes.exe" (OUTPUT_NAME), the same
        # name earlier releases shipped, so shortcuts, registry entries, and the
        # installer script don't change; this copy just stages it (from its own
        # build subdir) into DEPLOY_DIR.
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:ProjectNotes>"
            "${DEPLOY_DIR}/Project Notes.exe"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/admin/Project Notes Remote Host.exe"
            "${DEPLOY_DIR}/Project Notes Remote Host.exe"

        # 3. Qt DLLs and plugins via windeployqt
        # --qmldir points windeployqt at the QML sources so it can statically
        # scan imports (qt_add_qml_module compiles them into the binary, so
        # they aren't visible on disk next to the exe). qmltooling (the QML
        # debugger/profiler backends) is dev-only and skipped in the shipped
        # installer.
        COMMAND "${_qt_bin}/windeployqt.exe"
            --release --no-translations
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/ProjectNotesDesktop"
            --skip-plugin-types qmltooling
            --dir "${DEPLOY_DIR}"
            "${DEPLOY_DIR}/Project Notes.exe"

        # 4. ProjectNotes vcpkg runtime DLLs
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/hunspell-1.7-0.dll"
            "${DEPLOY_DIR}/hunspell-1.7-0.dll"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/libcrypto-3-x64.dll"
            "${DEPLOY_DIR}/libcrypto-3-x64.dll"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/legacy.dll"
            "${DEPLOY_DIR}/legacy.dll"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/libssl-3-x64.dll"
            "${DEPLOY_DIR}/libssl-3-x64.dll"

        # 5. SqliteSyncPro runtime DLLs
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/admin/libpq.dll"
            "${DEPLOY_DIR}/libpq.dll"

        # 6. Application content
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/plugins"    "${DEPLOY_DIR}/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/dictionary" "${DEPLOY_DIR}/dictionary"

        DEPENDS ProjectNotes
        VERBATIM
    )

    # Copy DirectX Shader Compiler DLLs required by Qt Quick (RHI backend)
    add_custom_command(TARGET deploy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ProjectNotesThunderbirdContacts> "${DEPLOY_DIR}/ProjectNotesThunderbirdContacts.exe"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ProjectNotesOutlookContacts> "${DEPLOY_DIR}/ProjectNotesOutlookContacts.exe"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/ProjectNotesContacts/projectnotes-contacts.xpi" "${DEPLOY_DIR}/projectnotes-contacts.xpi")
    if(_dxcompiler_dll AND EXISTS "${_dxcompiler_dll}")
        add_custom_command(TARGET deploy POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${_dxcompiler_dll}" "${DEPLOY_DIR}/dxcompiler.dll"
            COMMAND ${CMAKE_COMMAND} -E copy "${_dxil_dll}"       "${DEPLOY_DIR}/dxil.dll"
            COMMENT "Copying DirectX Shader Compiler DLLs"
            VERBATIM
        )
    endif()
    endif() # TARGET ProjectNotes
endif()

