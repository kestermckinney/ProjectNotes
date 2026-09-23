find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Network)

# Native meeting scheduling. It shares audiences, staging and the Office 365
# account with ProjectNotesEmail, but has its own request type and backends:
# a meeting is not an EmailRequest with a calendar attachment.
add_library(ProjectNotesMeetings STATIC
    MeetingTypes.h
    MeetingTypes.cpp
    MeetingBackend.h
    MeetingService.h
    MeetingService.cpp
    IcsWriter.h
    IcsWriter.cpp
    MeetingPreparationFactory.h
    MeetingPreparationFactory.cpp
    backends/StagedInvite.h
    backends/StagedInvite.cpp
    backends/IcsFileMeetingBackend.h
    backends/IcsFileMeetingBackend.cpp
    backends/ThunderbirdMeetingBackend.h
    backends/ThunderbirdMeetingBackend.cpp
    backends/GraphMeetingBackend.h
    backends/GraphMeetingBackend.cpp
)

if(APPLE)
    target_sources(ProjectNotesMeetings PRIVATE
        backends/MacCalendarBackend.h
        backends/MacCalendarBackend.mm
    )
    target_link_libraries(ProjectNotesMeetings PRIVATE "-framework AppKit" "-framework ApplicationServices" "-framework CoreServices" "-framework EventKit")
endif()

target_include_directories(ProjectNotesMeetings PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ProjectNotesMeetings PUBLIC
    ProjectNotesEmail
    ProjectNotesIntegrations
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
)
