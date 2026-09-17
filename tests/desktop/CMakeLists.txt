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
    ${MOBILE_DIR}/TextFormatter.cpp         ${MOBILE_DIR}/TextFormatter.h
    ${ROOT_DIR}/plugin.cpp          ${ROOT_DIR}/plugin.h
    ${ROOT_DIR}/pluginmanager.cpp   ${ROOT_DIR}/pluginmanager.h
    ${ROOT_DIR}/pythonworker.cpp    ${ROOT_DIR}/pythonworker.h
    ${ROOT_DIR}/credentialstore.cpp ${ROOT_DIR}/credentialstore.h
    ${ROOT_DIR}/updatemanager.cpp   ${ROOT_DIR}/updatemanager.h
)

target_include_directories(tst_qmldesktop PRIVATE
    ${DESKTOP_DIR}
    ${MOBILE_DIR}
    ${Python3_INCLUDE_DIRS}
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

target_link_libraries(tst_qmldesktop PRIVATE
    Qt6::Core Qt6::CorePrivate Qt6::Gui Qt6::Widgets Qt6::Qml Qt6::Quick Qt6::QuickControls2
    Qt6::Sql Qt6::Xml Qt6::Network Qt6::Test
    "${HUNSPELL_LIBRARIES}"
    "${Python3_LIBRARIES}"
    SqliteSyncProLib
    ProjectNotesCore
    ProjectNotesFileFinder
    ProjectNotesEmail
    ProjectNotesEmailRendering
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

# Deploy dictionary + plugins/threads next to the test binary so the embedded
# Python plugin engine and spell checker behave exactly as in the real app.
add_custom_command(TARGET tst_qmldesktop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:tst_qmldesktop>/plugins"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:tst_qmldesktop>/threads"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:tst_qmldesktop>/dictionary"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ROOT_DIR}/dictionary/en_US.aff" "${ROOT_DIR}/dictionary/en_US.dic"
        "$<TARGET_FILE_DIR:tst_qmldesktop>/dictionary/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ROOT_DIR}/plugins" "$<TARGET_FILE_DIR:tst_qmldesktop>/plugins"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ROOT_DIR}/threads" "$<TARGET_FILE_DIR:tst_qmldesktop>/threads"
    COMMENT "Deploying dictionary + plugins/threads for tst_qmldesktop")

enable_testing()
add_test(NAME qmldesktop COMMAND tst_qmldesktop)
set_tests_properties(qmldesktop PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Fast, headless coverage for the native reconciliation path. This exercises a
# dedicated SQLite connection, the shared lock, active-project filtering,
# legacy-row adoption, batched upsert, and no-op repeat scans.
add_executable(tst_filefinder
    tst_filefinder.cpp
    ${ROOT_DIR}/credentialstore.cpp ${ROOT_DIR}/credentialstore.h
)
target_link_libraries(tst_filefinder PRIVATE
    Qt6::Core Qt6::Sql Qt6::Network Qt6::Test
    ProjectNotesFileFinder
)
if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(tst_filefinder PRIVATE Qt6::DBus)
elseif(APPLE)
    target_link_libraries(tst_filefinder PRIVATE "-framework Security")
elseif(WIN32)
    target_link_libraries(tst_filefinder PRIVATE Advapi32)
endif()
add_test(NAME filefinder COMMAND tst_filefinder)

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

# This is intentionally a real WebEngine process test, separate from the
# headless contract suite.  Some container runtimes prohibit Chromium helper
# shutdown even when its sandbox is disabled, so maintainers opt in only on a
# native/package-like host where it can be valid evidence for G4.
option(ENABLE_WEBENGINE_RENDERER_TESTS "Register native Qt WebEngine renderer tests" OFF)
if(ENABLE_WEBENGINE_RENDERER_TESTS)
    add_executable(tst_webenginerenderer tst_webenginerenderer.cpp)
    target_link_libraries(tst_webenginerenderer PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test Qt6::WebEngineCore Qt6::WebEngineQuick
        ProjectNotesEmail ProjectNotesEmailRendering)
    add_test(NAME webenginerenderer COMMAND tst_webenginerenderer)
    set_tests_properties(webenginerenderer PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;QTWEBENGINE_DISABLE_SANDBOX=1;QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu")
endif()

add_executable(tst_communicationrepository tst_communicationrepository.cpp)
target_link_libraries(tst_communicationrepository PRIVATE Qt6::Core Qt6::Sql Qt6::Test ProjectNotesEmail)
add_test(NAME communicationrepository COMMAND tst_communicationrepository)

add_executable(tst_microsoftoauth tst_microsoftoauth.cpp)
target_link_libraries(tst_microsoftoauth PRIVATE Qt6::Core Qt6::Network Qt6::Test ProjectNotesIntegrations)
add_test(NAME microsoftoauth COMMAND tst_microsoftoauth)

add_executable(tst_httptransport tst_httptransport.cpp)
target_link_libraries(tst_httptransport PRIVATE Qt6::Core Qt6::Network Qt6::Test ProjectNotesIntegrations)
add_test(NAME httptransport COMMAND tst_httptransport)

add_executable(tst_reportformatters tst_reportformatters.cpp)
target_link_libraries(tst_reportformatters PRIVATE Qt6::Core Qt6::Test ProjectNotesEmail)
add_test(NAME reportformatters COMMAND tst_reportformatters)

# Native report-content coverage is deliberately independent from the broader
# communication-contract target, whose adapter cases may be held on a client
# platform.  This target neither initializes a backend nor starts WebEngine.
add_executable(tst_reportbuilders tst_reportbuilders.cpp)
target_link_libraries(tst_reportbuilders PRIVATE Qt6::Core Qt6::Test ProjectNotesEmail)
add_test(NAME reportbuilders COMMAND tst_reportbuilders)

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
