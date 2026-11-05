# tests/desktop — automated tests for the QML (Qt Quick) desktop frontend.
#
# Builds tst_qmldesktop, which reuses the exact DesktopAppController bridge +
# QML module the shipping ProjectNotesDesktop app uses, plus a small headless
# tst_filefinder executable for native reconciliation. The QML test runs two
# groups:
#
#   ParityTest  — drives DesktopAppController's full CRUD/search/filter surface
#                 against a throwaway database, verifying the QML app can do
#                 everything the Widgets app does.
#   UiSmokeTest — loads the real Main.qml headless (offscreen), navigates every
#                 section + detail page, and fails on ANY QML runtime warning.
#
# Enabled with -DBUILD_QML_DESKTOP_TESTS=ON. Build it in its own build dir with
# -DBUILD_QML_DESKTOP=OFF so this is the only target declaring the
# "ProjectNotesDesktop" QML module (avoids a duplicate-URI clash).

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Sql Xml Network Test)
find_package(Qt6 REQUIRED COMPONENTS CorePrivate)  # QZipWriter for sendLogsToSupport
if(UNIX AND NOT APPLE AND NOT ANDROID)
    find_package(Qt6 REQUIRED COMPONENTS DBus)
endif()

find_path(Hunspell_INCLUDE_DIRS NAMES hunspell/hunspell.hxx)
find_library(HUNSPELL_LIBRARIES NAMES hunspell hunspell-1.7)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

set(DESKTOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../ProjectNotesDesktop)
set(MOBILE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../../ProjectNotesMobile)
set(ROOT_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/../..)

qt_add_executable(tst_qmldesktop
    tst_main.cpp
    tst_parity.cpp
    tst_uismoke.cpp
    # Reused verbatim from the shipping desktop app.
    ${DESKTOP_DIR}/DesktopAppController.cpp ${DESKTOP_DIR}/DesktopAppController.h
    ${DESKTOP_DIR}/Office365SettingsModel.cpp ${DESKTOP_DIR}/Office365SettingsModel.h
    ${DESKTOP_DIR}/FolderManager.cpp        ${DESKTOP_DIR}/FolderManager.h
    ${DESKTOP_DIR}/LogViewerController.cpp  ${DESKTOP_DIR}/LogViewerController.h
    ${DESKTOP_DIR}/HelpController.cpp       ${DESKTOP_DIR}/HelpController.h
    ${DESKTOP_DIR}/SpellCheck.cpp           ${DESKTOP_DIR}/SpellCheck.h
    ${DESKTOP_DIR}/PluginUiController.cpp   ${DESKTOP_DIR}/PluginUiController.h
    ${DESKTOP_DIR}/DesktopPluginServices.cpp ${DESKTOP_DIR}/DesktopPluginServices.h
    ${DESKTOP_DIR}/DesktopAppControllerPlugins.cpp
    ${DESKTOP_DIR}/SecurityScopedBookmarks.cpp ${DESKTOP_DIR}/SecurityScopedBookmarks.h
    ${MOBILE_DIR}/TextFormatter.cpp         ${MOBILE_DIR}/TextFormatter.h
    ${ROOT_DIR}/credentialstore.cpp ${ROOT_DIR}/credentialstore.h
    ${ROOT_DIR}/updatemanager.cpp   ${ROOT_DIR}/updatemanager.h
)

target_compile_definitions(tst_qmldesktop PRIVATE PN_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
target_include_directories(tst_qmldesktop PRIVATE
    ${DESKTOP_DIR}
    ${MOBILE_DIR}
)

# ── Re-declare the desktop QML module ──────────────────────────────────────────
# The pages live under ../../ProjectNotesDesktop/qml. Give every file a
# QT_RESOURCE_ALIAS relative to that dir so they land at the SAME resource paths
# the app uses (qrc:/qt/qml/ProjectNotesDesktop/qml/...), which the pages'
# internal `import ProjectNotesDesktop` and font/help lookups depend on.
file(GLOB  DESK_QML_TOP   "${DESKTOP_DIR}/qml/*.qml" "${DESKTOP_DIR}/qml/*.js")
file(GLOB  DESK_QML_PAGES "${DESKTOP_DIR}/qml/pages/*.qml")
set(DESK_QML ${DESK_QML_TOP} ${DESK_QML_PAGES})
foreach(_f ${DESK_QML})
    file(RELATIVE_PATH _rel "${DESKTOP_DIR}" "${_f}")
    set_source_files_properties("${_f}" PROPERTIES QT_RESOURCE_ALIAS "${_rel}")
endforeach()
# Theme.qml and AppShortcuts.qml are QML-file singletons (pragma Singleton) —
# mirrors ProjectNotesDesktop/CMakeLists.txt's own set_source_files_properties
# call. Missing this for AppShortcuts.qml here (while the app's own
# CMakeLists.txt had it) previously left every AppShortcuts.map/.text() lookup
# broken (undefined) specifically in this test target, caught by UiSmokeTest.
set_source_files_properties("${DESKTOP_DIR}/qml/Theme.qml" "${DESKTOP_DIR}/qml/AppShortcuts.qml"
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

set(_font "${DESKTOP_DIR}/resources/MaterialSymbolsRounded.ttf")
set(_ico  "${DESKTOP_DIR}/icons/projectnotes.ico")
set_source_files_properties("${_font}" PROPERTIES QT_RESOURCE_ALIAS "resources/MaterialSymbolsRounded.ttf")
set_source_files_properties("${_ico}"  PROPERTIES QT_RESOURCE_ALIAS "icons/projectnotes.ico")

qt_add_qml_module(tst_qmldesktop
    URI ProjectNotesDesktop
    VERSION 1.0
    QML_FILES ${DESK_QML}
    RESOURCES ${_font} ${_ico}
)

# In-app help content (same tree the app bundles) so the User Guide page renders
# offline without missing-resource warnings.
file(GLOB_RECURSE HELP_MD  "${ROOT_DIR}/docs/*.md")
file(GLOB         HELP_IMG "${ROOT_DIR}/docs/img/*")
set(HELP_TOC "${ROOT_DIR}/docs/help_toc.json")
qt_add_resources(tst_qmldesktop "helpcontent"
    PREFIX "/qt/qml/ProjectNotesDesktop/help"
    BASE   "${ROOT_DIR}/docs"
    FILES  ${HELP_MD} ${HELP_IMG} ${HELP_TOC}
)

qt_add_resources(tst_qmldesktop "plugin_ui_module"
    PREFIX "/qt/qml/ProjectNotes/PluginUI"
    BASE ${DESKTOP_DIR}/pluginui
    FILES ${DESKTOP_DIR}/pluginui/qmldir
)

target_link_libraries(tst_qmldesktop PRIVATE
    Qt6::Core Qt6::CorePrivate Qt6::Gui Qt6::Widgets Qt6::Qml Qt6::Quick Qt6::QuickControls2
    Qt6::Sql Qt6::Xml Qt6::Network Qt6::Test
    "${HUNSPELL_LIBRARIES}"
    SqliteSyncProLib
    ProjectNotesCore
    ProjectNotesFileFinder
    ProjectNotesEmail
    ProjectNotesEmailRendering
    ProjectNotesMeetings
    ProjectNotesContacts
    ProjectNotesPlugins
)

if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(tst_qmldesktop PRIVATE Qt6::DBus)
elseif(APPLE)
    target_link_libraries(tst_qmldesktop PRIVATE "-framework Security")
elseif(WIN32)
    target_link_libraries(tst_qmldesktop PRIVATE Advapi32)
endif()

if(Hunspell_INCLUDE_DIRS AND NOT "${Hunspell_INCLUDE_DIRS}" MATCHES "NOTFOUND")
    target_include_directories(tst_qmldesktop PRIVATE ${Hunspell_INCLUDE_DIRS})
endif()

# Deploy the dictionary next to the test binary so the spell checker behaves
# as in the real app. tst_main points the plugin folders at empty directories.
add_custom_command(TARGET tst_qmldesktop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:tst_qmldesktop>/plugins"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:tst_qmldesktop>/threads"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:tst_qmldesktop>/dictionary"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ROOT_DIR}/dictionary/en_US.aff" "${ROOT_DIR}/dictionary/en_US.dic"
        "$<TARGET_FILE_DIR:tst_qmldesktop>/dictionary/"
    COMMENT "Deploying the dictionary for tst_qmldesktop")

enable_testing()
add_test(NAME qmldesktop COMMAND tst_qmldesktop)
set_tests_properties(qmldesktop PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Release API and installer-link coverage uses a fake network, never the live service.
add_executable(tst_updatemanager
    tst_updatemanager.cpp
    ${ROOT_DIR}/updatemanager.cpp ${ROOT_DIR}/updatemanager.h
    ${ROOT_DIR}/QLogger.cpp ${ROOT_DIR}/QLogger.h
    ${ROOT_DIR}/QLoggerWriter.cpp ${ROOT_DIR}/QLoggerWriter.h
)
target_include_directories(tst_updatemanager PRIVATE ${ROOT_DIR})
target_link_libraries(tst_updatemanager PRIVATE Qt6::Widgets Qt6::Network Qt6::Test)
add_test(NAME updatemanager COMMAND tst_updatemanager)
set_tests_properties(updatemanager PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Fast, headless coverage for the native reconciliation path. This exercises a
# dedicated SQLite connection, the shared lock, active-project filtering,
# legacy-row adoption, batched upsert, and no-op repeat scans.
add_executable(tst_filefinder
    tst_filefinder.cpp
    ${ROOT_DIR}/credentialstore.cpp ${ROOT_DIR}/credentialstore.h
)
target_link_libraries(tst_filefinder PRIVATE
    Qt6::Core Qt6::Sql Qt6::Network Qt6::Test
    ProjectNotesFileFinder ProjectNotesCore
)
if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(tst_filefinder PRIVATE Qt6::DBus)
elseif(APPLE)
    target_link_libraries(tst_filefinder PRIVATE "-framework Security")
elseif(WIN32)
    target_link_libraries(tst_filefinder PRIVATE Advapi32)
endif()
add_test(NAME filefinder COMMAND tst_filefinder)

# The platform credential store, including secrets too long for one Windows
# Credential Manager entry.
add_executable(tst_credentialstore
    tst_credentialstore.cpp
    ${ROOT_DIR}/credentialstore.cpp ${ROOT_DIR}/credentialstore.h
)
target_include_directories(tst_credentialstore PRIVATE ${ROOT_DIR})
target_link_libraries(tst_credentialstore PRIVATE Qt6::Core Qt6::Test)
if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(tst_credentialstore PRIVATE Qt6::DBus)
elseif(APPLE)
    target_link_libraries(tst_credentialstore PRIVATE "-framework Security")
elseif(WIN32)
    target_link_libraries(tst_credentialstore PRIVATE Advapi32)
endif()
add_test(NAME credentialstore COMMAND tst_credentialstore)

# Fast, headless coverage for the open-time Office deep-link builder used by
# both DesktopAppController::openProjectLocation() and the mobile
# AppController's equivalent.
add_executable(tst_officedeeplink tst_officedeeplink.cpp)
target_link_libraries(tst_officedeeplink PRIVATE
    Qt6::Core Qt6::Test
    ProjectNotesCore
)
add_test(NAME officedeeplink COMMAND tst_officedeeplink)

# T01 value-contract coverage. No profile, database, account, renderer, client,
# or process is initialized by this test.
add_executable(tst_communicationcontracts
    tst_communicationcontracts.cpp
    email/fakes/RecordingEmailBackend.h
    email/fakes/DeferredEmailBackend.h
)
target_link_libraries(tst_communicationcontracts PRIVATE
    Qt6::Core Qt6::Test
    ProjectNotesEmail ProjectNotesCore
)
add_test(NAME communicationcontracts COMMAND tst_communicationcontracts)

add_executable(tst_artifactstore tst_artifactstore.cpp)
target_link_libraries(tst_artifactstore PRIVATE Qt6::Core Qt6::Test ProjectNotesEmail)
add_test(NAME artifactstore COMMAND tst_artifactstore)

# QTextDocument/QPdfWriter report renderer. Headless; set PN_RENDER_OUT to keep
# the PDFs for visual review.
add_executable(tst_textdocumentrenderer tst_textdocumentrenderer.cpp)
target_link_libraries(tst_textdocumentrenderer PRIVATE
    Qt6::Core Qt6::Gui Qt6::Test ProjectNotesEmail ProjectNotesEmailRendering)
add_test(NAME textdocumentrenderer COMMAND tst_textdocumentrenderer)
set_tests_properties(textdocumentrenderer PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# XML export/import parity across the RecordTree refactor (golden files).
add_executable(tst_recordtree tst_recordtree.cpp)
target_include_directories(tst_recordtree PRIVATE ${ROOT_DIR})
target_link_libraries(tst_recordtree PRIVATE Qt6::Core Qt6::Sql Qt6::Xml Qt6::Test ProjectNotesCore)
target_compile_definitions(tst_recordtree PRIVATE
    PN_RECORDTREE_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/recordtree")
add_test(NAME recordtree COMMAND tst_recordtree)

# QJSEngine plugin host: manifest, data API, permissions, menus, watchdog,
# install/approval and hot reload, against a throwaway database.
add_executable(tst_pluginhost tst_pluginhost.cpp)
target_include_directories(tst_pluginhost PRIVATE ${ROOT_DIR})
target_link_libraries(tst_pluginhost PRIVATE Qt6::Core Qt6::Sql Qt6::Xml Qt6::Qml Qt6::Network Qt6::Test Qt6::CorePrivate
                      ProjectNotesPlugins ProjectNotesIntegrations ProjectNotesCore)
target_compile_definitions(tst_pluginhost PRIVATE PN_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
                                                PN_REPO_PLUGINS="${ROOT_DIR}/plugins"
                                                PN_REPO_ROOT="${ROOT_DIR}")
add_test(NAME pluginhost COMMAND tst_pluginhost)

add_executable(tst_communicationrepository tst_communicationrepository.cpp)
target_link_libraries(tst_communicationrepository PRIVATE Qt6::Core Qt6::Sql Qt6::Test ProjectNotesEmail)
add_test(NAME communicationrepository COMMAND tst_communicationrepository)

add_executable(tst_microsoftoauth tst_microsoftoauth.cpp)
target_link_libraries(tst_microsoftoauth PRIVATE Qt6::Core Qt6::Network Qt6::Test ProjectNotesIntegrations ProjectNotesCore)
add_test(NAME microsoftoauth COMMAND tst_microsoftoauth)

# Project folders: local, and Microsoft Teams through a scripted Graph.
add_executable(tst_projectfolderstore tst_projectfolderstore.cpp)
target_include_directories(tst_projectfolderstore PRIVATE ${ROOT_DIR})
target_link_libraries(tst_projectfolderstore PRIVATE Qt6::Core Qt6::Network Qt6::Test ProjectNotesIntegrations ProjectNotesCore)
add_test(NAME projectfolderstore COMMAND tst_projectfolderstore)

add_executable(tst_httptransport tst_httptransport.cpp)
target_link_libraries(tst_httptransport PRIVATE Qt6::Core Qt6::Network Qt6::Test ProjectNotesIntegrations)
add_test(NAME httptransport COMMAND tst_httptransport)
set_tests_properties(httptransport PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(tst_contacts tst_contacts.cpp)
target_link_libraries(tst_contacts PRIVATE Qt6::Core Qt6::Sql Qt6::Test ProjectNotesContacts)
add_test(NAME contacts COMMAND tst_contacts)
add_executable(tst_contactproviders tst_contactproviders.cpp)
target_link_libraries(tst_contactproviders PRIVATE Qt6::Core Qt6::Test ProjectNotesContacts)
add_test(NAME contactproviders COMMAND tst_contactproviders)
find_program(NODE_EXECUTABLE node)
if(NODE_EXECUTABLE AND UNIX)
    add_test(NAME thunderbirdcontactsbridge COMMAND ${NODE_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_thunderbird_bridge.js
        $<TARGET_FILE:ProjectNotesThunderbirdContacts>
        ${CMAKE_CURRENT_SOURCE_DIR}/../../ProjectNotesContacts/thunderbird/background.js)
endif()

add_executable(tst_reportformatters tst_reportformatters.cpp)
target_link_libraries(tst_reportformatters PRIVATE Qt6::Core Qt6::Test ProjectNotesEmail)
add_test(NAME reportformatters COMMAND tst_reportformatters)

# Native report-content coverage is deliberately independent from the broader
# communication-contract target, whose adapter cases may be held on a client
# platform.  This target neither initializes a backend nor starts WebEngine.
add_executable(tst_reportbuilders tst_reportbuilders.cpp)
target_link_libraries(tst_reportbuilders PRIVATE Qt6::Core Qt6::Test ProjectNotesEmail)
add_test(NAME reportbuilders COMMAND tst_reportbuilders)

qt_add_executable(tst_projectemail tst_projectemail.cpp)
target_link_libraries(tst_projectemail PRIVATE Qt6::Core Qt6::Network Qt6::Test ProjectNotesEmail)
add_test(NAME projectemail COMMAND tst_projectemail)

# Pure audience selection/model coverage is kept runnable while email-client
# adapter checks are intentionally held.  It has no process, browser, or
# network dependency.
add_executable(tst_audienceresolver tst_audienceresolver.cpp)
target_link_libraries(tst_audienceresolver PRIVATE Qt6::Core Qt6::Test ProjectNotesEmail)
add_test(NAME audienceresolver COMMAND tst_audienceresolver)

# G5 evidence only: exercise Qt's rich-text parser without rendering or loading
# resources. This records what a QTextDocument round trip retains before it can
# be selected as the policy for imported template prose.
add_executable(tst_richtextpolicy tst_richtextpolicy.cpp)
target_link_libraries(tst_richtextpolicy PRIVATE Qt6::Core Qt6::Gui Qt6::Test ProjectNotesEmailRendering)
add_test(NAME richtextpolicy COMMAND tst_richtextpolicy)
set_tests_properties(richtextpolicy PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Native meeting scheduling: iCalendar output, preparation, the calendar file,
# Thunderbird, iCloud and Graph backends, and the temporary-cache cleaner. Every
# launcher, opener and account is a fake; no calendar or invitation is used.
add_executable(tst_meetings tst_meetings.cpp)
target_link_libraries(tst_meetings PRIVATE Qt6::Core Qt6::Gui Qt6::Test ProjectNotesMeetings ProjectNotesIntegrations ProjectNotesCore)
add_test(NAME meetings COMMAND tst_meetings)
