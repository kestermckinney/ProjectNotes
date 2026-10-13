# Data Types and Entity Names

Every kind of record in Project Notes has a name: `projects`, `contacts`, `item_tracker` and so on. Plugins use these names to put menus on the right records, to read and save data, and in the `[$entity.column.row]` placeholders that [My Shortcuts](<../StandardPlugins/MyShortcuts.md>) and other plugins fill in.

## Names

Plugins and shortcuts use the names in the first column. [Project Notes XML](<ProjectNotesXML.md>) files still use the database's own table names, which differ for three entities.

| Name | XML table name | Where the records appear |
| :--- | :--- | :--- |
| `projects` | `projects` | Project List, Project Page |
| `contacts` | `people` | Contacts List |
| `companies` | `clients` | Companies List |
| `project_team` | `project_people` | Project Page, Team tab |
| `project_locations` | `project_locations` | Project Page, Files & Folders tab |
| `project_notes` | `project_notes` | Notes, and the Project Page Notes tab |
| `meeting_attendees` | `meeting_attendees` | A meeting's Attendees |
| `item_tracker` | `item_tracker` | Item Tracker, and action items on projects and meetings |
| `item_tracker_updates` | `item_tracker_updates` | A tracker item's comments |
| `status_report_items` | `status_report_items` | Project Page, Status Report tab |
| `project_email_audiences` | `project_email_audiences` | Saved report and meeting audiences |

Columns follow the same rule. A column that points to a contact is `contact_id` (stored as `people_id` or `person_id`), and one that points to a company is `company_id` (stored as `client_id`). The company's name is `company_name`.

## Menus

A plugin's record menu names one entity. Right-click a record of that kind and the menu appears; on the Project Page, for example, a menu for `project_team` shows on the Team tab's rows and one for `project_locations` on Files & Folders. A menu that names no entity is a global menu and appears under **Plugins** in the [app menu](<../InterfaceOverview/ApplicationMenu.md>).

## Placeholders

A placeholder has the form `[$entity.column.row]`: the entity, a column, and the row, starting at 1 for the record you right-clicked. `[$projects.project_number.1]` is the project number of the project, `[$contacts.email.1]` the email address of the contact.

Shortcuts saved before plugins moved to JavaScript used the XML names (`[$people.name.1]`). My Shortcuts converts them the first time it starts.

## Finding the columns

Export a record to XML to see every column it has, or ask from a plugin with `data.schema("projects")`, which also gives each column's type and allowed values. See [JavaScript Plugins](<JavaScriptPlugins.md#working-with-data>).
