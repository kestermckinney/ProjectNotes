# QJSEngine plugin system plan

Branch: `feature/qjsengine_plugins`. Prepared: 2026-10-12. Status: proposed plan with scope decisions recorded. No application code has changed.

## Goal

Replace the embedded Python plugin system with a JavaScript plugin system built on `QJSEngine`. Every bundled Python plugin gets rewritten, and the Python interpreter, PyQt6, and their packaging leave the product entirely.

The new system should do more than the old one:

- Read and write data through a structured object API that keeps the database normalized the same way XML import and export do (lookup values in, foreign keys stored, unique keys matched, owned children cascaded).
- Reach the native email, calendar, and contacts integrations instead of re-implementing them in script.
- Contribute QML dialogs, menu items, and Settings sections that look and scale like the rest of the app.
- Remove the obstacles Python creates for Mac App Store distribution of the desktop app.

## Decisions

Recorded 2026-10-12.

1. **App Store target.** The Mac App Store, for the desktop app only. Plugin support in the iOS app is out of scope.
2. **User plugins.** Users can install their own plugins by placing them in their profile folder, `~/Project Notes/plugins/`, in every build including the App Store build.
3. **Windows Office automation.** Keep the Windows-only plugins that automate Office through COM (the five template plugins, Open MS Project, Close Stranded Excel/Word, Outlook Classic email download, Word conversion in SSRS capture). The JS system gets a Windows-only COM API so they can be ported faithfully.
4. **IFS plugins.** Ship separately from the app, installed into the user's profile plugin folder.
5. **Vocabulary.** Plugins see the Contact/Company names used in the UI, not the stored `people`/`clients` names.

## Where things stand today

The host side is three classes. `PluginManager` (`pluginmanager.cpp`) boots one CPython interpreter, appends `plugins/`, `threads/`, and `~/Project Notes/plugins|threads/` to `sys.path`, and exposes a `projectnotes` module with six functions: `get_data`, `update_data`, `force_reload`, `get_secret`, `set_secret`, `delete_secret`. `Plugin` and `PythonWorker` load one module each, optionally on its own `QThread`, read the `pluginname`, `plugindescription`, `plugintimerdelay`, and `pluginmenus` globals, and call `event_startup`, `event_shutdown`, `event_timer`, and menu functions with GIL handling. A `QFileSystemWatcher` hot-reloads changed files and their regex-detected imports.

`DesktopAppController` turns `pluginmenus` into record menus (`pluginMenusForTable`, `runPluginMenuForTable`) and global menus (`globalPluginMenus`, `runGlobalPluginMenu`). A record menu exports the selected row with `createXMLExportDoc(model, tablefilter)` and passes the XML string to the plugin. Plugins write back by returning XML or calling `update_data`, which runs `DatabaseObjects::importXMLDoc` on a private connection and forwards `rowChanged` to `global_DBObjects`.

Everything a plugin shows on screen is a PyQt6 widget, usually loaded from `plugins/forms/*.ui`. That single fact is why the app still links `Qt::Widgets` and constructs a `QApplication`.

There is already one sandboxed `QJSEngine` in the codebase: `PN::Comm::ComputedFieldRunner` (`ProjectNotesIntegrations/ComputedFieldRunner.cpp`). It copies a frozen, scalar `context` object into a fresh engine, removes `console`, and enforces per-field and total time limits with `setInterrupted()` from a watchdog thread. The plugin host should reuse its interruption pattern.

Native COM automation also exists already. `ProjectNotesContacts/helpers/OutlookContacts.cpp` and `ProjectNotesMeetings/backends/OutlookClassicMeetingBackend.cpp` each carry their own small `IDispatch` call/property/object helpers. These become the starting point for a shared COM wrapper.

### Inventory of bundled plugins

| Plugin | What it does | Platforms today | Disposition |
| --- | --- | --- | --- |
| `base_plugin.py` | Copy email (contact, team member, attendee) or path to clipboard; Script Editor launcher; Download Emails from Outlook (Outlook Classic, when not using Graph) | All; download is Windows | Port clipboard actions to JS. Editor launcher uses `process.launch` (absent in the App Store build). Outlook Classic download ports to JS on the `com` API, Windows only. |
| `base_plugin_settings.py` | Settings dialogs: Editor, Outlook Integration, My Shortcuts, Settings Migrator | All | Each dialog becomes a Settings section contributed by its owning plugin. Settings Migrator is retired after the one-time migration described below. |
| `myshortcuts_plugin.py` | User-defined URL menus with `[$table.column.row]` variable substitution | All | Port to JS with dynamic menu registration. Variables switch to the Contact/Company vocabulary, with old spellings translated when settings migrate. |
| `team_member_quick_add_plugin.py` | Dialog to create a contact and add them to the project team | All | Port to JS plus a QML dialog. Reference example for plugin UI. |
| `open_msproject_plugin.py` | Find the `.mpp` in Files & Folders and open it | Windows | Port to JS, Windows only. |
| `newchangeorder`, `newpcrregistry`, `newpowerpoint`, `newriskregister`, `newmsproject` | Copy an Office template into the project folder, replace `<PROJECTNUMBER>` style tokens through COM, register a project location | Windows | Port to JS on the `com` API, Windows only, same templates and token behavior. |
| `excelkill_plugin.py`, `wordkill_plugin.py` | Close orphaned Excel/Word automation processes | Windows | Port to JS on the `com` API (WMI through `com.getObject("winmgmts:...")`), Windows only. |
| `threads/outlooksync_thread.py` | Timer-driven tracker-to-To-Do sync and project email download through Graph | All | Port to JS on `office365.graph()`, `files`, and `timers`. |
| `ifscloud_plugin_settings.py`, `ifs_ssrs_generate_plugin.py`, `threads/ifssync_thread.py`, `includes/ifs_tools.py` (1,760 lines) | IFS ERP project import, tracker export, SSRS status report capture | All; SSRS Word step is Windows | Port to JS on `http`, `secrets`, `timers`, and `com` for the Word step. Ships separately (see "IFS distribution"). |
| `includes/graphapi_tools.py` (955 lines), `collaboration_tools.py`, `outlook_tools.py`, `word_tools.py`, `excel_tools.py` | Shared helpers | | Replaced by host APIs plus small shared JS modules; not ported line by line. |
| `includes/icloud_tools.py`, `includes/noteformatter.py` | Former iCloud contact sync and meeting-note formatting | | No plugin imports either one. Delete now, independent of this project. |

## Architecture

### New library: `ProjectNotesPlugins`

A static library next to `ProjectNotesIntegrations`, linking `Qt::Core`, `Qt::Qml`, `Qt::Network`, and `ProjectNotesCore`. It stays free of `Qt::Widgets` and `Qt::Quick` so the app can later drop `QApplication` (see Phase 6). Desktop services (email backends, meeting backends, contact providers, file finder) register themselves into it through small interfaces, following the `EmailBackend` and `MeetingBackend` pattern.

```
ProjectNotesPlugins/
  PluginManifest.{h,cpp}        manifest parsing and validation
  PluginRegistry.{h,cpp}        discovery, install, enable/disable, hot reload, contribution index
  PluginHost.{h,cpp}            one QJSEngine + QThread per plugin, lifecycle, watchdog
  PluginContext.{h,cpp}         per-plugin identity, permissions, settings and storage roots
  PublishedSchema.{h,cpp}       Contact/Company names <-> stored table and column names
  RecordTree.{h,cpp}            format-neutral record graph shared with XML import/export
  api/                          one QObject per JS namespace (DataApi, UiApi, EmailApi, ...)
  win/ComDispatch.{h,cpp}       shared IDispatch wrapper (Windows only)
ProjectNotesDesktop/
  PluginUiController.{h,cpp}    GUI-thread bridge: dialogs, settings sections, menus
  qml/plugins/                  PluginDialogHost.qml, PluginSettingsSection.qml, SchemaForm.qml
```

The existing `PluginManager`, `Plugin`, and `PythonWorker` stay untouched until the removal phase. During migration `DesktopAppController` asks both registries for menus and merges the results, so Python and JS plugins coexist and can be ported one at a time.

### Plugin package format

A plugin is a folder with a manifest, an ES module entry point, and optional QML and assets:

```
team-member-quick-add/
  plugin.json
  main.mjs
  QuickAddDialog.qml
```

```json
{
  "id": "com.projectnotespro.team-member-quick-add",
  "name": "Team Member Quick Add",
  "version": "1.0.0",
  "apiVersion": 1,
  "description": "Add a team member who is not yet a contact.",
  "platforms": ["windows", "macos", "linux"],
  "main": "main.mjs",
  "permissions": ["data.read:contacts,companies", "data.write:contacts,project_team", "ui.dialogs"],
  "contributes": {
    "menus": [
      { "id": "quickAdd", "title": "Quick Add", "context": "record", "table": "project_team" }
    ],
    "settings": [
      { "id": "general", "title": "Team Member Quick Add", "schema": "settings.schema.json" }
    ]
  }
}
```

Static contributions in the manifest let the app build menus and Settings sections without starting the plugin's engine. Plugins that compute menus at run time (My Shortcuts, Outlook Classic vs Graph download) call `menus.register()` instead. `apiVersion` lets the host refuse plugins written for an incompatible API rather than failing halfway through a call. `platforms` keeps Windows-only plugins from loading on macOS and Linux; the registry lists them as unavailable rather than hiding them.

### Plugin locations and installation

Two locations, searched in order:

1. **Bundled:** `appResourcesPath()/plugins/<id>/`, read only.
2. **User profile:** `~/Project Notes/plugins/<id>/`, the folder users already know from the Python system. A user plugin with the same `id` as a bundled one overrides it.

Installing a user plugin works three ways, all ending in the same folder:

- Copy the plugin folder into `~/Project Notes/plugins/` by hand.
- **Settings > Plugins > Install Plugin…** picks a plugin folder or a `.pnplugin` file (a zip of the folder), validates the manifest, and copies it in. Upgrades replace the folder after confirming the version change.
- **Open Plugins Folder** reveals the folder in Finder, Explorer, or the file manager.

The App Store build needs the second and third options. Under the App Sandbox, `QDir::homePath()` resolves to the app's container (`~/Library/Containers/<bundle id>/Data/`), so `~/Project Notes/plugins/` still exists and still works, but it is not the folder the user sees in their real home directory. Install Plugin… works because the sandbox grants read access to whatever the user picks in the open panel. Help text and the IFS install instructions must describe Install Plugin… as the primary route on macOS.

A newly installed user plugin starts disabled until the user approves its permissions (see "Security and trust"). Python plugins in the same folder keep loading under the old system until Phase 6, because they are `.py` files and JS plugins are folders with `plugin.json`, so the two never collide.

### Execution model

Each enabled plugin gets its own `QJSEngine` on its own worker `QThread`. That matches today's per-plugin worker, isolates crashes and runaway loops, and makes hot reload a matter of destroying one engine. A shared watchdog calls `setInterrupted(true)` on any engine that holds a synchronous call longer than a configurable budget (default 10 s for menu handlers, longer for timer jobs that declare it), then reports the plugin as hung instead of freezing the app.

All host calls that touch the database, the network, other services, or the user return Promises. The `com` API is the exception: COM calls are synchronous by nature and already run off the GUI thread. A menu handler is an async function:

```js
// main.mjs
import { data, ui } from "projectnotes";

export async function onMenu(id, context) {
    if (id !== "quickAdd") return;
    const result = await ui.showDialog("QuickAddDialog.qml", {
        projectId: context.record.project_id.id,
        companies: await data.list("companies", { fields: ["company_name"] })
    });
    if (!result) return;
    await data.transaction(async tx => {
        const contact = await tx.save("contacts", {
            name: result.name, email: result.email,
            company_id: { lookup: result.company }
        });
        await tx.save("project_team", {
            project_id: { id: context.record.project_id.id },
            contact_id: { id: contact.contact_id },
            role: result.role
        });
    });
}
```

Lifecycle exports replace the Python globals: `onStart()`, `onStop()`, `onMenu(id, context)`, and timers registered with `timers.every(minutes, fn)` instead of a single `plugintimerdelay`. The host also provides `setTimeout`/`setInterval` and routes `console.*` to `QLogger` under the plugin's id, so plugin output shows up in the existing log viewer.

The `projectnotes` module is registered with `QJSEngine::registerModule()` (Qt 6.5+, already the project minimum via `qt_standard_project_setup(REQUIRES 6.5)`). Plugins can import their own sibling `.mjs` files; `importModule()` resolves them relative to the plugin folder and the registry watches every resolved file for hot reload, replacing the regex import scanner in `PythonWorker::findImportedModules`. Shared JS helpers (for example a port of `ProjectNotesCommon.replace_variables`) ship as a bundled library module, `projectnotes/common`.

Engines never see raw `QObject`s from the app. Each API namespace is a thin `QObject` that lives on the plugin thread, checks the plugin's permissions, then marshals the request to the owning service (database connection, GUI thread, network transport) and resolves the Promise when the reply comes back.

### UI contributions

QML must run in the application's `QQmlEngine` on the GUI thread, while plugin logic runs in its worker engine. `PluginUiController` bridges the two by message passing: `ui.showDialog(file, data)` sends the QML path and a JSON-safe copy of `data` to the GUI thread, which instantiates the component inside `PluginDialogHost.qml`, and the Promise resolves with whatever the dialog passes to `pluginDialog.accept(result)` (or `null` on cancel).

Two tiers of UI, because they suit different needs:

1. **Schema forms.** A JSON description of fields (text, multiline, number, date, checkbox, choice, file/folder picker, table of rows) rendered by the app's own `FormField`, `ComboField`, `DateField`, and `StyledComboBox`. Consistent theme, keyboard navigation, spell check, and `Theme.uiScale` zoom for free. Most settings pages and simple prompts should use this. Every existing `.ui` settings form (export subfolder, editor path, IFS credentials, SSRS options, shortcuts table) fits it.
2. **Plugin QML.** Full QML files for richer interfaces. They import a new `ProjectNotes.PluginUI` module that re-exports the themed controls and exposes a `pluginDialog` object (accept, reject, and `pluginDialog.invoke(name, args)` to call back into the plugin for live lookups).

Both tiers must render inside the app's dialog host. The zoom work found that each `Popup` needs `Theme.uiScale` applied to it explicitly, so a plugin dialog created anywhere else would ignore zoom. Centralizing creation in `PluginDialogHost.qml` fixes that once.

**Menus.** Contributions declare `context: "record"` (with `table`, matching today's `dataexport`) or `context: "global"` (today's empty `dataexport`), plus an optional `submenu`. They feed the same `RecordContextMenu`, `RecordRowMenu`, and AppMenu Plugins group the Python menus use now. The record context handed to `onMenu` is a `RecordTree` object (below), not XML; `include` on the menu replaces `tablefilter`.

**Settings sections.** `SettingsPage.qml` builds its section list from a hard-coded array. That array becomes the built-in sections, then a new **Plugins** section (installed plugins, enable/disable, permissions, Install Plugin…, Open Plugins Folder), then one section per plugin contribution, each loading either `SchemaForm.qml` or the plugin's QML in a `Loader`. A plugin settings section reads and writes through the plugin's own `settings` namespace.

QML cannot be sandboxed the way a bare `QJSEngine` can: it has `Qt.openUrlExternally`, `XMLHttpRequest`, and access to any context property in scope. Plugin QML is therefore exactly as trusted as the plugin itself. The permission model below is a consent and review mechanism, not a security boundary, for plugins that ship QML. Plugins that only use schema forms do stay inside the boundary.

## Data access

### One normalization path

`SqlQueryModel::setData(QDomElement*)` already contains the rules that keep imported data normalized: unique-key matching (`m_uniqueKeys`, `checkRowUniqueKeys`), `lookupvalue` resolution into foreign keys (`m_lookupTable`, `m_lookupFkColumnName`, `m_lookupValueColumnName`), soft delete on `delete="true"`, and owned-child cascade for `DBExportable` relations. `DatabaseObjects::importXMLDoc` fixes the table order (clients, people, projects, audiences, project_people, ...). `createXMLExportDoc` produces the nested tree with `lookupvalue` attributes.

Rather than give plugins a second, SQL-shaped write path that would drift from those rules, extract them into a format-neutral layer:

- `RecordTree`: a nested `{table, rows[{columns, children, delete}]}` structure where a foreign-key column holds `{id, lookup}`.
- `RecordTreeImporter`: the body of today's XML `setData` and `importXMLDoc`, operating on `RecordTree`.
- `RecordTreeExporter`: the body of `createXMLExportDoc`, producing `RecordTree`.
- `XmlRecordCodec` and `JsRecordCodec`: thin converters. XML import/export, the Python bridge during migration, and the JS API all go through the same importer and exporter.

The refactor is behavior-preserving and must land first, with round-trip tests: export each table to XML through the old code and the new codec and require identical documents; import fixtures through both and require identical database state. The delete-cascade rules (owned children cascade, `DBNotExportable` references block) need explicit cases.

### Export performance

The export's cost today is SQL, not XML. `SqlQueryModel::toQDomElement` (`sqlquerymodel.cpp`) runs one `SELECT` per foreign-key cell to fetch its `lookupvalue`, creates and refreshes a child model for every exportable relation of every row, refreshes child models before checking `tablefilter` (so excluded tables are still queried and discarded), and always loads every column. `createXMLExportDoc` adds two header lookups per call, and the Python `update_data` callback opens a new database connection on every call. Switching the text format to JSON would not change any of that. `RecordTreeExporter` fixes it instead:

1. **Batched lookups.** Collect foreign-key ids per column across all rows and resolve each column's display values with one `WHERE id IN (...)` query (or a join in the base query), instead of one query per cell.
2. **Batched children.** Load each child relation once for all parent keys in the current level and group rows in memory, instead of one child query per parent row. Nested includes repeat this per level, so query count grows with tree depth, not row count.
3. **Filter before querying.** Apply `include` (the replacement for `tablefilter`) before any child relation is queried. A relation that is not requested is never touched.
4. **Requested columns only.** Honor `fields` by selecting only the requested columns plus the key and the columns needed to join children and resolve lookups.
5. **One connection per plugin.** The plugin host opens its `DatabaseObjects` connection once when the plugin starts and closes it on unload, replacing the per-call open and close in today's `update_data`.

The XML Import/Export menu items go through the same exporter and get the same speedup. The round-trip tests guarantee the batched exporter produces the same output as the per-row one, and a benchmark in `tests/desktop/` exports a large synthetic project (hundreds of tracker items with comments) through both and records query counts and elapsed time.

### Published schema: Contact and Company names

Plugins never see the stored names. `PublishedSchema` maps between the names plugins use and the stored ones, and `JsRecordCodec` applies it in both directions, so the database and the XML file format do not change.

| Stored | Published |
| --- | --- |
| `people` | `contacts` |
| `clients` | `companies` |
| `project_people` | `project_team` |
| `people_id` | `contact_id` |
| `client_id` | `company_id` |
| `client_name` | `company_name` |

The table above covers the names found so far. Phase 1 includes an audit of every table, column, and view (`databasestructure.cpp`, `databaseviews.cpp`, and each model's `addColumn` calls) for remaining `people`/`client` names, settled in one list before the API freezes at `apiVersion: 1`. `people_source` in `project_email_audiences` holds values rather than naming a relation, so its values need a decision in the same audit.

The mapping applies everywhere a plugin names data: `data.*` calls, manifest `permissions` and menu `table` values, record contexts passed to `onMenu`, and `data.onChanged` events. Stored names are rejected with an error that suggests the published name.

`data.exportXml()` and `data.importXml()` are the one deliberate exception. They read and write the Project Notes XML file format, which keeps its stored names so existing exports, saved files, and other users' databases stay compatible.

### JS data API

```js
// Read: nested records, foreign keys carry both id and display value
const project = await data.get("projects",
    { where: { project_number: "P4000" }, include: ["project_team", "project_locations"] });
project.company_id;          // { id: "{ba96...}", lookup: "Special Law Firm" }
project.project_team[0].role; // "Lead Support"

const open = await data.list("item_tracker",
    { where: { project_id: project.project_id, status: ["New", "Assigned"] },
      orderBy: ["priority", "date_due"], limit: 200 });

// Write: same matching as XML import. Lookups resolve to ids; unique keys find existing rows.
await data.save("project_locations", {
    project_id: { lookup: "P4000" },
    location_type: "Word Document",
    location_description: "Change Request : P4000 PCR01.docx",
    full_path: path
});
await data.remove("project_locations", { id: locationId });   // soft delete + owned-child cascade
await data.transaction(async tx => { /* several saves, all or nothing */ });

// Events the Python system never had
data.onChanged(["item_tracker"], change => { /* {table, id, operation} */ });
```

Details to settle during implementation:

- **Queries.** `where` supports equality, `IN` lists, and a small operator set (`$lt`, `$gt`, `$like`, `$null`) compiled to bound parameters. No raw SQL is exposed. A plugin that needs an aggregate can request a named read-only view added to `databaseviews.cpp`.
- **Threading.** Like `update_data` today, each plugin host opens its own `DatabaseObjects` connection on its thread, forwards `rowChanged` to `global_DBObjects` with a queued connection, and the controller drains it with `updateDisplayData()` and `FolderManager::reload()`. Writes go through the same path as XML import, so SqliteSyncPro change capture keeps working without plugin-specific code.
- **Permissions.** `data.read:<tables>` and `data.write:<tables>` in the manifest, enforced in `DataApi` before anything reaches the importer. Includes check read permission per child table.

## Plugin API reference (apiVersion 1)

This is the surface a plugin author codes against. Each namespace wraps an existing native service; nothing here re-implements email, calendar, or contact logic in script. Signatures use TypeScript notation, and the same declarations ship with the app as `projectnotes.d.ts` so editors such as VS Code offer completion and type checking while writing plugins.

### Overview

| Namespace | Backed by | Purpose | Availability |
| --- | --- | --- | --- |
| `plugin` | `PluginContext` | The running plugin's identity and folders | All |
| `data` | `RecordTree*`, `PublishedSchema`, `DatabaseObjects` | Read, write, search, and watch records | All |
| `app` | `DesktopAppController` | Version, platform, capabilities, current view, navigation | All |
| `menus` | `PluginRegistry` | Dynamic menu items | All |
| `ui` | `PluginUiController` | Messages, forms, QML dialogs, pickers, progress, clipboard | All |
| `settings` | `QSettings` | Per-plugin settings | All |
| `secrets` | `CredentialStore` | Per-plugin credentials | All |
| `storage` | plugin data folder | Private files and state | All |
| `files` | `QFile`, project folder resolution | Files in permitted locations | All |
| `email` | `CommunicationsController`, `EmailService`, `CommunicationTemplateStore` | Reviewed email drafts, audiences, templates | All |
| `calendar` | `MeetingService`, `IcsWriter` | Reviewed meeting drafts, `.ics` files | All |
| `contacts` | contact sync, `ContactPickerDialog`, `VCard` | External address books, contact picker, vCard | All |
| `office365` | `MicrosoftOAuthManager`, `Office365Service` | Microsoft Graph with the shared sign-in | All |
| `http` | `QtNetworkHttpTransport` | HTTP to hosts declared in the manifest | All |
| `timers` | host event loop | Repeating and one-shot jobs | All |
| `log`, `console` | `QLogger` | Plugin-tagged log output | All |
| `com` | `ComDispatch` | Windows COM automation | Windows |
| `process` | `QProcess` | Launch external programs | Not in the App Store build |
| `reports` | `ReportService` | Run the native reports | Later version |

`app.capabilities.has(name)` reports what the current build offers (`"com"`, `"process.launch"`, `"email.compose"`, `"calendar.draft"`, `"office365"`, ...), so one plugin can adapt rather than test the platform.

### Conventions

```ts
import { data, ui, menus, settings } from "projectnotes";
import { replaceVariables } from "projectnotes/common";
```

- **Async.** Every call that touches the database, network, user, or another service returns a `Promise`. Pure getters, `log`, `timers`, and `com` are synchronous.
- **Values.** Strings, numbers, booleans, and `null`. Dates are `"YYYY-MM-DD"` strings and date-times are ISO 8601 strings in local time, converted at the codec from the stored epoch values and the XML `MM/dd/yyyy` form. Currency and percentage columns are numbers.
- **References.** A foreign-key column holds a `Ref`. When writing, give either `id` or `lookup`; lookups resolve exactly as XML import resolves `lookupvalue`.
- **Names.** Entities and columns use the published Contact/Company vocabulary (see "Published schema" above).
- **Errors.** Rejected Promises and thrown exceptions are `PluginError` objects.

```ts
interface Ref { id: string | null; lookup: string | null; }

interface PluginError extends Error {
  code: string;            // "permission-denied", "not-found", "validation-failed",
                           // "reference-blocked", "unique-conflict", "cancelled",
                           // "unavailable", "timeout", "network", "com-failed", ...
  message: string;
  permission?: string;     // the missing permission for "permission-denied"
  details?: object;        // e.g. the failing column and value for "validation-failed"
}
```

### Plugin entry points

`main.mjs` exports any of these. All may be `async`.

```ts
export function onStart(): void;                    // after load and after each hot reload
export function onStop(): void;                     // before unload, reload, or app exit
export function onMenu(menuId: string, context: MenuContext): void;
export function onSettingsChanged(keys: string[]): void;

interface MenuContext {
  menuId: string;
  kind: "record" | "global";
  entity: string | null;     // "projects", "contacts", "project_team", ...
  key: string | null;        // primary key of the clicked record
  record: DataRecord | null;     // loaded with the menu's declared "include"
  parameter: string;         // the menu item's "parameter" value
}
```

### `plugin`

```ts
const plugin: {
  readonly id: string;          // "com.projectnotespro.team-member-quick-add"
  readonly name: string;
  readonly version: string;
  readonly apiVersion: number;
  readonly bundled: boolean;    // shipped with the app vs installed in the profile folder
  readonly packagePath: string; // read-only plugin folder
  readonly dataPath: string;    // writable private folder (same as storage.path)
};
```

### `data`

Records are plain objects. Every record carries two read-only metadata properties plus its columns, and any requested child entities appear as arrays under their entity name.

```ts
interface DataRecord {
  readonly $entity: string;     // "projects"
  readonly $key: string;        // primary key value
  [column: string]: string | number | boolean | null | Ref | DataRecord[];
}

interface Query {
  where?: Where;
  include?: (string | IncludeSpec)[];   // child entities, e.g. ["project_team", "project_locations"]
  fields?: string[];                     // columns to return; default all
  orderBy?: (string | { column: string; descending?: boolean })[];
  limit?: number;
  offset?: number;
  includeDeleted?: boolean;              // soft-deleted rows; default false
}
interface IncludeSpec { entity: string; where?: Where; include?: (string | IncludeSpec)[];
                        orderBy?: Query["orderBy"]; }

// Equality by value, IN by array, or an operator object. A Ref column matches an id string or { lookup }.
type Where = { [column: string]: Value | Value[] | Ref | Operator };
interface Operator { $ne?: Value; $lt?: Value; $lte?: Value; $gt?: Value; $gte?: Value;
                     $like?: string; $null?: boolean; $between?: [Value, Value]; }

interface EntityInfo {
  name: string;                 // "item_tracker"
  displayName: string;          // "Tracker Items"
  key: string;                  // primary key column
  uniqueKeys: string[][];       // the business keys used for matching on save
  columns: ColumnInfo[];
  children: { entity: string; column: string; owned: boolean }[];  // owned = cascades on remove
}
interface ColumnInfo {
  name: string; displayName: string;
  type: "string" | "text" | "date" | "datetime" | "bool" | "number" | "currency" | "percent" | "ref";
  required: boolean; readOnly: boolean; unique: boolean; searchable: boolean;
  ref?: { entity: string };     // for type "ref"
  choices?: string[];           // fixed value lists, e.g. tracker priority and status
}

interface SaveResult   { record: DataRecord; operation: "insert" | "update"; }
interface RemoveResult { removed: { entity: string; key: string }[]; }  // includes cascaded children
interface ImportResult { inserted: number; updated: number; removed: number; }
interface ChangeEvent  { entity: string; key: string; operation: "insert" | "update" | "delete";
                         source: "user" | "sync" | "plugin"; pluginId?: string; }
interface Subscription { unsubscribe(): void; }

const data: {
  entities(): Promise<EntityInfo[]>;
  schema(entity: string): Promise<EntityInfo>;

  get(entity: string, keyOrWhere: string | Where, options?: Omit<Query, "where">): Promise<DataRecord | null>;
  list(entity: string, query?: Query): Promise<DataRecord[]>;
  count(entity: string, where?: Where): Promise<number>;
  search(text: string, options?: { entities?: string[]; limit?: number })
        : Promise<{ entity: string; key: string; title: string; snippet: string }[]>;

  save(entity: string, record: object): Promise<SaveResult>;
  saveAll(entity: string, records: object[]): Promise<SaveResult[]>;   // one transaction
  remove(entity: string, keyOrRecord: string | DataRecord): Promise<RemoveResult>;
  transaction<T>(work: (tx: Transaction) => Promise<T>): Promise<T>;   // commit on resolve, roll back on throw

  onChanged(entities: string | string[], handler: (e: ChangeEvent) => void): Subscription;

  exportXml(entity: string, key: string, options?: { include?: string[] }): Promise<string>;
  importXml(xml: string): Promise<ImportResult>;
};

interface Transaction {
  get: typeof data.get; list: typeof data.list; count: typeof data.count;
  save: typeof data.save; saveAll: typeof data.saveAll; remove: typeof data.remove;
}
```

Write rules, all inherited from XML import: `save` updates when the record's key or one of the entity's unique keys matches an existing row and inserts otherwise. Read-only columns in the input are rejected with `validation-failed`, not silently dropped. `remove` soft-deletes, cascades to owned children, and fails with `reference-blocked` when a non-owned record still refers to it. `exportXml` and `importXml` use the stored XML names.

### `app`

```ts
interface ViewInfo { section: "projects" | "contacts" | "companies" | "items" | "search" | "settings";
                     entity: string | null; key: string | null; tab: string | null; }

const app: {
  readonly version: string;            // "6.4.0"
  readonly apiVersion: number;
  readonly platform: "windows" | "macos" | "linux";
  readonly buildFlavor: "standard" | "appstore";
  readonly profile: string;            // developer profile, "" for the default
  readonly locale: string;
  readonly capabilities: { has(name: string): boolean; list(): string[] };

  currentView(): ViewInfo;
  currentRecord(options?: { include?: string[] }): Promise<DataRecord | null>;
  navigate(entity: string, key: string, options?: { tab?: string }): Promise<void>;
  projectFolder(projectKey: string): Promise<string | null>;   // resolved project folder, if any
  refresh(): void;                                             // reload visible lists
  onViewChanged(handler: (view: ViewInfo) => void): Subscription;
};
```

### `menus`

Static items come from the manifest. Dynamic items go through `register` and are dispatched to `onMenu` or to their own `handler`.

```ts
interface MenuItem {
  id: string;
  title: string;
  context: "record" | "global";
  entity?: string;              // required for "record"
  submenu?: string;             // "Templates", "Utilities", "Settings", ...
  include?: string[];           // children loaded into MenuContext.record
  parameter?: string;
  platforms?: ("windows" | "macos" | "linux")[];
  handler?: (context: MenuContext) => void | Promise<void>;
}

const menus: {
  register(items: MenuItem | MenuItem[]): void;
  unregister(ids: string | string[]): void;
  clear(): void;                // remove all dynamic items for this plugin
  list(): MenuItem[];           // static and dynamic
};
```

### `ui`

```ts
const ui: {
  message(text: string, options?: { title?: string; kind?: "info" | "warning" | "error" }): Promise<void>;
  confirm(text: string, options?: { title?: string; okText?: string; cancelText?: string;
                                    destructive?: boolean }): Promise<boolean>;
  prompt(label: string, options?: { title?: string; value?: string; placeholder?: string;
                                    multiline?: boolean; password?: boolean }): Promise<string | null>;
  choose<T extends string>(label: string, choices: T[],
                           options?: { title?: string; multiple?: boolean }): Promise<T | T[] | null>;

  showForm(form: FormSchema, values?: object,
           options?: { title?: string; okText?: string; width?: number }): Promise<object | null>;
  showDialog(qmlFile: string, data?: object, options?: {
               title?: string; width?: number; height?: number;
               handlers?: { [name: string]: (args: any) => any };   // reachable from pluginDialog.invoke()
             }): Promise<any | null>;

  pickFile(options?: { title?: string; filters?: string[]; startIn?: string; multiple?: boolean })
          : Promise<string | string[] | null>;
  pickFolder(options?: { title?: string; startIn?: string }): Promise<string | null>;
  saveFile(options?: { title?: string; suggestedName?: string; filters?: string[] }): Promise<string | null>;

  notify(text: string, options?: { kind?: "info" | "success" | "warning" | "error"; seconds?: number }): void;
  progress<T>(title: string, task: (p: Progress) => Promise<T>, options?: { cancellable?: boolean }): Promise<T>;
  openUrl(url: string): Promise<boolean>;            // http, https, and mailto only
  clipboard: { setText(text: string): void; text(): Promise<string> };
};

interface Progress { update(fraction: number | null, text?: string): void; readonly cancelled: boolean; }
```

**Form schema.** Rendered with the app's own controls, also used for manifest Settings sections.

```ts
interface FormSchema { fields: FormField[]; }
interface FormField {
  key: string;
  label: string;
  type: "text" | "multiline" | "password" | "number" | "date" | "checkbox" | "choice" | "combo"
      | "file" | "folder" | "contact" | "table" | "section" | "note";
  required?: boolean;
  placeholder?: string;
  help?: string;
  choices?: string[];           // "choice" and "combo"
  filters?: string[];           // "file"
  columns?: FormField[];        // "table": an editable grid of rows
  visibleWhen?: { [key: string]: any };
}
```

**Objects available to plugin QML.** A plugin QML file imports `ProjectNotes.PluginUI` and gets the themed controls plus a context object.

| QML name | Members |
| --- | --- |
| `pluginDialog` (dialogs) | `data`, `title`, `accept(result)`, `reject()`, `invoke(name, args): Promise`, `busy` |
| `pluginSettings` (Settings sections) | `values`, `get(key)`, `set(key, value)`, `invoke(name, args): Promise` |
| Controls | `FormField`, `ComboField`, `DateField`, `StyledComboBox`, `SpellCheckField`, `Button`, `KebabButton`, `Card`, `MaterialIcon`, `ContactPicker` |
| `Theme` | Colors, fonts, spacing, radius, and `uiScale`, read only |

`invoke` calls a function registered in `showDialog`'s `handlers` or exported from `main.mjs`, running on the plugin's engine thread. QML never receives `data`, `files`, or any other namespace directly.

### `settings`

Stored under `PluginSettings/<plugin id>/`. Values are JSON-compatible.

```ts
const settings: {
  get<T = any>(key: string, defaultValue?: T): T;
  set(key: string, value: any): void;
  remove(key: string): void;
  keys(): string[];
  all(): object;
  onChanged(handler: (keys: string[]) => void): Subscription;
};
```

### `secrets`

Backed by the operating system credential store. The service name is always the plugin id.

```ts
const secrets: {
  get(account: string): Promise<string | null>;
  set(account: string, secret: string): Promise<void>;
  remove(account: string): Promise<void>;
};
```

### `storage`

The plugin's private writable folder. Paths are relative to it and cannot escape it.

```ts
const storage: {
  readonly path: string;
  readText(path: string): Promise<string | null>;
  writeText(path: string, text: string): Promise<void>;
  readJson<T = any>(path: string): Promise<T | null>;
  writeJson(path: string, value: any): Promise<void>;
  remove(path: string): Promise<void>;
  list(dir?: string): Promise<string[]>;
};
```

### `files`

Absolute paths, permitted only inside the plugin's package (read only), its storage, resolved project folders, locations the user picked through `ui` in this session, and their descendants. In the App Store build, picked locations persist through security-scoped bookmarks.

```ts
interface FileInfo { path: string; name: string; isDir: boolean; size: number; modified: string; }

const files: {
  exists(path: string): Promise<boolean>;
  stat(path: string): Promise<FileInfo | null>;
  list(dir: string, options?: { filter?: string[]; recursive?: boolean }): Promise<FileInfo[]>;
  readText(path: string): Promise<string>;
  writeText(path: string, text: string): Promise<void>;
  readBytes(path: string): Promise<ArrayBuffer>;
  writeBytes(path: string, bytes: ArrayBuffer): Promise<void>;
  copy(from: string, to: string, options?: { overwrite?: boolean }): Promise<void>;
  move(from: string, to: string, options?: { overwrite?: boolean }): Promise<void>;
  mkdir(path: string): Promise<void>;
  remove(path: string): Promise<void>;
  packagePath(relative: string): string;          // e.g. files.packagePath("templates/PCR Template.docx")
  tempPath(name: string): string;                 // in the app cache, removed on exit
  openWithSystem(path: string): Promise<boolean>; // default application
  reveal(path: string): Promise<void>;            // Finder, Explorer, or file manager
};
```

### `email`

Every message goes through the existing review dialog and becomes a draft in the user's chosen backend. Plugins cannot send mail silently.

```ts
type Recipient = string | { name?: string; email: string } | { contact: string };   // contact key

interface EmailMessage {
  to?: Recipient[]; cc?: Recipient[]; bcc?: Recipient[];
  subject: string;
  html?: string; text?: string;
  attachments?: string[];       // file paths
  projectKey?: string;          // lets the review dialog offer the project's audiences
}
interface Audience { id: string; name: string; workflow: string; isDefault: boolean; }

const email: {
  backends(): Promise<{ id: string; name: string; available: boolean; preferred: boolean;
                        supportsHtml: boolean; supportsAttachments: boolean }[]>;
  compose(message: EmailMessage): Promise<{ status: "handed-off" | "cancelled"; backend?: string }>;
  audiences(projectKey: string, workflow?: string): Promise<Audience[]>;
  resolveAudience(projectKey: string, audienceId: string)
        : Promise<{ to: Recipient[]; cc: Recipient[]; bcc: Recipient[] }>;
  templates(): Promise<{ id: string; name: string; workflow: string }[]>;
  renderTemplate(templateId: string, context: { projectKey?: string; noteKey?: string })
        : Promise<{ subject: string; html: string }>;
};
```

### `calendar`

```ts
interface Meeting {
  subject: string;
  start: string; end: string;   // ISO 8601
  allDay?: boolean;
  location?: string;
  attendees?: Recipient[];
  html?: string;
  projectKey?: string; noteKey?: string;
}

const calendar: {
  backends(): Promise<{ id: string; name: string; available: boolean; preferred: boolean }[]>;
  draftMeeting(meeting: Meeting): Promise<{ status: "handed-off" | "cancelled"; backend?: string }>;
  writeIcs(meeting: Meeting, path: string): Promise<string>;
};
```

### `contacts`

External address books. Contacts stored in Project Notes are read and written through `data` (`contacts`, `companies`).

```ts
const contacts: {
  sources(): Promise<{ id: string; name: string; kind: string; enabled: boolean; lastSync: string | null }[]>;
  syncNow(sourceId?: string): Promise<{ added: number; updated: number; conflicts: number }>;
  pick(options?: { multiple?: boolean; projectKey?: string }): Promise<string | string[] | null>;  // contact keys
  toVCard(contactKey: string): Promise<string>;
  fromVCard(text: string): Promise<object[]>;     // contact-shaped objects ready for data.save("contacts", ...)
};
```

### `office365`

Uses the app's existing Microsoft sign-in. A plugin lists the Graph scopes it needs in the manifest; scopes beyond what the app already holds require the user to consent again.

```ts
interface GraphResponse { status: number; headers: { [name: string]: string }; body: any; }

const office365: {
  status(): Promise<{ signedIn: boolean; account: string | null; scopes: string[] }>;
  signIn(): Promise<boolean>;                     // opens the existing sign-in flow
  graph(method: "GET" | "POST" | "PATCH" | "PUT" | "DELETE", path: string, options?: {
          body?: any; query?: { [k: string]: string }; headers?: { [k: string]: string };
          responseType?: "json" | "text" | "arraybuffer" }): Promise<GraphResponse>;
  graphPages(path: string, options?: { query?: { [k: string]: string } }): AsyncIterable<any>;  // follows @odata.nextLink
};
```

### `http`

Only hosts listed in the manifest's `http` array are reachable.

```ts
interface HttpResponse {
  status: number; ok: boolean; headers: { [name: string]: string };
  text(): Promise<string>; json(): Promise<any>; arrayBuffer(): Promise<ArrayBuffer>;
}

const http: {
  fetch(url: string, options?: {
          method?: string; headers?: { [k: string]: string };
          body?: string | ArrayBuffer | object;           // objects are sent as JSON
          auth?: { type: "basic"; username: string; password: string } | { type: "bearer"; token: string };
          timeoutSeconds?: number }): Promise<HttpResponse>;
};
```

### `timers`

The global `setTimeout`, `setInterval`, `clearTimeout`, and `clearInterval` are also available.

```ts
interface Timer { cancel(): void; }

const timers: {
  every(minutes: number, job: () => void | Promise<void>, options?: { runAtStart?: boolean }): Timer;
  after(milliseconds: number, job: () => void | Promise<void>): Timer;
};
```

A repeating job never overlaps itself; if a run is still going when the next is due, that tick is skipped and logged.

### `log` and `console`

```ts
const log: { debug(...args: any[]): void; info(...args: any[]): void;
             warn(...args: any[]): void; error(...args: any[]): void; };
// console.log/info/warn/error/debug map to the same functions.
```

Output goes to `QLogger` tagged with the plugin id and appears in the log viewer; `error` also goes to the error log.

### `com` (Windows)

Present only when `app.capabilities.has("com")`. ProgIDs and monikers must be listed in the manifest's `com` array. Calls are synchronous. The design is described under "Windows COM bridge" below.

```ts
const com: {
  createObject(progId: string): ComObject;              // CoCreateInstance
  getActiveObject(progId: string): ComObject | null;    // a running instance, or null
  getObject(moniker: string): ComObject;                // e.g. "winmgmts:{impersonationLevel=impersonate}!\\\\.\\root\\cimv2"
  release(object: ComObject): void;                     // all remaining objects are released on unload
};

// A Proxy: properties read and write directly, methods are called directly,
// collections iterate with for...of through _NewEnum.
interface ComObject {
  [member: string]: any;
  $call(name: string, ...args: any[]): any;
  $get(name: string, ...index: any[]): any;             // indexed properties
  $set(name: string, value: any, ...index: any[]): void;
  [Symbol.iterator](): Iterator<any>;
}
```

Type mapping: `BSTR` to string, integer and floating types to number, `VT_BOOL` to boolean, `VT_DATE` to ISO string, `VT_EMPTY`/`VT_NULL` to null, `SAFEARRAY` to array, `VT_DISPATCH` to `ComObject`. Failures throw `PluginError` with code `com-failed` and the `HRESULT` in `details`.

### `process` (desktop builds except App Store)

Present only when `app.capabilities.has("process.launch")`.

```ts
const process: {
  launch(program: string, args?: string[], options?: { workingDirectory?: string }): Promise<{ pid: number }>;
};
```

### `reports` (later version)

```ts
const reports: {
  generate(workflow: "tracker-items" | "status" | "meeting-notes", projectKey: string, options?: object)
          : Promise<{ pdf?: string; html?: string }>;
};
```

### `projectnotes/common`

Pure helpers shipped as a bundled module rather than host calls.

```ts
export function replaceVariables(template: string, record: DataRecord): string;  // "[$projects.project_number.1]" syntax from My Shortcuts
export function escapeHtml(text: string): string;
export function textToHtml(text: string): string;
export function validFilename(name: string): string;
export function formatDate(isoDate: string, pattern: string): string;      // Qt date patterns, e.g. "MM/dd/yyyy"
export function parseDate(text: string, pattern?: string): string | null;
```

### Manifest reference

```ts
interface PluginManifest {
  id: string;                    // reverse-DNS, unique
  name: string;
  version: string;               // semantic version
  apiVersion: 1;
  description: string;
  author?: string;
  homepage?: string;
  minAppVersion?: string;
  platforms?: ("windows" | "macos" | "linux")[];   // default: all
  main: string;                  // entry module, e.g. "main.mjs"
  permissions: string[];         // see the table below
  http?: string[];               // reachable hosts, wildcards allowed: "*.ifs.cloud"
  com?: string[];                // ProgIDs and monikers: "Word.Application", "winmgmts:"
  office365?: { scopes: string[] };                 // e.g. ["Tasks.ReadWrite", "Mail.Read"]
  contributes?: {
    menus?: Omit<MenuItem, "handler">[];
    settings?: { id: string; title: string; schema?: string; qml?: string }[];
  };
}
```

### Permissions

| Permission | Grants |
| --- | --- |
| `data.read:<entities>` | `data.get`, `list`, `count`, `search`, `onChanged`, `exportXml` for the listed entities (`*` for all) |
| `data.write:<entities>` | `data.save`, `saveAll`, `remove`, `importXml` for the listed entities |
| `ui.dialogs` | `ui.showDialog` and `ui.showForm` (messages, confirm, prompt, pickers, and notify need no permission) |
| `clipboard` | `ui.clipboard` |
| `files` | `files.*` within the permitted locations |
| `email.compose` | `email.compose`, `audiences`, `resolveAudience`, `templates`, `renderTemplate` |
| `calendar.draft` | `calendar.*` |
| `contacts.sync` | `contacts.sources`, `syncNow` |
| `contacts.pick` | `contacts.pick`, `toVCard`, `fromVCard` |
| `office365` | `office365.*` within the manifest's scopes |
| `http` | `http.fetch` to the manifest's hosts |
| `com` | `com.*` for the manifest's ProgIDs and monikers |
| `process.launch` | `process.launch` |
| `app.navigate` | `app.navigate` (other `app` members need no permission) |

`plugin`, `settings`, `secrets`, `storage`, `menus`, `timers`, `log`, and `projectnotes/common` are always available, scoped to the plugin.

### Windows COM bridge

The Office plugins depend on driving Word, Excel, PowerPoint, Project, and Outlook, plus WMI for the process cleanup plugins. A general COM bridge preserves that behavior without a native service per plugin.

**Native side.** `ComDispatch` consolidates the `IDispatch` helpers now duplicated in `OutlookContacts.cpp` and `OutlookClassicMeetingBackend.cpp` into one tested wrapper: `CoCreateInstance` from a ProgID, `GetActiveObject` for a running instance, `CoGetObject` for monikers such as `winmgmts:`, method calls, property get/put, indexed properties, `_NewEnum` enumeration, and `VARIANT` conversion for strings, numbers, booleans, dates, null, arrays, and nested dispatch objects. The two existing backends move onto it once it is tested, so the app has a single COM implementation. ActiveQt's `QAxObject` was considered and rejected because it pulls in `Qt::Widgets` and does not bind monikers.

**JS side.** A JS `Proxy` over the primitives gives plugins the same shape the Python code uses:

```js
import { com, files } from "projectnotes";

const word = com.createObject("Word.Application");
try {
    const doc = word.Documents.Open(projectFile);
    doc.Content.Find.Execute("<PROJECTNUMBER>", false, false, false, false, false,
                             true, 1, false, projectNumber, 2);
    doc.Save();
    word.Visible = true;
} finally {
    com.release(word);   // also released when the plugin unloads
}
```

**Threading and lifetime.** On Windows each plugin worker thread initializes COM as a single-threaded apartment before the engine starts, and uninitializes on unload after releasing every object the plugin still holds. COM calls are synchronous and run on the plugin thread, never the GUI thread.

**Hangs.** The engine watchdog cannot interrupt a call blocked inside an Office process. The host registers an `IMessageFilter` on plugin threads so a busy Office application retries for a bounded time and then fails the call, and the watchdog reports a plugin stuck in native code instead of interrupting it.

**Permissions.** Manifests list the ProgIDs and monikers they use (`"com": ["Word.Application", "Excel.Application", "winmgmts:"]`) and the API rejects anything else. The `com` namespace and capability do not exist on macOS or Linux, and these plugins declare `"platforms": ["windows"]`.

## IFS distribution

The IFS plugins leave the app's bundled plugin set and become one separately built package.

- **Source.** Kept in this repository under `extras/ifs/` as a normal plugin folder (`plugin.json`, modules, settings schema, Word conversion on `com`). No CMake `install()` rule touches it, so no app package ever contains it.
- **Package.** `packaging/plugins/build-pnplugin.sh` zips a plugin folder into `com.projectnotespro.ifs-<version>.pnplugin` and checks the manifest. The same script can package any future separately shipped plugin.
- **Install.** Users run Install Plugin… with the `.pnplugin` file, or unzip it into `~/Project Notes/plugins/`. The README and `docs/StandardPlugins/IFSCloud.md` cover both, with Install Plugin… as the primary route on macOS.
- **Release that switches.** The release that ships the JS IFS plugin separately also adds the bundled IFS Python files (`ifscloud_plugin_settings`, `ifs_ssrs_generate_plugin`, `threads/ifssync_thread`, `includes/ifs_tools`) to `packaging/RemoveRetiredPlugins.cmake.in` and its Windows and macOS counterparts. Release notes tell existing IFS users to install the package. Their settings and credentials migrate on its first run like any other port.

## Security and trust

- Bundled plugins are trusted and enabled by default.
- A user plugin is disabled until the user enables it in Settings > Plugins, where a sheet lists its declared permissions, network hosts, COM ProgIDs, and whether it ships QML. An update that changes any of those re-prompts.
- Every API checks permissions on the plugin thread before marshaling. Undeclared access rejects the Promise with an error naming the missing permission.
- Engines get no `QObject` handles, no file, network, or COM access except through the APIs, and run under the watchdog.
- Secrets and settings are namespaced by plugin id.

## Mac App Store (desktop)

Removing Python addresses several real blockers: an interpreter and PyQt6 binaries that must each be signed inside the bundle, `.pyc` files written next to sources (which breaks the sealed bundle), `sys.path` entries pointing outside the sandbox container, `subprocess` launches, and PyQt6's GPL licensing. QML and `QJSEngine` code bundled inside the app is ordinary Qt app content.

Removing Python is necessary but not sufficient. The remaining work:

1. **App Sandbox and folders.** Project folders, File Finder search roots, and template destinations live outside the container. They need security-scoped bookmarks saved when the user picks a folder, and every path-based feature (project locations, File Finder, the `files` API) must go through them. This is the largest App Store task and is mostly independent of plugins.
2. **User plugins.** Handled by Install Plugin… and the container-relative profile folder described above. App Review guideline 2.5.2 bars apps from downloading code that adds features. The app never downloads plugins; users supply them, the way scripting apps accept user scripts. The review notes should say this plainly, and `PN_APP_STORE` should be able to turn user plugins off without a code fork if review objects.
3. **Other processes.** Thunderbird launching and `process.launch` are unavailable in the sandbox and absent from that build. The Mac Mail and Calendar backends use Apple Events and need the `com.apple.security.automation.apple-events` entitlement with a usage description, which App Review accepts when justified.
4. **Qt WebEngine.** WebEngine's only job is printing report HTML to PDF, and Qt has historically documented that WebEngine apps cannot go to the Mac App Store. Decided 2026-10-12: replace it with `QTextDocument` and `QPdfWriter` in every build and remove WebEngine entirely. See the [QTextDocument PDF renderer plan](TextDocumentPdfRendererPlan.md). That work is independent of the plugin phases and can run in parallel.
5. **Licensing.** The project is GPL-3.0. As the copyright holder you can distribute your own code through the App Store, but contributed GPL code and GPL dependencies need checking. PyQt6 leaves with Python; Qt itself is LGPL/commercial.

## Phases

Each phase leaves the app shippable. Python keeps working until Phase 6.

**Phase 0: spikes** (about 1 week)
- Worker-thread `QJSEngine` with `registerModule`, an async host call that resolves a Promise from another thread, and watchdog interruption.
- A plugin QML file inside `PluginDialogHost.qml` with zoom, round-tripping a result to the worker engine.
- On Windows: `ComDispatch` plus the JS `Proxy` opening Word, replacing a token, and saving, from a plugin thread.

**Phase 1: RecordTree refactor and published schema** (3 weeks)
- Extract `RecordTree`, importer, exporter, and the XML codec from `SqlQueryModel` and `DatabaseObjects`.
- Build the exporter with the five fixes under "Export performance": batched lookups, batched children, filtering before querying, requested columns only, and one connection per plugin (wired up in Phase 2 when the host exists). Add the query-count and timing benchmark.
- Switch XML import/export, `runPluginMenuForTable`, and the Python `get_data`/`update_data` callbacks to the new layer.
- Audit stored names and fix the `PublishedSchema` table.
- Round-trip and cascade tests under `tests/desktop/`. No user-visible change.

**Phase 2: plugin host core** (3 weeks)
- `ProjectNotesPlugins` library: manifest, registry, both plugin locations, per-plugin host thread with its own long-lived database connection, watchdog, `console`, timers, lifecycle, hot reload.
- APIs: `plugin`, `data`, `settings`, `secrets`, `storage`, `timers`, `log`, `app` (read-only parts), and `projectnotes/common`. Start `projectnotes.d.ts` and keep it in step with every API added after this phase.
- Merge JS and Python menus in `DesktopAppController`.
- Headless tests that load fixture plugins and drive the API.

**Phase 3: UI contributions and plugin management** (3 weeks)
- `PluginUiController`, `PluginDialogHost.qml`, `SchemaForm.qml`, the `ProjectNotes.PluginUI` module.
- Settings > Plugins: list, enable/disable, permission sheet, Install Plugin… with `.pnplugin` support, Open Plugins Folder. Plugin-contributed Settings sections.
- `ui` and `menus` APIs. Plugin author guide in `docs/PluginsOverview/`.

**Phase 4: integration and Windows APIs** (3 to 4 weeks)
- `email`, `calendar`, `contacts`, `office365`, `http`, `files`, `process`.
- `com` on `ComDispatch`, with `IMessageFilter` and per-thread apartment handling. Move the Outlook contacts helper and Outlook Classic meeting backend onto `ComDispatch`.

**Phase 5: port plugins** (5 to 7 weeks, smallest first so the API gets exercised early)
1. Clipboard actions from `base_plugin.py`.
2. My Shortcuts (dynamic menus, variable substitution, settings table).
3. Team Member Quick Add (QML dialog, transactional writes).
4. Editor settings and launcher; Outlook Integration settings.
5. Windows Office plugins on `com`: Close Stranded Excel/Word, Open MS Project, then the five template plugins, then Outlook Classic email download.
6. Outlook sync: tracker-to-To-Do and Graph email download.
7. IFS integration and SSRS capture under `extras/ifs/`, plus the `.pnplugin` packaging script.

For each port: record the Python behavior and settings first, port, compare outputs on the same records, then add the Python file to `packaging/RemoveRetiredPlugins.cmake.in` (and its Windows and macOS counterparts) in the same change that stops shipping it. Windows ports need testing on a machine with desktop Office, which the Linux and macOS CI builds cannot cover.

**Settings migration.** On first launch with the JS plugin, copy that plugin's values from `PluginSettings/<pluginname>/` (for example `New Change Order/ExportSubFolder`) and `Global Settings/` into `PluginSettings/<id>/`, translate stored `people`/`clients` names (My Shortcuts data types and URL variables), mark them migrated, and leave the originals in place for one release so downgrading still works.

**Phase 6: remove Python** (1 to 2 weeks)
- Delete `pluginmanager.*`, `plugin.*`, `pythonworker.*`, `plugins/projectnotes.py`, `plugins/includes/`, `plugins/forms/`, and `threads/`.
- Remove Python discovery and linking from both `CMakeLists.txt` files, the `python313.zip` and `site-packages` deployment steps, pywin32, `packaging/linux/python-deps.yaml`, `python-pyqt6.yaml`, and the Windows and macOS Python bundling. (`python-pyqt6-webengine.yaml` and `AA_ShareOpenGLContexts` leave earlier, with WebEngine.)
- Drop the Python half of `DesktopAppController`'s menu merge. Leftover `.py` files in a user's plugin folder are ignored, with a one-time notice listing them.
- Update `CLAUDE.md`, `AGENTS.md`, `docs/PluginsOverview/`, and `docs/StandardPlugins/`.
- Optional follow-up: once PyQt6 is gone, the only reason left for `QApplication` and `Qt::Widgets` is `QMessageBox` in `ProjectNotesCore` behind `m_gui`. Replacing that with an error callback lets the desktop app use `QGuiApplication` and drop `Qt::Widgets`. The COM bridge does not stand in the way because it avoids ActiveQt.

**Phase 7: Mac App Store build** (depends on the WebEngine removal in the renderer plan)
- `PN_APP_STORE` build flavor: sandbox entitlements, security-scoped bookmarks, capability gating (`process`, Thunderbird), App Store packaging script beside `packaging/macos/build_installer.sh`, review notes covering user plugins.

## Risks

- **Behavior drift in the RecordTree refactor.** XML import is the riskiest code to move. The round-trip tests in Phase 1 are the mitigation; do not start Phase 2 until they pass on real exported databases, not just fixtures.
- **Cross-thread Promises.** Resolving a Promise created in one `QJSEngine` from a reply produced on another thread must always hop back to the engine's own thread. Get this right in the Phase 0 spike before building APIs on it.
- **COM hangs and leaks.** Office automation can block or leave processes behind, which is why the cleanup plugins exist. Explicit release, apartment teardown on unload, and the message filter reduce this but cannot remove it.
- **Published schema freeze.** Renaming is cheap before `apiVersion: 1` ships and expensive after. Finish the audit before any plugin outside this repository depends on the names.
- **QML trust.** Plugin QML runs with full QML capabilities. Keep bundled plugins on schema forms where practical.
- **App Store scope creep.** Sandboxing may cost more than the plugin rewrite itself. Keep Phase 7 estimates separate so they do not hide inside plugin work, and track the WebEngine replacement in its own plan.

## Remaining questions

1. The user plugin folder is `~/Project Notes/plugins/` for every developer profile, as it is today. Should developer profiles (`--developer-profile`) get their own plugin folder so a test profile can run different plugins?
2. Keep `extras/ifs/` in this repository, or move it to its own repository later?
