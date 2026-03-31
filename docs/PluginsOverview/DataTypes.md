# Data Types and Table References

**Data Types** (also called table names) are the fundamental data structures in Project Notes. They represent the different kinds of records stored in the database — projects, people, notes, etc. Understanding data types is essential for:

- Configuring plugin menus to appear on the correct right-click context menus
- Creating plugin shortcuts that work with specific record types
- Structuring XML exports and imports
- Writing plugins that process the correct data

## Data Type and Application Views

Each data type corresponds to one or more views (pages/tables) in the Project Notes application. When you right-click on a record or navigate to a table in the application, the data type context determines which plugin menus appear.

### Available Data Types

| Data Type | Application View(s) | Right-Click Menu Appears On | Purpose |
| :--- | :--- | :--- | :--- |
| `projects` | Project List Page, Project Page | Project rows in the Project List; Project detail pages | Core project records with schedule, budget, and status information |
| `people` | People List Page | People rows in the People List; People context menus | Contact and team member information |
| `clients` | Client List Page | Client rows in the Client List | Client company information and contact details |
| `project_people` | Project Page (Team tab) | Team member rows within a project | Association between people and projects (roles, responsibilities) |
| `project_locations` | Project Page (Files & Folders tab) | File/folder entries in the Files & Folders list | Project files, folders, and document locations |
| `project_notes` | Notes Page, Project Page (Notes tab) | Note entries in the Notes section | Meeting minutes, status notes, and project documentation |
| `meeting_attendees` | Notes Page (Action Items tab for meetings) | Meeting attendee records | Attendees and participants for specific meetings |
| `item_tracker` | Item Tracker Page | Tracker item rows in the table | Issues, risks, and action items (primary records) |
| `item_tracker_updates` | Item Tracker Page (Updates tab) | Status update entries for tracker items | Progress updates and status changes for issues/risks/actions |
| `status_report_items` | Project Page (Status Report tab) | Status report entries | Project status information tied to status reporting periods |

## How Data Types Relate to the Application Interface

### Projects View
- **Page**: Project List Page
- **Data Type**: `projects`
- **Contains**: All project records with columns for project number, name, client, status, budget, and schedule
- **Right-Click Menu**: Shows menus configured for `projects` data type

### People View
- **Page**: People List Page
- **Data Type**: `people`
- **Contains**: All contact/team member records with contact information
- **Right-Click Menu**: Shows menus configured for `people` data type

### Clients View
- **Page**: Client List Page
- **Data Type**: `clients`
- **Contains**: Client company records with company information and primary contacts
- **Right-Click Menu**: Shows menus configured for `clients` data type

### Project Detail Page
**Multiple data types interact on this page:**

- **Tab: Overview** → `projects` data type
  - Project summary, schedule, budget, earned value metrics

- **Tab: Team** → `project_people` data type
  - Team members assigned to the project
  - Right-click menus configured for `project_people` appear here

- **Tab: Files & Folders** → `project_locations` data type
  - Files and folders associated with the project (found by File Finder or manually added)
  - Right-click menus configured for `project_locations` appear here

- **Tab: Notes** → `project_notes` data type
  - Meeting notes, status notes, and documentation
  - Right-click menus configured for `project_notes` appear here

- **Tab: Status Report** → `status_report_items` data type
  - Project status reporting entries

### Notes Page
**Used for detailed meeting management:**

- **Tab: Notes** → `project_notes` data type
  - Meeting minutes and notes

- **Tab: Attendees** → `meeting_attendees` data type
  - List of meeting participants
  - Right-click menus configured for `meeting_attendees` appear here

- **Tab: Action Items** → `item_tracker` data type with context from the meeting
  - Action items assigned from the meeting

### Item Tracker Page
**Comprehensive view for all tracker items:**

- **Main Table** → `item_tracker` data type
  - Issues, risks, and action items
  - Right-click menus configured for `item_tracker` appear here

- **Tab: Updates** → `item_tracker_updates` data type
  - Status updates and progress on tracker items
  - Right-click menus configured for `item_tracker_updates` appear here

## How Data Types Affect Plugin Menus

### Right-Click Context Menus

When you right-click on a record in Project Notes, the available context menu items are determined by the **data type** of that record. Plugins configure which data type(s) their menus apply to.

**Example:**
- If a plugin specifies `dataexport: "projects"`, its menu will appear when you right-click on a project row
- If a plugin specifies `dataexport: "item_tracker"`, its menu will appear when you right-click on an issue/risk/action item
- If a plugin specifies `dataexport: ""` (empty), its menu appears in the **Plugins** menu instead of right-click menus

### Plugins Menu

When a plugin menu does **not** specify a data type (the `dataexport` field is empty), the menu appears in the **Plugins** menu at the top of the application. These menus are not context-specific and can be invoked from anywhere in the application.

## Data Types in Plugin XML

When a plugin processes data, the XML structure is determined by the data type being exported. For example:

### Exporting a Project (projects data type)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<projectnotes>
<table name="ix_projects">
    <row id="{8f4e1b2a-5c3d-4e7f-9a2b-c1d4e5f6a7b8}">
        <column name="project_id">{8f4e1b2a-5c3d-4e7f-9a2b-c1d4e5f6a7b8}</column>
        <column name="project_number">ABC-001</column>
        <column name="project_name">Website Redesign</column>
        <column name="client_id" lookupvalue="Acme Corp">{d2e3f4a5-b6c7-4d8e-9f0a-b1c2d3e4f5a6}</column>
        ...
    </row>
</table>
</projectnotes>
```

### Exporting a Person (people data type)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<projectnotes>
<table name="ix_people">
    <row id="{c2d3e4f5-a6b7-4c8d-9e0f-a1b2c3d4e5f6}">
        <column name="people_id">{c2d3e4f5-a6b7-4c8d-9e0f-a1b2c3d4e5f6}</column>
        <column name="name">John Smith</column>
        <column name="email">john@company.com</column>
        ...
    </row>
</table>
</projectnotes>
```

## Data Type Usage in Plugins

### My Shortcuts Plugin

The [My Shortcuts](<../StandardPlugins/MyShortcuts.md>) plugin uses data types to determine when shortcuts appear:

| Menu | Data Type | Behavior |
| :--- | :--- | :--- |
| Open in Jira | `projects` | Appears when right-clicking on a project; can access `$projects.project_number.1` |
| Email Contact | `people` | Appears when right-clicking on a person; can access `$people.email.1` |
| (blank) | (none) | Appears in the **Plugins** menu; available globally |

### Meeting and Email Types Plugin

The [Meeting and Email Types](<../StandardPlugins/PluginSettings.md>) plugin associates template types with data types to determine which templates appear in right-click menus for scheduling meetings and sending emails.

### File Finder Plugin

The [File Finder](<../StandardPlugins/FileFinder.md>) plugin classifies files and adds them with the `project_locations` data type, making them appear in the **Files & Folders** tab.

## Summary

- **Data Types** are table names that organize records in the database
- Each **data type corresponds to application pages/tabs** where those records are displayed
- **Right-click menus appear based on the data type** of the record being clicked
- **Plugins configure which data types** they work with to appear in the correct context
- **XML exports follow the data type structure**, with different layouts for different record types

Understanding data types helps you:
- Configure plugins to appear in the right menus
- Create shortcuts and menu items for specific record types
- Write plugins that process the correct data structures
