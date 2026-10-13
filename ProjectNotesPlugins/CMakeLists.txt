# JavaScript plugin host (QJSEngine). See docs/QJSEnginePluginsPlan.md.
#
# Deliberately free of Qt::Widgets and Qt::Quick: the application supplies
# dialogs, navigation and integrations through PluginServices. CorePrivate is
# needed only for QZipReader (installing .pnplugin packages).
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Qml Network Sql Xml CorePrivate)

add_library(ProjectNotesPlugins STATIC
    ComBridge.h
    DataService.cpp         DataService.h
    JsConvert.h
    PluginHost.cpp          PluginHost.h
    PluginManifest.cpp      PluginManifest.h
    PluginRegistry.cpp      PluginRegistry.h
    PluginRuntime.cpp       PluginRuntime.h
    PluginServices.h
    PluginTypes.h
    PublishedSchema.cpp     PublishedSchema.h
)
if(WIN32)
    target_sources(ProjectNotesPlugins PRIVATE ComBridge_win.cpp)
    target_link_libraries(ProjectNotesPlugins PRIVATE ole32 oleaut32 uuid)
else()
    target_sources(ProjectNotesPlugins PRIVATE ComBridge_stub.cpp)
endif()

qt_add_resources(ProjectNotesPlugins "projectnotes_plugins_js"
    PREFIX "/projectnotes/plugins"
    BASE js
    FILES js/prelude.js js/projectnotes.d.ts
)

set_target_properties(ProjectNotesPlugins PROPERTIES AUTOMOC ON)
target_include_directories(ProjectNotesPlugins PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ProjectNotesPlugins PUBLIC
    ProjectNotesCore
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::CorePrivate
)
