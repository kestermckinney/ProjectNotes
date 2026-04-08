cmake_minimum_required(VERSION 3.16)

project(ProjectNotes VERSION 5.0.1 LANGUAGES CXX)

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

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Sql Xml Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Sql Xml Network)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENV{QT_LOGGING_RULES} "qt.*.debug=true")
endif()

if(APPLE)
    set(Python3_ROOT_DIR "/Library/Frameworks/Python.framework/Versions/3.14" CACHE PATH "Python 3.13 root (python.org install)")
endif()

if(WIN32)
    find_package(Python3 3.13 REQUIRED COMPONENTS Interpreter Development)
elseif(APPLE)
    find_package(Python3 3.13 REQUIRED COMPONENTS Interpreter Development)
else()
    find_package(Python3 3.14 REQUIRED COMPONENTS Interpreter Development)
endif()

# Ensure we use the release libraries even in debug mode
set(Python3_LIBRARIES ${Python3_LIBRARY_RELEASE})
set(Python3_LIBRARY_DIRS ${Python3_LIBRARY_DIRS_RELEASE})


# Find the Hunspell library
find_path(Hunspell_INCLUDE_DIRS NAMES hunspell/hunspell.hxx)
find_library(HUNSPELL_LIBRARIES NAMES hunspell hunspell-1.7)

message(STATUS "Hunspell includes: ${Hunspell_INCLUDE_DIRS}")
message(STATUS "Hunspell library: ${HUNSPELL_LIBRARIES}")

include_directories(${Hunspell_INCLUDE_DIRS})

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp 
    aboutdialog.cpp
    filterdatadialog.cpp 
    findreplacedialog.cpp 

    preferencesdialog.cpp
    spellcheckdialog.cpp
    aboutdialog.ui 
    filterdatadialog.ui 
    findreplacedialog.ui 
    mainwindow.ui 

    preferencesdialog.ui 
    spellcheckdialog.ui
    Resources.qrc
)

if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc")
elseif(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/AppIcon.icns")
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ProjectNotes
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${APP_ICON_RESOURCE_WINDOWS}
        ${APP_ICON_MACOS}
        actionitemprojectnotesmodel.cpp actionitemsdetailsmeetingsmodel.h actionitemsdetailsmeetingsmodel.cpp actionitemprojectnotesmodel.h clientslistview.cpp clientslistview.h clientsmodel.cpp clientsmodel.h clientspage.cpp clientspage.h columnview.cpp columnview.h comboboxdelegate.cpp comboboxdelegate.h databasestructure.cpp databasestructure.h databasecreate.cpp databasecreate.h databaseviews.cpp databaseviews.h databaseupgrade_v1_0_0.cpp databaseupgrade_v1_0_0.h databaseupgrade_v1_2_0.cpp databaseupgrade_v1_2_0.h databaseupgrade_v5_0_0.cpp databaseupgrade_v5_0_0.h     filterdatadialog.cpp filterdatadialog.h filterdatadialog.ui filtersavestructure.h   itemdetailsdelegate.cpp itemdetailsdelegate.h itemdetailspage.cpp itemdetailspage.h itemdetailteamlistmodel.cpp itemdetailteamlistmodel.h meetingattendeesmodel.cpp meetingattendeesmodel.h meetingattendeesview.cpp meetingattendeesview.h notesactionitemsmodel.cpp notesactionitemsmodel.h notesactionitemsview.cpp notesactionitemsview.h peoplelistview.cpp peoplelistview.h peoplemodel.cpp peoplemodel.h peoplepage.cpp peoplepage.h basepage.cpp basepage.h checkboxdelegate.cpp checkboxdelegate.h columnmodel.cpp columnmodel.h combobox.cpp combobox.h sqlcomboboxdelegate.cpp sqlcomboboxdelegate.h databaseobjects.cpp databaseobjects.h dateeditdelegate.cpp dateeditdelegate.h dateeditex.cpp dateeditex.h inlinespellchecker.cpp inlinespellchecker.h lineeditfilebutton.cpp lineeditfilebutton.h lineeditfilebuttondelegate.cpp lineeditfilebuttondelegate.h plaintexteditdelegate.h plaintextedit.h plaintexteditdelegate.cpp plaintextedit.cpp
        appsettings.cpp appsettings.h sortfilterproxymodel.cpp sortfilterproxymodel.h spellchecker.cpp spellchecker.h sqlquerymodel.cpp sqlquerymodel.h tableview.cpp tableview.h textedit.cpp textedit.h texteditdelegate.cpp texteditdelegate.h projectdetailsdelegate.cpp projectdetailsdelegate.h projectdetailspage.cpp projectdetailspage.h projectlocationsmodel.cpp projectlocationsmodel.h projectlocationsview.cpp projectlocationsview.h projectnotesdelegate.cpp projectnotesdelegate.h projectnotesmodel.cpp projectnotesmodel.h projectnotespage.cpp projectnotespage.h projectnotesview.cpp projectnotesview.h projectsmodel.h projectsmodel.cpp projectslistview.h projectslistview.cpp projectslistpage.h projectslistpage.cpp projectslistmodel.h projectslistmodel.cpp projectteammembersmodel.cpp projectteammembersmodel.h projectteammembersview.cpp projectteammembersview.h runguard.cpp runguard.h searchpage.cpp searchpage.h searchresultsmodel.cpp searchresultsmodel.h searchresultsview.cpp searchresultsview.h statusreportitemsmodel.cpp statusreportitemsmodel.h statusreportitemsview.cpp statusreportitemsview.h teamsmodel.cpp teamsmodel.h trackeritemcommentsmodel.cpp trackeritemcommentsmodel.h trackeritemcommentsview.cpp trackeritemcommentsview.h trackeritemsmodel.cpp trackeritemsmodel.h trackeritemsview.cpp trackeritemsview.h valueselectmodel.cpp valuesview.h valuesview.cpp valueselectmodel.h
        combobox.cpp combobox.h
        allitemsview.cpp allitemsview.h allitemspage.cpp allitemspage.h
        spellcheckdialog.cpp spellcheckdialog.h spellcheckdialog.ui
        cloudsyncsettingsdialog.cpp cloudsyncsettingsdialog.h cloudsyncsettingsdialog.ui
        plugin.cpp plugin.h pluginmanager.cpp pluginmanager.h
        vcardparser.h
        logviewer.cpp logviewer.h
        pythonworker.cpp pythonworker.h QLogger.cpp QLogger.h QLoggerLevel.h QLoggerWriter.cpp QLoggerWriter.h
        pythonworker.h

    )

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ProjectNotes APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ProjectNotes SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ProjectNotes
            ${PROJECT_SOURCES}
        )
    endif()
endif()

if(APPLE)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/AppIcon.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

# see what libraries we are really using for python
message(STATUS "Python3 executable: ${Python3_EXECUTABLE}")
message(STATUS "Python3 library: ${Python3_LIBRARIES}")
message(STATUS "Python3 include dir: ${Python3_INCLUDE_DIRS}")

# include directories for Python
include_directories(${Python3_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})

# Include the source directory in the include directories for the executable
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(../SqliteSyncPro/src SqliteSyncPro)

target_link_libraries(ProjectNotes PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${Python3_LIBRARIES} Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Network "${HUNSPELL_LIBRARIES}" SqliteSyncProLib)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(ProjectNotes PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.kestermckinney.projectnotes"
    MACOSX_BUNDLE_BUNDLE_NAME "Project Notes"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

set_target_properties(ProjectNotes PROPERTIES OUTPUT_NAME "Project Notes")

include(GNUInstallDirs)
install(TARGETS ProjectNotes
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ProjectNotes)
endif()

if(APPLE)
    # ── Fast plugin-only sync target ───────────────────────────────────────────
    # Build just this target (instead of the full ProjectNotes target) when only
    # .py files changed.  In Qt Creator: right-click project → Build → sync_plugins,
    # or add a custom Build Step with "Make arguments: sync_plugins".
    # Uses rsync --checksum so only content-changed files are actually copied.
    add_custom_target(sync_plugins
        COMMAND rsync -rv --checksum
            "${CMAKE_CURRENT_SOURCE_DIR}/plugins/"
            "$<TARGET_BUNDLE_DIR:ProjectNotes>/Contents/Resources/plugins/"
        COMMAND rsync -rv --checksum
            "${CMAKE_CURRENT_SOURCE_DIR}/threads/"
            "$<TARGET_BUNDLE_DIR:ProjectNotes>/Contents/Resources/threads/"
        COMMENT "Syncing Python plugins to app bundle (changed files only)"
        VERBATIM
    )

    # $<TARGET_FILE_DIR:ProjectNotes> resolves to .app/Contents/MacOS at build time.
    set(BUNDLE_RESOURCES   "$<TARGET_FILE_DIR:ProjectNotes>/../Resources")
    set(BUNDLE_FRAMEWORKS  "$<TARGET_FILE_DIR:ProjectNotes>/../Frameworks")

    # Compute the Python.framework root from the version-specific root dir.
    # Python3_ROOT_DIR = /Library/Frameworks/Python.framework/Versions/X.Y
    get_filename_component(_python_framework_root "${Python3_ROOT_DIR}/../.." REALPATH)

    # Locate macdeployqt next to qmake.
    get_target_property(_qmake_loc Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qmake_loc}" DIRECTORY)

    # ── 1. Copy plugins (real files — symlinks are rejected by notarization) ──
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${BUNDLE_RESOURCES}/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/plugins"
            "${BUNDLE_RESOURCES}/plugins"
        COMMENT "Copying plugins into bundle Resources"
    )

    # ── 2. Copy threads ────────────────────────────────────────────────────────
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${BUNDLE_RESOURCES}/threads"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/threads"
            "${BUNDLE_RESOURCES}/threads"
        COMMENT "Copying threads into bundle Resources"
    )

    # ── 3. Copy dictionary ─────────────────────────────────────────────────────
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${BUNDLE_RESOURCES}/dictionary"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/dictionary"
            "${BUNDLE_RESOURCES}/dictionary"
        COMMENT "Copying dictionary into bundle Resources"
    )

    # ── 4. Deploy Qt frameworks into the bundle ────────────────────────────────
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND "${_qt_bin}/macdeployqt"
            "$<TARGET_BUNDLE_DIR:ProjectNotes>"
            -always-overwrite
        COMMENT "Deploying Qt into app bundle (macdeployqt)"
        VERBATIM
    )

    # ── 5. Bundle Python.framework ─────────────────────────────────────────────
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUNDLE_FRAMEWORKS}"
        COMMAND ${CMAKE_COMMAND}
            "-DPYTHON_FRAMEWORK_SRC=${_python_framework_root}"
            "-DFRAMEWORKS=${BUNDLE_FRAMEWORKS}"
            "-DEXECUTABLE=$<TARGET_FILE:ProjectNotes>"
            "-DPYTHON_VERSION=${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/BundlePythonFramework.cmake"
        COMMENT "Bundling Python.framework into Contents/Frameworks"
        VERBATIM
    )
else()
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_CURRENT_SOURCE_DIR}/plugins"
            "${CMAKE_CURRENT_BINARY_DIR}/plugins"
        COMMENT "Creating symbolic link to plugins folder"
    )

    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_CURRENT_SOURCE_DIR}/threads"
            "${CMAKE_CURRENT_BINARY_DIR}/threads"
        COMMENT "Creating symbolic link to threads folder"
    )

    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/dictionary"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/dictionary"
            "${CMAKE_CURRENT_BINARY_DIR}/dictionary"
        COMMENT "Copying dictionary"
    )
endif()

# ── Windows installer staging target ──────────────────────────────────────────
if(WIN32)
    # Paths to the Python environment — override on the cmake command line if needed
    set(PYTHON_EMBED_DIR
        "C:/Users/paulmckinney/Documents/python-3.13.2-embed-amd64"
        CACHE PATH "Python embeddable distribution directory")
    set(PYTHON_PACKAGES_DIR
        "C:/Program Files/Python313/Lib/site-packages"
        CACHE PATH "Full Python site-packages directory to bundle")
    set(PYTHON_USER_PACKAGES_DIR
        "C:/Users/paulmckinney/AppData/Roaming/Python/Python313/site-packages"
        CACHE PATH "User-level Python site-packages to bundle (supplements PYTHON_PACKAGES_DIR)")
    set(SQLITESYNCPRO_BUILD_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../SqliteSyncPro/build/Desktop_Qt_6_10_0_MSVC2022_64bit-Release"
        CACHE PATH "SqliteSyncPro release build directory")

    # Locate windeployqt from the Qt installation used to build
    get_target_property(_qmake_loc Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qmake_loc}" DIRECTORY)

    set(DEPLOY_DIR  "${CMAKE_BINARY_DIR}/deploy")
    set(NSIS_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows")

    add_custom_target(deploy
        COMMENT "Staging Windows deployment to ${DEPLOY_DIR}"

        # 1. Clean slate
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory   "${DEPLOY_DIR}"

        # 2. Application executables
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:ProjectNotes>"
            "${DEPLOY_DIR}/Project Notes.exe"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/admin/Project Notes Remote Host.exe"
            "${DEPLOY_DIR}/Project Notes Remote Host.exe"

        # 3. Qt DLLs and plugins via windeployqt
        COMMAND "${_qt_bin}/windeployqt.exe"
            --release --no-translations
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

        # 5. SqliteSyncPro vcpkg runtime DLLs
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/vcpkg_installed/x64-windows/bin/charset-1.dll"
            "${DEPLOY_DIR}/charset-1.dll"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/vcpkg_installed/x64-windows/bin/iconv-2.dll"
            "${DEPLOY_DIR}/iconv-2.dll"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/vcpkg_installed/x64-windows/bin/libecpg.dll"
            "${DEPLOY_DIR}/libecpg.dll"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/vcpkg_installed/x64-windows/bin/libecpg_compat.dll"
            "${DEPLOY_DIR}/libecpg_compat.dll"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/vcpkg_installed/x64-windows/bin/libpgtypes.dll"
            "${DEPLOY_DIR}/libpgtypes.dll"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/vcpkg_installed/x64-windows/bin/libpq.dll"
            "${DEPLOY_DIR}/libpq.dll"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/vcpkg_installed/x64-windows/bin/lz4.dll"
            "${DEPLOY_DIR}/lz4.dll"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SQLITESYNCPRO_BUILD_DIR}/vcpkg_installed/x64-windows/bin/zlib1.dll"
            "${DEPLOY_DIR}/zlib1.dll"

        # 6. Python embeddable distribution (python313.dll, .zip, .pyd files, etc.)
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PYTHON_EMBED_DIR}"
            "${DEPLOY_DIR}"

        # 7. Filter and copy site-packages; generate install_files.nsh + remove_files.nsh
        COMMAND "${Python3_EXECUTABLE}"
            "${NSIS_DIR}/collect_packages.py"
            "--source" "${PYTHON_PACKAGES_DIR}"
            "--source" "${PYTHON_USER_PACKAGES_DIR}"
            "--dest"   "${DEPLOY_DIR}/site-packages"
            "--nsis-install"   "${NSIS_DIR}/install_files.nsh"
            "--nsis-uninstall" "${NSIS_DIR}/remove_files.nsh"

        # 8. Application content
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/plugins"    "${DEPLOY_DIR}/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/threads"    "${DEPLOY_DIR}/threads"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/dictionary" "${DEPLOY_DIR}/dictionary"

        DEPENDS ProjectNotes
        VERBATIM
    )
endif()

