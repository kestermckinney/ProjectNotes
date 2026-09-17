# Native email compatibility ledger

All entries are observations, not support claims. Do not mark an OS/client combination verified until it is tested on that platform with synthetic content and the exact packaged/native Qt build.

| Platform / build | Operation | Observed behavior | Status / limitation |
| --- | --- | --- | --- |
| Linux container; Qt 6.11.2; Python 3.14.7 / PyQt6 | Import legacy report plugins | Qt WebEngine sandbox aborts during import before a workflow executes. | Not a client test; no renderer/page-layout evidence. |
| Linux container; source `0507b53` | AST-only pure legacy builder capture | Can capture synthetic HTML, subjects and recipient labels without importing plugins or triggering side effects. | Capture tooling only; no PDF/rendering/client validation. |
| Linux container; Qt 6.11.2; Python 3.14.7 | Optional Widgets frontend build (`BUILD_QML_DESKTOP=OFF`, `BUILD_WIDGETS_LEGACY=ON`) | `ProjectNotesWidgets` configured and built successfully alongside the new native modules. | Historic build-only coexistence evidence. Widgets is explicitly unsupported for future releases (2026-09-14); it remains outside native report/email release scope and must receive no further changes. |
| Windows | Outlook Classic / Graph / mailto / Thunderbird | Not run. | Planned prototype work (G3). |
| macOS | Mail / Graph / mailto / Thunderbird | Not run. | Planned prototype work (G3). |
| Linux desktop | Graph / mailto / Thunderbird | Not run. | Planned prototype work (G3). |

## Pending renderer capture fields

- Legacy Qt and QtWebEngine versions; `QPageLayout` page size, orientation, margins and units for each report.
- Font availability and substitution; HTML viewport width; PDF page count and dimensions.
- Report-specific pagination, attachment behavior and inline presentation on each supported client.
