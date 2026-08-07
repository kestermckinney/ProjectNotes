# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ProjectNotes is a Qt/C++ desktop application (v6.0.0) for project management — tracking notes, meeting minutes, risks, issues, action items, contacts, and clients. It embeds a Python plugin system for extensibility and integrates with tools like Outlook, IFS ERP, and MS Office.

## Build Commands

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

- Requires CMake 3.16+, Qt6 (Qt5 fallback supported), Python 3 dev libraries, Hunspell 1.7
- Qt modules used: Widgets, Core, SQL, XML, Network
- Also requires `SqliteSyncPro` checked out as a sibling directory (`../SqliteSyncPro/src`) — it's linked as `SqliteSyncProLib`
- No automated test framework exists in this project
- No linting configuration

## Architecture

### Application Structure

Single-instance Qt application enforced via `RunGuard` (UUID-based). Two frontends share a common backend:

1. **Qt Widgets (Legacy)** - Original desktop UI via `main.cpp` → `MainWindow` (`mainwindow.cpp`)
2. **Qt QML (Current Desktop)** - Modern QML-based desktop UI via `ProjectNotesDesktop/` with reactive components

**MainWindow** (`mainwindow.cpp`) manages (Widgets frontend):
- Page navigation with a history stack (max 20 nodes, forward/back)
- Text formatting toolbar
- Plugin menu injection

**Main.qml** (QML frontend) manages:
- Section-based navigation (People, Clients, Projects, Items, Search)
- Dynamic page component stack
- Filter/Sort dialog coordination
- Plugin menu integration with RecordContextMenu

### Page System (MVC/QML)

**Widgets pages:** All inherit from `BasePage`:
- `ProjectsListPage`, `ProjectDetailsPage`, `ProjectNotesPage`
- `ItemDetailsPage` (risks, issues, action items)
- `PeoplePage`, `ClientsPage`, `SearchPage`

**QML pages:** Modern reactive components in `ProjectNotesDesktop/qml/pages/`:
- `PeoplePage.qml`, `ClientsPage.qml` - Master lists with card-based UI
- `PersonDetailPage.qml`, `ClientDetailPage.qml` - Detail pages with inline editing
- `ProjectDetailPage.qml` - Complex detail page with multiple sub-tabs
- `SearchPage.qml` - Unified search results

### Data Layer

- `databaseobjects.h/cpp` — ORM-like wrappers for database interaction
- `databasestructure.cpp` — Full schema definition and incremental upgrade logic via `UpgradeDatabase()`; versioned upgrade logic is split into `databaseupgrade_v*.cpp` files
- `SqlQueryModel` → `SortFilterProxyModel` → `TableView` pipeline for all tabular data
- SQLite database at `database/ProjectNotes.db`; schema version tracked in `application_version` table

Key tables: `projects`, `project_notes`, `item_tracker`, `item_tracker_updates`, `people`, `clients`, `meeting_attendees`, `project_locations`, `project_people`, `application_settings`

### Python Plugin System

- `Plugin` / `PluginManager` — Qt wrappers around Python modules; `PluginManager` watches plugin files and hot-reloads on changes
- `PythonWorker` — Runs Python in a separate `QThread` with proper GIL management
- Plugins live in `plugins/`; background worker threads in `threads/`
- Plugins inject menu items via `pluginmenus`, can run on timers, and exchange data via XML

### QML Context Menus & Navigation

The QML desktop frontend uses `RecordContextMenu` for consistent right-click behavior across all detail pages:
- **RecordContextMenu** - Main menu component showing Open/New/Delete/Duplicate/MoveTo actions, navigation actions (Go To Person/Client), export/filter/refresh, and plugins
- **RecordRowMenu** - Extends RecordContextMenu for detail page sub-table rows (team, tracker items, etc.)
- **Navigation Actions** - "Go To Person" and "Go To Client" actions allow quick navigation between related records
  - People → Client: Navigate from a person's list/detail to their assigned client
  - Team Member → Person: Navigate from a project team list to a team member's full person record

### Custom Delegates

Cell editors in table views are implemented as delegates: `CheckboxDelegate`, `ComboboxDelegate`, `DateEditDelegate`, `LineEditFileButtonDelegate`, `PlainTextEditDelegate`, `TextEditDelegate`

### Other Key Utilities

- `AppSettings` — QSettings wrapper for persistent preferences
- `PlainTextEdit` + `InlineSpellChecker` — Custom text editor with Hunspell spell-checking
- `ImportExport` — XML-based data import/export
- `QLogger` — Structured logging; log viewer available via `LogViewer`

## Naming Conventions

- `*Model` for data models, `*Page` for page widgets, `*Dialog` for dialogs, `*Delegate` for cell editors

## Plugin Development

Plugins are Python files in `plugins/`. See `docs/PluginsOverview/` for the plugin API and `docs/PluginsOverview/ProjectNotesXML.md` for the XML data format used to exchange data between C++ and Python.
