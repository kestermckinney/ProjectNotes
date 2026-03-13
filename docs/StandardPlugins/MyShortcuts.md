# My Shortcuts

The **My Shortcuts** plugin lets you define configurable URLs that appear as menu items throughout Project Notes. URLs can contain variable placeholders that are automatically filled in from the current project data when the menu item is invoked.

## How It Works

Each shortcut defines a menu item name, a sub-menu, an optional data context, and a URL. When you click the menu item, the URL's variables are replaced with values from the current project and the link is opened in your default browser.

Shortcuts can appear in two places:

- **Plugins menu** — for shortcuts that don't depend on a specific data context
- **Right-click context menu** — for shortcuts tied to a specific table (e.g., projects, people, meeting_attendees)

## Variable Substitution

The URL field supports variable placeholders that Project Notes replaces at runtime. Common variables include:

| Variable | Description |
| :--- | :--- |
| `{project_number}` | The current project number |
| `{project_name}` | The current project name |
| `{name}` | The name of the right-clicked person or attendee |
| `{email}` | The email of the right-clicked person |

See [Project Notes XML](<../PluginsOverview/ProjectNotesXML.md>) for the complete list of available XML fields that can be used as variables.

## Configuring My Shortcuts

**To open the My Shortcuts settings:**

1. From the **Plugins** menu choose **Settings > My Shortcuts**.

Each row in the shortcuts table defines one menu item:

| Column | Description |
| :--- | :--- |
| **Menu** | The label shown in the menu. |
| **Submenu** | An optional submenu group name to organize shortcuts. |
| **Data Type** | The table name this shortcut applies to (e.g., `projects`, `people`). Leave blank for a global shortcut with no data context. |
| **URL** | The URL to open. May contain variable placeholders. |

**To add a shortcut:**

1. Open **Settings > My Shortcuts**.
2. Add a new row to the table.
3. Fill in the **Menu**, **Submenu**, **Data Type**, and **URL**.
4. Click **OK** to save.

The new menu item will appear the next time Project Notes loads the plugin, or after a plugin reload.
