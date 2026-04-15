# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ProjectNotes is a Qt/C++ desktop application (v5.0.0) for project management — tracking notes, meeting minutes, risks, issues, action items, contacts, and clients. It embeds a Python plugin system for extensibility and integrates with tools like Outlook, IFS ERP, and MS Office.

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

Single-instance MDI Qt application enforced via `RunGuard` (UUID-based). Entry point is `main.cpp`, which sets Qt library paths for PyQt6 and launches `MainWindow`.

**MainWindow** (`mainwindow.cpp`) manages:
- Page navigation with a history stack (max 20 nodes, forward/back)
- Text formatting toolbar
- Plugin menu injection

### Page System (MVC)

All pages inherit from `BasePage`. Key pages:
- `ProjectsListPage`, `ProjectDetailsPage`, `ProjectNotesPage`
- `ItemDetailsPage` (risks, issues, action items)
- `PeoplePage`, `ClientsPage`, `SearchPage`

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
