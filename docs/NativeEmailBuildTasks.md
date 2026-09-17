# Native email/report implementation task guide

Branch: `feature/email`. Prepared: 2026-09-12. All tasks are **planned**, not implemented or tested. This file is an execution guide for a coding model; writing the plan does not authorize real emails, meeting invitations or account changes.

## Start here

Read repository AGENTS.md, [product requirements](NativeEmailImplementationPlan.md), and the sections of [code architecture](NativeEmailCodeArchitecture.md) named by the assigned task. Use CodeGraph first for code discovery. Preserve unrelated work. Implement one task or explicitly named subtask, compile/test it, and stop with a concise handoff. Do not remove Python early or mark a platform capability supported because it compiled on another OS.

The tasks below specify intended file ownership, not permission to edit unrelated code. If a necessary integration file is missing from a task, explain the narrow change in the handoff. If it changes a public contract or product behavior, update the spec and obtain direction where needed before continuing. Do not add placeholder success returns or permanently skip failing tests to satisfy a checklist.

Suggested instruction for each coding session:

```text
On feature/email, implement task T__ from docs/NativeEmailBuildTasks.md only.
Read AGENTS.md and the referenced architecture/product sections first.
Recheck the current worktree and prerequisite task status; do not assume they passed.
Preserve unrelated edits and use the established interfaces and test seams.
No real emails, calendar writes, account changes or plugin deletion unless the task
explicitly includes that step and any stated maintainer approval has been obtained.
If a prototype gate fails, record evidence; do not substitute reduced behavior.
Run the task's checks. Report changed files, actual commands/results, unresolved
issues and the next eligible task. Do not commit unless asked.
```

## Evidence and checkpoints

During T00 create `docs/email-build/Progress.md` and `docs/email-build/Compatibility.md`. These files do not exist merely because this guide names them. Use Progress for task status (`planned`, `in progress`, `implemented`, `verified`, `blocked`) with actual commands/results and concise pending work. Compatibility records OS/client/Qt/build versions, exact operation, observed formatting/attachments/presentation behavior, and limitations. Never label unrun platform tests verified.

Keep provider tokens, real addresses, report contents and private filesystem paths out of evidence committed to the repository. Use synthetic fixtures and sanitized diagnostics. Do not check in personal screenshots, mailbox responses or real note exports. Record source revision/hash and intentional legacy bug corrections next to fixture metadata so expected output is reproducible.

### Prototype/review gates

| Gate | Required evidence | What must wait if unresolved |
| --- | --- | --- |
| G1 Report baseline | Synthetic fixtures for all four workflows, defaults, subject/filenames, report layout units, calculations, internal/date behavior; explicit correction decisions | Final report parity and plugin removal; infrastructure work can continue |
| G2 OAuth/identity isolation | FileFinder behavior preserved; production/developer credentials separated; feature consent and account-switch cancellation tests reviewed | Real-account email handoff and release of auth extraction |
| G3 Backend feasibility | Graph exact-draft opening; mailto encoding bounds; Classic helper/display; Thunderbird grammar/sandbox access; signed Mail inline HTML + attachments | Only the unproven adapter/mode is blocked; do not advertise its support |
| G4 Renderer/deployment | Native HTML/PDF parity, signed/packaged native QtWebEngine helper/resources, app and real QML tests load | Report replacement/removal and release |
| G5 HTML/script handling | Formatting-preserving rich-content normalization choice, resource-denial tests, field escaping, bounded trusted JavaScript and import activation | Arbitrary imported HTML/script previews and release of template import |
| G6 Removal | All four native paths pass with replaced modules absent; optional Widgets path explicitly handled; upgrade cleanup exact | Deleting bundled Python paths and declaring completion |
| G7 Calendar contract (future) | Verified no-invitation compose route or explicit user approval for Schedule and send invitations | All native calendar POST/PATCH and scheduling-plugin removal |

G5 is deliberately a focused design/prototype task: compare the existing Qt rich-prose round trip with a parser-based policy using synthetic legacy HTML. Preserve the report templates separately. Record exactly which elements/styles/resources survive and which require user review. Do not let an implementing model invent a regex sanitizer, silently strip layout, or add a new third-party parser without reviewing its license/deployment cost. If neither evaluated approach satisfies parity/security, stop that part for a maintainer decision while other work proceeds.

## Dependency map

```text
T00 inventory/baselines
  T01 contracts/build -> T02 artifacts/settings -> T03 snapshots -> T04 audience
  T01 -> T05 template tokens -> T06 computed fields -> T07 template migration
  T03 -> T08 notes content -> T09 status math -> T10 tracker/status content
  T02 + T08 -> T11 renderer
  T04 + T07 + T10 + T11 -> T12 preparation controller (fakes)
  T01 -> T13 auth extraction -> T14 scopes/identity -> T15 Graph
  T02 + T01 -> T16 mailto / T17 Classic / T18 Thunderbird / T19 Mail
  T12 -> T20 report/email UI -> T21 audience UI -> T22 template/settings UI
  T15..T22 -> T23 action routing and end-to-end integration
  T23 -> T24 packaging + Widgets bridge -> T25 full verification
  T25 + G6 -> T26 Python removal/upgrade validation -> T27 release handoff
```

The map indicates dependencies, not an instruction to run parallel agents. Platform prototype portions of T15–T19 and the G5 comparison may be performed early with synthetic content, without claiming their final implementations complete. Each adapter can be developed/tested independently once its contract and fakes exist. Missing a particular OS need not stop platform-independent tasks.

## T00 — Inventory, synthetic fixtures and capability ledger

Read: product parity/audience/templates/scheduling sections; architecture §§2, 5–8. Edit: `tests/desktop/fixtures/email/*`, `docs/email-build/*`; test-only fixture capture tooling if necessary. No production behavior changes.

1. Record current source revision, actual Qt/Python versions and build availability. Inventory existing schema and report template functions, option controls/defaults, menu identities and shared helper consumers.
2. Create synthetic projects with managing/client/partner/unknown companies, manager/non-manager contacts, duplicate/missing emails, attendees versus project membership, internal/external notes/items, every status/type and date edge cases.
3. Capture legacy HTML/subjects/recipients/options/filenames without invoking email/calendar adapters. Pure functions can be called in isolated test capture tooling; intercept all collaboration, process and network paths. Do not import a plugin blindly if import-time setup can touch production settings.
4. Capture renderer metadata including actual QPageLayout units, fonts and page sizes; create visual baselines only in a suitable environment. Mark any missing capture pending, not passed.
5. Record intended-vs-actual internal-note/report-date discrepancies and obtain explicit expected-fixture decisions. Preserve actual context-based default recipients even when menu labels suggest something else.

Accept: fixtures contain no personal data; all four workflows have a documented coverage matrix; no send/launch/account changes occurred; G1 status is explicit.

## T01 — Libraries, value contracts and test harness

Read: architecture §§3–4, 12. Edit: new module CMake files, contract headers, root CMake, desktop/test CMake registration as needed, test fakes.

Create Integrations, Email and Rendering target boundaries; register queued value types; add stable enum/string conversions and validation result types. Declare asynchronous repository/renderer/backend/HTTP/presentation seams with explicit operation identity. Add minimal fake implementations in tests only. Keep platform-specific code out of common sources and resource registration explicit. Do not move OAuth or add QML menus yet.

Accept: app and isolated test configuration still build; focused contract tests cover enum round-trip, invalid values and fake call recording. No “unsupported” stub may report a successful real handoff. Interfaces reviewed before dependent implementations proceed.

## T02 — Settings scope, artifact store and operation recovery

Read: architecture §10. Edit: `EmailSettingsStore.*`, `ArtifactStore.*`, related types/tests.

Implement profile/path-based database scoping, atomic JSON persistence and exact operation-owned staging manifests. Add immutable user-attachment copies, hashes, filename validation, destination conflict handling, corruption recovery and startup status loading. Keep local-client files beyond operation/UI lifetime. Support explicit cleanup only for validated manifest-owned files. Persist enough draft identity for recovery without tokens or upload URLs. Do not launch anything on recovery.

Accept: tests cover profile/database collisions, path traversal/symlink escape, user original preservation, duplicate output names, failed writes, partial manifest, app restart, cancelled operations and cleanup restrictions. Unwritable destinations retain staged output.

## T03 — Read-only communication snapshots

Read: architecture §5; existing FileFinder worker and model/schema sources. Edit: `SnapshotTypes.h`, `CommunicationRepository.*`, repository tests/fixtures.

Implement thread-owned read-only connection and transactional snapshot queries with bound IDs and deleted-row filters. Map typed dates/display values and candidate provenance. Preserve missing-company contacts. Validate project/note relationships. Add fingerprints and database-generation invalidation. Freeze actual legacy ordering in fixtures rather than relying on unordered SQL.

Accept: repository tests include deleted/missing rows, filtered UI independence, concurrent read/change, invalid dates, multi-meeting contexts, DB switch and orderly shutdown. No cross-thread shared SQL handle or lock held while rendering/waiting for the user.

## T04 — Pure audience resolution and recipient model

Read: product company-audience section; architecture §6. Edit: `RecipientAudienceResolver.*`, `RecipientSelectionModel.*`, audience persistence/tests.

Implement shortcut rules, source/company intersection, manager-ID exclusion, unknown-company opt-in, role deduplication and explicit overrides. Model counts/reasons/manual additions/reset and default scoping. Add empty-result/setup diagnostics and explicit address-later state. Keep content filtering independent.

Accept: full company/source matrix, receive-status flags, changed addresses/membership, duplicate provenance, To/Cc/Bcc conflicts, no Bcc promotion, preset reload and source-change behavior. Same final recipient list must be usable by every adapter.

## T05 — Field catalog, parser and legacy replacement

Read: architecture §7. Edit: `TemplateTypes.h`, `TemplateContext.*`, `TemplateParser.*`, `LegacyTemplateExpander.*`, template tests.

Implement fields-v1 grammar/escaping, typed field catalog, fallback diagnostics, rich block structure and subject validation. Implement legacy-v1 literal traversal for supported contexts. Map snapshot fields explicitly, including lookup values and optional fields; never eval replacement text. Add empty/default envelope fixtures that preserve legacy content exactly.

Accept: repeated/nested legacy row tests, unsupported contexts, malicious replacement strings, unresolved fields, escaping, literal token syntax, header injection and exactly-one generated block. No JavaScript is required for basic template use.

## T06 — Optional computed fields and G5 review

Read: architecture §7 and G5 above. Edit: `ComputedFieldRunner.*`, `CommunicationTemplateService.*`, associated policy helper chosen by G5, tests.

Implement structured read-only context, bounded scalar outputs, fresh worker-owned engine, synchronized cross-thread interruption and explicit cancellation. Imported scripts are inactive until reviewed/enabled. Complete or explicitly record G5's HTML-policy decision before exposing import previews. Keep report HTML outside the editable prose normalization path.

Accept: syntax/runtime errors, primitive throws, infinite loop, output/context limits, cancellation/destruction race, forbidden bindings, data-as-code strings and inactive import tests. Report diagnostics; do not crash/freeze QML or claim hard-memory sandboxing. G5 evidence must show formatting and resource policy tests, not just script tests.

## T07 — Template/default migration and storage

Read: architecture §§7, 10. Edit: `CommunicationTemplateStore.*`, envelope resources, settings migration tests.

Implement bundled read-only templates and user revisions, per-workflow defaults, legacy JSON import with duplicate-row identity, conflict handling and preserved originals. Wire per-mode bodies and optional audience preset references without coupling content save to recipient save. Import scheduling definitions only as preserved data, not active native calendar commands. Unsupported generic contexts remain visibly unsupported.

Accept: idempotent import, edited-original conflicts, identical duplicate definitions, corrupt JSON, profile/database isolation, fallback/default precedence and unchanged Python settings. Do not request OAuth or load real mailbox data to preview templates.

## T08 — Native meeting-note content builders

Read: architecture §8; legacy noteformatter/exportnotes functions and fixtures. Edit: `MeetingNotesEmailBuilder.*`, `MeetingNotesReportBuilder.*`, relevant HTML/CSS resources and content tests.

Port the separate SendMeetingNotes and project-wide MeetingNotesReport layouts; retain rich notes/action tables, subjects, attendee names, date formatting, filenames and branding. Apply the explicitly resolved internal/date behavior from G1. Build ReportDocument values without Python, dialogs, filesystem writes or network calls.

Accept: structural/golden HTML comparison on representative fixtures; generated fragment/body and standalone document are consistent; note-date versus reporting-date behavior is tested. Legacy layouts must not be rewritten as a new generic table design.

## T09 — Status report calculations and formatting

Read: architecture §8; `compute_review_period_dates` and `compute_earned_value_metrics`. Edit: `ReportFormatters.*` and status calculation helpers/tests.

Port parsing, zero/missing guards, original CV/SV/EAC conventions, two-decimal formatting and review-period dates. Separate numeric calculation from translated/display text. Test Python rounding boundary behavior explicitly. Do not change the shared Projects model or redefine formulas as part of this port.

Accept: table-driven tests for missing/zero/negative/normal values, formatted inputs, halfway rounding, leap years/month-end/weekly/biweekly/unknown report period. Compare against stored legacy expected outputs, not a second copy of the new formula.

## T10 — Tracker and status report builders

Read: product option/default parity; architecture §8. Edit: `TrackerItemsReportBuilder.*`, `StatusReportBuilder.*`, `ReportService.*`, HTML/CSS resources and content tests.

Implement every option/default, correct status/type filters, stable grouping/sorting, comments, status activities/stakeholders and EVM appendix. Keep report destination behavior in the artifact/service layer and recipient resolution in the audience layer. Validate irrelevant options instead of silently applying them.

Accept: all status/type combinations, unknown/missing dates, empty results, internal/external cases, section counts/order, exact subjects/stems and HTML structure. ReportService dispatches all four workflows with no plugin call or QML business logic.

## T11 — Real WebEngine renderer and report publication

Read: architecture §§8, 10, 12. Edit: Rendering module, renderer tests, native resource/profile policy, initialization/CMake links.

Implement GUI-thread QWebEnginePage lifecycle with explicit layout and async completion; restricted operation-local resources; safe file publication; timeouts and renderer crash/cancellation handling. Add WebEngineQuick preview dependency/init as specified. Validate QtWebEngineProcess selection against linked native Qt, not just Python wheel paths. Avoid changing report styles to repair deployment errors.

Accept: real PDFs for each report match baseline sizes/margins/pagination/content; large HTML works; failure/cancel/stale completion retain safe state. Test no-email without account/backend and every retain-HTML/email-mode combination. G4 is pending until real platform/package evidence exists.

## T12 — Preparation controller and dispatch state machine with fakes

Read: architecture §§9, 11. Edit: `EmailService.*`, `EmailContentBuilder.*`, desktop controller/preparation/options models, focused tests.

Wire snapshot -> filtered report -> template -> artifacts -> review -> source revalidation -> immutable request. Use fakes for all external adapters. Add one-active-operation rule, Busy handling, preview revisions, explicit internal/external acknowledgement, account-generation validation and nonmodal recovery. Implement staged-versus-created distinction and one-shot handoff tracking. Closing a dialog must not lose an external result.

Accept: duplicate clicks, source changes, manual-edit preservation, cancelled/stale callbacks, failed publishing, account change, partial draft and unknown outcome tests. Presentation retry makes zero create/upload calls. Display PDF precedes final email activation. No-email never asks for an account or calls an email backend.

## T13 — Extract Office 365 service without behavior changes

Read: architecture §§2, 10; current OAuth/FileFinder sources. Edit: moved OAuth files, Office365Service, FileFinder injection, composition root/CMake, OAuth/FileFinder tests.

Move, do not duplicate, MicrosoftOAuthManager. Inject HTTP/secrets/settings dependencies and make shared service sole tenant/client writer. FileFinder forwards compatibility properties and receives only the token capability it needs. Preserve existing device-code flow/scopes and session behavior in this first extraction task; implement profile-qualified secret isolation as specified and record required developer-profile re-sign-in. Do not add email consent/UI yet.

Accept: existing FileFinder tests plus sign-in/restore/refresh/error/sign-out mocks; saving roots does not overwrite account settings; normal profile key preserved; developer sign-out cannot remove production credentials. Review this diff separately before T14.

## T14 — Feature scopes, account identity and token-free facade

Read: architecture §10. Edit: Office365/OAuth service, `Office365SettingsModel.*`, tests.

Add explicit capability consent, serialized refresh/sign-in, granted-scope tracking, identity lookup for email, account/session generation and safe token-free facade. Preserve valid FileFinder operation after denied email consent. Expose actual account label before draft handoff; never equate a configured client/tenant with a verified mailbox.

Accept: email-only sign-in avoids Teams/files scopes; FileFinder-only restore does not acquire new identity scopes; email asks only for planned mail/identity/offline scopes; no Mail.Send/calendar scopes. Test missing scopes, failed `/me`, renewed consent, concurrent consumers, account/tenant/client change, late callbacks and credential failure. Complete G2 review before real-account tests.

## T15 — Graph drafts, attachments and presentation

Read: architecture §9; product Graph adapter contract. Edit: GraphEmailBackend, HTTP support needed for streamed upload, recovery tests.

Implement create-ID-record-attach-open sequence; small/large attachment branches, size preflight, throttling, cancellation and outcome certainty. Constrain accepted endpoints and returned links; keep upload-session secrets out of logs and bearer headers off preauthorized upload URLs. Open the existing draft only after required attachments finish. Implement presentation retry and separate remaining-upload recovery. Verify that a browser/account issue preserves draft identity.

Accept: recorded fake requests show no send/calendar endpoints; tests cover boundaries, Unicode, missing attachment, consent failure, lost POST response, partial uploads, duplicate clicks, wrong account and presentation failure. Controlled real-mailbox draft tests require maintainer authorization and must not send; G3 records editability/focus observations separately from draft creation.

## T16 — mailto and manual fallback

Read: architecture §9 and product mailto limitations. Edit: MailtoEmailBackend, URL/presentation seam, tests.

Implement explicit plain-text capability preflight, RFC6068 encoding and tested encoded-length limit. Preserve original requested HTML/attachment mode in UI diagnostics. Manual workflow stages/reveals files and copies content only after explicit user choice; it does not pretend attachments were added. Remember one-shot launch attempts and support only honest activation/recovery.

Accept: plus/percent/ampersand/comma/quotes/newlines/Unicode encoding, header-injection rejection, length boundaries and explicit empty-recipient behavior. No silent truncation or HTML/attachment success claim.

## T17 — Outlook Classic helper

Read: architecture §9 and product Classic requirements. Edit: Windows backend/helper sources, helper CMake, fake IPC tests; package declaration later in T24.

Implement versioned typed IPC and STA late-bound COM with exact item identity, recipient roles/resolution, signature-aware HTML/body and attachments. Stage then display/activate as the final handoff. Handle helper crash/hang/protocol failure as uncertain when necessary. Avoid user-data interpolation into shell commands and never call Send or terminate Outlook.

Accept: IPC/command rejection and timeout tests on all build hosts where possible; Windows Classic manual tests with closed/running/minimized client, signatures, Unicode files and unresolved recipients. New-Outlook-only is unavailable, not falsely detected as Classic. G3 needs actual Windows evidence.

## T18 — Thunderbird compose adapter

Read: product Thunderbird section, architecture §9. Edit: Thunderbird backend/discovery/argument encoder and tests.

Implement validated executable selection, UTF-8 body-file staging, compose grammar and file URL encoding without a shell. Preserve all role/subject/body/attachment fields. Handle forwarding to an existing process and unobservable completion. Prototype sandboxed installation launch/file access separately; an unsupported installation reports why.

Accept: fake-process argument assertions for punctuation/Unicode and files; manual release/ESR tests on each supported OS and separate sandbox tests. Reopening/activation does not launch a duplicate compose request. No extension dependency introduced without a scope decision.

## T19 — macOS Mail bridge

Read: product Mail section, architecture §9. Edit: MacMailBackend, Apple-only source/CMake and platform tests.

First prove rich editable body, To/Cc/Bcc, HTML/PDF attachment and correct-window activation using a fixed bridge with typed parameters. Select the proven implementation from G3; do not proceed as if an unproven scripting content property renders HTML. Handle missing account, automation refusal/revocation and file-access errors. Native application identity is `com.apple.mail`, not the system default.

Accept: signed-build manual evidence for all requested formats and composer states; non-Apple stub reports unavailable. If inline HTML cannot meet the requirement, record the gap and request a product decision; do not silently ship PDF-only Mail support.

## T20 — Report/options and email preparation QML

Read: product UX/parity, architecture §11. Edit: `ReportDialog.qml`, `ReportOptionsPane.qml`, `EmailDialog.qml`, `EmailBodyEditor.qml`, `ReportPreview.qml`, related model bindings and QML tests.

Build thin themed views over T12 models. Expose matching per-report options, template selection, subject/prose editing, attachments and preview. Show backend capability conflicts before handoff. Wire close-for-handoff acknowledgement and persistent nonmodal recovery. No report calculations, recipient resolver or network logic in QML.

Accept: real module smoke tests for each report/options default and all email modes; keyboard navigation/validation; no-email path; edited subject/body preserved on failure; popup release before activation. Runtime QML warnings fail tests.

## T21 — Audience picker QML

Read: product audience UX, architecture §6. Edit: `RecipientAudiencePicker.qml`, recipient model bindings/tests.

Build shortcuts, optional source/company refinement, live counts, grouped checklists, exclusions and manager override. Add To/Cc/Bcc moves, manual additions, explicit address-later, reset and save-default scope review. Company IDs stay hidden behind actual labels/counts. Keep internal report mismatch visible and independent of content choices.

Accept: smoke scenarios for managing/client/partners/unknown, missing setup, empty results, customized/reset state, counts and default isolation. UI choices produce the same resolver output tested in T04; no parallel QML recipient algorithm.

## T22 — Template editor and settings QML

Read: product customization UX, architecture §§7, 10–11. Edit: template editor/model, EmailSettingsPane, SettingsPage Office365 binding, tests.

Add Insert field with examples, rich prose + protected content block, per-mode body, preview errors, duplicate/reset/save and explicit default scope. Advanced computed fields are collapsed/optional with import activation. Show imported meeting definitions as preserved for later migration, not working native calendar commands. Backend preferences and shared sign-in are discoverable without moving credentials into QML.

Accept: field-chip round trip, plain template use without JavaScript, failed/disabled computed fields, missing context diagnostics, manual-message versus saved-template edits, reset defaults and profile isolation. Imported formatting changes are reviewed rather than silently accepted; G5 must be passed for enabled import previews.

## T23 — Native action routing and end-to-end desktop wiring

Read: architecture §§2, 11–12. Edit: DesktopAppController, Main/AppMenu/record menus, project/note pages, registrations/CMake and smoke tests.

Wire shared service ownership and verified adapters, Report actions and SendMeetingNotes. An adapter awaiting its platform gate may remain explicitly unavailable during integration; this permits other work to continue but is not full feature acceptance. Centralize save-before-action including pending child edits and persisted-ID validation. Respect DB switch/shutdown lifecycle. Suppress only exact migrated bundled plugin action identities during coexistence; unrelated/custom plugin actions remain. Do not delete Python files yet.

Accept: all four workflows reachable through intended project/note contexts, correct titles/Generate Report terminology and unchanged Export XML. A rejected/staged save causes no snapshot/Graph/launch. End-to-end fake handoffs prove reviewed recipients/content/artifacts flow unchanged to each adapter.

## T24 — Packaging and optional Widgets compatibility

Read: product removal/packaging requirements, architecture §12; actual platform deployment code. Edit: desktop/root/platform packaging files and a thin legacy frontend bridge where needed.

Package templates, native WebEngine libraries/helper/resources, Windows Outlook helper and signed Mac Mail entitlement/usage text. Test Qt/PyQt helper-path coexistence and Linux sandbox access. Update optional Widgets entry points to shared native services before deleting shared plugin workflows; if not feasible, report the specific release-scope decision needed. Prepare an exact obsolete-bundled-file manifest for upgrade cleanup, preserving user-modified/unrelated files and settings.

Accept: signed/package smoke checks on supported platforms and Widgets build/entry-point evidence or explicit maintainer scope decision. No source-tree symlink-only success presented as distributable success. G4 packaging evidence recorded; no broad recursive cleanup.

## T25 — Complete regression and acceptance run

Read: all product acceptance requirements; gate ledger. Edit: tests/fixtures/help as needed; narrowly scoped fixes should be assigned back to their owner task.

Run content/audience/template/OAuth/service/repository tests, real renderer comparisons and actual QML smoke tests. Execute all four workflows × available email modes × backend compatibility, plus retain HTML/display PDF toggles. Exercise focus with clients closed/running/minimized/backgrounded and OS-denied activation. Verify partial/unknown outcomes, no duplicate creation, offline no-email and profile/database isolation. Audit native paths for Python execution and sending endpoints.

Accept: evidence distinguishes automated pass, manual pass, not run and unsupported capability. Every requested backend/format gate must be satisfied or explicitly decided by the user before full completion. No platform test is inferred from Linux-only compilation. G1–G5 passed before G6 removal approval.

## T26 — Remove replaced Python paths and validate upgrade

Read: product Python replacement/removal section; exact dependency inventory from T00/T24. Edit: only obsolete report modules/forms, replaced note handler/definition, unused helpers proven by current references, package cleanup/help/tests.

Remove the three export plugins and dedicated forms after G6. Remove only SendMeetingNotes' shared-base entry/handler; remove noteformatter only if unused. Retain shared dialogExportLocation, remaining dynamic email/meeting definitions, replacement helper consumers, contacts/tasks and unrelated integrations. Apply exact bundled-file upgrade cleanup with appropriate user-file protection. Rerun tests against a package where the removed files are actually absent.

Accept: no stale/duplicate menu or runtime imports, all four native workflows work without replaced modules, Widgets entry points handled, upgrade preserves settings/user plugins. Report exactly what was removed and version-control/recovery status. Do not delete staging or user data as part of plugin cleanup.

## T27 — Final help, review and release handoff

Read: product documentation section and evidence ledger. Edit: user/developer docs and final narrowly scoped fixes.

Update native Report names, email draft review, templates/audiences, scopes/identity, backend limits, focus attempts and cleanup help. Preserve old help links/redirects. Record remaining deferred scheduling scope and Graph invitation caveat. Summarize actual build/test/platform results and any explicit scope decisions. Review diffs for tokens, private fixture data, debug switches and accidental send/calendar calls.

Accept: a maintainer can reproduce the checks and identify supported client versions and remaining limitations. Planning checkboxes alone do not count as implemented or verified work.

## Common check commands

Run from the repository root, with Qt/Python development dependencies and sibling SqliteSyncPro available. Use existing task-specific build directories if already configured correctly; do not overwrite a user's unrelated build cache.

```sh
git status --short
git diff --check
cmake -S . -B build-email -DCMAKE_BUILD_TYPE=Debug
cmake --build build-email --config Debug --target ProjectNotes
cmake -S . -B build-email-tests -DCMAKE_BUILD_TYPE=Debug -DBUILD_QML_DESKTOP=OFF -DBUILD_QML_DESKTOP_TESTS=ON
cmake --build build-email-tests --config Debug
ctest --test-dir build-email-tests -C Debug --output-on-failure
```

For one task, select the relevant newly added CTest names with `-R`, then run broader regressions at integration gates. Do not claim tests that do not exist yet were run. A missing dependency is an environment blocker with its actual configure error; do not alter production Qt/Python requirements or disable coverage to hide it. Do not run cleanup commands against a broad home/workspace path.

## Required task handoff

Each implementing session ends with: task ID/status; changed files; commands and actual results; contract/product deviations (if any); unresolved gate evidence; remaining work; next eligible task. Update Progress only to the status supported by that evidence. Keep the implementation resumable without rereading an entire conversation.
