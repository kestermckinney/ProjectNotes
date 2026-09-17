# Shared integration contracts. Concrete OAuth/template work is introduced by
# later tasks; this target deliberately has no dependency on the desktop UI.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Qml Sql Xml Widgets)

add_library(ProjectNotesIntegrations STATIC
    CommunicationTypes.cpp
    CommunicationTypes.h
    MicrosoftOAuthManager.cpp
    MicrosoftOAuthManager.h
    Office365Service.cpp
    Office365Service.h
    HttpTransport.h
    QtNetworkHttpTransport.cpp
    QtNetworkHttpTransport.h
    TemplateTypes.h
    TemplateParser.h
    TemplateParser.cpp
    ComputedFieldRunner.h
    ComputedFieldRunner.cpp
    ComputedFieldService.h
    ComputedFieldService.cpp
    CommunicationTemplateStore.h
    CommunicationTemplateStore.cpp
    TemplateEditorModel.h
    TemplateEditorModel.cpp
    TemplateApplication.h
    TemplateApplication.cpp
)

target_include_directories(ProjectNotesIntegrations PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ProjectNotesIntegrations PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Widgets
)
