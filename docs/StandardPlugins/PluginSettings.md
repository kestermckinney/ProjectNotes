# Standard Plugins and Plugin Settings

## Built-In Standard Plugins

Project Notes comes with a comprehensive set of standard plugins installed and enabled out of the box. These plugins provide powerful functionality for common project management tasks, including:

- **Meeting and Email Integration** — Schedule meetings, send emails, and archive communications
- **Document Management** — Export meeting notes and tracker items to PDF, create new documents from templates
- **File Organization** — Automatically collect and organize project files
- **Report Generation** — Generate tracker item reports and status reports
- **Enterprise Integration** — Connect to Outlook, Office 365, and IFS Cloud ERP
- **Customization** — Define custom shortcuts and configure plugin behavior

All standard plugins are **highly configurable**. You can customize their behavior through settings dialogs accessible from the **Plugins > Settings** menu. This allows you to tailor Project Notes to your organization's specific workflows without modifying code.

### Standard Plugins Overview

The standard plugins are organized into several categories:

**Communication Plugins:**
- Send Meeting Notes — Email meeting notes to attendees
- Send Project Emails — Send emails about projects to team members
- Outlook Integration — Integrate with Outlook and Office 365 for scheduling and email
- Schedule Meeting Plugins — Schedule customer and internal kick-off, status, and lessons learned meetings

**Export and Report Plugins:**
- Export Meeting Notes — Export meeting notes to PDF
- Export Tracker Items — Export tracker items and action items to PDF
- Tracker Report Generation — Generate customizable tracker/action item reports
- Meeting Notes Archive — Archive meeting notes to a designated folder
- Project Email Archive — Archive project-related emails

**File and Document Plugins:**
- File Finder — Automatically monitor and organize project-related files
- New Document Templates — Create new documents from templates (MS Project, PowerPoint, Change Orders, etc.)
- Team Member Quick Add — Quickly add team members to projects

**Utility Plugins:**
- Editor — Configure external script editors
- My Shortcuts — Define custom menu shortcuts
- IFS Cloud — Integrate with IFS Cloud ERP system
- Find Project Email — Search Outlook for project emails by team member (Windows only)

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
| Plugin configuration | Export sub-folders, IFS credentials, Outlook/Office 365 credentials, File Finder search locations and classifications, My Shortcuts definitions, meeting/email templates |
| Cloud sync connection | Sync enabled flag, server URL, email, password, encryption phrase, Supabase key |
| Window positions and sizes | Position and size of every dialog and the main window |
| Table column widths and order | Column layout for every list view |
| Table sort column and direction | Last sort applied to each list view |
| Application font size | The global font size set via the View menu |
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

Project Notes internally represents all data as XML. When you invoke a plugin, the application exports the relevant data (project, person, meeting, etc.) as XML and passes it to the plugin. The plugin uses this XML to:

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
- `[$people.email.1]` — The first person's email address
- `[$people.name.1]` — The first person's name

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

**When right-clicking a Person:**
- `[$people.name.1]` — Person's name
- `[$people.email.1]` — Person's email address
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

#### Example 3: Email and Recipient Configuration

When exporting meeting notes and sending via email, the plugin uses project data to populate the subject line:

Template:
```
Meeting Minutes - [$project_notes.project_number.1] [$project_notes.project_name.1] - [$project_notes.note_date]
```

For Project P-001 (Sample Project) on 03/23/2026:
```
Subject: Meeting Minutes - P-001 Sample Project - 03/23/2026
```


### Common Plugin Settings Examples

#### Outlook Integration Settings

| Setting | Example Value | Purpose |
| :--- | :--- | :--- |
| **Integration Type** | Office 365 Application | Use Graph API instead of Outlook COM |
| **Client ID** | a1b2c3d4-e5f6-47a8-b9c0-d1e2f3a4b5c6 | Azure AD app ID for OAuth |
| **Tenant ID** | f1e2d3c4-b5a6-4978-c9d0-e1f2a3b4c5d6 | Azure AD tenant for your organization |

#### File Finder Settings

| Setting | Example Value | Purpose |
| :--- | :--- | :--- |
| **Search Location 1** | \\company\projects | Root folder to scan for project files |
| **Search Location 2** | C:\Users\{username}\Documents | User's local documents folder |
| **Classification: PDF** | Contract (.pdf) | Files ending in .pdf are marked as "Contract" type |
| **Classification: Mpp** | Project Plan (.mpp) | Files ending in .mpp are marked as "Project Plan" type |

#### Export Meeting Notes Settings

| Setting | Example Value | Purpose |
| :--- | :--- | :--- |
| **Sub-folder** | Meeting Minutes | Save exports to `[Project Folder]/Meeting Minutes/` |

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
| **File Finder** | Base Plugins Settings | Configure folders to scan and file classification rules for the File Finder background process. |
| **Editor** | Base Plugins Settings | Set the path to the Python script editor used by the Script Editor utility. |
| **Outlook Integration** | Base Plugins Settings | Configure Office 365 Graph API credentials or Outlook COM options. See [Outlook Integration](<OutlookIntegration.md>). |
| **My Shortcuts** | Base Plugins Settings | Configure custom URL shortcuts that appear in the Plugins or right-click menus. See [My Shortcuts](<MyShortcuts.md>). |
| **Meeting and Email Types** | Base Plugins Settings | Configure meeting invitation templates and email templates that appear in the **Schedule Meeting** and **Send Email** sub-menus. |
| **Settings Migrator** | Base Plugins Settings | Migrate plugin settings between machines or installations. |
| **Export Notes** | Export Meeting Notes | Configure the project sub-folder where meeting notes exports are saved. |
| **Export Tracker Items** | Export Tracker Items | Configure the project sub-folder where tracker item exports are saved. |
| **IFS Cloud Settings** | IFS Cloud Plugins Settings | Configure IFS Cloud ERP integration credentials and options. See [IFS Cloud](<IFSCloud.md>). |
| **New Change Order** | New Change Order | Configure the sub-folder for new change order documents. |
| **New MS Project** | New MS Project | Configure the sub-folder for new MS Project files. |
| **PCR Register** | PCR Register | Configure the sub-folder for new PCR register files. |
| **New PowerPoint** | New PowerPoint | Configure the sub-folder for new PowerPoint presentations. |
| **New Risk Register** | New Risk Register | Configure the sub-folder for new Risk Register files. |

