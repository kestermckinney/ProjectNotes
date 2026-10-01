set(CMAKE_AUTOMOC ON)
add_library(ProjectNotesContacts STATIC
    ContactTypes.cpp ContactTypes.h ContactMatcher.cpp ContactMatcher.h
    ContactRepository.cpp ContactRepository.h ContactProvider.cpp ContactProvider.h
    ContactSyncSettingsModel.cpp ContactSyncSettingsModel.h ContactSyncWorker.cpp ContactSyncWorker.h
    ThunderbirdBridge.cpp ThunderbirdBridge.h
    VCard.cpp VCard.h ../credentialstore.cpp)
target_include_directories(ProjectNotesContacts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ProjectNotesContacts PUBLIC ProjectNotesIntegrations ProjectNotesCore Qt6::Network)
if(UNIX AND NOT APPLE)
    target_link_libraries(ProjectNotesContacts PRIVATE Qt6::DBus)
elseif(APPLE)
    target_link_libraries(ProjectNotesContacts PRIVATE "-framework Security")
elseif(WIN32)
    target_link_libraries(ProjectNotesContacts PRIVATE Advapi32)
endif()

add_executable(ProjectNotesThunderbirdContacts helpers/ThunderbirdHost.cpp)
target_link_libraries(ProjectNotesThunderbirdContacts PRIVATE Qt6::Core Qt6::Network)
if(WIN32)
    add_executable(ProjectNotesOutlookContacts helpers/OutlookContacts.cpp)
    target_link_libraries(ProjectNotesOutlookContacts PRIVATE Qt6::Core ole32 oleaut32)
endif()
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/projectnotes-contacts.xpi
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/projectnotes-contacts.xpi --format=zip manifest.json background.js
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/thunderbird
    DEPENDS thunderbird/manifest.json thunderbird/background.js)
add_custom_target(thunderbird_contacts_addon ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/projectnotes-contacts.xpi)
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS ProjectNotesThunderbirdContacts RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/projectnotes-contacts.xpi DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
