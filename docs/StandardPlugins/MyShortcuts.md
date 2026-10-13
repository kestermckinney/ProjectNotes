# My Shortcuts

**My Shortcuts** adds menus that open a web address filled in from the record you right-click: a project in your ERP system, a contact in your CRM, a search in your mail.

## Adding shortcuts

Open **Settings > My Shortcuts**. Each row is one menu:

| Column | What to enter |
| :--- | :--- |
| **Menu** | The menu's title. |
| **Submenu** | Optional. Shortcuts with the same submenu are grouped under it. |
| **Entity** | The kind of record the menu appears on, such as `projects` or `contacts`. Leave it empty for a menu under **Plugins** in the app menu. See [Data Types](<../PluginsOverview/DataTypes.md>). |
| **URL** | The address to open. It may contain placeholders. |

The menus change as soon as you save.

## Placeholders

A placeholder is replaced with a value from the record you right-clicked:

| Placeholder | Becomes |
| :--- | :--- |
| `[$projects.project_number.1]` | The project number |
| `[$projects.project_name.1]` | The project name |
| `[$contacts.name.1]` | The contact's name |
| `[$contacts.email.1]` | The contact's email address |
| `[$managing_company_name]` | The managing company from Preferences |
| `[$managing_manager_name]` | The project manager from Preferences |

For example, `https://jira.example.com/browse/[$projects.project_number.1]` on project P-100 opens `https://jira.example.com/browse/P-100`. See [Plugin Settings](<PluginSettings.md#placeholders>) for more.

A URL can also use other schemes, such as `mailto:` or an application's own scheme (`msteams:`, `onenote:`).

## Shortcuts from earlier versions

Shortcuts saved by the Python version of My Shortcuts are converted the first time the plugin starts. Their data types and placeholders change to the names plugins use now: `people` becomes `contacts`, `clients` becomes `companies`, and `project_people` becomes `project_team`.
