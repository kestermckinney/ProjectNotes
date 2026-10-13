# QTextDocument PDF renderer plan

Branch: `feature/qjsengine_plugins`. Prepared: 2026-10-12. Status: proposed plan. No application code has changed.

## Goal

Replace Qt WebEngine with `QTextDocument` and `QPdfWriter` for report PDF generation, then remove Qt WebEngine (`WebEngineCore` and `WebEngineQuick`) from the desktop app and every package. This removes the Chromium payload from all three desktop platforms and clears a Mac App Store obstacle, since Qt has historically documented that WebEngine apps cannot be submitted there. It is a companion to the [QJSEngine plugin plan](QJSEnginePluginsPlan.md).

## What WebEngine does today

WebEngine has exactly one job: printing a report's HTML to PDF. No QML uses `WebEngineView`, and inline HTML and HTML-attachment emails never touch it.

- `ProjectNotesEmailRendering/WebEngineReportRenderer.cpp` implements `PN::Comm::ReportRenderer` (`ProjectNotesEmail/ReportRenderer.h`). It loads `RenderRequest::html` into an off-the-record `QWebEnginePage` with JavaScript off, local and remote resource access off, and a request interceptor that allows only the `about:`/`data:` main frame (`RendererResourcePolicy.cpp`). It calls `printToPdf(outputPdfPath, pageLayout)` with a 30 s timeout.
- `DesktopAppController::stageRequestedGeneratedAttachmentAsync()` calls it when a workflow needs a PDF: email mode **PDF attachment**, **Display Report when complete**, or a PDF export saved to the project folder.
- `ProjectNotesDesktop/main.cpp` calls `QtWebEngineQuick::initialize()` before constructing the application. That call is the only reason the executable links `WebEngineQuick`.

Four document sources reach the renderer:

| Source | Page layout | HTML features in use |
| --- | --- | --- |
| `TrackerItemsReportBuilder` | Letter landscape | `<style>` block with class selectors, fixed-layout 13-column table with percentage widths, `border-collapse`, status group header rows, `page-break-inside: avoid` on rows, `page-break-after: avoid` on group headers, `white-space: pre-wrap`, priority colors |
| `StatusReportBuilder` | Letter portrait, 20 mm margins | `<style>` block, header/issues/appendix tables, **`display: flex`** for the two earned-value tables side by side, **`display: inline-flex`** for stacked numerator/denominator fractions in the appendix formulas, `Cambria Math` italic formula blocks, `h1` with background color |
| `MeetingNotesReportBuilder`, `MeetingNotesEmailBuilder` | A4 portrait | Inline styles applied by class, label/value tables, the meeting's rich-text notes (already produced by the `QTextDocument`-based note editor) |
| `ProjectEmailPreparationFactory` | caller's layout | User-authored communication template HTML wrapped in a bare document |

## Why QTextDocument fits

`QTextDocument` renders a defined subset of HTML 4 and CSS 2.1 and prints it with `print(QPagedPaintDevice*)`. The reports are already close to that subset because they were designed to survive Outlook, whose Word-based HTML engine has similar limits. The `emailFragment` builders already rewrite class styles into `width`/`bgcolor` attributes and inline styles for that reason.

What carries over directly: tables with `colspan`, cell padding and per-side borders, `border-collapse`, `bgcolor` and `background-color`, `width` attributes on cells, fonts, colors, bold and italic, `white-space: pre-wrap`, `<style>` blocks with type and class selectors, `<h1>`, `<br>`, links, and `page-break-before`/`page-break-after: always`. A `<thead>` becomes `QTextTableFormat::headerRowCount`, which repeats the header row on every page. WebEngine's `printToPdf` does not do that today, so the long Tracker Items report gets better.

What does not carry over, and what each needs:

| Feature | Used by | Replacement |
| --- | --- | --- |
| `display: flex` side-by-side tables | Status Report earned-value section | A two-cell outer table, each cell holding one metrics table (nested tables are supported) |
| `display: inline-flex` fractions | Status Report appendix formulas | A small nested table per fraction with a bottom border on the numerator cell, centered in the formula block. Inline tables inside a text line are not supported, so each formula becomes a one-row table: left-hand side, `=`, fraction |
| `table-layout: fixed` | Tracker, Status | Column widths through `width` attributes, which `QTextDocument` maps to fixed percentage column constraints. Verify the 13 tracker columns hold their widths when a cell contains a long unbroken string |
| `word-wrap: break-word` | Tracker, Status | `QTextOption::WrapAtWordBoundaryOrAnywhere` on the document's default text option |
| `page-break-inside: avoid` on rows, `page-break-after: avoid` on group headers | Tracker | Not supported. Rows can split across pages and a group header can end a page. Accept for the first version, or add a post-layout pass that inserts `PageBreak_AlwaysBefore` on rows that would split, using `QAbstractTextDocumentLayout` positions |
| CSS `text-align` on cells by class | All | Confirm in the spike; fall back to `align` attributes, which the email path already emits |
| `:hover`, `<meta viewport>` | All | Ignored. No effect on print output |

Fonts behave as they do now. `Calibri` and `Cambria Math` exist on Windows and on Macs with Office, and fall back on Linux, the same as under WebEngine.

User-authored communication templates can use any CSS. Under `QTextDocument`, unsupported properties are ignored rather than rendered. The template editor help should list the supported subset, and the template preview should use the same renderer so what the user sees matches the PDF.

## Design

### `TextDocumentReportRenderer`

A new `PN::Comm::ReportRenderer` implementation in `ProjectNotesEmailRendering`, so `DesktopAppController` changes only the class it constructs.

```cpp
class TextDocumentReportRenderer final : public QObject, public ReportRenderer {
public:
    void render(RenderRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;
};
```

Render steps:

1. Validate the request exactly as `WebEngineReportRenderer` does: absolute, non-existent `.pdf` path in an existing directory, non-empty HTML, non-null operation id.
2. Build a `RestrictedTextDocument`, a `QTextDocument` subclass whose `loadResource()` applies `rendererResourceRequestAllowed()`. Embedded `data:` images load; `file:`, `qrc:`, and network URLs return nothing. This keeps the current resource policy. It matters because plain `QTextDocument` would otherwise read image files named in template HTML.
3. Set `documentMargin(0)`, the default font, and the wrap mode, then `setHtml()`.
4. Create a `QPdfWriter` on the output path with `setPageLayout(request.pageLayout)` (A4 portrait if invalid, as today), `setResolution(1200)`, and the title and creator metadata.
5. Size the document to the page's paint rectangle (`pageLayout.paintRectPixels(resolution)`) before calling `print()`. Without an explicit page size, `QTextDocument::print()` applies its own 2 cm margins and ignores the report's layout.
6. `print(&writer)`, close the writer, check the file is non-empty, and complete with the same `RenderResult` and `Artifact` shape. Error codes stay the same (`renderer-input-invalid`, `operation-cancelled`, `renderer-pdf-failed`); `renderer-crashed` and `renderer-load-failed` can no longer occur.

**Threading.** Rendering runs on a `QThreadPool` worker. The document and writer are created, used, and destroyed on that thread, and only the `RenderResult` crosses back, queued to the renderer's thread. `QTextDocument` and `QPdfWriter` work off the GUI thread as long as no `QPixmap` is involved; `data:` images load as `QImage`. If the spike shows font or layout problems on a worker, render on the GUI thread instead. These documents are small, and the large tracker report is the case to time.

**Cancellation and timeout.** `print()` cannot be interrupted. `cancel()` records the id; a cancelled operation deletes its output on completion and reports `operation-cancelled`, which is what the controller already expects. The 30 s timeout becomes unnecessary because nothing waits on an outside process. Keep a generous watchdog that reports `renderer-timeout` without blocking the caller, in case pathological template HTML takes too long to lay out.

### Builder changes

One `htmlDocument` stays the single source for the PDF, the **Generate HTML Report** export, and the HTML attachment. The changes below look the same in a browser, so the HTML export does not need a separate variant.

- **Tracker Items:** wrap the header row in `<thead>`; emit `width` attributes on header cells (the email path already does); keep the CSS for browsers.
- **Status Report:** replace `.ev-columns` with a two-cell table and the `.fraction` spans with nested tables; keep the stylesheet otherwise.
- **Meeting Notes and Send Meeting Notes:** no structural change expected. Confirm label column widths and `white-space: nowrap` in the spike.
- **Email fragments:** unchanged. Outlook and other clients still receive the inline-styled fragment.

### Removing WebEngine

Once the new renderer ships as the default:

- Delete `WebEngineReportRenderer.*`, `tests/desktop/tst_webenginerenderer.cpp` and its `ENABLE_WEBENGINE_RENDERER_TESTS` option. Link `ProjectNotesEmailRendering` against `Qt::Gui` instead of `Qt::WebEngineCore`.
- Remove `WebEngineCore` and `WebEngineQuick` from `find_package` in the root and `ProjectNotesDesktop` `CMakeLists.txt`, and `Qt6::WebEngineCore`/`Qt6::WebEngineQuick` from the link list.
- Remove `QtWebEngineQuick::initialize()` and the WebEngine deployment comments from `main.cpp`.
- Delete `packaging/verify_native_webengine_package.cmake` and its calls in both `CMakeLists.txt` files.
- Windows: remove the WebEngine DLLs, `QtWebEngineProcess.exe`, resources, and locales from `setupscript.nsi`, keeping the uninstall `Delete` lines for one release so upgrades clean up. Stop passing WebEngine to `windeployqt`.
- macOS: remove the `QtWebEngineProcess.app` entitlement handling in `build_installer.sh`.
- Flatpak: drop `base: io.qt.qtwebengine.BaseApp`, the `QTWEBENGINE*` environment variables, `python-pyqt6-webengine.yaml`, and `build-flatpak.sh verify-webengine`, plus the BaseApp install in `build-flatpak.sh` and `packaging/gitlab/build-release.sh`.
- Docs: update `NativeEmailCodeArchitecture.md`, `NativeEmailImplementationPlan.md`, and `CLAUDE.md`.

**PyQt6 WebEngine.** Dropping the Flatpak BaseApp also drops the PyQt6-WebEngine bindings, and the Windows and macOS Python bundles should stop shipping them in the same release. No bundled plugin imports them, but a user's own Python plugin might. List that in the release notes. The `AA_ShareOpenGLContexts` attribute in `main.cpp` exists only for those bindings and goes with them.

## Phases

**Phase A: spike** (3 to 4 days)
- Render the synthetic project (`tests/desktop/fixtures/email/synthetic-project.json`) through each of the four builders with a throwaway `TextDocumentReportRenderer`, current HTML unchanged, and compare the PDFs with WebEngine's output side by side.
- Confirm: `<style>` class selectors on cells, `text-align` by class, percentage column widths under long strings, `<thead>` repetition, hyperlink annotations in the PDF, worker-thread rendering, timing for a 500-item tracker report.
- Output: a short list of required HTML changes, which should match the table above or correct it.

**Phase B: renderer and builder changes** (1 week)
- `TextDocumentReportRenderer` and `RestrictedTextDocument` as designed.
- Builder changes for Tracker Items and Status Report.
- CMake option `PN_PDF_RENDERER` (`textdocument` default, `webengine` available) so both can be compared during review. The controller picks the class at construction.

**Phase C: tests** (3 to 4 days)
- Headless tests that need no WebEngine and run in the normal suite: for each workflow, render to PDF and assert page count, page size and orientation, and non-empty output; render the same document into per-page `QImage`s and compare against approved snapshots with a small tolerance.
- Resource-policy tests: a template referencing `file:`, `qrc:`, and `https:` images produces no request and no image; a `data:` image renders.
- Parity checks against `baseline-summary.json` for text content (titles, counts, columns, footers) by inspecting the `QTextDocument` before printing.

**Phase D: release with WebEngine still linked** (one release)
- Ship `textdocument` as the default with `webengine` still buildable, so a report regression can be fixed by switching renderers in a patch build rather than a code revert.

**Phase E: remove WebEngine** (2 to 3 days)
- Everything under "Removing WebEngine". Verify installers on all three platforms and the Flatpak build without the BaseApp.

## Risks

- **Visual differences.** Rows that split across pages and group headers stranded at a page bottom are the visible regressions. Repeating header rows partly compensate. Decide in Phase A whether the post-layout pass is needed.
- **Template HTML.** Users who wrote templates with modern CSS will see simpler output. The editor's supported-subset help and a matching preview are the mitigation.
- **Formula rendering.** Nested-table fractions look slightly different from the current flexbox layout. Review the appendix page specifically.
- **Rich notes.** Meeting notes HTML comes from the `QTextDocument` editor, so it should render more faithfully than under WebEngine. Confirm with notes that contain tables, lists, and highlighted text.
