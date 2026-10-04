find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Sql Xml Network)

add_library(ProjectNotesEmail STATIC
    EmailTypes.cpp
    EmailTypes.h
    ReportTypes.h
    EmlWriter.h
    EmlWriter.cpp
    EmailSettingsStore.cpp
    EmailSettingsStore.h
    ArtifactStore.cpp
    ArtifactStore.h
    TransientCacheCleaner.cpp
    TransientCacheCleaner.h
    SnapshotTypes.h
    RecipientAudienceResolver.h
    RecipientAudienceResolver.cpp
    RecipientSelectionModel.h
    RecipientSelectionModel.cpp
    AudiencePresetStore.h
    AudiencePresetStore.cpp
    reports/ReportFormatters.h
    reports/ReportFormatters.cpp
    CommunicationRepository.h
    CommunicationRepository.cpp
    CommunicationDiagnostics.h
    CommunicationDiagnostics.cpp
    CommunicationTemplateContext.h
    CommunicationTemplateContext.cpp
    MeetingNotesEmailBuilder.h
    MeetingNotesEmailBuilder.cpp
    MeetingNotesPreparationFactory.h
    MeetingNotesPreparationFactory.cpp
    MeetingNotesReportBuilder.h
    MeetingNotesReportBuilder.cpp
    MeetingNotesReportPreparationFactory.h
    MeetingNotesReportPreparationFactory.cpp
    ProjectReportPreparationFactory.h
    ProjectReportPreparationFactory.cpp
    StatusReportBuilder.h
    StatusReportBuilder.cpp
    TrackerItemsReportBuilder.h
    TrackerItemsReportBuilder.cpp
    ReportService.h
    ReportService.cpp
    EmailContentBuilder.h
    EmailContentBuilder.cpp
    CommunicationsController.h
    CommunicationsController.cpp
    backends/EmlFileEmailBackend.h
    backends/EmlFileEmailBackend.cpp
    backends/GraphEmailBackend.h
    backends/GraphEmailBackend.cpp
    backends/ICloudEmailBackend.h
    backends/ICloudEmailBackend.cpp
    backends/ThunderbirdLauncher.h
    backends/ThunderbirdLauncher.cpp
    backends/ThunderbirdEmailBackend.h
    backends/ThunderbirdEmailBackend.cpp
    backends/OutlookHelperProtocol.h
    backends/OutlookHelperProtocol.cpp
    backends/OutlookLauncher.h
    backends/OutlookLauncher.cpp
    EmailService.h
    EmailService.cpp
    EmailBackend.h
    ReportRenderer.h
)

if(APPLE)
    target_sources(ProjectNotesEmail PRIVATE
        backends/MacMailBackend.h
        backends/MacMailBackend.mm
    )
    target_link_libraries(ProjectNotesEmail PRIVATE "-framework AppKit")
endif()

if(WIN32)
    # OutlookLauncher opens files in the new Outlook with ShellExecuteEx.
    target_link_libraries(ProjectNotesEmail PRIVATE shell32)
endif()

target_include_directories(ProjectNotesEmail PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ProjectNotesEmail PUBLIC
    ProjectNotesIntegrations
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Network
)
