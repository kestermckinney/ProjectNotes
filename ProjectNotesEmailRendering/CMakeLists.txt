# T01 establishes the renderer boundary only. Qt WebEngine linkage and its
# implementation are deliberately deferred to T11, after the renderer gate.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebEngineCore)

add_library(ProjectNotesEmailRendering STATIC
    RenderingModule.cpp
    WebEngineReportRenderer.cpp
    WebEngineReportRenderer.h
    RendererResourcePolicy.cpp
    RendererResourcePolicy.h
)
target_include_directories(ProjectNotesEmailRendering PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ProjectNotesEmailRendering PUBLIC ProjectNotesEmail Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebEngineCore)
