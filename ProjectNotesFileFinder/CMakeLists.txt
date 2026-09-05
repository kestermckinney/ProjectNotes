set(CMAKE_AUTOMOC ON)

add_library(ProjectNotesFileFinder STATIC
    FileFinderService.cpp FileFinderService.h
    FileFinderWorker.cpp FileFinderWorker.h
    FileFinderTypes.h
    MicrosoftOAuthManager.cpp MicrosoftOAuthManager.h
    MicrosoftGraphSource.cpp MicrosoftGraphSource.h
)

target_include_directories(ProjectNotesFileFinder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ProjectNotesFileFinder PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
)
