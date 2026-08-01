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

qt_add_executable(ProjectNotesDesktop
    main.cpp
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
)

# So main.cpp can #include "TextFormatter.h"; Python headers for the plugin engine.
target_include_directories(ProjectNotesDesktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../ProjectNotesMobile
    ${Python3_INCLUDE_DIRS}
)

# ── QML module ────────────────────────────────────────────────────────────────
set(DESKTOP_QML_FILES
    qml/Main.qml
    qml/Theme.qml
    qml/MaterialIcon.qml
    qml/IconRail.qml
    qml/ProjectSidebar.qml
    qml/FolderGroup.qml
    qml/TopBar.qml
    qml/AppMenu.qml
    qml/LogViewerWindow.qml
    qml/HelpPage.qml
    qml/FilterDialog.qml
    qml/RecordContextMenu.qml
    qml/RecordRowMenu.qml
    qml/FindReplaceBar.qml
    qml/KebabButton.qml
    qml/Card.qml
    qml/FormField.qml
    qml/ComboField.qml
    qml/DateField.qml
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

# Theme.qml is a QML-file singleton (pragma Singleton).
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(ProjectNotesDesktop
    URI ProjectNotesDesktop
    VERSION 1.0
    QML_FILES ${DESKTOP_QML_FILES}
    RESOURCES
        resources/MaterialSymbolsRounded.woff2
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
qt_add_resources(ProjectNotesDesktop "helpcontent"
    PREFIX "/qt/qml/ProjectNotesDesktop/help"
    BASE   "${CMAKE_CURRENT_SOURCE_DIR}/../docs"
    FILES  ${HELP_MD} ${HELP_IMG} ${HELP_TOC}
)

# ── Shared data layer ─────────────────────────────────────────────────────────
# ProjectNotesCore is added once at the top level (it is a sibling target).
target_link_libraries(ProjectNotesDesktop PRIVATE
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
    target_link_libraries(ProjectNotesDesktop PRIVATE Qt6::DBus)
elseif(APPLE)
    target_link_libraries(ProjectNotesDesktop PRIVATE "-framework Security")
elseif(WIN32)
    target_link_libraries(ProjectNotesDesktop PRIVATE Advapi32)
endif()

if(Hunspell_INCLUDE_DIRS AND NOT "${Hunspell_INCLUDE_DIRS}" MATCHES "NOTFOUND")
    target_include_directories(ProjectNotesDesktop PRIVATE ${Hunspell_INCLUDE_DIRS})
endif()

# Deploy the en_US dictionary next to the executable so DesktopSpellChecker can
# load it at runtime (appDir/dictionary/en_US.{aff,dic}).
add_custom_command(TARGET ProjectNotesDesktop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ProjectNotesDesktop>/dictionary"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../dictionary/en_US.aff"
        "${CMAKE_CURRENT_SOURCE_DIR}/../dictionary/en_US.dic"
        "$<TARGET_FILE_DIR:ProjectNotesDesktop>/dictionary/"
    COMMENT "Deploying en_US spell-check dictionary")

# Deploy the Python plugins + threads next to the executable so the embedded
# interpreter (PluginManager, which reads appDir/plugins and appDir/threads)
# can load them at runtime.
add_custom_command(TARGET ProjectNotesDesktop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../plugins"
        "$<TARGET_FILE_DIR:ProjectNotesDesktop>/plugins"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../threads"
        "$<TARGET_FILE_DIR:ProjectNotesDesktop>/threads"
    COMMENT "Deploying Python plugins and threads")

# The runtime output goes to the shared build root (inherited setting), where a
# build subdirectory named "ProjectNotesDesktop" already exists — so give the
# executable a distinct filename to avoid a name collision at link time.
set_target_properties(ProjectNotesDesktop PROPERTIES OUTPUT_NAME "ProjectNotesQml")

# ── Windows app icon ──────────────────────────────────────────────────────────
if(WIN32)
    set_target_properties(ProjectNotesDesktop PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
