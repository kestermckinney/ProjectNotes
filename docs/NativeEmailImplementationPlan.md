# Native QML email implementation plan

Branch: `feature/email`. Prepared: 2026-09-12. Status: proposed implementation plan; application code has not been changed.

This plan narrows the earlier [investigation](EmailIntegrationInvestigation.md) to five requested backends: Microsoft Graph using existing Office 365 settings, default mail handler (`mailto:`), Outlook Classic, Thunderbird, and macOS Mail.

Implementation companions: [code architecture](NativeEmailCodeArchitecture.md) fixes proposed interfaces, modules, lifecycle/threading, data contracts and integration points; [build tasks](NativeEmailBuildTasks.md) breaks the work into bounded coding assignments with prerequisites and acceptance checks. This document remains the product-requirements reference. No companion task has been implemented merely by documenting it.

## Product scope and defaults

Build a desktop C++ email/report service with QML dialogs for Windows, macOS, and Linux. Fully replace these four Python workflows with native Send Meeting Notes, Tracker Items Report, Status Report, and Meeting Notes Report. Their content preparation, report generation, options and email handoff must run without Python or QWidget dialogs. The three export plugins and the replaced meeting-note sending code will be removed after native parity is verified; a bridge to those old implementations is not a completed replacement.

Confirmed naming: use **Report**, replacing **Export**, for this feature's native menu group, actions, dialog titles, settings labels and help. The menu is **Report > Tracker Items / Status Report / Meeting Notes**; standalone dialog titles are **Tracker Items Report**, **Status Report**, and **Meeting Notes Report**, with **Generate Report** as the generation action. **Send Meeting Notes** retains its name. Historical plugin paths, migration keys such as `ExportSubFolder`, and existing generated filenames remain compatible. References to old Export names below identify the legacy implementation, not proposed UI labels. Unrelated actions such as Export XML are outside this rename.

Confirmed first-version behavior: use draft/composer handoff so users review and send in Outlook, Thunderbird, or Mail. Microsoft Graph creates an unsent mailbox draft for review in Outlook. Direct Graph sending is deferred beyond the first version. No SMTP, Gmail-specific API, inbox synchronization, or mobile integration is included in this desktop scope.

Design subject/body customization as a shared communications-template foundation now, including compatibility with existing meeting templates. Native meeting scheduling is a proposed subsequent phase, not an additional first-release implementation or permission to send invitations. Its separate review/sending contract must be settled before implementing calendar writes.

Preserve the current report appearance, options and defaults. All three current export dialogs default to **Email as Inline HTML**; retain that default rather than switching to PDF. Preserve **Email as a PDF attachment**, **Email as an HTML attachment**, and **Do not email** as separate choices. Send Meeting Notes retains its formatted HTML body and subject convention. Inline HTML means the report appears as formatted email-body content; an HTML attachment means the complete generated `.html` file is attached. A plain-text conversion, PDF or link is not an equivalent replacement for either HTML mode.

The supported-client capability limits still apply: `mailto:` cannot automatically carry HTML bodies or attachments. Preserve the selected report mode and require a capable backend or an explicitly chosen manual workflow; never remove a report option or silently change its format to accommodate the selected backend. Graph, Outlook Classic and Thunderbird must demonstrate inline HTML and both attachment formats. macOS Mail's rich-body path is an explicit prototype/acceptance item, not permission to silently replace inline HTML with PDF.

## Required workflow and option parity

Use the current plugin code and `.ui` forms as the migration reference. Capture sample inputs, outputs and option states before removal. Preserve each workflow's distinct template; Send Meeting Notes is not the same report as the project-wide Meeting Notes Report.

| Native workflow | Current reference | Required behavior |
| --- | --- | --- |
| Send Meeting Notes | `plugins/base_plugin.py::menu_send_notes`, `plugins/includes/noteformatter.py` | Native note-context action; existing subject with project number/name, meeting date/title; formatted title/date, attendees, rich notes and action-item table. Preserve existing recipient resolution, manager exclusion and explicit attachment behavior for the selected note context. |
| Tracker Items Report | `plugins/exporttrackeritems_plugin.py`, `plugins/forms/dialogExportTrackerOptions.ui` | Project report with Tracker Items / Action Items type filters; New / Assigned / Resolved / Defered / Cancelled status filters; internal-report option; display-on-completion; generate-HTML option; all four email choices. Preserve status group banners/counts, priority colors, columns, comments and sorting. |
| Status Report | `plugins/exportstatusreport_plugin.py`, `plugins/forms/dialogExportStatusReportOptions.ui` | Reporting date; display-on-completion; internal-report option; generate-HTML option; all four email choices. Preserve review-period calculations, stakeholder list, activity sections, issue filtering/sorting, earned-value calculations/appendix and Receives Status recipients. |
| Meeting Notes Report | `plugins/exportnotes_plugin.py`, `plugins/forms/dialogExportNotesOptions.ui` | Project-wide meeting report; reporting date; display-on-completion; internal-report option; generate-HTML option; all four email choices. Preserve meeting order, title/date, attendees, rich notes, action-item tables, footer and branding. |

Tracker defaults are Tracker Items, New and Assigned checked; Action Items, Resolved, Defered and Cancelled unchecked. Keep the existing stored `Defered` status value compatible. Across the three export forms, Display Report when complete, Generate Internal Report and Generate HTML Report start unchecked; Email as Inline HTML starts checked. Meeting/status reporting dates initialize to today; tracker uses the current date without a reporting-date picker. Preserve any existing remembered state and document intentional additions separately from parity.

Preserve the independent meanings of Generate HTML Report and Email as an HTML attachment: the former retains a standalone HTML export in the destination folder, while the latter requires an HTML attachment even when permanent HTML retention is off. PDF generation and export must still work with Do not email and without any email backend or Office 365 account configured. Display Report when complete opens the generated PDF, independently of email mode.

Keep per-report ExportSubFolder settings and migrate their current values from `PluginSettings` into the native report settings once, preserving user overrides. Defaults are `Project Management/Issues List`, `Project Management/Status Reports`, and `Project Management/Meeting Minutes`. Preserve project-folder resolution, optional project-folder copies, output filename conventions (`Tracker Items`, `Status Report`, `Meeting Minutes`, with the ` Internal` suffix), HTML/PDF extensions and email subjects. Surface missing/unwritable destinations without losing generated output or silently changing filenames.

Establish recipient parity by testing the same record/export context through the old and native builders: Send Meeting Notes calls the Full Project Team resolver, Status Report explicitly uses Receives Status, and the other exports use recipients present in their exported context. Include deduplication and managing-manager exclusion. Retain these initial audiences until the user chooses a different audience or explicitly saves a new default. The audience picker below is a deliberate usability improvement; do not substitute all project contacts or only attendees based on a help-page summary.

Record source/help discrepancies as migration issues before establishing expected fixtures. For example, existing help claims automatic Graph sending and that internal meeting exports are never emailed, while the current paths create drafts and expose email modes independently. Meeting Notes Export also computes an internal inclusion flag without using it to gate rendering, and its selected reporting date needs a binding audit. Preserve the intended option semantics and report design rather than freezing accidental filtering/date bugs; document corrections explicitly and test them with internal/external and non-today date cases.

## Backend contract

| Backend | Platforms | Initial implementation | Message capabilities / limit |
| --- | --- | --- | --- |
| Microsoft 365 (Graph) | Windows, macOS, Linux | Qt Network requests using shared Office 365 authentication; create draft and offer Open in Outlook | To/CC/BCC, HTML/plain text, attachments; requires mail consent and an eligible Microsoft mailbox |
| Default email application (`mailto:`) | Windows, macOS, Linux | `QDesktopServices::openUrl` with carefully encoded URL | Short plain-text body and recipients; no automatic attachments or HTML |
| Outlook Classic | Windows | Native C++ COM automation of `Outlook.Application` | To/CC/BCC, HTML/plain text, attachments, existing client signature; display only |
| Thunderbird | Windows, macOS, Linux | `QProcess` invokes Thunderbird's `-compose` interface | To/CC/BCC, subject, body file and attachments; verify supported versions, argument encoding and packaging |
| macOS Mail | macOS | Objective-C++ bridge targeting `com.apple.mail`; prototype rich-body handoff before finalizing the mechanism | To/CC/BCC and HTML/PDF attachments; inline report HTML is an explicit acceptance gate, with its mechanism still to be verified |

`mailto:` is a supported basic-message route, not an attachment fallback that silently discards files. When the prepared message has attachments, require the user to choose another backend or explicitly choose a manual-attachment workflow that saves/reveals the files. Keep the actual handoff summary visible.

Start the macOS prototype with explicit Mail automation because the user requested Mail specifically and the dialog needs To/CC/BCC. Compare an AppKit rich-content handoff if needed, but do not accept a path that routes to another default client or loses fields. If no reliable Mail inline-HTML route is found, report that concrete gap for a scope decision; do not declare full parity or automatically downgrade to PDF.

## User experience

Add an Email section to Settings with preferred backend, availability/status and an optional Thunderbird executable/application location, plus native export-folder settings for the three reports. Keep report email-mode choices/defaults consistent with the existing dialogs rather than overriding them with a global PDF preference. Microsoft 365 configuration and sign-in stay in the existing Office 365 Integration section; link to it from Email settings. Store backend choice, export settings and executable location in profile-specific local settings, outside the synchronized database.

On first use, offer available routes and remember the choice. Do not initiate provider sign-in, change the system default mail client, or silently select another account merely because detection found a backend. A saved unavailable backend remains identifiable with a repair/change action.

Add native Send Meeting Notes to note actions/context menus and Report > Tracker Items, Status Report, and Meeting Notes to project actions/context menus. Each report opens a QML options dialog with the existing controls and defaults listed above; use shared controls where useful without flattening report-specific options. An email preparation view contains recipients with expandable CC/BCC, subject, an optional introduction, generated-content preview and attachment filename/size. Do not insert a mandatory introduction or duplicate report body that changes the current output. Do not open email preparation or request authentication for Do not email. Use the existing theme, form controls, spellchecking and keyboard conventions. Use stable record IDs; save/commit pending page edits before taking the content snapshot.

Reuse the current context-specific recipient rules above, show the selected audience and allow editing before handoff. Apply each report's explicit internal/external rules consistently across preview, inline HTML, HTML file and PDF. Preserve the existing three export calls' `ignoreattachments=True` behavior: attach only the selected generated report, not every project location. Audit Send Meeting Notes separately because its current path permits context-provided attachments; display any such attachments for review.

Use action labels that describe behavior: Create draft in Microsoft 365, Open in Outlook Classic, Open in Thunderbird, Open in Mail, and Open default email application. While work is in progress, prevent duplicate submission. Preserve the dialog on failure; distinguish Draft created from Composer requested. Do not show Email sent after opening a client or creating a draft.

### Audience selection by company

#### What the Python implementation does

`MeetingEmailTypesSettings` stores an `Invitees` choice with each named email/meeting template in `Meeting and Email Types/MeetingEmailTypes`. `populate_dynamic_menu` uses that choice and a data-table context to create menu entries; `CollaborationTools.list_builder` resolves recipients from the available project team, attendees and people rows.

`Internal Project Team` compares each person's company name with the Managing Company preference. `Only Client` compares with the current project's client company; `Exclude Client` excludes that company but can include partner/subcontractor companies. `Receives Status` uses the project-team receive-status flag. Manager exclusion is currently by display name; address deduplication is case-insensitive. Company choices and actual recipient lists are not visible together when preparing a message.

Two labels also hide implementation ambiguity: `Individual` has no unique single-person branch in `list_builder`, and `Attachment Only` changes the dynamic menu's exported tables but does not explicitly clear recipients. Both rely on context or differ from the documented meaning. Migrate the user's intent with visible review rather than copying these accidental behaviors.

#### Proposed QML interaction

Add an **Audience** selector directly above To/CC/BCC in the shared email preparation view, available to Send Meeting Notes and all three Report workflows. Users choose a named audience, immediately see the resulting names/companies/count, and can adjust recipients before opening the draft. Routine use must not require editing a template table or understanding database table names.

| Audience shortcut | Rule and user-facing explanation |
| --- | --- |
| Project team — all companies | People assigned to this project, across all companies; never the entire People database |
| Our company only — {company name} | Project team members whose affiliation matches Managing Company in Preferences |
| Client only — {company name} | Project team members whose affiliation matches this project's client |
| Everyone except the client — {company name} | Project team members from known companies other than this project's client; may include partners/subcontractors, so do not label it Internal |
| Status report recipients | Project team members with Receive Status enabled |
| Meeting attendees | Attendees of the selected meeting, or the explicitly listed meetings for a multi-meeting context |
| Choose people | Search/select specific contacts or enter addresses; no automatic audience expansion |

Keep a **Current selection** entry when needed to represent the existing context-derived recipient set accurately. Initialize it from the legacy workflow's actual context, display its count and source, and offer the clearer shortcuts without silently broadening the audience. Status Report starts with Status report recipients. Once a user explicitly sets a default for a workflow/project, use that rule on subsequent messages and show the resolved people each time.

Keep common choices to one click. An expandable **Refine audience** area exposes two independent controls: **People** (project team, meeting attendees, status report recipients, current selection, or chosen people) and **Company** (all companies, our company, project client, everyone except client, or selected companies). Selected companies uses checkboxes with actual names and live recipient counts, including an Other/Unknown company section for review. This allows combinations such as status report recipients from our company, or meeting attendees from two partner companies, without creating separate email templates.

Within a selected source, multiple chosen companies are a union; source membership, company criteria and address eligibility are intersected. Render a readable summary, for example: **6 recipients · Status report recipients · Acme Consulting and Northwind**. Company filters never search outside the selected source or silently change the source to fill an empty group. Offer an explicit Include people with no company option where appropriate; they are not automatically included by restrictive company rules, including Everyone except the client.

Under the summary, show a searchable checklist grouped by company. Each row shows name, email, company and selection reason such as Meeting attendee or Receives status. Display **To: 4 · CC: 2 · BCC: 0**, and a collapsible **Not included** list with reasons such as missing email, company not selected, duplicate address, or manager excluded. Counts mean unique eligible addresses after exclusions. Let users uncheck a person, move them between To/CC/BCC, or add an address manually. Additions outside the selected audience remain visibly marked as manual additions; they do not change project-team/company records.

Show **Exclude project manager — {name}**, enabled by default to preserve current behavior, with an explicit override. Resolve the configured manager by person ID rather than display name. Do not label this Exclude me: the configured manager may be different from the authenticated sender. If the manager is unset or unavailable, explain that no manager exclusion is applied. Company filtering uses stored affiliations, never email-domain guessing; an external-looking email domain does not determine company membership.

Display a clear setup hint/link if Managing Company or the project's client is missing. Disable only dependent shortcuts rather than treating an unresolved company as All companies. A company-filtered empty result remains empty, with an explanation and options to change the filter or choose people. Users may explicitly choose **Address later in email application** to create an unaddressed draft where supported; a failed group lookup must not silently select that mode.

Audience changes recompute the preview before handoff. Keep explicit additions/exclusions visible as a **Customized** state, with Reset to audience to clear them. When a source/company rule changes, remove old automatic recipients, apply saved exclusions and retain only clearly marked manual additions; show a compact added/removed count. Recheck source data before creating the draft; if membership/addresses changed since preview, update the visible selection for review rather than quietly handing off a different audience. Freeze the final To/CC/BCC lists into the immutable email request.

Changing company/audience does not silently change Generate Internal Report or modify the report contents. If an internal report has recipients outside the managing company or with unknown affiliation, show an inline notice listing the mismatch and offer Change audience or turn off Generate Internal Report. Keep report content and audience controls separate and visible; users can deliberately retain their selection. No mailing lists are expanded by contacting a provider, and all backends receive the same resolved addresses.

#### Defaults, reusable audiences and migration

Provide **Save audience…** and **Use as default for this report** as explicit actions. Store a small named rule (source, company roles/IDs, manager exclusion and optional individual overrides), not an opaque HTML template or a stale address snapshot. Allow presets such as Client update or Internal status, usable across report types. Relative roles Our company and Project client resolve against the current context, while specific-company/person selections are scoped to the current database. Store presets/defaults locally per developer profile and database identity, with optional per-project/workflow defaults; do not let the last audience used for one project silently become the default for another. Deleted or unresolved IDs require visible review.

Map legacy Invitees choices where they are relevant to a native workflow or where the user imports an existing audience:

| Legacy choice | Native representation |
| --- | --- |
| Full Project Team | Project-team source, all companies; retain Current selection during migration when the old context included a different pool |
| Internal Project Team | Project-team source, Our company filter |
| Only Client | Project-team source, Project client filter |
| Exclude Client | Project-team source, Everyone except the client filter; call out previously included unknown-company contacts for review |
| Receives Status | Status-report-recipient source, all companies |
| Individual | Explicitly selected context person, with review if the context cannot identify one person |
| Attachment Only | Address later in email application plus separately reviewed attachment selection; flag the old recipient ambiguity during import |

Preserve existing template names, subject/body templates, data contexts and meeting definitions. Migrate audience semantics for native email/report preparation and design their reuse in the subsequent scheduling phase below; do not delete the remaining Meeting and Email Types feature during email replacement. Import presets idempotently, retain legacy settings for remaining plugin consumers, and never silently infer a native default from one of several unrelated legacy templates.

#### Native model and acceptance

Implement `RecipientAudienceResolver` and a QML-facing `RecipientSelectionModel` shared by all four workflows. Resolve company membership using `people.client_id`, project client ID and `DesktopAppController.managingCompanyId`; traverse project membership via `project_people.people_id` and attendees via `meeting_attendees.person_id`. Company names are display labels only. Reuse `projectManagerId` for manager exclusion. Load candidates from stable record IDs with deleted rows excluded, not from whatever rows happen to be visible in a filtered QML table.

Apply status membership, company filtering and nonblank/valid-address checks as separate predicates, avoiding the legacy receive-status branch bypassing address validation. Deduplicate by trimmed, case-insensitive address as the existing feature does, without provider-specific dot/plus rewriting. Keep one address in only one of To/CC/BCC; moving it is explicit, and automatic group merging must not promote a BCC address into To or CC. Multiple people sharing an address retain visible provenance so users can understand why the count is smaller.

Test the preset-to-recipient matrix with managing company, client, two partner companies and unknown affiliation; missing role companies; renamed companies; project membership versus attendees; Receive Status on/off; missing/invalid email; manager ID/name changes; duplicate addresses; manual additions/exclusions; To/CC/BCC collisions; deleted records; changed membership before handoff; and profile/database/project default isolation. Assert identical resolved recipients for every backend, meaningful empty results, no accidental source broadening and no changes to report format/options. Add QML smoke coverage for choosing a company group, inspecting exclusions, customizing recipients, resetting, and saving an explicit default.

### Customizable subjects and bodies

#### Existing behavior and recommendation

`plugins/includes/common.py::replace_variables` recursively traverses exported XML and replaces literal `[$table.column.row]` markers, with one-based row numbers, plus root attributes such as `[$managing_company_name]`. It prefers a column's `lookupvalue` over its stored value. This is string replacement, not expression evaluation; missing markers can remain in the output, and replacement has no HTML-context escaping. `MeetingEmailTypesSettings` in `plugins/base_plugin_settings.py` stores Type, Name, Invitees, Subject, Template and Data Type; its separate editor saves a rich-text HTML body. Dynamic Email and Meeting actions in `plugins/base_plugin.py` use these same definitions through `CollaborationTools`.

Recommendation: retain easy field insertion as the normal interface, with **optional QJSEngine-backed computed fields** for advanced customization. Do not require users to write JavaScript to change a subject, greeting or agenda. Do not evaluate an entire rich-text document as JavaScript or turn this into a general replacement plugin runtime. QtQml is already an application dependency, but the observed `QJSEngine*` singleton-factory arguments are not an existing standalone template evaluator. Use a new dedicated engine, never the live QML engine. [Qt QJSEngine](https://doc.qt.io/qt-6/qjsengine.html)

#### Friendly template editor

Add **Communication Templates** under Email settings, accessible through **Customize template…** in preparation. Provide bundled defaults for Send Meeting Notes and each Report workflow, matching current subjects and email bodies exactly until the user customizes them. Let users duplicate, rename, preview and reset a template. Keep bundled defaults read-only and save user variations separately so upgrades do not overwrite customization. Meeting definitions can be imported/preserved for the future scheduler, clearly marked as not yet used by native scheduling; the existing plugin continues using its own settings.

The editor contains a subject field, rich-text body, **Insert field**, a preview using the selected project/meeting, and an optional **Advanced computed fields** panel. The field picker shows friendly groups and sample values, such as Project > Name and Meeting > Date, rather than exposing database tables and numbered XML rows. Insert visible field chips backed by versioned tokens, for example this proposed syntax:

```text
Subject: {{ project.number }} — {{ project.name }} — Meeting notes

Hello,

Please find the notes for {{ meeting.title }} below.

[Meeting notes content]

Thank you,
{{ preferences.managerName }}
```

The content block is a typed generated fragment, not a text substitution containing arbitrary HTML. The example is an optional customized template, not a change to the legacy default. Finalize canonical field mappings against actual native models; `preferences.managerName` means the configured manager, not necessarily the signed-in sender. Show only fields applicable to the selected workflow and offer documented date/number formatting and an explicit fallback for optional values. No extra Office 365 permissions should be needed just to preview local templates.

For inline mode, a protected **Report content** or **Meeting notes content** block occurs exactly once, with editable text before/after it. Preserve the generated tables, CSS and report contents independently of the editor. For attachment modes, offer a separate **Attachment message** body, initially matching the legacy behavior, and show the generated attachment in the preview. Switching modes selects the appropriate body without discarding either customization. The standalone HTML/PDF report is unchanged by email-envelope edits; report layout customization is outside this addition. Do not insert report HTML into both the body and attachment unless the user explicitly chooses such behavior in a future option.

Allow one-message subject/body edits without saving a template. **Save as template** and **Use as default** are explicit, with profile/database/project/workflow scoping consistent with audience defaults. Saving content never silently saves or changes an audience: a template may reference a named audience default, but the audience picker remains independently editable. Clearly distinguish a template edit from an already-resolved message edit; regenerating after source/options change must offer to preserve manual edits or reload, never silently overwrite them.

Resolve and validate before any provider call. Highlight missing/unsupported fields with a useful explanation; permit saving an incomplete template but block handoff until required fields are fixed or an explicit fallback is supplied. Distinguish an intentionally blank field from a missing field. Escape ordinary values for HTML text, validate allowed link targets separately, and reject subject/header newlines. Only native approved rich-content blocks bypass text escaping. Disable executable content, external resource loading and local-file access in previews; define a formatting-preserving HTML policy for imported rich text and show any material migration differences. Unresolved tokens and JavaScript `undefined` must never silently appear in a prepared message.

#### Shared context and advanced JavaScript

Introduce a versioned `TemplateContext` with named project, client, preferences, meeting and report data, and explicit collections where needed. Include typed dates, locale and time-zone information alongside display values. Build it from the same saved-record snapshot and internal/external filtering used for the preview/report; do not expose the entire database or excluded internal notes just because a script asks for them. Use separate namespaces for selected source meetings and a future proposed calendar appointment so their dates and titles cannot be confused. The normal token syntax supports named fields and formatting, not arbitrary inline JavaScript.

Advanced users define a computed field, for example `custom.introduction`, through a function such as:

```javascript
function compute(context, helpers) {
    return context.report.internal
        ? "For internal review only."
        : "Please review the attached status report.";
}
```

Insert `{{ custom.introduction }}` through the same field picker. Computed fields return bounded text/scalars, not raw HTML, recipients, file paths or provider operations. They may read the supplied snapshot and pure formatting helpers but cannot modify the audience, report filters, application settings or source records. Initially compute each field independently from context/helpers, without inter-field dependencies or execution-order surprises. Use ordinary editor fields for fully customized subjects/bodies, assembling more complex text through computed fields only when needed.

Pass context as structured values, never interpolate project data into JavaScript source. Use an isolated, short-lived QJSEngine on its owning worker thread and a deeply read-only data copy. Expose no application/controller/database QObjects, credentials, network/file/process APIs or application module loader. Do not install unnecessary extensions. Enforce input/result size limits, execution deadlines and cancellation; the watchdog must call `setInterrupted` from another thread, since a timer on the executing thread cannot interrupt its busy script. Protect engine lifetime during watchdog callbacks, discard the engine after evaluation, and report syntax/runtime errors with field name and line. [QJSEngine interruption](https://doc.qt.io/qt-6/qjsengine.html#setInterrupted)

Treat in-process JavaScript as **trusted-user customization**, not a security sandbox: deadlines and limited API exposure do not impose a hard memory limit. Imported scripts remain inactive until explicitly reviewed/enabled; previews must not execute them automatically. A future requirement to execute arbitrary untrusted scripts would require a separately isolated process and resource controls. Test supported syntax against the application's shipped Qt versions, not whichever Qt documentation is newest. Freeze the rendered result used for final review and handoff; scripts must not rerun independently in each backend.

#### Legacy migration and template acceptance

Import the existing `Meeting and Email Types/MeetingEmailTypes` definitions without changing their Type, Name, Invitees, Subject, Template or Data Type in the original settings. Keep a native legacy-token compatibility mode implemented without Python, preserving lookup display values and one-based traversal behavior. Convert to friendly fields only where mapping is unambiguous; retain and visibly flag unsupported contexts, nested/repeated table ambiguities and missing values for review. Never reinterpret an old template or its replacement values as JavaScript. Keep originals available for rollback, make import idempotent, and do not overwrite a user-edited native template on re-import.

Separate importing definitions from replacing their dynamic menu actions. The four email/report workflows ship first; generic legacy Send Email and Schedule Meeting actions remain intact until their contexts/actions have an explicit native migration. Existing meeting templates must not vanish when report plugins are removed. The shared `replace_variables` helper also has unrelated consumers, so it cannot be deleted merely because native email no longer calls it.

Add tests for lookup versus stored values, one-based/nested legacy rows, missing fields/fallbacks, Unicode/HTML escaping, rich-text migration, multiline subjects, unsupported contexts, date formatting, per-mode bodies, protected block placement and unchanged standalone report layouts. Cover JavaScript errors, loops/deadline cancellation, oversized results, data-as-code injection, inactive imported scripts, profile isolation, preview regeneration/manual edits and identical rendered content across adapters. QML tests exercise Insert field, preview diagnostics, reset, duplicate/save/default and the optional advanced panel.

### Show the prepared email and request focus

Confirmed requirement for every backend: after preparing the message and completing its required attachments, attempt to show that specific email in the selected application and bring its compose/draft window to the foreground with keyboard focus. This is the normal handoff behavior, not an optional preference or a requirement for the user to find the message manually. For Microsoft Graph, automatically open the created draft in Outlook on the web and request foreground activation of its browser window/tab; a background draft alone does not complete the normal workflow.

| Backend | Required presentation/focus attempt |
| --- | --- |
| Microsoft Graph | Open the returned draft link after attachment completion and request browser activation. Target the existing draft, not a blank compose page or a newly created duplicate. |
| Default handler (`mailto:`) | Use the system's foreground compose/URL-launch mechanism and activation context where available, allowing the configured handler to show its compose window. |
| Outlook Classic | Display and activate the Inspector belonging to the created MailItem, restoring its window if minimized where supported. Do not just activate an unrelated Outlook inbox. |
| Thunderbird | Request a compose window for the prepared message, then attempt application/compose-window activation using supported platform mechanisms. Account for a launch being forwarded to an already-running Thunderbird process. |
| macOS Mail | Show the newly created outgoing message and activate Mail, attempting to make that message window frontmost. |

Perform activation in response to the user's email action, carrying the platform activation context through asynchronous generation/upload where supported. Close or release the ProjectNotes modal preparation dialog before foreground handoff, and do not subsequently raise ProjectNotes or show a completion dialog that steals focus back. When Display Report when complete is also selected, open the PDF before the final email-window activation so the email remains the intended foreground destination. Do not email opens only the requested report and performs no mail-window activation.

Treat focus as an attempt with a separate result from successful message preparation: the OS, browser or receiving application may prevent foreground activation or provide no confirmation. Track confirmed, requested/unconfirmed and failed presentation as supported; never label an unobservable focus change as confirmed. If opening or activation fails, retain the prepared content/draft ID and provide a nonmodal status with an Open draft or Bring email forward action where supported. A focus retry must target the existing draft/window and must not recreate the message, repeat attachment uploads or send anything. For backends without a reusable message/window handle, explain that limitation and offer manual application activation rather than automatically launching another compose request.

## Architecture and file boundaries

| Proposed component | Responsibility |
| --- | --- |
| `ProjectNotesIntegrations/Office365Service` | Shared settings, consent, sign-in, refresh, credential access and account/session lifecycle |
| `ProjectNotesIntegrations/TemplateContext`, `CommunicationTemplateService` and `CommunicationTemplateStore` | Versioned shared email/meeting fields, literal-token compatibility, safe composition, optional isolated QJSEngine computed fields and local template migration/defaults |
| `ProjectNotesEmail/EmailTypes` | Typed recipients, request, attachment metadata, capabilities and result states |
| `ProjectNotesEmail/EmailService` | Backend discovery, validation, request lifecycle, attachment staging and dispatch |
| `ProjectNotesEmail/EmailContentBuilder` | Native note/report snapshots, approved rich-content blocks and plain-text/HTML composition using the shared template service |
| `ProjectNotesEmail/RecipientAudienceResolver` and `RecipientSelectionModel` | Company/source audience rules, defaults/presets, editable To/CC/BCC selections and inclusion/exclusion explanations |
| `ProjectNotesEmail/ReportService` and report-specific builders | Four native workflows, typed per-report options, settings migration, export filenames/destinations and generation lifecycle |
| `ProjectNotesEmail/templates/*` | Ported HTML/CSS templates preserving each existing report and the separate Send Meeting Notes body |
| `ProjectNotesEmail/ReportRenderer` | Native Qt WebEngine HTML/PDF rendering with each report's existing print layout and asynchronous completion/errors |
| `ProjectNotesEmail/backends/*` | Five independent adapters; OS-specific code compiled only on its platform |
| `ProjectNotesDesktop/qml/EmailDialog.qml` | Preparation/review workflow using the native service |
| `ProjectNotesDesktop/qml/RecipientAudiencePicker.qml` | Audience shortcuts, optional company/source refinement, counts and per-person review |
| `ProjectNotesDesktop/qml/CommunicationTemplateEditor.qml` | Friendly field insertion, subject/body customization, per-mode preview, explicit defaults and optional advanced computed fields |
| `ProjectNotesDesktop/qml/ReportDialog.qml` and report-specific option components | Matching options for Tracker Items Report, Status Report and Meeting Notes Report, with shared email/report controls |
| `ProjectNotesDesktop/DesktopAppController.*` | Exposes `email` and `office365` objects plus record-specific preparation methods |
| `ProjectNotesDesktop/qml/pages/SettingsPage.qml` | Shared Office 365 binding and new Email settings |
| `tests/desktop/` | Content/service failure tests and real QML smoke coverage with mocked handoffs |

These names are proposed, not existing classes. Link the new modules into the desktop app and desktop tests. Keep transport/UI code out of `ProjectNotesCore` and the shared database classes. The separately built iOS app does not acquire a new dependency through this work.

The detailed architecture refines this component outline: a desktop `CommunicationsController` owns preparation through an `EmailService`, a token-free `Office365SettingsModel` exposes shared account settings, and `ProjectNotesEmailRendering` supplies the concrete WebEngine renderer separately from content/service code. Use the companion's file/interface contracts when implementing rather than creating duplicate owners for these responsibilities.

The service accepts an immutable request containing operation ID, To/CC/BCC, subject, plain text, optional HTML, attachment metadata and source context. Attachments refer to completed readable files with explicit ownership/lifetime. Provider/client adapters never query project models or decide the audience.

Use explicit results: validation failed, unavailable, authentication required, permission denied, composer requested, draft created, cancelled, partial draft, failed, and outcome unknown. Record provider draft IDs as soon as known so failed attachment uploads can be retried against the same draft. A lost response after draft creation may have created a draft; do not blindly repeat the POST.

## Office 365 extraction and migration

The current settings page binds to `DesktopAppController.fileFinder`. `FileFinderService` owns `MicrosoftOAuthManager`, stores tenant/client IDs under `FileFinder/tenantId` and `FileFinder/clientId` in `AppSettings`, and uses credential service `Office365FileFinder`. `MicrosoftOAuthManager::requestedScopes()` currently hardcodes Teams/channel/files scopes. Email must work even when File Finder and its Office 365 scanning option are disabled.

1. Move reusable OAuth machinery into the integrations module and create one desktop-owned `Office365Service`. File Finder becomes a consumer. Preserve device-code sign-in initially rather than changing sign-in methods during extraction.
2. Make settings ownership explicit. Preserve the existing keys and credential service/account lookup for the first extraction; use one canonical writer and document the compatibility names. A rename is unnecessary to deliver email and risks losing saved sessions. Avoid copying or importing the legacy Python plaintext token cache.
3. Preserve developer-profile isolation, including the credential namespace. The current OAuth account key is only tenant/client, so audit profile scoping before reusing it and test that signing out of a test profile cannot erase the production profile's sign-in.
4. Request feature-appropriate Graph scopes: email drafts require `Mail.ReadWrite`; File Finder retains its existing scopes. An email-only user should not need Teams/files permissions. Adding a feature may require renewed consent. Track granted capabilities separately from generic Signed in status, and handle consent denial without disabling unrelated authorized features.
5. Serialize authentication/refresh operations and notify both consumers of sign-out, account/tenant/client changes and expired consent. Cancel/invalidate outstanding work associated with the old identity; retain already-created draft information rather than claiming it vanished. Clear File Finder's identity-dependent cache as it does today.
6. Keep refresh tokens in `CredentialStore`, access tokens in C++, and credentials out of QML/logs. Use a session generation to reject late callbacks. Keep the signed-in mailbox explicit; initially use the user's own mailbox (`/me`), with delegated/shared-mailbox sending outside this scope.

Code-level identity choice: email setup additionally requests delegated `User.Read` to retrieve only the signed-in user's ID/display name/mail/userPrincipalName for visible account selection and draft-recovery binding. This does not enable sending or directory-wide access, and does not add a new requirement to existing FileFinder-only session restoration. Keep identity permission/lookup failure separate from mail permission and general sign-in status. [Graph signed-in user lookup](https://learn.microsoft.com/en-us/graph/api/user-get?view=graph-rest-1.0)

## Adapter implementation details

### Microsoft Graph

Create the draft with `/me/messages`, retain its ID, then upload attachments. Use the small-attachment endpoint below 3 MB and an upload session for 3–150 MB files. Validate aggregate message size as well: those upload bounds do not override the mailbox's lower message limit. Handle authorization failures, throttling with `Retry-After`, timeouts, partial uploads and user cancellation without blocking the QML event loop.

Automatically open the service-returned `webLink` and attempt to focus its browser window/tab after all required attachments succeed, following the shared presentation requirement. It is an Outlook-on-the-web link, not a guaranteed desktop-client or direct-edit deep link; test draft editing and provide an Open draft action and Drafts-folder instruction if needed. A browser-opening/focus failure must preserve the successfully created draft and retry presentation only. Show partial-draft recovery explicitly and offer to discard only the exact draft created by this operation. Do not automatically delete drafts once handed off for user editing.

The first version has no direct Send action, does not invoke Graph send endpoints, and does not request `Mail.Send` for this feature. Users complete review and sending in Outlook. Direct sending would require a separate future scope decision.

### Default handler / mailto

Build and test RFC 6068 encoding in one C++ function, including literal plus signs, percent signs, ampersands, newlines and Unicode. Reject header injection. Generate a plain-text preview and apply a conservative tested URL-size limit. For oversized messages offer Copy message plus a minimal addressed compose URL, or another backend; never silently truncate. Successful `openUrl` means a launch request only.

### Outlook Classic

Use late-bound COM with explicit recipient types, subject, HTMLBody/Body, attachment calls and Display/Inspector activation. Preserve the existing signature behavior without concatenating invalid complete HTML documents. Validate attachment files before creating the item, report unresolved recipients and handle HRESULT failures. Do not infer Classic availability from an installed new Outlook or the default mail handler.

Use a small desktop-shipped helper process owning a single-threaded COM apartment, with structured request/result IPC, so an unresponsive Outlook call cannot freeze QML. Keep message contents off command-line arguments. A helper timeout is an uncertain handoff, not permission to create another message automatically. Do not terminate the user's Outlook process.

### Thunderbird

Discover a conventional installation, with an explicit path override. Invoke it with an executable and argument list, without a shell. Use a staged UTF-8 body file through the compose `message` parameter, and file URLs for attachments; apply separate encoding for Thunderbird's comma/quote argument grammar. Source inspection confirms body-file, HTML and recipient/attachment handling, but supported release/ESR versions must be tested rather than assuming development-source behavior.

Test both existing and new Thunderbird processes on all three desktop OSes. Linux Flatpak/Snap installations need a specific launch/file-access strategy; detect them and validate forwarded access to body and attachment files. Also test ProjectNotes' Flatpak packaging. Do not advertise sandboxed attachment support until proven; display a useful availability reason and permit another supported route. No Thunderbird extension is required for the initial conventional-installation backend.

### macOS Mail

Use a fixed bundled script or Apple Events handler targeting Mail's bundle ID; supply recipients, content and attachment paths as typed parameters, never interpolated script source. Create/show an outgoing message and let the user choose the sender/signature and send. Verify standalone HTML/PDF attachments and rich report-body composition separately. Do not assume a plain-text Mail scripting property can render HTML. Resolve the rich-body mechanism in the compatibility prototype and preserve the selected mode on failure; HTML attachment or PDF must be a user choice, not an automatic substitute.

Include `NSAppleEventsUsageDescription` and the hardened-runtime Apple Events entitlement in the actual desktop bundle/signing pipeline. Existing `packaging/macos/ProjectNotes.entitlements` does not contain that entitlement. Handle first-use automation consent, denial, unavailable accounts and revoked permission. Validate using a signed build, not only an unsigned debug executable. No UI scripting or Accessibility permission should be required.

## Native content and report appearance

Port all four workflows into native builders with representative fixtures before replacing menu actions. The three exports are full native replacements, including filtering, calculations, template generation, PDF/HTML export and email preparation. A finished-report bridge may remain for unrelated generators such as SSRS, but cannot be used to postpone Tracker Items, Status Report or Meeting Notes Export migration.

Use native Qt WebEngine rendering and port the existing HTML/CSS templates to retain the same browser rendering family as the Python exports. Match print settings: Tracker Items uses Letter landscape and `QMarginsF(12, 12, 12, 12)`; Status Report uses Letter portrait and `QMarginsF(20, 20, 20, 20)`; Meeting Notes Export uses A4 portrait and `QMarginsF(20, 20, 20, 20)`. Preserve the original margin units, viewport widths, fonts/fallbacks, column widths, colors, borders, row grouping, headers/footers, branding and page-break behavior. Wire Qt WebEngine Quick initialization, resources and deployment into both the shipping and test builds. Do not switch these reports to QTextDocument's limited HTML rendering merely to reduce dependencies.

Concrete renderer choice: use GUI-thread `QWebEnginePage` from WebEngineCore for PDF generation with explicit `QPageLayout`, and WebEngineQuick for the QML preview. No QWidget renderer/dialog is required. Keep real rendering in a separate target from pure content/service tests and verify that packaged QtWebEngineProcess matches native Qt rather than accidentally selecting an incompatible PyQt helper. [QWebEnginePage PDF API](https://doc.qt.io/qt-6/qwebenginepage.html#printToPdf)

Generate the standalone HTML, PDF and inline email representation from the same report snapshot/template. Preserve existing styles when embedding into an email body; any CSS inlining or document-wrapper normalization must pass comparison checks. Keep generated report HTML independent of the QML introduction editor so editing introductory text does not rewrite report tables. Match the old output on the same OS/fonts/rendering version; account for rendering differences between email clients rather than promising identical pixels across unrelated clients.

Before removing Python, capture representative legacy HTML, rendered pages/PDFs and email bodies for all four workflows. Compare native output visually and structurally: section/column order, text, calculations, formatting, page sizes, pagination and filenames. Cover multi-page data, long rich notes, missing fields, special characters, all tracker statuses/types, priority/due-date sorting (including missing dates), internal/external views and all report email options. Pixel differences from rasterization/font substitution may be tolerated only when documented and visually immaterial; content, missing options and changed report layouts are release failures.

## Python replacement and removal

After parity checks pass, remove `plugins/exporttrackeritems_plugin.py`, `plugins/exportstatusreport_plugin.py`, `plugins/exportnotes_plugin.py` and their report-specific `.ui` forms. Replace and remove the Send Meeting Notes menu entry/handler in `plugins/base_plugin.py`; remove `plugins/includes/noteformatter.py` only after confirming no remaining imports. Keep unrelated functionality in `base_plugin.py`, `collaboration_tools.py` and Graph/Outlook helpers intact where still used.

Do not delete `plugins/forms/dialogExportLocation.ui`: unrelated SSRS and document-generation plugins also use it. Audit all shared helpers, menus, settings references and package lists before removing any other file. Update the native menus/help so the old plugin entries do not coexist with duplicate native actions.

Update packaging and upgrade cleanup, including explicit Python/form entries in `packaging/windows/setupscript.nsi`, copied macOS plugin bundles and Linux packaging. Existing installations must not rediscover stale copies of the removed bundled plugins after upgrading; remove only the exact obsolete bundled files, preserving unrelated/user plugins and migrated settings. Account for the optional legacy Widgets target explicitly: route retained legacy entry points to the shared native services with a thin frontend adapter where needed, rather than leaving calls to deleted plugins or retaining the replaced report implementations.

Completion requires all four workflows to work with those Python modules absent, native QML options only, and no Python execution in their content/render/email path. Removing Python from the entire application or migrating unrelated plugins is outside this scope.

## Subsequent phase: native meeting scheduling

### Current scheduling behavior and migration risks

The existing Schedule Meeting entries are generated from Type=Meeting rows in the same MeetingEmailTypes configuration, not independent implementations for each meeting type. Preserve the configured kickoff, status, lessons-learned and proposal-working variants, their subjects/agendas and data contexts. Some defaults labeled Internal use Exclude Client, which can include partner companies; migrate the actual audience rule with visible review rather than converting the label into Our company only.

`CollaborationTools.schedule_a_meeting` resolves the same audience and subject/body replacements as email, then chooses Graph or Windows Outlook Classic using the legacy Outlook Integration settings. It gathers attachment paths but does not pass them to either scheduling adapter. The Graph path chooses the next local whole hour and a one-hour duration, then uses the mailbox time zone for the request; this needs explicit local/mailbox conversion, midnight rollover and DST validation. The Outlook path creates an appointment, adds recipients, copies formatted body content through a temporary mail item, sets a 60-minute duration and displays/activates the Inspector. Do not retain the unused-attachment behavior as intentional parity or assume the two paths currently choose identical start times.

Critical finding: `GraphAPITools.draft_a_meeting` posts `/me/events` with attendees and `isDraft: true`, but Microsoft's create-event contract says attendees receive invitations when the event is created. The event resource describes `isDraft` in terms of unsent Outlook meeting updates; it is not a documented switch that makes creation with attendees invitation-free. The legacy comment claiming this prevents sending is therefore unsafe to rely on. No live meeting was created during this investigation. [Graph create-event behavior](https://learn.microsoft.com/en-us/graph/api/user-post-events?view=graph-rest-1.0), [event isDraft](https://learn.microsoft.com/en-us/graph/api/resources/event?view=graph-rest-1.0)

### Proposed native workflow and boundaries

Reuse the template editor/store, context builder, company-audience rules, Office 365 account service, attachment staging and presentation/error conventions. Add a separate `MeetingService` and typed `MeetingRequest`/calendar capability model; do not encode a meeting as an ordinary `EmailRequest` or make email adapters responsible for scheduling.

Provide a native **Schedule Meeting** preparation view with a meeting-template chooser, editable subject/agenda, company audience, required/optional attendees, date, start/end or duration, explicit time zone, location and reviewed attachments. Keep a 60-minute initial duration; choose the next valid whole-hour slot with proper day rollover and show it for review. Do not infer the invitation date from an old meeting note unless the user explicitly selects that action. An organizer comes from the selected account, not the Exclude project manager preference. Required/optional/resource roles are calendar concepts, not To/CC/BCC aliases; a resource/room must be explicitly chosen. Make time-zone differences visible before handoff.

Keep meeting content customization separate from scheduling fields. A template supplies subject/agenda and may reference an audience preset; it cannot execute scheduling actions or silently change attendees, time, recurrence or provider. Preserve legacy contexts before replacing their dynamic menu entries. Recurrence, free/busy search, automatic room booking and Teams/online-meeting creation are separate follow-on scope choices, not assumptions inherited from Office 365 sign-in.

First prototype Outlook Classic's editable, unsent meeting Inspector and Graph's available review mechanisms. Every supported route must attempt to display the specific prepared meeting and focus its application/browser window, with the same distinction between preparation success and presentation success as email. Calendar support must be tested independently: Thunderbird mail compose does not establish calendar-compose support; macOS Mail is not macOS Calendar; `mailto:` is not a scheduling API. An explicit `.ics` file workflow may be investigated as a manual fallback, but is not equivalent to an editable organizer draft, actual scheduling or invitation-response tracking.

For Graph, keep preparation local with **no calendar POST/PATCH** until a no-invitation Outlook compose handoff has been verified or the user has approved a different scheduling contract. If Graph cannot provide the requested unsent review window, present that limitation for a decision: use a proven calendar-client handoff, or introduce a separately approved **Schedule and send invitations** action after complete local review. Neither event creation with attendees nor creating an empty appointment and later adding attendees is an acceptable hidden draft workaround. This does not change first-version email's draft-only requirement.

When calendar integration is approved, request `Calendars.ReadWrite` only for that feature; evaluate whether mailbox time-zone lookup needs additional consent, rather than importing the Python helper's broad scope list. Reuse the shared account but keep mail/calendar capability checks separate. Retain exact event IDs/results for recovery, use documented duplicate-prevention facilities where available, and handle unknown create outcomes without blind retry. Do not delete or update a user-edited event as cleanup.

### Scheduling exit gates

Before removing any scheduling plugin path, establish fixtures for every configured meeting type and context, subject/agenda formatting, audience migration, required/optional attendees, attachments, date/time-zone conversion, midnight and DST boundaries, and meeting-window focus attempts. Verify invitation behavior in an explicitly authorized controlled calendar environment, including failures and ambiguous responses; automated tests use mocks and send no invitations. Record whether each adapter offers an unsent composer or an explicitly authorized scheduling action. Replace/remove only the migrated scheduling entries and now-unused helpers, retaining remaining contact/task/email integrations. The Graph sending-contract decision is a prerequisite for this phase, not a blocker to the native email/report implementation.

## Attachment lifetime

Stage generated files in operation-specific directories with restricted permissions. Use a manifest and distinguish app-generated files from user originals. Keep files after external handoff because most adapters cannot tell when the mail client has copied them. Do not delete on dialog close or at app exit. Release files only when copying is confirmed, or offer explicit cleanup of older staged operations with a warning that open drafts may still reference them; do not promise safe automatic age-based deletion. Graph uploads can release staging once complete, unless retained for a pending retry. Never remove user source files.

## Delivery sequence and acceptance gates

| Milestone | Deliverable | Exit condition |
| --- | --- | --- |
| 1. Baselines and compatibility prototypes | Capture all four legacy outputs/options and subject/body definitions; exercise To/CC/BCC, Unicode, inline report HTML and HTML/PDF attachments through each backend | Record versions/capabilities and legacy token mappings; resolve Mail rich-body feasibility and renderer parity; no message needs to be sent |
| 2. Shared Office 365 service | Extract auth/settings ownership; move File Finder to consumer | Existing settings/session preserved; File Finder works; email-only consent and profile isolation tested |
| 3. Native email foundation | Types, service, audience resolver/selection model, shared communication templates/editor, legacy-token migration, optional computed fields, presets/settings, staging, Graph and mailto adapters | Company/source rules and defaults tested; template preview/validation and script safeguards pass; draft creation and failures recoverable; no silent recipient, attachment or body loss |
| 4. Native notes workflows | Send Meeting Notes and Meeting Notes Report, matching templates/options plus the shared QML audience picker | Users can choose/refine companies and review actual recipients; both workflows match baseline report appearance and formats without Python or QWidget dialogs |
| 5. Local-client adapters | Outlook helper, Thunderbird, Mail integration and packaging | Supported native clients open complete editable messages, attempt to focus the correct compose window and report failures accurately |
| 6. Native tracker and status reports | Port all report-specific options, defaults, filters, calculations, templates and report-folder settings | Both reports match appearance and behavior for every email/report mode; no dependency on the replaced plugins |
| 7. Plugin removal and release validation | Remove replaced Python paths/forms, migrate settings, update package/upgrade cleanup, tests and help | All four workflows operate with removed modules absent; all five backend contracts validated; no stale plugin menus or File Finder/QML regressions |
| Proposed next phase (separate approval) | Native meeting scheduling using shared templates/audience/account services; migrate legacy meeting types and actions | Calendar-specific capabilities proven, Graph review/invitation contract approved, date/time-zone and presentation tests pass before scheduling paths are removed |

Milestone 1 should resolve the uncertain Mail HTML, Thunderbird sandbox/argument, Graph draft-link and PDF-rendering behavior before those choices become expensive to change. Retain the intentionally limited mailto route, but do not use its limitations to reduce the native report feature set. Any unresolved Mail rich-HTML gap needs an explicit decision before declaring the planned parity complete.

## Verification and documentation

The current tree has real tests in `tests/desktop/CMakeLists.txt`, despite the older AGENTS.md statement. Extend the existing `tst_qmldesktop` and `tst_filefinder` coverage; add a focused `tst_email` target for meaningful content/adapter failure cases. Inject network, process, URL-launch and credential seams so automated tests never open real mail clients, alter real accounts or send messages.

Cover recipient grouping and internal-data filtering; encoding of punctuation/Unicode; missing attachments; Graph small/large uploads and partial drafts; insufficient consent; identity changes; ambiguous POST results; duplicate clicks; unavailable clients; mailto limitations; staging lifetime; helper failure; and QML keyboard/layout/error paths. Add all four workflow fixtures, export-option/default parity, report-folder setting migration, selected reporting dates, visual HTML/PDF comparison and removal/upgrade tests. For each of the three exports exercise inline HTML, HTML attachment, PDF attachment and Do not email, including Generate HTML Report both on and off and PDF display both on and off. Verify that Graph operations create unsent drafts and never invoke a send endpoint. Use the real shipping QML module in the smoke test and fail on runtime warnings.

Use separate build directories because the app and test builds declare the same QML module URI. With Qt, the sibling SqliteSyncPro repository and the required Python development installation available:

```sh
cmake -S . -B build-email -DCMAKE_BUILD_TYPE=Debug
cmake --build build-email --config Debug --target ProjectNotes
cmake -S . -B build-email-tests -DCMAKE_BUILD_TYPE=Debug -DBUILD_QML_DESKTOP=OFF -DBUILD_QML_DESKTOP_TESTS=ON
cmake --build build-email-tests --config Debug
ctest --test-dir build-email-tests -C Debug --output-on-failure
```

Perform manual handoff checks on Windows with Outlook Classic, new-Outlook-only and Thunderbird; macOS with Mail and Thunderbird (including Mail not being the default); Linux with conventional and sandboxed Thunderbird plus default mail handlers. Check signed macOS automation and packaged Linux file access. Use an isolated developer profile and controlled sample recipients; actual delivery checks, if performed during implementation, require explicit maintainer authorization.

For every backend, verify opening and focus attempts with the target application closed, already running, minimized and behind ProjectNotes. Check that the prepared message, rather than an unrelated inbox/window, is presented where the adapter can address it. Test Graph after a delayed attachment upload and browser sign-in, Thunderbird forwarding to an existing process, and OS-denied/unconfirmed activation. With Display Report when complete enabled, verify that PDF opening and ProjectNotes completion handling do not take focus back after email activation. Mock launch/activation failures to prove that retries reuse the existing message and never create duplicate drafts, uploads or sends.

Update the Office 365 help page, add native Email help, and update the help table of contents and all four workflow pages using the native names: Send Meeting Notes, Tracker Items Report, Status Report and Meeting Notes Report. Replace obsolete plugin menu/settings instructions and automatic-send claims with native Report menu actions and the confirmed Graph draft workflow. Preserve or redirect existing documentation paths so links to the old Export pages continue to work. Verify the new Report terminology in UI smoke tests, including dialog titles, Generate Report actions and report-folder settings labels. Document preserved report options/defaults, mail consent, draft location, client discovery, mailto/manual attachments, Mail automation permission, formatting limits and file staging/cleanup. Keep investigation and implementation-plan documents identifiable as developer material.

Document Communication Templates with a simple field-insertion walkthrough, inline versus attachment bodies, one-message edits versus saved defaults, legacy import diagnostics and optional trusted JavaScript examples. Keep future calendar instructions separate from email draft instructions; never describe the legacy Graph event call as guaranteed not to send invitations.

## Primary references

- [Microsoft Graph draft creation](https://learn.microsoft.com/en-us/graph/api/user-post-messages?view=graph-rest-1.0) and [message webLink](https://learn.microsoft.com/en-us/graph/api/resources/message?view=graph-rest-1.0).
- [Graph attachment upload thresholds and limits](https://learn.microsoft.com/en-us/graph/outlook-large-attachments).
- [Qt external URL handling](https://doc.qt.io/qt-6/qdesktopservices.html) and [RFC 6068](https://datatracker.ietf.org/doc/html/rfc6068).
- [Outlook Classic Display](https://learn.microsoft.com/en-us/office/vba/api/outlook.mailitem.display) and [Classic/new Outlook capability comparison](https://support.microsoft.com/en-us/outlook/getstarted/feature-comparison-between-new-outlook-and-classic-outlook).
- [Thunderbird upstream compose implementation](https://github.com/mozilla/releases-comm-central/blob/master/mail/components/compose/content/MsgComposeCommands.js) (development branch; validate release versions).
- [AppleScript structured event invocation](https://developer.apple.com/documentation/foundation/nsapplescript), [automation usage description](https://developer.apple.com/documentation/bundleresources/information-property-list/nsappleeventsusagedescription), and [Apple Events entitlement](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.security.automation.apple-events).
- [Qt native rich-text rendering](https://doc.qt.io/qt-6/qtextdocument.html).
- [Qt QJSEngine and interruption](https://doc.qt.io/qt-6/qjsengine.html).
- [Graph calendar event creation](https://learn.microsoft.com/en-us/graph/api/user-post-events?view=graph-rest-1.0) and [event resource/isDraft semantics](https://learn.microsoft.com/en-us/graph/api/resources/event?view=graph-rest-1.0).
