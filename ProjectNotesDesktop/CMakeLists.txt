# ProjectNotesDesktop — QML (Qt Quick Controls 2) desktop frontend.
#
# Phase 1 of the UI overhaul. Runs alongside the existing Widgets "ProjectNotes"
# target and reuses the shared ProjectNotesCore data layer via a lean QML bridge
# (DesktopAppController) plus the new project-folders feature (FolderManager).
#
# Added from the top-level CMakeLists via add_subdirectory, guarded by the
# BUILD_QML_DESKTOP option so it never disturbs the Widgets build.

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Sql Xml Network)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Sql Xml Network)
# CorePrivate exposes QtCore's private QZipWriter (qzipwriter_p.h), used by
# DesktopAppController::sendLogsToSupport to build the support-log archive.
find_package(Qt6 REQUIRED COMPONENTS CorePrivate)
# DBus backs the Linux credential store used by the plugin secret API.
if(UNIX AND NOT APPLE AND NOT ANDROID)
    find_package(Qt6 REQUIRED COMPONENTS DBus)
endif()

# Hunspell — for inline spell-check in the note editor.
find_path(Hunspell_INCLUDE_DIRS NAMES hunspell/hunspell.hxx)
find_library(HUNSPELL_LIBRARIES NAMES hunspell hunspell-1.7)

qt_standard_project_setup(REQUIRES 6.5)

# Modern resource layout: QML lives under qrc:/qt/qml/ProjectNotesDesktop/
qt_policy(SET QTP0001 NEW)
# Flat module layout — .qml files under qml/pages/ belong to the same module,
# no per-directory qmldir needed.
qt_policy(SET QTP0004 NEW)

if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc")
endif()

qt_add_executable(ProjectNotes
    main.cpp
    ${APP_ICON_RESOURCE_WINDOWS}
    DesktopAppController.cpp DesktopAppController.h
    FolderManager.cpp        FolderManager.h
    LogViewerController.cpp  LogViewerController.h
    HelpController.cpp       HelpController.h
    SpellCheck.cpp           SpellCheck.h
    # Shared rich-text bridge — reused verbatim from the mobile app.
    ../ProjectNotesMobile/TextFormatter.cpp ../ProjectNotesMobile/TextFormatter.h
    # Embedded-Python plugin system — same sources the Widgets app compiles.
    # PluginManager was decoupled from MainWindow (self-registered singleton) so
    # it builds here too. credentialstore is pulled in by the plugin secret API.
    ../plugin.cpp          ../plugin.h
    ../pluginmanager.cpp   ../pluginmanager.h
    ../pythonworker.cpp    ../pythonworker.h
    ../credentialstore.cpp ../credentialstore.h
    # Unattended software update (GitHub releases → silent install + relaunch).
    ../updatemanager.cpp   ../updatemanager.h
    # Single-instance guard — same class the Widgets app uses, own UUID key
    # (see main.cpp) so the two apps' guards don't interfere with each other.
    ../runguard.cpp ../runguard.h
)

# So main.cpp can #include "TextFormatter.h"; Python headers for the plugin engine.
# CMAKE_CURRENT_SOURCE_DIR is also required so the Windows resource compiler can
# resolve the relative "icons/projectnotes.ico" path in appicon.rc — rc.exe looks
# up resource files on its include path, and this is the dir that contains icons/.
target_include_directories(ProjectNotes PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../ProjectNotesMobile
    ${Python3_INCLUDE_DIRS}
)

# ── QML module ────────────────────────────────────────────────────────────────
set(DESKTOP_QML_FILES
    qml/Main.qml
    qml/Theme.qml
    qml/AppShortcuts.qml
    qml/MaterialIcon.qml
    qml/MaterialIconCodepoints.js
    qml/IconRail.qml
    qml/ProjectSidebar.qml
    qml/FolderGroup.qml
    qml/TopBar.qml
    qml/Button.qml
    qml/AppMenu.qml
    qml/MenuRow.qml
    qml/MenuFlyout.qml
    qml/ClickShield.qml
    qml/MenuScrollView.qml
    qml/ZoomMenuRow.qml
    qml/LogViewerWindow.qml
    qml/HelpPage.qml
    qml/FilterDialog.qml
    qml/SortMenu.qml
    qml/RecordContextMenu.qml
    qml/RecordRowMenu.qml
    qml/MoveReviewDialog.qml
    qml/MoveToDialog.qml
    qml/MoveToFolderDialog.qml
    qml/FindReplaceBar.qml
    qml/KebabButton.qml
    qml/Card.qml
    qml/FormField.qml
    qml/ComboField.qml
    qml/DateField.qml
    qml/PeoplePickerDialog.qml
    qml/NoteFormatToolbar.qml
    qml/SpellCheckField.qml
    qml/SpellSquiggle.qml
    qml/SpellCheckDialog.qml
    qml/pages/ProjectsListPage.qml
    qml/pages/ProjectDetailPage.qml
    qml/pages/ProjectNoteDetailPage.qml
    qml/pages/PeoplePage.qml
    qml/pages/PersonDetailPage.qml
    qml/pages/ClientsPage.qml
    qml/pages/ClientDetailPage.qml
    qml/pages/ItemsPage.qml
    qml/pages/ItemDetailPage.qml
    qml/pages/SearchPage.qml
    qml/pages/SettingsPage.qml
    qml/pages/StubPage.qml
)

# Theme.qml and AppShortcuts.qml are QML-file singletons (pragma Singleton).
set_source_files_properties(qml/Theme.qml qml/AppShortcuts.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(ProjectNotes
    URI ProjectNotesDesktop
    VERSION 1.0
    QML_FILES ${DESKTOP_QML_FILES}
    RESOURCES
        resources/MaterialSymbolsRounded.ttf
        icons/projectnotes.ico
)

# ── In-app help content ───────────────────────────────────────────────────────
# Bundle the shared Markdown docs (the same tree that builds the mkdocs website)
# so the QML User Guide (HelpController + HelpPage.qml) renders them offline.
# BASE = ../docs makes each file land at
#   qrc:/qt/qml/ProjectNotesDesktop/help/<path-relative-to-docs>
# CONFIGURE_DEPENDS re-globs when files are added/removed on the next build.
file(GLOB_RECURSE HELP_MD  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../docs/*.md")
file(GLOB         HELP_IMG CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../docs/img/*")
set(HELP_TOC "${CMAKE_CURRENT_SOURCE_DIR}/../docs/help_toc.json")
qt_add_resources(ProjectNotes "helpcontent"
    PREFIX "/qt/qml/ProjectNotesDesktop/help"
    BASE   "${CMAKE_CURRENT_SOURCE_DIR}/../docs"
    FILES  ${HELP_MD} ${HELP_IMG} ${HELP_TOC}
)

# ── Shared data layer ─────────────────────────────────────────────────────────
# ProjectNotesCore is added once at the top level (it is a sibling target).
target_link_libraries(ProjectNotes PRIVATE
    Qt6::Core
    Qt6::CorePrivate
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Xml
    Qt6::Network
    "${HUNSPELL_LIBRARIES}"
    "${Python3_LIBRARIES}"
    SqliteSyncProLib
    ProjectNotesCore
)

# Platform credential backend used by the plugin secret API (credentialstore).
if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(ProjectNotes PRIVATE Qt6::DBus)
elseif(APPLE)
    target_link_libraries(ProjectNotes PRIVATE "-framework Security")
elseif(WIN32)
    # Advapi32: credentialstore (DPAPI). Shell32: SetCurrentProcessExplicitAppUserModelID
    # in main.cpp, which gives this build its own taskbar/Task Manager identity.
    target_link_libraries(ProjectNotes PRIVATE Advapi32 Shell32)
endif()

if(Hunspell_INCLUDE_DIRS AND NOT "${Hunspell_INCLUDE_DIRS}" MATCHES "NOTFOUND")
    target_include_directories(ProjectNotes PRIVATE ${Hunspell_INCLUDE_DIRS})
endif()

if(APPLE AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    # ── macOS Release: full app bundle — real files + Qt/Python deployment ─────
    # Mirrors the Widgets ProjectNotes target's macOS Release deploy steps (see
    # top-level CMakeLists.txt) so "Project Notes.app" is self-contained and
    # notarizable. Symlinks (used below for Debug/Win/Linux dev convenience) are
    # rejected by notarization, so plugins/threads/dictionary are real copies here.
    add_custom_target(sync_plugins_desktop
        COMMAND rsync -rv --checksum
            "${CMAKE_CURRENT_SOURCE_DIR}/../plugins/"
            "$<TARGET_BUNDLE_DIR:ProjectNotes>/Contents/Resources/plugins/"
        COMMAND rsync -rv --checksum
            "${CMAKE_CURRENT_SOURCE_DIR}/../threads/"
            "$<TARGET_BUNDLE_DIR:ProjectNotes>/Contents/Resources/threads/"
        COMMENT "Syncing Python plugins to Project Notes (QML) app bundle (changed files only)"
        VERBATIM)

    # The legacy Widgets target owns the plain "sync_plugins" name, but it is off
    # by default now. Keep that name working — it is the one documented in
    # AGENTS.md/CLAUDE.md and wired into Qt Creator build steps — by aliasing it
    # to this target whenever the legacy frontend is not being built.
    if(NOT BUILD_WIDGETS_LEGACY)
        add_custom_target(sync_plugins DEPENDS sync_plugins_desktop)
    endif()

    set(DESKTOP_BUNDLE_RESOURCES  "$<TARGET_FILE_DIR:ProjectNotes>/../Resources")
    set(DESKTOP_BUNDLE_FRAMEWORKS "$<TARGET_FILE_DIR:ProjectNotes>/../Frameworks")

    # Python3_ROOT_DIR is set (APPLE-only) at the top of the top-level
    # CMakeLists.txt, before this subdirectory is added, so it is inherited here.
    get_filename_component(_desktop_python_framework_root "${Python3_ROOT_DIR}/../.." REALPATH)

    get_target_property(_desktop_qmake_loc Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_desktop_qt_bin "${_desktop_qmake_loc}" DIRECTORY)

    # ── 1. Copy plugins (real files) ────────────────────────────────────────────
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${DESKTOP_BUNDLE_RESOURCES}/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../plugins"
            "${DESKTOP_BUNDLE_RESOURCES}/plugins"
        COMMENT "Copying plugins into bundle Resources")

    # ── 2. Copy threads ─────────────────────────────────────────────────────────
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${DESKTOP_BUNDLE_RESOURCES}/threads"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../threads"
            "${DESKTOP_BUNDLE_RESOURCES}/threads"
        COMMENT "Copying threads into bundle Resources")

    # ── 3. Copy dictionary ──────────────────────────────────────────────────────
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${DESKTOP_BUNDLE_RESOURCES}/dictionary"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../dictionary"
            "${DESKTOP_BUNDLE_RESOURCES}/dictionary"
        COMMENT "Copying dictionary into bundle Resources")

    # ── 4. Deploy Qt frameworks into the bundle ─────────────────────────────────
    # -qmldir points macdeployqt's qmlimportscanner at the QML sources so it also
    # deploys the QML modules this app imports (QtQuick.Controls.Basic, .Layouts,
    # .Dialogs, etc.) into Contents/Resources/qml — without it, only the
    # directly-linked Qt frameworks are copied and the app fails to load its UI
    # at runtime ("module ... is not installed").
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND "${_desktop_qt_bin}/macdeployqt"
            "$<TARGET_BUNDLE_DIR:ProjectNotes>"
            "-qmldir=${CMAKE_CURRENT_SOURCE_DIR}/qml"
            -always-overwrite
        COMMENT "Deploying Qt into app bundle (macdeployqt)"
        VERBATIM)

    # ── 5. Bundle Python.framework ───────────────────────────────────────────────
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DESKTOP_BUNDLE_FRAMEWORKS}"
        COMMAND ${CMAKE_COMMAND}
            "-DPYTHON_FRAMEWORK_SRC=${_desktop_python_framework_root}"
            "-DFRAMEWORKS=${DESKTOP_BUNDLE_FRAMEWORKS}"
            "-DEXECUTABLE=$<TARGET_FILE:ProjectNotes>"
            "-DPYTHON_VERSION=${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../packaging/macos/BundlePythonFramework.cmake"
        COMMENT "Bundling Python.framework into Contents/Frameworks"
        VERBATIM)
else()
    # ── macOS Debug / Windows / Linux: no bundle deployment — symlink resources
    # for fast dev iteration (macOS Debug skips the bundle entirely, see the
    # MACOSX_BUNDLE property below).
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ProjectNotes>/dictionary"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../dictionary/en_US.aff"
            "${CMAKE_CURRENT_SOURCE_DIR}/../dictionary/en_US.dic"
            "$<TARGET_FILE_DIR:ProjectNotes>/dictionary/"
        COMMENT "Deploying en_US spell-check dictionary")

    # Deploy the Python plugins + threads next to the executable so the embedded
    # interpreter (PluginManager, which reads appDir/plugins and appDir/threads)
    # can load them at runtime.
    # Use symbolic links (dev convenience) instead of copying directories.
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_CURRENT_SOURCE_DIR}/../plugins"
            "$<TARGET_FILE_DIR:ProjectNotes>/plugins"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_CURRENT_SOURCE_DIR}/../threads"
            "$<TARGET_FILE_DIR:ProjectNotes>/threads"
        COMMENT "Symlinking Python plugins and threads")
endif()

# Ship under the same executable name as the Widgets app ("Project Notes.exe")
# so the built binary, installer shortcuts, and registry entries all line up.
# The Widgets ProjectNotes target also emits "Project Notes.exe", so pin this
# target's output to its own build subdirectory (CMAKE_CURRENT_BINARY_DIR =
# .../ProjectNotesDesktop) to guarantee the two never clobber each other,
# regardless of any inherited CMAKE_RUNTIME_OUTPUT_DIRECTORY.
set_target_properties(ProjectNotes PROPERTIES
    OUTPUT_NAME "Project Notes"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

if(UNIX AND NOT APPLE AND NOT ANDROID)
    # Linux/Flatpak: the .desktop Exec and the Flatpak manifest invoke
    # "projectnotes" (no space), matching the legacy Widgets target's Linux
    # binary name — see the equivalent override on ProjectNotesWidgets in the
    # top-level CMakeLists.txt.
    set_target_properties(ProjectNotes PROPERTIES OUTPUT_NAME "projectnotes")
endif()

# ── Windows app icon ──────────────────────────────────────────────────────────
if(WIN32)
    set_target_properties(ProjectNotes PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ── macOS bundle identity ─────────────────────────────────────────────────────
# Reuses the Widgets app's bundle identifier/name so this build replaces it in
# place: same /Applications/Project Notes.app, same LaunchServices identity, so
# packaging/macos/build_installer.sh (which pkgbuilds with the identifier below)
# upgrades existing installs cleanly instead of installing a second app.
# The icon, however, is NOT shared with the Widgets app — icons/projectnotes.icns
# is generated from icons/projectnotes.ico (the QML app's own redesigned icon,
# added in 5c72ee5 and already used for the Windows build via appicon.rc). The
# .ico's largest embedded frame is 256x256, so the .icns's 512/1024 sizes are
# upscaled from that; regenerate from a higher-res source if a crisper large
# icon is ever needed (Finder Info window, Launchpad).
# Debug builds skip the bundle entirely, matching the Widgets app's macOS Debug
# behavior (see the ProjectNotes target's MACOSX_BUNDLE in the top-level
# CMakeLists.txt) — faster build/run cycle, no code signing needed.
if(APPLE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_desktop_macos_bundle FALSE)
    else()
        set(_desktop_macos_bundle TRUE)
    endif()

    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/icons/projectnotes.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(ProjectNotes PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/icons/projectnotes.icns")

    set_target_properties(ProjectNotes PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.projectnotespro.ProjectNotes"
        MACOSX_BUNDLE_BUNDLE_NAME "Project Notes"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE projectnotes.icns
        MACOSX_BUNDLE ${_desktop_macos_bundle})
endif()
