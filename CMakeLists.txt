cmake_minimum_required(VERSION 3.16)

project(ProjectNotes VERSION 4.0.0 LANGUAGES CXX)

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

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Sql Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Sql Xml)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENV{QT_LOGGING_RULES} "qt.*.debug=true")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

message(STATUS "Python3 executable: ${Python3_EXECUTABLE}")
message(STATUS "Python3 library: ${Python3_LIBRARIES}")
message(STATUS "Python3 include dir: ${Python3_INCLUDE_DIRS}")


# Find the Hunspell library
#find_package(Hunspell REQUIRED)
if (WIN32)
    # this isn't the best way to do this, but for now until i figure out a better way
    set(Hunspell_INCLUDE_DIRS "C:/Users/Paul McKinney/Documents/hunspell/src")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(HUNSPELL_LIBRARIES "C:/Users/Paul McKinney/Documents/hunspell/msvc/x64/Debug_dll/libhunspell.lib")
        set(HUNSPELL_DLL "C:/Users/Paul McKinney/Documents/hunspell/msvc/x64/Debug_dll/libhunspell.dll")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(HUNSPELL_LIBRARIES "C:/Users/Paul McKinney/Documents/hunspell/msvc/x64/Release_dll/libhunspell.lib")
        set(HUNSPELL_DLL "C:/Users/Paul McKinney/Documents/hunspell/msvc/x64/Release_dll/libhunspell.dll")
    endif()
else()
    set(Hunspell_INCLUDE_DIRS /usr/include/hunspell)
    set(HUNSPELL_LIBRARIES hunspell-1.7)
endif()

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

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ProjectNotes
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        actionitemprojectnotesmodel.cpp actionitemsdetailsmeetingsmodel.h actionitemsdetailsmeetingsmodel.cpp actionitemprojectnotesmodel.h clientslistview.cpp clientslistview.h clientsmodel.cpp clientsmodel.h clientspage.cpp clientspage.h columnview.cpp columnview.h comboboxdelegate.cpp comboboxdelegate.h databasestructure.cpp databasestructure.h filterdatadialog.cpp filterdatadialog.h filterdatadialog.ui FilterSaveStructure.h importexport.cpp importexport.h itemdetailsdelegate.cpp itemdetailsdelegate.h itemdetailspage.cpp itemdetailspage.h itemdetailteamlistmodel.cpp itemdetailteamlistmodel.h meetingattendeesmodel.cpp meetingattendeesmodel.h meetingattendeesview.cpp meetingattendeesview.h notesactionitemsmodel.cpp notesactionitemsmodel.h notesactionitemsview.cpp notesactionitemsview.h peoplelistview.cpp peoplelistview.h peoplemodel.cpp peoplemodel.h peoplepage.cpp peoplepage.h pnbasepage.cpp pnbasepage.h pncheckboxdelegate.cpp pncheckboxdelegate.h pncolumnmodel.cpp pncolumnmodel.h pncombobox.cpp pncombobox.h pncomboboxdelegate.cpp pncomboboxdelegate.h pndatabaseobjects.cpp pndatabaseobjects.h pndateeditdelegate.cpp pndateeditdelegate.h pndateeditex.cpp pndateeditex.h pninlinespellchecker.cpp pninlinespellchecker.h pnlineeditfilebutton.cpp pnlineeditfilebutton.h pnlineeditfilebuttondelegate.cpp pnlineeditfilebuttondelegate.h pnplaintexteditdelegate.h pnplaintextedit.h pnplaintexteditdelegate.cpp pnplaintextedit.cpp
        pnsettings.cpp pnsettings.h pnsortfilterproxymodel.cpp pnsortfilterproxymodel.h pnspellchecker.cpp pnspellchecker.h pnsqlquerymodel.cpp pnsqlquerymodel.h pntableview.cpp pntableview.h pntextedit.cpp pntextedit.h pntexteditdelegate.cpp pntexteditdelegate.h projectdetailsdelegate.cpp projectdetailsdelegate.h projectdetailspage.cpp projectdetailspage.h projectlocationsmodel.cpp projectlocationsmodel.h projectlocationsview.cpp projectlocationsview.h projectnotesdelegate.cpp projectnotesdelegate.h projectnotesmodel.cpp projectnotesmodel.h projectnotespage.cpp projectnotespage.h projectnotesview.cpp projectnotesview.h projectsmodel.h projectsmodel.cpp projectslistview.h projectslistview.cpp projectslistpage.h projectslistpage.cpp projectslistmodel.h projectslistmodel.cpp projectteammembersmodel.cpp projectteammembersmodel.h projectteammembersview.cpp projectteammembersview.h runguard.cpp runguard.h searchpage.cpp searchpage.h searchresultsmodel.cpp searchresultsmodel.h searchresultsview.cpp searchresultsview.h statusreportitemsmodel.cpp statusreportitemsmodel.h statusreportitemsview.cpp statusreportitemsview.h teamsmodel.cpp teamsmodel.h trackeritemcommentsmodel.cpp trackeritemcommentsmodel.h trackeritemcommentsview.cpp trackeritemcommentsview.h trackeritemsmodel.cpp trackeritemsmodel.h trackeritemsview.cpp trackeritemsview.h valueselectmodel.cpp valuesview.h valuesview.cpp valueselectmodel.h
        pncombobox.cpp pncombobox.h
        spellcheckdialog.cpp spellcheckdialog.h spellcheckdialog.ui
        plugin.cpp plugin.h pluginmanager.cpp pluginmanager.h
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

# include directories for Python
include_directories(${Python3_INCLUDE_DIRS})
link_directories(${Python3_INCLUDE_DIRS})

# Include the source directory in the include directories for the executable
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ProjectNotes PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${Python3_LIBRARIES} Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::Xml "${HUNSPELL_LIBRARIES}")

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ProjectNotes)
endif()
set_target_properties(ProjectNotes PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ProjectNotes
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ProjectNotes)
endif()

add_custom_command(TARGET ProjectNotes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/plugins ${CMAKE_CURRENT_BINARY_DIR}/plugins
    COMMENT "Creating symbolic link to plugins folder"
    # COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/plugins
    # COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/plugins ${CMAKE_CURRENT_BINARY_DIR}/plugins
    # COMMAND ${CMAKE_COMMAND} -E echo "Copying plugins"
    # DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/*
)

add_custom_command(TARGET ProjectNotes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/threads ${CMAKE_CURRENT_BINARY_DIR}/threads
    COMMENT "Creating symbolic link to threads folder"
    # COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/threads
    # COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/threads ${CMAKE_CURRENT_BINARY_DIR}/threads
    # COMMAND ${CMAKE_COMMAND} -E echo "Copying threads"
    # DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/threads/*
)

add_custom_command(TARGET ProjectNotes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/dictionary
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/dictionary ${CMAKE_CURRENT_BINARY_DIR}/dictionary
    COMMAND ${CMAKE_COMMAND} -E echo "Copying dictionary"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dictionary/*
)

if(WIN32)
    add_custom_command(TARGET ProjectNotes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${HUNSPELL_DLL} ${CMAKE_CURRENT_BINARY_DIR}/libhunspell.dll
        COMMAND ${CMAKE_COMMAND} -E echo "Copying Hunspell"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*
    )
endif()

# if(WIN32)
#     set(APP_ICON_RESOURCE resources/app_icon.rc)
#     configure_file(${APP_ICON_RESOURCE} ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc COPYONLY)
#     set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc PROPERTIES LANGUAGE RC)
#     target_sources(MyApp PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc)
# endif()
