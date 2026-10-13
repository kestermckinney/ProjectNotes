# Report HTML to PDF rendering with QTextDocument + QPdfWriter
# (TextDocumentReportRenderer).

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

