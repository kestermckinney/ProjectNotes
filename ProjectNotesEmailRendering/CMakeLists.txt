# Report HTML to PDF rendering. TextDocumentReportRenderer (QTextDocument +
# QPdfWriter) is the default. PN_PDF_RENDERER (root CMakeLists.txt) =webengine
# keeps the previous Qt WebEngine renderer buildable for one release.

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui)

add_library(ProjectNotesEmailRendering STATIC
    RenderingModule.cpp
    TextDocumentReportRenderer.cpp
    TextDocumentReportRenderer.h
    RendererResourcePolicy.cpp
    RendererResourcePolicy.h
)
target_include_directories(ProjectNotesEmailRendering PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ProjectNotesEmailRendering PUBLIC ProjectNotesEmail Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui)

if(PN_PDF_RENDERER STREQUAL "webengine")
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS WebEngineCore)
    target_sources(ProjectNotesEmailRendering PRIVATE WebEngineReportRenderer.cpp WebEngineReportRenderer.h)
    target_link_libraries(ProjectNotesEmailRendering PUBLIC Qt${QT_VERSION_MAJOR}::WebEngineCore)
    target_compile_definitions(ProjectNotesEmailRendering PUBLIC PN_PDF_RENDERER_WEBENGINE)
endif()
