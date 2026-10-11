# Standard Plugins and Plugin Settings

## Built-In Standard Plugins

Project Notes ships with a small set of Python plugins for document templates and enterprise connections. Contact synchronization is a native application service configured under [Preferences](../InterfaceOverview/Preferences.md#contact-synchronization).

Email, meeting invitations and the four project reports are **not** plugins. They are built into Project Notes and are configured on the main **Settings** page. See [Email and Meeting Setup](<../InterfaceOverview/EmailAndMeetingSetup.md>), [Reports and Email](<../InterfaceOverview/ReportsAndEmail.md>) and [Scheduling Meetings](<../InterfaceOverview/ScheduleMeeting.md>).

File Finder and the native Office 365 connection are application services rather than Python plugins, so they also have their own sections on the main **Settings** page. See [File Finder](<../InterfaceOverview/FileFinder.md>) and [Office 365 Integration](<../InterfaceOverview/Office365Integration.md>).

**Reaching Plugin Settings:** open the app menu (the menu icon at the top of the icon rail); plugin settings are grouped as **Plugins · Settings**.

### Standard Plugins Overview

**Contacts and enterprise connections:**
- Outlook Integration — Download project-related email and sync assigned items. See [Outlook Integration](<OutlookIntegration.md>).
- IFS Cloud — Connect to a customized IFS Cloud ERP instance

**Documents and team:**
- New Document Templates — Create documents from templates for MS Project, PowerPoint, change orders and registers
- Team Member Quick Add — Quickly add team members to projects

**Utility:**
- Editor — Configure the external editor used to edit plugin scripts
- My Shortcuts — Define custom menu shortcuts

Two features that appear alongside the plugins are application services rather than Python code, with their own sections on the main **Settings** page: [File Finder](<../InterfaceOverview/FileFinder.md>) and [Office 365 Integration](<../InterfaceOverview/Office365Integration.md>).

### Important Note: IFS-Related Plugins

**The IFS Cloud plugin is tailored to a customized version of IFS ERP and may not work with all IFS installations.**

The IFS Cloud plugin was developed for a specific IFS implementation with custom modifications. If you use IFS Cloud ERP in your organization:

1. **Verify compatibility** — Before enabling the IFS Cloud plugin, confirm that your IFS instance has the required customizations
2. **Contact your IFS administrator** — They can verify whether your installation matches the expected schema and customizations
3. **Test before relying on it** — Enable the plugin in a test environment first to confirm it works with your ERP system
4. **Alternative integration** — If your IFS instance does not match the expected customization, you may need to develop a custom plugin tailored to your specific ERP schema

The IFS Cloud settings and documentation are available at [IFS Cloud Integration](<IFSCloud.md>), which includes more details about compatibility and setup.

## Managing and Storing Settings

Settings in Project Notes are stored in two places depending on what they control.

### Settings Stored in the Local OS Profile

Plugin settings, UI state, and connection credentials are stored in your **operating system's user profile** using Qt's QSettings mechanism. These settings are local to the machine and user account — they do not sync with other users or machines.

**What is stored locally:**

| Category | Examples |
| :--- | :--- |
| Local application and plugin configuration | Document template sub-folders, IFS credentials, Outlook credentials, Office 365 connection details, File Finder search locations/exclusions/classifications, My Shortcuts definitions, email templates |
| Cloud sync connection | Sync enabled flag, Project Notes Pro account email, password, and encryption phrase |
| Window position and size | Position and size of the main window, restored on the next launch |
| UI Zoom level | The zoom percentage set via **Ctrl +/-/0** or the [Application Menu](<../InterfaceOverview/ApplicationMenu.md>)'s **View** group zoom row — see [UI Zoom](<../InterfaceOverview/VectorZoom.md>) |
| Spell check dictionary | Selected dictionary and personal word list |

**Storage location by platform:**

| Platform | Location |
| :--- | :--- |
| **Windows** | Registry under `HKEY_CURRENT_USER\Software\ProjectNotes\ProjectNotes` |
| **macOS** | `~/Library/Preferences/com.projectnotes.projectnotes.plist` |
| **Linux** | `~/.config/ProjectNotes/ProjectNotes.ini` |

When using the `--developer-profile` option, the profile name is appended to the organization key so each profile has its own isolated settings.

### Settings Stored in the Database

Application preferences and view state that should be consistent for anyone using the same database are stored in the `application_settings` table inside `ProjectNotes.db`. Because they live in the database, they are included in cloud sync and move with the database when it is copied or shared.

**What is stored in the database:**

| Setting | Key in database |
| :--- | :--- |
| Project Manager (Preferences) | `Preferences:ProjectManager` |
| Managing Company (Preferences) | `Preferences:ManagingCompany` |
| Show Resolved Tracker/Action Items (View menu) | `ViewFilter:ShowResolvedTrackerItems` |
| Show Closed Projects (View menu) | `UserFilter:ShowClosedProjects` |
| Show Internal Items (View menu) | `UserFilter:ShowInternalItems` |
| Active project filter (Filter Tool) | `UserFilter:ProjectFilter` |
| Meeting types (Settings > Meeting Templates) | `Meeting/v1/Templates` |

Saved report and meeting audiences also live in the database, in the `project_email_audiences` table, so they travel with the project.

These settings persist across application restarts, so you only need to configure them once per database.

The **Base Plugins Settings** script (`base_plugin_settings.py`) contains the user interface code for managing settings of the standard plugins included with Project Notes. Qt Designer forms for these settings are located in the `plugins/forms/` folder.

Plugins conventionally add menu items to the **Plugins** menu under a **Settings** sub-menu to provide access to their configuration.

## Variable Replacement in Plugins

Several standard plugins support **variable replacement** — the ability to automatically substitute placeholder values in text, URLs, or file names with actual project data at runtime. This allows you to create flexible, reusable configurations that adapt to different projects without manual editing.

### How Variable Replacement Works

Variable replacement bridges Project Notes data with plugin configurations:

1. **Configuration time** — You define a template with placeholder variables (e.g., a URL with `[$projects.project_number.1]`)
2. **Runtime** — When the plugin runs, it extracts the relevant data as XML from the current project context
3. **Substitution** — The plugin replaces placeholders with actual values from the XML
4. **Execution** — The plugin uses the completed, project-specific value

### Relationship to XML

Project Notes internally represents all data as XML. When you invoke a plugin, the application exports the relevant data (project, contact, meeting, etc.) as XML and passes it to the plugin. The plugin uses this XML to:

- Extract field values for variable substitution
- Determine which variables are available in the current context
- Replace placeholders with the extracted values

For example, when you right-click a project and invoke a plugin, the project's complete data (including number, name, dates, team members) is exported as XML. The plugin can access any field from that XML using the field's column name as a variable.

### Variable Syntax

Variables use the syntax `[$tablename.columnname.rownumber]` where:

- `[` and `]` — Square brackets containing the variable path
- `$` — Dollar sign prefix denoting a variable
- `tablename` — The XML table/element name
- `columnname` — The column/field name within that table
- `rownumber` — The row index (starting at 1 for the first/current record)

The variable name must follow the XML tree path from the exported data. For example:

- `[$projects.project_number.1]` — The first project's project number
- `[$projects.project_name.1]` — The first project's name
- `[$people.email.1]` — The first contact's email address
- `[$people.name.1]` — The first contact's name

### Understanding XML Tree Paths

When you invoke a plugin by right-clicking a record, Project Notes exports that record's data as XML. The XML has a hierarchical structure with parent and child elements. Variables reference specific fields by following this XML tree.

**Example XML structure for a project:**

```xml
<projects>
  <row>
    <column name="project_number">P-001</column>
    <column name="project_name">Website Redesign</column>
    <column name="project_status">Active</column>
    <column name="primary_contact">John Smith</column>
  </row>
</projects>
```

To access these values, you would use:
- `[$projects.project_number.1]` → "P-001"
- `[$projects.project_name.1]` → "Website Redesign"
- `[$projects.project_status.1]` → "Active"

### Available Variables by Context

The available variables depend on the XML structure of the record you right-clicked. For a complete list of available fields and their XML paths for each [data type](<../PluginsOverview/DataTypes.md>), see the [Project Notes XML](<../PluginsOverview/ProjectNotesXML.md>) documentation.

Common patterns include:

**When right-clicking a Project:**
- `[$projects.project_number.1]` — Project number
- `[$projects.project_name.1]` — Project name
- `[$projects.project_status.1]` — Project status

**When right-clicking a Contact:**
- `[$people.name.1]` — Contact's name
- `[$people.email.1]` — Contact's email address
- `[$people.office_phone.1]` — Office phone number

**When right-clicking a Meeting/Note:**
- `[$project_notes.note_title.1]` — Meeting note title
- Related project data accessible via the XML tree path

### Examples of Variable Replacement

#### Example 1: My Shortcuts with Project Variables

A shortcut URL configured with variables:

```
https://our-company.com/projects/[$projects.project_number.1]
```

When you click this shortcut while viewing Project P-001:
- The XML exports the project data including project_number as "P-001"
- The placeholder `[$projects.project_number.1]` is replaced with "P-001"
- The final URL opened is: `https://our-company.com/projects/P-001`

Another example with multiple variables:

```
https://outlook.office365.com/mail/?folderid=Inbox&searchText=[$projects.project_number.1] [$projects.project_name.1]
```

When clicked for Project "P-002 Website Redesign":
- Final URL: `https://outlook.office365.com/mail/?folderid=Inbox&searchText=P-002 Website Redesign`
- This searches Outlook for emails mentioning the project

#### Example 2: Template Documents with Placeholders

When you create a new document from a template, the plugin searches the document for placeholder tags and replaces them with project values.

Template placeholders are typically formatted as:

```
<PROJECTNUMBER>
<PROJECTNAME>
<DATE>
```

A Word document might contain:

```
Project: <PROJECTNUMBER> - <PROJECTNAME>
Created: <DATE>
```

After running the "New Change Order" plugin for Project P-003:

```
Project: P-003 - ERP Implementation
Created: 03/23/2026
```

### Common Settings Examples

#### Outlook Integration Settings

| Setting | Example Value | Purpose |
| :--- | :--- | :--- |
| **Integration Type** | Office 365 Application | Work through Microsoft 365. **Outlook Automation** drives Outlook (classic) instead, on Windows only |
| **Application ID** | a1b2c3d4-e5f6-47a8-b9c0-d1e2f3a4b5c6 | Microsoft Entra app ID used for the plugin’s email archive and task sync |
| **Tenant ID** | f1e2d3c4-b5a6-4978-c9d0-e1f2a3b4c5d6 | Microsoft Entra tenant for your organization |

File Finder's settings (search locations, folder exclusions, classification rules) are on the main **Settings > File Finder** page, not in a plugin dialog, so they aren't repeated here. See [File Finder](<../InterfaceOverview/FileFinder.md>) for the full settings reference.

These settings are on the main **Settings > File Finder** page, not in a plugin dialog. See [File Finder](<../InterfaceOverview/FileFinder.md>) for all controls.

#### New Document Templates Settings

| Setting | Example Value | Purpose |
| :--- | :--- | :--- |
| **New Change Order** | Change Orders | Save to `[Project Folder]/Change Orders/` |
| **New MS Project** | Planning | Save to `[Project Folder]/Planning/` |
| **New Risk Register** | Risk Management | Save to `[Project Folder]/Risk Management/` |

#### My Shortcuts Examples

| Menu | Submenu | [Data Type](<../PluginsOverview/DataTypes.md>) | URL |
| :--- | :--- | :--- | :--- |
| Open in Jira | Project Tools | projects | https://jira.company.com/projects/[$projects.project_number.1] |
| Email Contact | Utilities | people | mailto:[$people.email.1]?subject=Project Questions |
| View in SharePoint | Resources | projects | https://company.sharepoint.com/sites/[$projects.project_number.1] |
| Chat with Team | Communication | projects | https://teams.microsoft.com/l/channel/19%3a[$projects.project_number.1] |

For a complete list of available data types and how they relate to application views and right-click menus, see [Data Types](<../PluginsOverview/DataTypes.md>).

The variable `[$projects.project_number.1]` in the URL is replaced with the actual project number from the right-clicked project. The XML path notation ensures the plugin accesses the correct field from the exported XML data.

## Standard Plugin Settings

| Settings Entry | Plugin | Description |
| :--- | :--- | :--- |
| **Editor** | Base Plugins Settings | Set the path to the Python script editor used by the Script Editor utility. |
| **Outlook Integration** | Base Plugins Settings | Configure the remaining email and item synchronization functions. Contacts use native Preferences. See [Outlook Integration](<OutlookIntegration.md>). |
| **My Shortcuts** | Base Plugins Settings | Configure custom URL shortcuts that appear in the Plugins or right-click menus. See [My Shortcuts](<MyShortcuts.md>). |
| **Settings Migrator** | Base Plugins Settings | Migrate plugin settings between machines or installations. |
| **IFS Cloud Settings** | IFS Cloud Plugins Settings | Configure IFS Cloud ERP integration credentials and options. See [IFS Cloud](<IFSCloud.md>). |
| **New Change Order** | New Change Order | Configure the sub-folder for new change order documents. |
| **New MS Project** | New MS Project | Configure the sub-folder for new MS Project files. |
| **PCR Register** | PCR Register | Configure the sub-folder for new PCR register files. |
| **New PowerPoint** | New PowerPoint | Configure the sub-folder for new PowerPoint presentations. |
| **New Risk Register** | New Risk Register | Configure the sub-folder for new Risk Register files. |


Starting with 6.4.0, opening a database retires the old Widgets page-state and XML filter settings. Retired database settings are marked deleted so the cleanup syncs. Current QML filters, sort orders, project folders, templates, and shared preferences are preserved. The obsolete `project_risks` table, including any historical rows, is removed; current risks are stored in `item_tracker`. Desktop startup also removes the old Widgets window geometry, table layouts, and local XML filter settings while preserving current view options and plugin settings.
