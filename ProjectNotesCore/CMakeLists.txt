# ProjectNotesCore — shared data layer (models, database, logging)
# Built as a STATIC library and linked by both ProjectNotes (desktop)
# and ProjectNotesMobile (iOS/Android).
#
# Qt::Widgets is included because databaseobjects.h includes QMessageBox and
# SqliteSyncProLib also links Qt::Widgets.  All QMessageBox calls in the core
# are already inside "if (m_gui)" guards so they are never executed in the
# mobile app (which opens databases with gui=false for sync threads).

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Sql Xml Network Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql Xml Network Widgets)

set(CORE_SOURCES
    # Logging
    ../QLogger.cpp          ../QLogger.h
    ../QLoggerWriter.cpp    ../QLoggerWriter.h
    ../QLoggerLevel.h

    # Query / filter infrastructure
    ../sqlquerymodel.cpp    ../sqlquerymodel.h
    ../sortfilterproxymodel.cpp ../sortfilterproxymodel.h
    ../columnmodel.cpp      ../columnmodel.h
    ../valueselectmodel.cpp ../valueselectmodel.h
    ../filtersavestructure.h

    # Database layer
    ../databaseobjects.cpp  ../databaseobjects.h
    ../databasecreate.cpp   ../databasecreate.h
    ../databasestructure.cpp ../databasestructure.h
    ../databaseviews.cpp    ../databaseviews.h
    ../databaseupgrade_v1_0_0.cpp ../databaseupgrade_v1_0_0.h
    ../databaseupgrade_v1_2_0.cpp ../databaseupgrade_v1_2_0.h
    ../databaseupgrade_v5_0_0.cpp ../databaseupgrade_v5_0_0.h

    # Data models
    ../actionitemprojectnotesmodel.cpp  ../actionitemprojectnotesmodel.h
    ../actionitemsdetailsmeetingsmodel.cpp ../actionitemsdetailsmeetingsmodel.h
    ../clientsmodel.cpp     ../clientsmodel.h
    ../itemdetailteamlistmodel.cpp ../itemdetailteamlistmodel.h
    ../meetingattendeesmodel.cpp ../meetingattendeesmodel.h
    ../notesactionitemsmodel.cpp ../notesactionitemsmodel.h
    ../peoplemodel.cpp      ../peoplemodel.h
    ../projectlocationsmodel.cpp ../projectlocationsmodel.h
    ../projectnotesmodel.cpp ../projectnotesmodel.h
    ../projectslistmodel.cpp ../projectslistmodel.h
    ../projectsmodel.cpp    ../projectsmodel.h
    ../projectteammembersmodel.cpp ../projectteammembersmodel.h
    ../searchresultsmodel.cpp ../searchresultsmodel.h
    ../statusreportitemsmodel.cpp ../statusreportitemsmodel.h
    ../teamsmodel.cpp       ../teamsmodel.h
    ../trackeritemcommentsmodel.cpp ../trackeritemcommentsmodel.h
    ../trackeritemsmodel.cpp ../trackeritemsmodel.h
)

add_library(ProjectNotesCore STATIC ${CORE_SOURCES})

# Export parent directory so consumers can #include core headers directly
target_include_directories(ProjectNotesCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(ProjectNotesCore PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
)
