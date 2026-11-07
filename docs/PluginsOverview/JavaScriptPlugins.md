# JavaScript Plugins

JavaScript plugins add menus, dialogs, settings and automation to Project Notes. They run on Qt's JavaScript engine (`QJSEngine`) inside the application, so nothing extra needs to be installed, and they work the same way on Windows, macOS and Linux. They replace the older Python plugins, which will be removed in a later release.

Each plugin runs on its own thread with its own engine. A plugin that stalls cannot freeze the window, and a long synchronous loop is stopped after ten seconds.

## A first plugin

A plugin is a folder with a `plugin.json` manifest and a JavaScript module:

```
hello-team/
  plugin.json
  main.mjs
```

```json
{
  "id": "com.example.hello-team",
  "name": "Hello Team",
  "version": "1.0.0",
  "apiVersion": 1,
  "description": "Shows the size of a project's team.",
  "main": "main.mjs",
  "permissions": ["data.read:projects,project_team"],
  "contributes": {
    "menus": [
      { "id": "count", "title": "Count Team Members", "context": "record",
        "entity": "projects", "include": ["project_team"] }
    ]
  }
}
```

```js
// main.mjs
import { ui } from "projectnotes";

export function* onMenu(menuId, context) {
    const team = context.record.project_team;
    yield ui.message(context.record.project_name + " has " + team.length + " team members.");
}
```

Copy the folder into your plugins folder (see [Installing plugins](#installing-plugins)), then turn it on in **Settings > Plugins**. Right-click a project and choose **Plugins > Count Team Members**.

## Writing asynchronous code

Qt's JavaScript engine supports modern JavaScript up to about ECMAScript 2016, plus optional chaining (`a?.b`) and nullish coalescing (`a ?? b`). It does not support `async` and `await`.

Write asynchronous steps as a generator function (`function*`) and `yield` each promise where you would otherwise `await` it. Project Notes runs the generator for you: each `yield` waits for the promise and resumes with its value, or throws its error into the generator so `try`/`catch` works as usual.

```js
export function* onMenu(menuId, context) {
    try {
        const companies = yield data.list("companies", { orderBy: ["company_name"] });
        const choice = yield ui.choose("Company", companies.map(c => c.company_name));
        if (choice) yield ui.message("You chose " + choice);
    } catch (e) {
        yield ui.message(e.message, { kind: "error" });
    }
}
```

This works for `onStart`, `onStop`, `onMenu`, `onSettingsChanged`, menu `handler` functions, dialog handlers, `data.onChanged` handlers, timer jobs, and the function passed to `data.transaction()`. Yield an array of promises to wait for all of them. To get a promise-returning function from a generator, wrap it with `task()` from `projectnotes/common`.

Plain promise chains (`.then()`) work everywhere too. `Promise.prototype.finally`, `Object.fromEntries` and `Array.prototype.flat` are available even though the engine lacks them natively.

## The manifest

| Field | Purpose |
| --- | --- |
| `id` | Unique id, letters, digits, dots, dashes and underscores. Reverse-DNS style is recommended. |
| `name`, `version`, `description`, `author`, `homepage` | Shown in Settings > Plugins. |
| `apiVersion` | Must be `1`. |
| `main` | The entry module inside the folder. |
| `platforms` | Optional list of `windows`, `macos`, `linux`. A plugin is listed but not started on other platforms. |
| `permissions` | What the plugin may do. See [Permissions](#permissions). |
| `http` | Hosts `http.fetch()` may reach, for example `["*.ifs.cloud"]`. |
| `com` | Windows only: COM ProgIDs and monikers the plugin may use, for example `["Word.Application", "winmgmts:"]`. |
| `office365.scopes` | Microsoft Graph scopes the plugin needs. |
| `contributes.menus` | Menus: `id`, `title`, `context` (`record` or `global`), `entity` for record menus, optional `submenu`, `include`, `parameter`, `platforms`. |
| `contributes.settings` | Settings sections: `id`, `title`, and either `schema` (a form schema file) or `qml` (a QML file). |

## Entry points

Export any of these from the main module:

| Export | Called |
| --- | --- |
| `onStart()` | After the plugin loads, and after every hot reload. |
| `onStop()` | Before it unloads, reloads, or the application exits. Project Notes waits up to three seconds. |
| `onMenu(menuId, context)` | When one of its menus is chosen. `context.record` holds the clicked record, loaded with the menu's `include`. |
| `onSettingsChanged(keys)` | When the user changes the plugin's settings in Settings. |

## Working with data

Plugins read and write records with the `data` namespace. Records use the names you see in the application: **contacts** and **companies**, not the stored names. A foreign key is an object with the referenced record's key and display value:

```js
const project = yield data.get("projects", { project_number: "P4000" },
                               { include: ["project_team", "project_locations"] });
project.company_id;               // { id: "...", lookup: "Acme Corp" }
project.project_team[0].contact_id.lookup;   // "Jim Smith"
```

Writes follow the same rules as XML import. A save updates the record whose key or business key (for example a contact's name) matches, and inserts otherwise. Give a foreign key either its `id` or its `lookup`:

```js
yield data.save("project_locations", {
    project_id: { lookup: "P4000" },
    location_type: "Word Document",
    location_description: "Change Request : P4000 PCR01.docx",
    full_path: path
});
```

Group several writes with `data.transaction()`. If the function throws, nothing is saved:

```js
yield data.transaction(function* (tx) {
    const company = yield tx.save("companies", { company_name: "New Co" });
    yield tx.save("contacts", { name: "Pat Lee", company_id: { id: company.record.$key } });
});
```

Other operations: `list()` with `where`, `include`, `fields`, `orderBy`, `limit` and `offset`; `count()`; `remove()`, which also removes owned child records and refuses a record other records still use; `search()`; `schema()` and `entities()`; `onChanged()`; and `exportXml()`/`importXml()` for the [Project Notes XML](ProjectNotesXML.md) format.

A `where` matches values exactly, a list of values, or an operator object: `$ne`, `$lt`, `$lte`, `$gt`, `$gte`, `$like`, `$null`, `$between`. Dates are `YYYY-MM-DD` strings.

```js
const open = yield data.list("item_tracker", {
    where: { project_id: project.$key, status: ["New", "Assigned"], date_due: { $lte: "2026-12-31" } },
    orderBy: [{ column: "priority" }, { column: "date_due", descending: true }]
});
```

### Entity names

| Entity | Contents |
| --- | --- |
| `companies` | Companies |
| `contacts` | Contacts |
| `projects` | Projects |
| `project_team` | A project's team members |
| `project_locations` | Files and folders on a project |
| `project_notes` | Meeting notes |
| `meeting_attendees` | A meeting's attendees |
| `item_tracker` | Tracker and action items |
| `item_tracker_updates` | Tracker item comments |
| `status_report_items` | Status report items |
| `project_email_audiences` | Saved email audiences |

Columns that refer to contacts or companies use the same vocabulary: `contact_id`, `company_id`, `company_name`. Call `data.schema(entity)` for the full column list with types, required columns and allowed values.

A project's `folder_id` is the sidebar folder it sits in, as a reference like any other foreign key: `{ id: "...", lookup: "Favorites" }`, or `null` when the project is in no folder. Move a project by saving `folder_id: { lookup: "Favorites" }`, and take it out of its folder with `folder_id: null`. A project is in one folder at a time. Folders themselves are created and renamed in the application.

```js
const favorites = yield data.list("projects", { where: { folder_id: { lookup: "Favorites" } } });
yield data.save("projects", { project_number: "P4000", folder_id: { lookup: "Favorites" } });
```

A project's primary project folder is `primary_location_id`, the key of one of its `project_locations` rows, or `null` when the project has none. The row is either a `File Folder` on the user's computer or a Microsoft Teams channel folder, a `Web Link` to a SharePoint URL. `primary_location_manual` is `true` once the user has set or cleared it, and File Finder only fills in a folder while it is `false`. Projects are not given a folder from a location's description any more.

### Saving into the project folder

Use `projectFiles` to read and write the project folder. It works the same whether the folder is local or in Microsoft Teams; for a Teams folder the application talks to Microsoft 365 with the account signed in under **Settings > Office 365 Integration**, so the plugin needs no `office365` permission. Paths are relative to the project folder and may not leave it. It needs the `files` permission.

```js
const folder = yield projectFiles.location(project.$key);   // { kind: "local" | "teams", path } or null
if (folder) {
    yield projectFiles.mkdir(project.$key, "Project Management/Status Reports");
    const location = yield projectFiles.upload(project.$key, files.tempPath("report.pdf"),
                                               "Project Management/Status Reports/report.pdf", { overwrite: true });
    yield data.save("project_locations", { project_id: { id: project.$key }, location_type: "PDF File",
                                           location_description: "report.pdf", full_path: location });
    yield projectFiles.open(project.$key, "Project Management/Status Reports/report.pdf");
}
```

`list()`, `exists()`, `readText()` and `writeText()` round it out. Methods that write return the location to store in `project_locations`: a local path, or the Teams URL, which for Word, Excel, PowerPoint and Project files is the direct document URL that opens in the desktop app. `upload()` fails with code `exists` unless `overwrite` is `true`; `writeText()` replaces by default. A Teams folder can also fail with `teams-access-denied` when the sign-in lacks `Files.ReadWrite.All`, or with `teams-read-only` when **Save files into Microsoft Teams folders** is off. `location()` reports `writable: false` for such a folder; check it before writing and offer a local folder instead. When the project also has a local folder, `projectFiles` uses that one and `location()` returns it.

`app.projectFolder(projectKey)` returns the path only for a local folder, and `null` for a Teams folder or a project without one. A local folder from either call is also open to `files.*`. Office automation and other tools that need a real file write to `files.tempPath()` first and then `upload()` the result.

## User interface

### Messages, prompts and forms

`ui.message()`, `ui.confirm()`, `ui.prompt()` and `ui.choose()` show standard dialogs. `ui.showForm()` builds a dialog from a form schema and resolves with the entered values, or `null` if the user cancels:

```js
const values = yield ui.showForm({ fields: [
    { key: "number", label: "Change order number", type: "number", required: true },
    { key: "folder", label: "Save in", type: "folder" },
    { key: "notify", label: "Email the team", type: "checkbox" }
] }, { number: 1 }, { title: "New Change Order" });
```

Field types: `text`, `multiline`, `password`, `number`, `date`, `checkbox`, `choice`, `combo`, `file`, `folder`, `contact`, `table` (rows with `columns`), `section` (a heading) and `note` (help text). Fields accept `required`, `placeholder`, `help`, `choices`, `filters` and `visibleWhen`. A table column is a text cell unless its type is `choice` (a drop-down) or `file` (a text cell with a browse button that uses the column's `filters`).

Use `ui.pickFile()`, `ui.pickFolder()` and `ui.saveFile()` for file choosers, `ui.progress()` for long work, `ui.notify()` for a brief notice, and `ui.clipboard` for the clipboard.

### QML dialogs

For a richer dialog, ship a QML file and open it with `ui.showDialog()`:

```js
const result = yield ui.showDialog("QuickAdd.qml", { projectKey: context.key }, {
    title: "Quick Add", width: 520, height: 420,
    handlers: { companies: function () { return data.list("companies"); } }
});
```

```qml
// QuickAdd.qml
import QtQuick
import QtQuick.Layouts
import ProjectNotes.PluginUI

ColumnLayout {
    FormField { id: name; Layout.fillWidth: true; label: qsTr("Name") }
    Button {
        text: qsTr("Add")
        onClicked: pluginDialog.accept({ name: name.text })
    }
    Component.onCompleted: pluginDialog.invoke("companies").then(function (list) { /* ... */ })
}
```

`ProjectNotes.PluginUI` gives the dialog the application's own controls and `Theme`: `FormField`, `ComboField`, `DateField`, `StyledComboBox`, `SpellCheckField`, `Button`, `Card`, `MaterialIcon`, `ContactPickerDialog` and `SchemaForm`. The `pluginDialog` object provides `data`, `title`, `accept(result)`, `reject()` and `invoke(name, args)`, which calls a handler or an exported function in the plugin and returns a promise.

Plugin QML runs in the application with full QML capabilities, so a plugin that ships QML is trusted as much as the application itself. Settings > Plugins says so when you review such a plugin. Plugins that only use forms stay within their declared permissions.

### Menus

Declare menus in the manifest when they are always present. Register them at run time when they depend on settings or the platform:

```js
menus.register({ id: "open-site", title: "Open Project Site", context: "record", entity: "projects",
                 handler: function* (context) { yield ui.openUrl(siteFor(context.record)); } });
```

### Settings sections

A `contributes.settings` entry adds a section to Settings. With a `schema`, Project Notes renders the form and saves each value as it changes; the plugin reads it with `settings.get()` and hears about changes in `onSettingsChanged()`. A `password` field is the exception: its value goes to the operating system's credential store, never to the settings, so read it with `secrets.get(key)`. Leaving a password field blank keeps the stored value. With `qml`, the section is the plugin's own QML file, which gets a `pluginSettings` object with `values`, `get(key)`, `set(key, value)` and `invoke(name, args)`.

## Integrations

| Namespace | What it does |
| --- | --- |
| `email` | `compose()` creates a draft in the user's email application straight away, with no review dialog. The user reviews and sends it there, so plugins cannot send mail without the user. `chooseRecipients(projectKey, { workflow, title, okText })` opens the recipient panel the built-in reports use, with To/Cc/Bcc, saved audiences and **Set as default** for `workflow` (`status-report`, `tracker-items-report`, `meeting-notes-report` or `project-email`). It resolves to `{ to, cc, bcc, audienceId }`, ready to spread into `compose()`, or `null` on Cancel. Audiences belong to the project: `audiences(projectKey, workflow)` lists all of them, with `isDefault` for that workflow, and `resolveAudience()` gives a saved audience's recipients. Also `templates()` and `renderTemplate()`. |
| `calendar` | `draftMeeting()` shows a review and creates a draft meeting; `writeIcs()` saves an `.ics` file. |
| `contacts` | `pick()` opens the contact picker; `syncNow()`, `sources()`, `toVCard()`, `fromVCard()`. |
| `office365` | `graph()` calls Microsoft Graph with the user's Office 365 sign-in; `graphPages()` follows paging links. |
| `http` | `fetch()` to the hosts listed in the manifest, with basic or bearer authentication. |
| `auth` | `browserSignIn()` opens a sign-in page in the browser and returns the redirect to a `localhost` address, for OAuth sign-ins to services other than Office 365. |
| `files` | Files inside the plugin folder (read only), its data folder, folders the user picked, and local project folders from `app.projectFolder()` or `projectFiles.location()`. |
| `projectFiles` | The project's primary project folder, local or Microsoft Teams (see [Saving into the project folder](#saving-into-the-project-folder)). Needs `files`. |
| `storage`, `settings`, `secrets` | Private files, settings, and credentials kept in the operating system's credential store. |
| `shortcuts` | `list()` reads the My Shortcuts table. `add(rows)` appends rows of `{ menu, submenu, entity, url }` and returns how many it added. A row is skipped when My Shortcuts already has one with the same entity, submenu and menu, ignoring case. Write the URL out in full: My Shortcuts fills in only `{{ entity.column }}` and `{{ preferences.* }}` tokens. Added rows carry `addedBy` with the plugin's id. |
| `com` | Windows only: automate Office and other COM applications (see below). |
| `process` | Start another program (not in App Store builds). |

Check `app.capabilities.has(name)` before using an integration that a build might not offer, for example `com` or `process.launch`.

### Windows COM automation

On Windows, `com.createObject()`, `com.getActiveObject()` and `com.getObject()` return objects whose properties and methods you use directly:

```js
const word = com.createObject("Word.Application");
try {
    const doc = word.Documents.Open(path);
    doc.Content.Find.Execute("<PROJECTNUMBER>", false, false, false, false, false,
                             true, 1, false, project.project_number, 2);
    doc.Save();
    word.Visible = true;
} finally {
    com.release(word);
}
```

List every ProgID and moniker in the manifest's `com` array. COM calls are synchronous. Objects are released when the plugin stops, but releasing them yourself frees the application sooner.

Reading a member asks the object for a property and falls back to calling it as a method. A few objects, WMI's among them, run a method when it is read. Use `object.$call("Terminate")` for those, and `$get`/`$set` for properties that take arguments.

`com` is only present where the build supports it, so read it as `pn.com` from `import * as pn from "projectnotes"` rather than importing it by name. A named import of a missing export stops the module from loading.

### Browser sign-in

Services that sign in with OAuth in a browser, such as Keycloak, use `auth.browserSignIn()`. Project Notes opens the page, listens on the loopback address in `redirectUri` until the browser comes back, and returns that URL. The plugin then exchanges the code for a token with `http.fetch()`:

```js
const { url } = yield auth.browserSignIn({
    url: authorizeUrl + "?response_type=code&client_id=app&redirect_uri=" + encodeURIComponent(redirect) + "&state=" + state,
    redirectUri: redirect,            // "http://localhost:8080/callback"
    timeoutSeconds: 120,
    serviceName: "Keycloak",          // optional; named on the "Login Successful" page
});
const code = /[?&]code=([^&]+)/.exec(url)[1];
```

The page's host must be in the manifest's `http` array, and `redirectUri` must be `http://localhost:<port>` or `http://127.0.0.1:<port>`. Check the `state` you sent against the one that comes back.

## Permissions

| Permission | Allows |
| --- | --- |
| `data.read:<entities>` | Reading the listed entities (`*` for all). |
| `data.write:<entities>` | Reading and changing them. |
| `ui.dialogs` | `ui.showDialog()` and `ui.showForm()`. Messages, prompts, pickers and notices need no permission. |
| `clipboard` | `ui.clipboard`. |
| `files` | `files.*` within the permitted locations. |
| `email.compose` | The `email` namespace. |
| `calendar.draft` | The `calendar` namespace. |
| `contacts.sync`, `contacts.pick` | Contact synchronization; the picker and vCard conversion. |
| `office365` | `office365.*` within the manifest's scopes. |
| `http` | `http.fetch()` to the manifest's hosts. |
| `com` | `com.*` for the manifest's ProgIDs and monikers. |
| `process.launch` | `process.launch()`. |
| `app.navigate` | `app.navigate()`. |
| `urls` | `ui.openUrl()` with schemes other than http, https and mailto. |
| `auth.browser` | `auth.browserSignIn()`; the sign-in page's host must also be in `http`. |
| `shortcuts` | `shortcuts.list()` and `shortcuts.add()`. |
| `reports` | `reports.generate()` (planned). |

A call without its permission fails with a `PluginError` whose `code` is `permission-denied` and whose `permission` names what to add to the manifest.

## Installing plugins

Plugins included with Project Notes are on by default. Plugins you add go in your plugins folder:

- **Windows:** `%APPDATA%\Project Notes\plugins`
- **macOS (direct download):** `~/Library/Application Support/Project Notes/plugins`
- **Linux:** `~/.local/share/projectnotes/plugins`, or the equivalent under `$XDG_DATA_HOME`
- **Mac App Store and Flatpak:** the app runs in a sandbox, so the folder is inside the app's own data area rather than at the paths above.

Settings > Plugins shows the exact folder. Use **Open Plugins Folder** to go there. You can also use **Install Plugin…**, which copies a plugin folder or `.pnplugin` file into place for you.

Plugins in the older `Project Notes/plugins` folder in your home folder are copied into the new folder the first time you start this version. The old folder is left as it was.

The Mac App Store version has no `process` namespace and no Script Editor; `app.buildFlavor` is `"appstore"` there. A plugin reaches the files and folders you pick for the rest of that session.

Settings > Plugins can install a plugin folder or a `.pnplugin` file, which is a zip of the folder. An installed plugin stays off until you review what it can do and turn it on, and it asks again if an update changes its permissions.

## Developing and debugging

- **Hot reload.** Saving any `.mjs`, `.js`, `.json` or `.qml` file in a plugin folder reloads that plugin.
- **Logging.** `console.log()` and `log.info()` write to the application log, tagged with the plugin id. Open it from the application menu's log viewer. Errors also appear as a notice and in Settings > Plugins.
- **Type checking.** `projectnotes.d.ts` (in `ProjectNotesPlugins/js` in the source tree) declares the whole API. Add it to a `jsconfig.json` next to your plugin for completion and type checking in editors such as Visual Studio Code.
- **Long work.** Keep synchronous loops short. Waiting on promises does not count toward the ten-second limit; a loop that never yields does.

Archive Mail calls `projectFiles.location(projectKey, { preferLocal: true })` to prefer an existing local project folder (including a synced SharePoint folder). It writes mail and `download_state.json` directly into that local folder; if none is available, it uses the usual project folder destination. Other callers keep the default destination selection.
