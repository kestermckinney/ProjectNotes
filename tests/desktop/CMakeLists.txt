# tests/desktop — automated tests for the QML (Qt Quick) desktop frontend.
#
# Builds a single test executable, tst_qmldesktop, that reuses the exact
# DesktopAppController bridge + QML module the shipping ProjectNotesDesktop app
# uses (no production code is modified). It runs two groups:
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
file(GLOB  DESK_QML_TOP   "${DESKTOP_DIR}/qml/*.qml")
file(GLOB  DESK_QML_PAGES "${DESKTOP_DIR}/qml/pages/*.qml")
set(DESK_QML ${DESK_QML_TOP} ${DESK_QML_PAGES})
foreach(_f ${DESK_QML})
    file(RELATIVE_PATH _rel "${DESKTOP_DIR}" "${_f}")
    set_source_files_properties("${_f}" PROPERTIES QT_RESOURCE_ALIAS "${_rel}")
endforeach()
set_source_files_properties("${DESKTOP_DIR}/qml/Theme.qml"
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

set(_font "${DESKTOP_DIR}/resources/MaterialSymbolsRounded.woff2")
set(_ico  "${DESKTOP_DIR}/icons/projectnotes.ico")
set_source_files_properties("${_font}" PROPERTIES QT_RESOURCE_ALIAS "resources/MaterialSymbolsRounded.woff2")
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
