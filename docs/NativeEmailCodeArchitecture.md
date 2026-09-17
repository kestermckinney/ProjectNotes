# Native email/report code architecture

Branch: `feature/email`. Design baseline: 2026-09-12. Status: specification only; the classes below are proposed unless explicitly identified as existing.

## 1. How to use this specification

Read [NativeEmailImplementationPlan.md](NativeEmailImplementationPlan.md) for product behavior and acceptance criteria. This document fixes the code boundaries and contracts. Execute [NativeEmailBuildTasks.md](NativeEmailBuildTasks.md) one task at a time; do not give an implementing model the entire feature as one coding assignment.

Priority: subsequent user decisions > product requirements > this architecture > task-local details. If these documents conflict, report the exact conflict; do not silently reduce the requested behavior. A capability marked unproven is a prototype gate, not an implementation instruction to invent an API. Changes to public contracts require updating this document and dependent tests in the same task.

### Non-negotiable constraints

- Four native workflows: Send Meeting Notes, Tracker Items Report, Status Report, Meeting Notes Report. Their options, default content, filenames and report appearance follow the product plan.
- Five adapters: Graph, mailto, Outlook Classic, Thunderbird, macOS Mail. No direct email sending. No calendar writes in this release.
- All adapters attempt to show the prepared message and focus its window; presentation retry must not recreate a message.
- No Python or QWidget dialogs in the four replacement paths. Existing unrelated plugins remain functional.
- Templates and company audiences are shared services, not duplicated in each report or adapter.
- All changes are desktop-scoped. Do not add dependencies to the separately built iOS app or change synchronized schema to store local email preferences.
- Never silently drop HTML, attachments, recipients, selected options or internal-data filtering to accommodate a backend.

## 2. Existing integration points

These were inspected in the current tree. Recheck symbols with CodeGraph before editing; line numbers can move.

| Existing file/symbol | Integration action |
| --- | --- |
| Root `CMakeLists.txt` | C++17; currently adds Core and FileFinder, then desktop/tests under their build options. Add new target declarations before their consumers, with desktop/platform guards. |
| `ProjectNotesDesktop/CMakeLists.txt` | Target `ProjectNotes`; explicit `DESKTOP_QML_FILES`; URI `ProjectNotesDesktop`; add sources, resource lists, links and native packaging here. |
| `tests/desktop/CMakeLists.txt` | `tst_qmldesktop` recompiles controller and real QML module; `tst_filefinder` exercises native DB work. App/test QML builds need separate build directories. |
| `ProjectNotesDesktop/main.cpp`, `tests/desktop/tst_main.cpp` | Both use QApplication and explicit QML registration. Add the same communication registration and WebEngine initialization to both. |
| `DesktopAppController` constructor | Currently creates `FileFinderService`; become the composition root for communications and shared Office 365 services. |
| `DesktopAppController::openOrCreateDatabase` | Already passes `dbPath`, `&db_rwlock` and profile settings organization to FileFinder. Initialize snapshot repository here after successful DB open. |
| `DesktopAppController::managingCompanyId/projectManagerId` | IDs come from database preferences; supply them to the snapshot request, not company/name matching. |
| `ProjectDetailPage.qml::_saveNow`, `ProjectNoteDetailPage.qml::_saveNow` | Use their success result before launching a report/email; reject a still-unpersisted staged project even when `_saveNow()` returns true. |
| `qml/AppMenu.qml`, `RecordContextMenu.qml`, `RecordRowMenu.qml`, `Main.qml` | Add first-class native actions routed by stable IDs; do not dispatch native actions through a plugin menu index. |
| `DesktopAppController::runPluginMenuForTable` | Existing XML/plugin execution path is a migration reference, not the new native pipeline. |
| `ProjectNotesFileFinder/FileFinderService.*` | Owns OAuth/settings today; delegate to the shared Office365 service without changing scan/reconciliation behavior. |
| `ProjectNotesFileFinder/MicrosoftOAuthManager.*` | Move implementation to Integrations; preserve device-code flow and add injected HTTP, configurable scopes and identity generation. |
| `credentialstore.*`, `appsettings.*` | Existing executable-owned credential backend and profile conventions; reuse through injected callbacks. Do not compile credentialstore twice into one executable. |
| `ProjectNotesMobile/TextFormatter.*`, desktop `SpellCheck.*` and `qml/NoteFormatToolbar.qml` | Reuse existing rich-prose editing and spellcheck conventions. Never round-trip generated report tables through the prose editor. |

## 3. Module dependencies and ownership

```text
ProjectNotes / tst_qmldesktop
  DesktopAppController (composition root)
    CommunicationsController + QML models/dialogs
      ProjectNotesEmail ---------> ProjectNotesIntegrations
      ProjectNotesEmailRendering   (implements Email's renderer interface)
    FileFinderService -----------> ProjectNotesIntegrations
    existing ProjectNotesCore     (unchanged dependencies)

ProjectNotesIntegrations: OAuth, HTTP seam, templates, local template store
ProjectNotesEmail: snapshots, audiences, report builders, dispatch, adapters
ProjectNotesEmailRendering: Qt WebEngine implementation; no report business rules
```

Arrows mean link/dependency direction. Neither Integrations nor Email depends on DesktopAppController, global QML singletons or Python. The repository accepts database path/lock/preferences as inputs instead of linking to global application controllers. Renderer implementation is a separate target so content/service tests do not need Chromium to run. Keep the library count at these three new static libraries; do not create a library for every class.

### Proposed file layout

```text
ProjectNotesIntegrations/
  CMakeLists.txt
  CommunicationTypes.h
  HttpTransport.h/.cpp
  Office365Service.h/.cpp
  MicrosoftOAuthManager.h/.cpp        # moved, not copied
  TemplateTypes.h
  TemplateContext.h/.cpp
  TemplateParser.h/.cpp
  LegacyTemplateExpander.h/.cpp
  ComputedFieldRunner.h/.cpp
  CommunicationTemplateService.h/.cpp
  CommunicationTemplateStore.h/.cpp
ProjectNotesEmail/
  CMakeLists.txt
  EmailTypes.h
  ReportTypes.h
  SnapshotTypes.h
  CommunicationRepository.h/.cpp
  RecipientAudienceResolver.h/.cpp
  RecipientSelectionModel.h/.cpp
  EmailSettingsStore.h/.cpp
  ReportService.h/.cpp
  EmailContentBuilder.h/.cpp
  EmailService.h/.cpp
  EmailBackend.h
  ReportRenderer.h                  # abstract async renderer
  ArtifactStore.h/.cpp
  ExternalPresentation.h/.cpp
  reports/
    MeetingNotesEmailBuilder.h/.cpp
    MeetingNotesReportBuilder.h/.cpp
    TrackerItemsReportBuilder.h/.cpp
    StatusReportBuilder.h/.cpp
    ReportFormatters.h/.cpp
  templates/                        # bundled report HTML/CSS + envelope defaults
  backends/
    GraphEmailBackend.h/.cpp
    MailtoEmailBackend.h/.cpp
    ThunderbirdEmailBackend.h/.cpp
    OutlookClassicBackend.h/.cpp
    MacMailBackend.h/.mm
    UnsupportedEmailBackend.h/.cpp
  outlook-helper/
    CMakeLists.txt
    main.cpp
    OutlookAutomation.h/.cpp
ProjectNotesEmailRendering/
  CMakeLists.txt
  WebEngineReportRenderer.h/.cpp
  ReportResourcePolicy.h/.cpp
ProjectNotesDesktop/
  CommunicationsController.h/.cpp
  EmailPreparationModel.h/.cpp
  ReportOptionsModel.h/.cpp
  CommunicationTemplateEditorModel.h/.cpp
  Office365SettingsModel.h/.cpp      # token-free QML facade
  qml/
    ReportDialog.qml
    ReportOptionsPane.qml
    EmailDialog.qml
    RecipientAudiencePicker.qml
    CommunicationTemplateEditor.qml
    EmailBodyEditor.qml
    ReportPreview.qml
    EmailSettingsPane.qml
tests/desktop/
  tst_communicationtemplates.cpp
  tst_communicationrepository.cpp
  tst_recipients.cpp
  tst_reportcontent.cpp
  tst_email.cpp
  tst_office365.cpp
  tst_reportrenderer.cpp
  email/fakes/                      # HTTP, backend, renderer, launch, secrets, clock
  fixtures/email/                   # synthetic input + frozen expected results
```

`.h/.cpp` denotes two files, not a literal filename. Small private helpers can remain in their owning `.cpp`. No template metaprogramming framework, service locator, generalized dependency container or runtime plugin loader is needed.

### Lifecycle/thread rules

The controller owns services for application lifetime; dialogs do not own network operations. QML-facing QObjects, backend orchestrators, QNetworkAccessManager and QWebEnginePage live on the GUI thread. Network work is asynchronous. A single worker owns a dedicated read-only SQLite connection; a separate worker owns each QJSEngine evaluation. Large pure report construction can use a worker with value-only input/output. Outlook automation runs in its own helper process. Never pass a QSqlQuery, model index, QJSValue or WebEngine page across threads.

Use QObject parents for same-thread service ownership and `QPointer` in asynchronous callbacks. Register value metatypes used in queued signals. The repository closes/releases queries and its database handle on its worker thread before `removeDatabase`; shutdown joins workers before the shared DB lock is destroyed. Cancelling a preview invalidates its generation and ignores stale results. Do not use `processEvents()`, blocking network waits, synchronous QProcess waits or arbitrary sleeps to sequence the UI.

For v1 permit one active preparation/handoff operation at a time; completed operation recovery records may coexist. A second request receives a visible Busy result and can reopen the current preparation; it never replaces in-flight state. This avoids designing multi-window draft editing before it is needed.

## 4. Value contracts

Use namespace `PN::Comm` for new C++ types. Use enum-class values internally and explicit stable string IDs in JSON/settings/QML boundaries; never persist enum ordinals. A small QML-registered enum facade may expose the same values. All IDs are strings/QUuid as appropriate, never proxy-model row numbers. The examples below specify fields and semantics, not copy-paste-ready complete headers.

```cpp
enum class Workflow { SendMeetingNotes, TrackerItemsReport,
                      StatusReport, MeetingNotesReport };
enum class EmailMode { InlineHtml, HtmlAttachment, PdfAttachment, None };
enum class BackendId { Graph, Mailto, OutlookClassic, Thunderbird, MacMail };
enum class RecipientRole { To, Cc, Bcc };

struct SourceContext {
    QString databaseKey;        // local identity described in section 10
    quint64 databaseGeneration;
    QString projectId;
    QStringList noteIds;        // one note for initial SendMeetingNotes
    Workflow workflow;
};

struct TrackerFilters {
    QSet<QString> itemTypes = {"Tracker"};
    QSet<QString> statuses = {"New", "Assigned"};
};

struct ReportOptions {
    Workflow workflow;
    QDate reportingDate;
    bool internalReport = false;
    bool displayPdf = false;
    bool retainHtml = false;
    EmailMode emailMode = EmailMode::InlineHtml;
    std::optional<TrackerFilters> tracker; // present only for TrackerItemsReport
};

struct EmailAddress {
    QString displayName;
    QString address;            // original trimmed presentation spelling
    RecipientRole role;
};

struct Artifact {
    QUuid artifactId;
    QString absolutePath;
    QString displayName;
    QString mimeType;
    qint64 byteSize;
    QByteArray sha256;
    bool generatedByApp;
};

struct EmailRequest {
    QUuid operationId;
    SourceContext source;
    quint64 previewRevision;
    BackendId backend;
    QString accountKey;        // Graph: selected verified identity, no secret
    quint64 accountGeneration;
    QList<EmailAddress> recipients;
    QString subject;
    QString plainText;
    QString html;              // optional, never an implicit substitute for text
    QList<Artifact> attachments;
    bool addressLaterExplicitlyChosen = false;
};
```

Use `QStringList` stable IDs at the QML boundary; convert to QSet only internally. Construct options through a workflow-specific default factory, which populates `tracker` only for TrackerItemsReport and sets today's reporting date from the injected clock. Status/type values must match the stored legacy strings, including `Defered`. Irrelevant fields are rejected rather than accidentally applying tracker filters to a status report. SendMeetingNotes does not acquire a new PDF/report-options dialog; its builder produces the existing formatted body and reviewed context attachments.

`ValidationIssue` contains a stable code, severity, field path, translated display text and optional related person/artifact ID. `ServiceError` contains a stable code, safe display text, retry kind and outcome certainty; provider response bodies are not user-safe error strings. Value-producing helpers return a small explicit result struct (`ok`, `value`, `issues`) or a local `std::variant`; do not use empty QString to mean success, missing data and failure simultaneously.

`BackendCapabilities` contains availability + reason, support for To/Cc/Bcc, plain text, inline HTML, HTML/PDF attachments, empty recipients, presentation retry and known size restrictions. Distinguish Supported, Unsupported and Unproven for content capabilities. Unproven cannot pass preflight. Availability inspection must not create a draft, ask for consent or launch a client.

## 5. Repository and snapshot design

`CommunicationRepository` is the only new component querying project data. Its async contract is `loadSnapshot(operationId, previewRevision, SourceContext, SnapshotPreferences)` with matching `snapshotReady(...)` / `snapshotFailed(...)` signals. `SnapshotPreferences` supplies managing company/manager IDs, locale and explicit clock values captured by the controller. Repository code uses parameterized SQL and explicit column names; no `SELECT *`, proxy view filters or SQL assembled from template expressions.

Open a named QSQLITE connection on the repository worker with read-only connect options and a bounded busy timeout. Acquire `QReadLocker` on the supplied shared lock and a consistent read transaction while loading all related rows; release both before rendering, scripting, authentication or user review. Query errors are errors, not an empty audience. Test concurrent sync/change behavior using the same lock conventions as FileFinderWorker. Do not reuse `global_DBObjects.getDb()` from this worker.

| Snapshot data | Existing schema/model reference |
| --- | --- |
| Project identity/client/report frequency/EVM inputs | `projects`: `id`, `project_number`, `project_name`, `client_id`, `status_report_period`, `budget`, `actual`, `bcwp`, `bcws`, `bac`; `projectsmodel.cpp` |
| Notes | `project_notes`: `id`, `project_id`, `note_title`, `note_date`, `note`, `internal_item`; `projectnotesmodel.cpp` |
| Membership and status audience | `project_people`: `project_id`, `people_id`, `receive_status_report`, `role`; `projectteammembersmodel.cpp` |
| Attendees | `meeting_attendees`: `note_id`, `person_id`; join through selected notes, not an assumed attendee `project_id` |
| People/company labels | `people.id/name/email/client_id`, `clients.id/client_name`; filter deleted entities in joins as well as base rows |
| Items and comments | `item_tracker` and `item_tracker_updates`; inspect `trackeritemsmodel.cpp`, `notesactionitemsmodel.cpp`, `trackeritemcommentsmodel.cpp` for actual columns and relation keys |
| Activity sections | `status_report_items`: `project_id`, `task_category`, `task_description` |
| Project destinations/context attachments | `project_locations` and `ProjectNotesCommon.get_projectfolder`; preserve path/type interpretation from fixtures |

Every relation excludes soft-deleted records. Preserve optional missing companies as Unknown instead of dropping people through an inner join. Check requested notes actually belong to the requested project. Distinguish no notes/no matching items from a missing/deleted project. Do not infer date storage: `ProjectNotesModel::newRecord` uses epoch seconds, while legacy export presents formatted dates. Keep raw typed dates, display text and parse status separate. Record collection ordering explicitly from legacy fixtures and add stable tie breakers only after identifying the old order.

`CommunicationSnapshot` contains value structs for project, people, memberships, notes, items, comments, activities and locations; stable source-order fields; a load timestamp; and a canonical content/audience fingerprint. It contains no QObject pointers. It can contain internal records for the native report filter, but `TemplateContextBuilder` receives only the filtered view applicable to the chosen report. Audience resolution uses the appropriate candidate pool separately from content filtering.

Before final handoff reload relevant source data and compare canonical fingerprints. If changed, return `SourceChanged`, update the visible review and require another deliberate handoff click. Preserve marked manual edits or offer explicit reload. Freeze the accepted snapshot/revision; do not hold a DB lock across external work or repeatedly rerender inside an adapter. A database switch invalidates old preparations even when project IDs happen to match.

## 6. Audience and recipient model

Keep pure resolution and mutable UI selection separate:

```cpp
enum class PeopleSource { ProjectTeam, MeetingAttendees, StatusRecipients,
                          CurrentSelection, ChosenPeople };
enum class CompanyFilter { All, ManagingCompany, ProjectClient,
                           ExceptProjectClient, SelectedCompanies };
struct AudienceRule {
    PeopleSource source;
    CompanyFilter companyFilter;
    QStringList companyIds;
    QStringList chosenPersonIds;
    bool includeUnknownCompany = false;
    bool excludeProjectManager = true;
};
// Pure functions; no database/network/settings reads:
AudienceResolution resolveAudience(const CommunicationSnapshot&, const AudienceRule&);
RecipientResolution applyRecipientOverrides(const AudienceResolution&,
                                             const RecipientOverrides&);
```

All-company source selections include unknown-company people; `includeUnknownCompany` is an explicit extra choice for restrictive company filters. Multiple chosen companies form a union within the source. Missing required company IDs produce a setup issue and empty result, never All. CurrentSelection comes from a documented per-workflow compatibility rule, not whichever rows are visible. Status starts with Receive Status. Do not assume SendMeetingNotes' Full Project Team label means a new all-project query: preserve its actual exported candidate pool in fixtures.

`RecipientOverrides` records exclusions by person ID/normalized address, explicit role moves and manual addresses, with provenance. Canonical address comparison is trimmed/case-insensitive, without dot/plus rewriting. Keep display spelling and all contributing person IDs. Validate one mailbox per entry; reject separators/header injection and surface malformed addresses, without trying to implement a full RFC mailbox grammar using one regex. Define and test the supported non-ASCII mailbox policy before advertising it.

`RecipientSelectionModel : QAbstractListModel` roles: `personId`, `addressKey`, `name`, `email`, `companyId`, `companyName`, `selected`, `recipientRole`, `includedBy`, `exclusionReason`, `manual`, `outsideAudience`. Provide `toCount/ccCount/bccCount`, `summary`, `issues`, `customized` and commands `setRule`, `setSelected(addressKey,bool)`, `setRole`, `addManual`, `removeManual`, `resetOverrides`. Normalize all mutations in C++, not separate QML array logic. Use one entry per unique address, retaining multi-person provenance. A role collision requires explicit resolution and never promotes Bcc into a visible role.

Run the full product-plan audience matrix. Internal-report/external-audience mismatch is a visible acknowledgement tied to the preview revision, not a silent content change. Source or audience changes invalidate that acknowledgement. Store a preset's rule/overrides, not a stale resolved address list, and re-resolve it for every message.

## 7. Templates: storage, evaluation and editing

### Canonical schema and grammar

`CommunicationTemplate` fields: `schemaVersion`, `id`, `revision`, `name`, `kind` (email/meeting), `workflowIds`, `sourceContexts`, `engine` (fields-v1/legacy-v1), `subject`, `inlineBody`, `attachmentBody`, `fieldFormats`, `fieldFallbacks`, `computedFields`, and `legacyOrigin` metadata. A template is not a report HTML layout. Bundled defaults have stable IDs (`builtin.send-meeting-notes`, `builtin.tracker-items`, `builtin.status-report`, `builtin.meeting-notes-report`) and are read-only; user duplicates get UUIDs.

Implement a literal token parser, not JavaScript eval, for `{{ project.name }}`. Grammar v1 is an identifier path of `[A-Za-z_][A-Za-z0-9_]*` segments separated by dots, optional surrounding whitespace, no indexing/calls/operators. Reserve and reject prototype-related segments (`__proto__`, `prototype`, `constructor`). Backslash escapes literal opening markers; define fixtures for `\{{` and escaped backslashes. Unknown fields are diagnostic nodes rather than copied unresolved text. Replacement output is never parsed recursively as another template.

Represent rich body composition as ordered blocks: `RichText` fragments and a `GeneratedContent` marker. Inline report templates require one generated block; attachment-message templates have none. Editing moves/edits prose without exposing generated report HTML. Ordinary field insertions inside rich text are text nodes, not HTML attributes, CSS or script contexts. Field chips are an editor presentation over stored tokens; they must serialize/reload losslessly. Date/number formats and optional fallback are typed metadata selected through the editor, not an expanding string-expression grammar. In v1 `fieldFormats` and `fieldFallbacks` are maps keyed by canonical field path, applied consistently to every occurrence in that template; an absent fallback differs from an explicitly empty fallback. Different per-occurrence formatting is deferred unless represented by separately named computed fields.

`TemplateContext` version 1 exposes `project.number/name`, `client.name`, `preferences.managerName/managingCompanyName`, `meeting.title/date`, and `report.date/internal/type`, backed by a field catalog that states applicable workflows, scalar type, display formatter and missing-value rules. Additional collections/fields require catalog entries and tests. Avoid automatic exposure of arbitrary QVariantMap keys. Future calendar data gets a distinct `appointment` namespace. Generated report HTML is a separate typed block, not a script-readable `context.html` property.

### Evaluation contract

`CommunicationTemplateService::render(operationId, revision, template, context, generatedContent, emailMode)` emits `rendered` with subject/plain-text/HTML + diagnostics, or `renderFailed`. Pure token expansion runs synchronously internally; computed-field evaluation is asynchronous. Final subject validation and HTML assembly follow computed-field completion. Preview and final handoff use the same result object and revision.

`ComputedFieldRunner` accepts a map of field names to `function compute(context, helpers) { ... }` definitions. Evaluate each independently in a fresh QJSEngine worker context, passing structured read-only values. Return only strings/numbers/booleans; reject objects, promises, functions, non-finite numbers, null/undefined without an explicit fallback, and oversized text. No inter-field dependency graph in v1. Format helpers are pure and receive explicit locale/time-zone values. Freeze results for review; no reevaluation at backend dispatch.

Initial configurable engineering limits: 64 computed fields/template, 64 KiB source/field, 1 MiB serialized context, 64 KiB scalar output/field, 250 ms execution budget/field and 2 s total/template, excluding thread startup. These are safeguards, not report-length limits. Return actionable diagnostics when exceeded; never truncate. Benchmark on supported hardware before release. A cross-thread watchdog invokes `QJSEngine::setInterrupted`; synchronize watchdog/engine destruction. GUI-thread timers alone cannot stop a script that is executing on that same thread. No application QObjects, filesystem/process/network bindings, arbitrary module loader or credentials are exposed. QJSEngine is not a hard-memory security sandbox; only trusted local code runs, and imported scripts require explicit activation. [Qt QJSEngine](https://doc.qt.io/qt-6/qjsengine.html)

For edited prose reuse TextFormatter/QTextDocument's supported rich-text features, not its renderer for generated reports. Imported bodies retain their original source and must pass a reviewed formatting/security conversion. Do not invent a regex HTML sanitizer. Gate G5 in the task guide chooses and verifies the approved rich-HTML normalization policy, including notes and imported templates, while preserving report CSS. Until G5 passes, do not expose arbitrary imported HTML in an unrestricted WebEngine page. Preview profiles deny remote/local arbitrary resources, navigation, downloads and executable content.

### Legacy compatibility

`LegacyTemplateExpander` implements the current XML token traversal without Python. Tests record root attributes, lookup values, one-based rows, nested table ordering and missing markers. The canonical snapshot mapper supplies legacy-shaped values for supported native workflows. Generic legacy contexts not yet modeled remain preserved/importable but are marked unsupported for native execution. Do not reintroduce export-XML/Python as the primary pipeline just to support an unmapped template.

Import reads `PluginSettings`, `Meeting and Email Types/MeetingEmailTypes`, preserving the original JSON and every row field. A stable import origin uses the original definition fingerprint plus stored migration mapping; identical duplicate rows need occurrence identity so neither disappears. Re-import detects changed originals and offers conflict review, never overwrites native edits. Legacy meeting definitions do not become native menu actions until scheduling is implemented.

## 8. Report construction and rendering

`ReportService` dispatches on Workflow and validates the corresponding options. Each builder is a pure function from filtered snapshot/options to `ReportDocument`:

```cpp
struct ReportDocument {
    Workflow workflow;
    QString htmlDocument;
    QString emailFragment;
    QString plainText;
    QString defaultSubject;
    QString fileStem;
    QPageLayout pdfLayout;
};
```

HTML/CSS templates live in resources and use an internal fixed-placeholder assembler. Do not run user scripts to generate report tables, filter records or calculate metrics. `EmailContentBuilder` composes the approved fragment with the user's envelope template and selected attachments; it never recalculates the report. A plain-text rendering is explicit and not permission to downgrade a requested inline-HTML handoff.

### Porting map

| Builder | Python functions to port and freeze in fixtures |
| --- | --- |
| MeetingNotesEmailBuilder | `NoteFormatter.process_xml`, header/attendee/notes/tracker row/footer helpers; default subject `{number} {name} - {note date} {title} Notes` |
| MeetingNotesReportBuilder | `generate_notes_html`, `build_meeting_block`, `build_action_item_row`, inclusion/date logic in `export_notes`; default subject `{number} {name} - {report date}` |
| TrackerItemsReportBuilder | `_is_item_included`, `build_group_status_header`, `generate_tracker_html`, `build_item_row`, grouping/sorting in `export_tracker`; subject `{number} {name} - Tracker Items {date}` |
| StatusReportBuilder | `compute_review_period_dates`, `_parse_number`, `compute_earned_value_metrics`, activity/issue/EVM/appendix HTML helpers, filtering in `export_status_report`; subject `{number} {name} - Status Report {date}` |

Legacy report dates format `MM/dd/yyyy`. Preserve note-date display conventions separately. Tracker/Status/Notes file stems are `{number} Tracker Items`, `{number} Status Report`, `{number} Meeting Minutes`, with ` Internal` when selected. Make filenames safe for the target filesystem while showing any necessary substitution, retaining the original naming convention for valid names. Do not use project text as a directory traversal component.

Tracker status groups: New, Assigned, Resolved, Defered, Cancelled. Priority High/Medium/Low order and due-date descending, invalid/missing last, follow baseline source order for ties. Preserve all columns, comments, colors and group counts. Report internal/external filtering is not the QML global Show Internal view filter. Audit legacy internal-note inclusion/date bugs against the intended product requirements and record the approved correction; do not let the coding model choose silently.

Status calculations intentionally preserve the existing conventions rather than replacing them with textbook alternatives. For actual `a`, earned value `ev`, planned value `pv`, budget-at-completion `bac`: EAC is `a + (bac-ev)/((ev/a)*(ev/pv))` under the original guards; CV is `(a-ev)/ev*100`; SV is `(ev-pv)/pv*100`; completion is `ev/bac*100`; CPI is `ev/a`. Keep None/zero guards, two-decimal display and `No Schedule`. Python rounding and C++ `round` differ at ties: add boundary fixtures and an explicit compatibility formatter, not a blind std::round substitution. Monthly review start uses calendar `addMonths(-1)`; Weekly/Bi-Weekly use -7/-14 days; unrecognized periods remain blank. Snapshot dates are not permission to filter activity records that have no such field.

### Renderer contract and choice

`ReportRenderer : QObject` defines asynchronous `renderPdf(operationId, revision, ReportDocument, destinationPath)`, `cancel(operationId)`, `pdfReady(...)`, `failed(...)`. `WebEngineReportRenderer` uses a non-widget `QWebEnginePage` from Qt WebEngineCore on the GUI thread, with an off-the-record profile and an explicit `QPageLayout`. QML preview uses WebEngineQuick separately. This refines the product plan's native WebEngine choice: no QWebEngineView or QWidget report dialog is needed, and C++ retains precise print-layout control. [QWebEnginePage](https://doc.qt.io/qt-6/qwebenginepage.html)

Preserve Letter landscape/12 margins for tracker, Letter portrait/20 for status, A4 portrait/20 for notes. Capture the legacy QPageLayout units in fixture metadata before making the units explicit in C++; do not infer them from a comment saying “pt.” Assert units as well as numeric margins. [QPageLayout](https://doc.qt.io/qt-6/qpagelayout.html)

Load a staged UTF-8 HTML file through the restricted resource policy, avoiding large-document data-URL constraints. Allow only that operation's approved resource files and bundled assets; never arbitrary `file:`/HTTP requests, popup navigation, downloads or permissions. Handle load failure, renderer termination, PDF failure, cancellation and timeout separately. Wait for the documented load/print completion signals; do not sleep or assume a file existing means it is finished. Validate nonempty PDF output before publishing/attaching. Late completion can only affect that operation's staging file.

Render PDF for each of the three Report workflows even in Inline HTML/Do not email modes as the legacy path does. Retain standalone HTML only when selected, while HTML attachment still requires an operation-staged HTML file. Preview body edits do not cause report PDF regeneration unless report source/options changed. No-email generation works without authentication/backend selection and never instantiates a handoff.

## 9. Preparation state machine and backend boundary

`CommunicationsController` orchestrates preparation. `ReportService` builds content/artifacts. `EmailService` validates immutable requests and dispatches backends. Do not combine these three into a monolithic controller.

```text
EditingOptions -> LoadingSnapshot -> BuildingReport -> RenderingArtifacts
  -> ReviewingEmail -> Revalidating -> FreezingRequest -> PreparingBackend
  -> PresentingExistingMessage -> Completed

No-email: RenderingArtifacts -> PublishingReport -> optional PDF open -> Completed
Any preparatory error -> preserve editable state + diagnostic
Possible external creation -> PartialDraft / OutcomeUnknown + recovery, not Editing
```

SendMeetingNotes skips the report-options/PDF stages. A preview-revision increase invalidates old work. Every callback carries operation ID + revision; account-bound calls also carry account generation. Final request values cannot be changed after dispatch. Backend-selection changes rerun capability preflight before any external mutation.

Before presenting an email, open the PDF if requested, release QML modal focus, then present/activate the email as the final foreground action. QML acknowledges the popup has closed before the controller calls presentation; do not sequence this with an arbitrary timer. Retain a nonmodal status/recovery surface in the main window. Never reopen a ProjectNotes success dialog over the email.

```cpp
class EmailBackend : public QObject {
    Q_OBJECT
public:
    virtual BackendCapabilities capabilities() const = 0;
    virtual void prepare(const EmailRequest&) = 0;
    virtual void present(const QUuid& operationId, const PresentationHandle&) = 0;
    virtual void cancel(const QUuid& operationId) = 0;
signals:
    void progress(QUuid operationId, QString stage, qint64 done, qint64 total);
    void prepared(QUuid operationId, PreparedMessage message);
    void presentationFinished(QUuid operationId, PresentationResult result);
    void failed(QUuid operationId, ServiceError error, RecoveryInfo recovery);
};
```

Some clients combine creation and display into one call. For those, `prepare` only stages/validates; `present` performs the single irreversible launch. `PreparedMessage.preparationKind` distinguishes StagedForLaunch from MailboxDraftCreated; do not report “draft created” for staged files. Once a one-shot launch is attempted, retain its outcome and do not call it again automatically.

`PresentationHandle` is a tagged C++ value with backend identity plus Graph draft ID/webLink, Outlook helper handle, Mail outgoing-item identity, or a one-shot compose launch descriptor. `PresentationResult` separates visibility and focus as Confirmed, RequestedUnconfirmed, Failed or NotAttempted. A URL launch success is not verified keyboard focus. Record actual reusable-handle support; Thunderbird/mailto may support only manual app activation after launch.

`EmailService::retryPresentation(operationId)` operates only on the stored handle, with no rebuilding/upload/create calls. `retryRemainingAttachments` is a separate explicit Graph recovery action. Discarding an exact app-created partial Graph draft requires confirmation and matching account identity; never delete after user handoff as cleanup. Cancel after a possibly processed request yields OutcomeUnknown or PartialDraft, not a claim that nothing happened.

### Adapter specifications

| Adapter | Implementation boundary and required tests |
| --- | --- |
| Graph | Inject Office365Service + HttpTransport. POST `/me/messages`, persist ID/webLink, attach completed files, then present existing link. Allowlist mail draft/attachment operations; no `/send`, `/sendMail` or calendar endpoints. Test HTTP errors, size boundaries, partial uploads, identity changes and lost create responses. |
| mailto | Pure builder uses QUrl/QUrlQuery with RFC6068 tests, then injected URL launcher. Explicit plain-text/manual route only. Prototype selects a conservative encoded-length limit; reject oversized payload instead of truncating. No attachment parameter or HTML claim. |
| Outlook Classic | Windows adapter starts bundled helper via QProcess pipes. Helper owns STA COM and MailItem/Inspector; typed JSON request, attachment validation, signature-aware composition, display and activation. No `.Send()`. Unavailable on non-Windows or new-Outlook-only systems. |
| Thunderbird | Discover executable or explicit override, build argument list without a shell, stage UTF-8 body, use proven compose grammar/body-file mechanism. Test quotes, commas, non-ASCII paths, already-running process and sandbox file access. Process exit does not prove draft completion. |
| macOS Mail | Objective-C++ bridge uses fixed Apple Events/script logic and typed parameters targeting `com.apple.mail`; no generated script source from user data. Prototype must prove editable inline HTML and attachment parity; denied consent/unproven rich HTML is explicit, not a PDF fallback. |

Graph small attachments and upload sessions follow the product plan and current Microsoft limits; stream large file chunks rather than base64-loading an entire large attachment. Validate aggregate mailbox size separately. Honor `Retry-After` with asynchronous capped backoff. Transport reports status/headers/network outcome and never generically retries mutating calls. GET and resumable chunk recovery need explicit operation-specific policies; a lost draft-creation response never leads to blind POST replay. Upload-session URLs are sensitive and must never receive an unrelated Graph bearer header or be logged. [Graph attachment guidance](https://learn.microsoft.com/en-us/graph/outlook-large-attachments)

Graph authenticates only after the user selected that route and requested handoff. Validate returned webLink as an allowed HTTPS Outlook link before launching; support the configured Microsoft cloud only, with other clouds explicitly unavailable until tested. Preserve account mismatch/browser-sign-in recovery. Draft ID and attachment completion are known independently of whether Outlook can open the exact draft in edit mode.

Helper IPC schema: `protocolVersion:1`, `operationId`, `command` (prepare/present/cancel/close), and typed payload; response includes the same operation ID, stage, outcome certainty and opaque item handle. Use newline-delimited JSON with escaped strings and a bounded message size; reject duplicate/unknown operations and mismatched protocol versions. No secrets or message bodies in process arguments. Bound helper requests to app-owned artifact paths already verified by the parent. Keep the helper alive while a transient item handle is needed; on helper loss offer uncertain-result/manual recovery, never recreate silently. Killing a helper is not permission to kill Outlook or discard a displayed email.

`ExternalPresentation` and process/URL seams are injectable, platform-specific only where necessary. Linux/Wayland focus restrictions are legitimate RequestedUnconfirmed outcomes; do not use brittle title matching as proof that the correct message has focus.

## 10. Local settings, authentication and artifact lifetime

### Settings

Use existing profile organization `ProjectNotes` + developer-profile and `AppSettings`, with fallbacks disabled. Store scalar settings under `Email/v1/PreferredBackend`, `Email/v1/ThunderbirdPath`, and `Reports/v1/{workflow}/ExportSubFolder`. Template/audience JSON stores live under the same profile-specific data directory, in `communications/v1/`, written atomically with QSaveFile. No credentials or synchronized DB changes.

Define `databaseKey` as a SHA-256 of the normalized canonical absolute DB path for v1; this is explicitly local, not a synchronized database UUID. A moved/copied database does not automatically inherit specific-person/company presets. Store its display path and offer explicit reviewed reassociation/import, rather than guessing two databases are the same. Test Windows path normalization and aliases. Defaults precedence: explicit project/workflow override > database/workflow override > profile workflow default > bundled default. Profile-wide presets cannot contain database-specific IDs without a database scope.

Version every stored JSON envelope. Validate before replacing the file, preserve corrupt originals and offer reset/import recovery. Migration markers are written only after successful native persistence and re-read. Never overwrite an existing native value with a legacy default. Import old ExportSubFolder strings intact; preserve original MeetingEmailTypes data and unrelated consumers.

### Office 365 extraction

Keep `FileFinder/tenantId`, `FileFinder/clientId` as canonical compatibility keys. `Office365Service` is their sole writer. FileFinder's current settings/properties may temporarily delegate for compatibility, then SettingsPage binds to the token-free `Office365SettingsModel`. FileFinder must not overwrite shared IDs from cached fields when saving roots/rules.

Move OAuth implementation once. Inject a shared service into FileFinder; tests inject a fake or a test-owned real service with fake transport/secrets. The composition root supplies CredentialStore callbacks, preserving service name `Office365FileFinder`. For the normal profile retain the current `tenant/client` account key initially. Nonempty developer profiles use a versioned profile-qualified key and never read/delete the unqualified production entry. Previously shared developer-profile credentials require a fresh sign-in; do not copy an ambiguous production secret. Keep one active Microsoft account per profile in v1.

Scopes are explicit capability bundles: FileFinder keeps Team.ReadBasic.All, Channel.ReadBasic.All, Files.Read.All; email drafts add Mail.ReadWrite; offline_access supports refresh. To show and bind the actual email account, the proposed email setup also requests User.Read and calls `/me?$select=id,displayName,mail,userPrincipalName`; this is an intentional identity-read addition, not a mail-send or directory-wide permission. Denied identity/mail consent prevents that feature, not unrelated authorized FileFinder work. Do not require User.Read just to restore existing FileFinder-only behavior. [Graph signed-in user permissions](https://learn.microsoft.com/en-us/graph/api/user-get?view=graph-rest-1.0)

Track requested versus granted scopes, verified mailbox identity and session generation separately. Broader consent is requested only after a feature action; serialize it with refresh and do not start simultaneous device-code sessions. A denied incremental-consent attempt must not destroy a still-valid FileFinder token/session. Publish only tokens with the capabilities required by each consumer; a missing scope is not inferred from generic authenticated=true. Clear identity-bound FileFinder state on account changes, not just tenant/client changes. Pause/invalidate queued work on sign-out or configuration changes.

Tokens remain in C++ implementation details and credential storage. The QML facade exposes tenant/client, status, account display label, device code/verification URL, feature consent status and sign-in/out commands only. Do not expose token-bearing QObject signals through the UI facade. No Python token-cache import. Draft recovery records carry account identity; never retry an old draft against a new `/me`.

### Artifacts and recovery

`ArtifactStore` owns `communications/staging/<operation UUID>/` under the profile directory. Manifest fields: schema version, operation ID, workflow/source identity, safe artifact metadata, backend, state, known provider ID/webLink, account key and timestamps. Restrict permissions; treat body/recipient data as sensitive. No refresh/access tokens in manifests. Treat upload URLs as secrets: keep in memory initially and use explicit recovery on restart rather than storing them as ordinary metadata.

Stage immutable copies of user-selected attachments for handoff, retaining provenance; never move/delete originals. Hash staged files and validate size/readability before dispatch. A changed original after copying does not alter the reviewed attachment. Destination report publishing uses an atomic temp-to-final operation; ask before overwriting an existing material report unless an explicitly saved overwrite policy covers it. Preserve staged output on destination failure and offer Save elsewhere.

Do not use QTemporaryDir's default auto-remove lifetime for handed-off files. Retain local-client staging after dialog/app close because copying is generally unobservable. Cleanup accepts only exact manifest-owned paths verified inside that operation directory, never arbitrary source paths/globs. Offer explicit cleanup with an open-draft warning; no age-only automatic deletion promise. Graph staging can be released only when all required uploads and possible recovery uses are complete. Startup offers recovery status and manual cleanup, but never auto-resumes external creation or sending.

## 11. QML/controller contract

Expose `DesktopAppController.communications` (CommunicationsController) and `DesktopAppController.office365` (Office365SettingsModel). Keep any product-plan `email` alias as a delegating facade only if needed; do not have two independent operation owners. New services are constructed in C++ and registered as uncreatable QML types where required; dialogs receive owned model references. Do not instantiate new account/services in every QML component.

`CommunicationsController` public commands:

```text
beginReport(workflowId, projectId) -> bool accepted
beginMeetingNotes(noteId) -> bool accepted
generatePreview()                 # validates ReportOptionsModel
submitHandoff()                   # snapshot/account/capability revalidation
cancelCurrent()
retryPresentation(operationId)
retryRemainingAttachments(operationId)
discardPartialDraft(operationId)  # explicit confirmation flow first
saveAudiencePreset(name, scope)
saveTemplateAs(name, scope)
```

Properties: `busy`, `stage`, `operationId`, `options`, `preparation`, `issues`, `recoveryAvailable`. Signals: `reportDialogRequested`, `emailDialogRequested`, `closePreparationForHandoff`, `statusChanged`. Add a matching `preparationClosedForHandoff(operationId)` acknowledgement; stale acknowledgements are ignored. Test failures reopen/reveal appropriate editable state without stealing focus after successful handoff.

`EmailPreparationModel` exposes subject, body-editor blocks, backendId, recipient model, attachment model, preview URL/plain text, revision, manual-edit flags, handoff label and canHandoff. Its setters invalidate validation and preview as appropriate; arbitrary setters cannot change an already frozen request. `ReportOptionsModel` owns report-specific validation/defaults, not QML-local duplicated logic.

`CommunicationTemplateEditorModel` exposes field catalog, draft template, rendered preview, diagnostics, advanced-enabled state and save/reset commands. Keep uncommitted editing separate from persisted template revisions. A token-free Settings pane reports backend capabilities/availability with repair hints. Reuse Theme and existing controls; keyboard focus order and screen-reader labels include audience counts and validation errors.

Route main-menu and record-menu actions through one save-before-action function in Main/current page. It calls current-page save, verifies a stable persisted ID, then invokes the native controller. Save pending child table editors too. A save failure produces no report, auth or external call. New Report actions are native first-class menu entries; unrelated plugin menus and Export XML remain untouched.

While migrating, suppress only exact replaced bundled plugin actions once their native counterpart is enabled, using module/function identity rather than translated labels. Do not hide user plugins with similar names. Final removal replaces temporary suppression with removal of obsolete definitions/files. The optional Widgets frontend requires a working thin adapter to shared native services before shared plugins are removed; if that cannot be built/tested, stop the removal task for a maintainer decision rather than shipping dangling menu calls.

## 12. CMake, packaging and test seams

New targets: Integrations links Qt Core/Network/Qml; Email links Integrations and Qt Core/Gui/Sql/Xml/Network; Rendering links Email and Qt WebEngineCore. Shipping desktop and QML smoke tests also link WebEngineQuick for preview. FileFinder links Integrations and removes its duplicate OAuth source entry. Root CMake adds targets in dependency order, guarded away from mobile-only configurations and linked only where consumed. Platform adapter sources use WIN32/APPLE guards; unsupported stubs have the same interface on other hosts. Enable OBJCXX only for Apple code and build the COM helper only on Windows.

Initialize `QtWebEngineQuick::initialize()` before QApplication in both app and WebEngine-enabled test runners. Ensure packaged QtWebEngineProcess/resources match the linked Qt libraries; the app's existing PyQt helper-path discovery must not select an incompatible Python-wheel helper for native rendering. Treat that deployment audit as a release gate, not an environment-variable workaround. [Qt WebEngine Quick initialization](https://doc.qt.io/qt-6/qtwebenginequick.html#initialize)

Keep new QML files in the top-level `qml/` directory so the current test glob finds them, and add them to shipping `DESKTOP_QML_FILES`. Add identical C++ sources/registration to app/test builds or factor only the new common source list into a small CMake include. Do not duplicate the entire QML module or broadly refactor packaging in an unrelated task. Explicitly initialize any static-library resource collection in both executables so linker dead-stripping cannot remove report templates.

Fast tests use fake HTTP, credentials, backend, launch, renderer, repository and clock; fail any unexpected external call. Repository tests use a throwaway SQLite database with real schema-relevant fixtures and no production profile. Renderer tests have a separate executable/environment using real WebEngine; a missing graphical/WebEngine environment is a reported gate, not a skipped assertion counted as passing. Existing QML smoke tests load the actual shipping module with fake backend handoffs.

Use the product plan's separate application/test build commands. Add CTest names `communicationtemplates`, `communicationrepository`, `recipients`, `reportcontent`, `email`, `office365`, `reportrenderer`. Exact test sources can share test-only fake helpers, not alternate production logic. Automated tests never use real accounts, mail clients, calendars or destructive cleanup targets.

Windows packaging includes the Outlook helper and removes exact obsolete bundled plugins on upgrade. Signed macOS builds include Mail automation entitlement/usage description and tested helper/resource deployment. Linux conventional and sandboxed launch/file access are separately recorded capabilities. Do not disable Chromium sandboxing in production to make a local test pass.

## 13. Scheduling extension: design now, code later

Reserve template kind `meeting` and `appointment` context, but do not add a fake calendar adapter or request calendar scopes now. A future `ProjectNotesMeetings` module can depend on Integrations and the extracted audience value rules; avoid creating it until approved. If that extraction is needed, move only audience data/resolution to a shared communications subdirectory, not the entire Email service.

Future `MeetingRequest` needs organizer/account, required/optional/resource attendees, typed zoned start/end, location, subject/agenda and attachment artifacts. It is not EmailRequest with an ICS flag. Share templates, identity, immutable snapshots and presentation semantics, but keep calendar operation outcomes and invitation authorization distinct.

The existing Graph scheduler's `isDraft:true` is not a proven no-invitation contract. No native `/me/events` POST/PATCH until the user approves a verified review path or an explicit Schedule and send invitations action. Thunderbird mail compose, macOS Mail and mailto do not automatically provide calendar scheduling. The future task must independently prove each calendar capability and migrate configured meeting types before deleting their Python handlers. [Graph event creation](https://learn.microsoft.com/en-us/graph/api/user-post-events?view=graph-rest-1.0)

## 14. Review checkpoints

Review contracts/types before builders; OAuth/credential isolation before real-account tests; HTML/script policy before import previews; backend prototypes before advertising support; visual report parity before removing plugins. Use focused review for these boundaries rather than asking a lower-cost model to improvise around an unresolved security or API question. The task guide names these gates and records which work can continue independently.
