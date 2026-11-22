# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ProjectNotes is a Qt/C++ desktop application (v6.5.2) for project management — tracking notes, meeting minutes, risks, issues, action items, contacts, and companies. It has a JavaScript (QJSEngine) plugin system for extensibility and integrates with tools like Outlook, IFS ERP, and MS Office. A companion iOS/iPadOS app, ProjectNotesMobile, shares the same data layer and syncs with the desktop app via SqliteSyncPro.

## Build Commands

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

- The default target is `ProjectNotes`, the **QML** desktop frontend in `ProjectNotesDesktop/`. A plain `cmake --build .` builds only that.
- The legacy Qt Widgets frontend has been **removed**. Its target (`ProjectNotesWidgets`), the `BUILD_WIDGETS_LEGACY` option, and all of its sources, `.ui` files, and icons are gone.
- Requires CMake 3.16+, Qt6 (Qt5 fallback supported), Hunspell 1.7
- Qt modules used: Core, SQL, XML, Network, Qml/Quick, Widgets
- **`Qt::Widgets` is still required.** No widget UI remains, but the app constructs a `QApplication` (not `QGuiApplication`) so `ProjectNotesCore` can report database errors through `QMessageBox`. Do not swap it for `QGuiApplication` or drop the module.
- Also requires `SqliteSyncPro` checked out as a sibling directory (`../SqliteSyncPro/src`) — it's linked as `SqliteSyncProLib`
- Headless Qt tests live in `tests/desktop/` (QtTest + ctest). Configure a separate build with `-DBUILD_QML_DESKTOP_TESTS=ON -DBUILD_QML_DESKTOP=OFF` and run `ctest` with `QT_QPA_PLATFORM=offscreen`.
- No linting configuration
- The macOS desktop app ships two ways: the notarized direct download (`packaging/macos/build_installer.sh`, published on GitHub) and the sandboxed Mac App Store flavor (`-DPN_APP_STORE=ON`; it copies the direct download's data on first start through `DirectDownloadMigration`), which `packaging/macos/build_appstore.sh` builds, signs, packages and uploads. Both scripts build the app themselves through `packaging/macos/build_common.sh`: universal (arm64 + x86_64), macOS 13+, with static Hunspell and OpenSSL from vcpkg. Don't package a Qt Creator build; Qt Creator leaves the deployment target empty, so the result only runs on the build machine's macOS. Both use the iOS app's bundle ID, `com.projectnotespro.projectnotesmobile` (`PN_MACOS_BUNDLE_ID`), so the Mac app shares the iOS app's App Store record; releases before 6.5.0 used `com.projectnotespro.ProjectNotes`. See `packaging/macos/AppStoreReleasing.md`.
- Pushing a `vX.Y.Z` tag runs `.github/workflows/release.yml` (Windows, macOS, iOS builds; GitHub release assets; App Store, Mac App Store and Microsoft Store submissions) and `flatpak-release.yml` (Linux). It calls the same packaging scripts as a local release. Secrets, setup and the pre-tag checklist are in `packaging/ci/RELEASING.md`.
- `ProjectNotesMobile/` (the iOS companion app) is **not** part of this build — it's a standalone CMake project cross-compiled for iOS via its own Xcode-generator configure, with a hardcoded vcpkg OpenSSL path for the maintainer's machine. It's built and archived separately; see `packaging/ios/build_appstore.sh`.

## Architecture

### Application Structure

Single-instance Qt application enforced via `RunGuard` (UUID-based). Two frontends share a common backend:

1. **Qt QML (Desktop)** - QML desktop UI via `ProjectNotesDesktop/` with reactive components; the default build target
2. **Qt QML (Mobile)** - iOS/iPadOS companion app via `ProjectNotesMobile/` with reactive components; built and shipped separately from the desktop targets

**Main.qml** (QML desktop frontend) manages:
- Section-based navigation (Contacts, Companies, Projects, Items, Search)
- Dynamic page component stack
- Filter/Sort dialog coordination
- Plugin menu integration with RecordContextMenu

**Main.qml** (QML mobile frontend, `ProjectNotesMobile/qml/`) manages:
- Bottom tab bar navigation (Projects, Contacts, Companies, Items) plus a hamburger-menu drawer for settings and utilities
- Its own page set under `ProjectNotesMobile/qml/pages/` — sized and interaction-patterned for touch, not a reuse of the desktop QML pages
- `AppController` (mobile) and `MobileSettings` fill the role `DesktopAppController` plays on desktop
- User-facing behavior is documented in `docs/Mobile/ProjectNotesMobile.md`

### Page System (QML)

**QML pages:** Reactive components in `ProjectNotesDesktop/qml/pages/`:
- `ContactsPage.qml`, `CompaniesPage.qml` - Master lists with card-based UI
- `ContactDetailPage.qml`, `CompanyDetailPage.qml` - Detail pages with inline editing
- `ProjectDetailPage.qml` - Complex detail page with multiple sub-tabs
- `SearchPage.qml` - Unified search results

### Data Layer

- `ProjectNotesCore/` builds the shared data layer (models, database, logging) as a static library, linked by both the QML desktop app and `ProjectNotesMobile`. Its `CMakeLists.txt` lists the source files, but they live one directory up (`databaseobjects.cpp`, `sqlquerymodel.cpp`, etc.) — it's a build wrapper, not a separate source tree. `Qt::Widgets` is a transitive dependency (see the comment at the top of `ProjectNotesCore/CMakeLists.txt`); all `QMessageBox` calls in core code are guarded by `if (m_gui)` so they never run in the mobile app.
- `databaseobjects.h/cpp` — ORM-like wrappers for database interaction
- `databasestructure.cpp` — Full schema definition and incremental upgrade logic via `UpgradeDatabase()`; versioned upgrade logic is split into `databaseupgrade_v*.cpp` files
- `SqlQueryModel` → `SortFilterProxyModel` → `TableView` pipeline for all tabular data
- SQLite database at `database/ProjectNotes.db`; schema version tracked in `application_version` table

Key tables: `projects`, `project_notes`, `item_tracker`, `item_tracker_updates`, `people`, `clients`, `meeting_attendees`, `project_locations`, `project_people`, `application_settings`

### JavaScript Plugin System

- `ProjectNotesPlugins/` is the plugin host library (no Widgets or Quick dependency). `PluginRegistry` discovers plugin folders in the bundled `plugins/` and the user's `plugins/` under the app data location (`QStandardPaths::AppDataLocation`), keeps enabled/approved state, and hot-reloads. Each plugin runs in its own `QJSEngine` on its own thread (`PluginHost`/`PluginRuntime`), with a watchdog.
- `js/prelude.js` builds the `projectnotes` and `projectnotes/common` modules over a private host bridge; `js/projectnotes.d.ts` documents them. QJSEngine is roughly ES2016, so plugins use generator functions (`function*` with `yield promise`) instead of async/await.
- Data access goes through `DataService` with published names (`contacts`, `companies`, `project_team`; see `PublishedSchema`) and the `RecordTree` import/export rules shared with XML import.
- Permissions are declared in `plugin.json`. User plugins need approval, and a change in what they ask for needs it again.
- The application side is `DesktopPluginServices` (dialogs, email, calendar, contacts, Office 365, secrets) and `PluginUiController` + `PluginUiHost.qml` (QML dialogs, schema forms, settings tabs).
- Bundled plugins live in `plugins/<name>/`; `extras/` holds plugins shipped separately as `.pnplugin` files (`packaging/plugins/build-pnplugin.sh`, or `build-pnplugin.ps1` on Windows). Embedded Python has been removed.

### QML Context Menus & Navigation

The QML desktop frontend uses `RecordContextMenu` for consistent right-click behavior across all detail pages:
- **RecordContextMenu** - Main menu component showing Open/New/Delete/Duplicate/MoveTo actions, navigation actions (Go To Contact/Company), export/filter/refresh, and plugins
- **RecordRowMenu** - Extends RecordContextMenu for detail page sub-table rows (team, tracker items, etc.)
- **Navigation Actions** - "Go To Contact" and "Go To Company" actions allow quick navigation between related records
  - Contact → Company: Navigate from a contact's list/detail to their assigned company
  - Team Member → Contact: Navigate from a project team list to a team member's full contact record

### Custom Delegates

Cell editors in table views are implemented as delegates: `CheckboxDelegate`, `ComboboxDelegate`, `DateEditDelegate`, `LineEditFileButtonDelegate`, `PlainTextEditDelegate`, `TextEditDelegate`

### Other Key Utilities

- `DesktopAppController` — QML bridge to the data layer; also owns the app's `QSettings` access ("ProjectNotes"[+profile] / "AppSettings")
- `PN::Identity` (`appidentity.h`) — the names everything is stored under: `Project Notes` (display name; data/cache folder on Windows, macOS, iOS), `projectnotes` (Linux folders, temp files), `ProjectNotes` (QSettings organization), plus the developer-profile helpers (`settingsOrganization(profile)`, `dataLocation(profile)`, `cacheLocation(profile)`) and the fixed credential namespace. Use these rather than writing the names out; they cannot change without moving existing users' data.
- `appResourcesPath()` (`appresourcespath.h`) — resolves the directory holding the bundled `plugins/` and `dictionary/`
- `SpellCheck` — Hunspell spell-checking exposed to QML
- `ImportExport` — XML-based data import/export
- `QLogger` — Structured logging; log viewer available via `LogViewerController` / `LogViewerWindow.qml`

## Naming Conventions

- `*Model` for data models, `*Page` for pages, `*Dialog` for dialogs, `*Controller` for QML bridge objects

## Plugin Development

A plugin is a folder with `plugin.json` and an ES module (`main.mjs`). See `docs/PluginsOverview/JavaScriptPlugins.md` for the API, permissions and manifest, and `docs/QJSEnginePluginsPlan.md` for the design. Plugins build UI with QML files that import `ProjectNotes.PluginUI` (a re-export of the desktop's controls) or with schema forms.

`tests/desktop/tst_pluginhost.cpp` runs every bundled plugin, and `extras/ifs-cloud`, against fake services and fake HTTP servers; add to it when you change a plugin. Plugins that replace Python ones copy the old settings once through the manifest's `legacySettings`.

## Documentation Writing

- Avoid using em-dashes
- Avoid clichés such as "delve," "tapestry," "leverage," "seamlessly," and "in today's fast-paced world." Replace these with direct, simple verbs and nouns.
- Vary Sentence Structure: Mix short, punchy sentences with longer, flowing ones to increase "burstiness." Avoid uniform sentence lengths and predictable three-item lists.
- Avoid Meta-Commentary: phrases like "In this section," "It is important to note," and "At the end of the day." Start directly with the point and avoid summarizing what you are about to say.

