cmake_minimum_required(VERSION 3.16)

project(ProjectNotesMobile VERSION 5.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(QT_ENABLE_QML_DEBUG ON)
endif()

# ── OpenSSL via local vcpkg iOS builds ───────────────────────────────────────
# iOS cross-compilation sets CMAKE_FIND_ROOT_PATH_MODE to ONLY, which causes
# FindOpenSSL to only search inside the Xcode sysroot and ignore OPENSSL_ROOT_DIR.
# Bypass the search entirely by pre-setting the exact cache variables that
# FindOpenSSL.cmake would normally populate — when these are already set it
# skips its search and uses them directly.
set(_VCPKG_IOS_ROOT "/Users/paulmckinney/Documents/Projects/vcpkg/installed")

if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
    # x86_64 simulator — runs via Rosetta on Apple Silicon.
    # Qt's ios package only provides x86_64 for simulator (arm64 slice is device-only).
    set(_OPENSSL_TRIPLET "${_VCPKG_IOS_ROOT}/x64-ios-simulator")
else()
    set(_OPENSSL_TRIPLET "${_VCPKG_IOS_ROOT}/arm64-ios")
endif()

set(OPENSSL_ROOT_DIR      "${_OPENSSL_TRIPLET}" CACHE PATH   "OpenSSL root (vcpkg iOS triplet)" FORCE)
set(OPENSSL_INCLUDE_DIR   "${_OPENSSL_TRIPLET}/include" CACHE PATH   "OpenSSL include dir" FORCE)
set(OPENSSL_CRYPTO_LIBRARY "${_OPENSSL_TRIPLET}/lib/libcrypto.a" CACHE FILEPATH "OpenSSL crypto library" FORCE)
set(OPENSSL_SSL_LIBRARY    "${_OPENSSL_TRIPLET}/lib/libssl.a"    CACHE FILEPATH "OpenSSL ssl library"    FORCE)

message(STATUS "OpenSSL triplet: ${_OPENSSL_TRIPLET}")

# ── Qt modules ───────────────────────────────────────────────────────────────
# Qt::Widgets is brought in transitively by SqliteSyncProLib and ProjectNotesCore.
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Sql Xml Network Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Sql Xml Network Widgets)

# ── iOS deployment target ─────────────────────────────────────────────────────
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0" CACHE STRING "Minimum iOS/macOS deployment version")
endif()

# ── Shared dependencies ───────────────────────────────────────────────────────
# Add SqliteSyncPro before ProjectNotesCore so SqliteSyncProLib target exists
# when ProjectNotesCore links against it (it doesn't link it directly, but
# the parent's link line is cleaner this way).
add_subdirectory(../../SqliteSyncPro/src SqliteSyncPro)

# Add the shared data-layer library.  Source files live in the parent directory.
add_subdirectory(../ProjectNotesCore ProjectNotesCore)

# ── Application target ────────────────────────────────────────────────────────
qt_add_executable(ProjectNotesMobile
    main.cpp
)

# ── iOS asset catalog (app icon) ──────────────────────────────────────────────
# Must be registered with both MACOSX_PACKAGE_LOCATION (so CMake puts it in the
# Resources build phase) and XCODE_EXPLICIT_FILE_TYPE (so Xcode recognizes it
# as an asset catalog and invokes actool on it).
set(IOS_ASSET_CATALOG "${CMAKE_CURRENT_SOURCE_DIR}/ios/Assets.xcassets")
set_source_files_properties("${IOS_ASSET_CATALOG}" PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
    XCODE_EXPLICIT_FILE_TYPE folder.assetcatalog
)
target_sources(ProjectNotesMobile PRIVATE "${IOS_ASSET_CATALOG}")

# ── Qt QML policies ───────────────────────────────────────────────────────────
# QTP0001: use :/qt/qml/ as the default resource prefix (recommended modern default)
# QTP0004: suppress warning about qmldir files for subdirectories; we use the
#          flat resource layout — all QML files are part of one module regardless
#          of source subdirectory.
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

# ── QML module (enables AOT compilation via qmlsc / qmltc in Qt 6.4+) ────────
qt_add_qml_module(ProjectNotesMobile
    URI "ProjectNotesMobile"
    VERSION 1.0
    SOURCES
        AppController.cpp   AppController.h
        MobileSettings.cpp  MobileSettings.h
        TextFormatter.cpp   TextFormatter.h
    QML_FILES
        qml/Main.qml
        qml/pages/ProjectsListPage.qml
        qml/pages/PeoplePage.qml
        qml/pages/ClientsPage.qml
        qml/pages/SyncSettingsPage.qml
        qml/pages/ViewOptionsPage.qml
        qml/pages/PreferencesPage.qml
        qml/pages/AboutPage.qml
        qml/pages/PersonDetailPage.qml
        qml/pages/ClientDetailPage.qml
        qml/pages/AllItemsPage.qml
        qml/pages/ProjectDetailsPage.qml
        qml/pages/DateFieldRow.qml
        qml/pages/StatusItemsPage.qml
        qml/pages/StatusItemDetailPage.qml
        qml/pages/TeamMembersPage.qml
        qml/pages/TeamMemberDetailPage.qml
        qml/pages/ProjectTrackerPage.qml
        qml/pages/ProjectLocationsPage.qml
        qml/pages/ProjectLocationDetailPage.qml
        qml/pages/ProjectNotesPage.qml
        qml/pages/ProjectNoteDetailPage.qml
        qml/pages/TrackerItemDetailPage.qml
        qml/pages/TrackerItemCommentsPage.qml
        qml/pages/TrackerItemCommentDetailPage.qml
        qml/pages/MeetingAttendeesPage.qml
        qml/pages/MeetingAttendeeDetailPage.qml
        qml/pages/NoteActionItemsPage.qml
        qml/pages/DatePickerPopup.qml
        qml/pages/FontPickerPopup.qml
)

# ── iOS bundle properties ─────────────────────────────────────────────────────
set_target_properties(ProjectNotesMobile PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER    "com.kestermckinney.projectnotesmobile"
    MACOSX_BUNDLE_BUNDLE_NAME       "Project Notes"
    MACOSX_BUNDLE_BUNDLE_VERSION    ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    MACOSX_BUNDLE TRUE
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
)

# iOS-specific: set the custom Info.plist
if(IOS OR (APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS"))
    set_target_properties(ProjectNotesMobile PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist"
    )
endif()

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(ProjectNotesMobile PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Xml
    Qt6::Network
    Qt6::Widgets
    ProjectNotesCore
    SqliteSyncProLib
)

# ── Include path (so AppController.h can include core headers) ───────────────
target_include_directories(ProjectNotesMobile PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../   # parent dir — where all shared .h files live
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ProjectNotesMobile)
endif()
