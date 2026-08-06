# Application Menu

The Application Menu is the single menu for the whole app — there is no traditional menu bar. It groups the same File, Edit, View, and Help commands a menu bar would, behind one icon.

**To open the Application Menu:**

1. Click the **menu** icon at the top of the icon rail (the leftmost, vertical strip of icons on the far left of the window).
2. Click a group name — **File**, **Plugins** (only shown when plugins add global menu items), **Edit**, **View**, or **Help** — to open its items in a flyout beside it. Hovering a group name for a moment opens it automatically, the same way a native menu bar works.

## File

| **Item** | **What it does** |
| :--- | :--- |
| New Record | Creates a new record in whatever section is currently active — a project, person, or client. Same action as the **+ New** button on those pages. |
| Search… | Switches to the [Search Page](<SearchPage.md>). |
| Export XML… | Exports the record currently open (a project, note, item, person, or client) to a [Project Notes XML](<../PluginsOverview/ProjectNotesXML.md>) file. Only enabled when a record is open — same action as the **Export** button in the top bar. |
| Import XML… | Imports records from a Project Notes XML file — see [Getting Started](<../Introduction/GettingStarted.md>). |
| Preferences | Opens the [Settings page](<Preferences.md>). |
| Sync Now | Runs an immediate incremental sync cycle — see [Cloud Sync](<RemoteHost.md>). |
| Sync All | Runs a full re-push and re-pull of the whole database — see [Cloud Sync](<RemoteHost.md>). |
| Exit | Closes Project Notes. |

## Plugins

When one or more installed plugins add global menu items (ones not tied to a specific record's table), they appear grouped under a **Plugins** entry, positioned right after **File**. Plugins that share a submenu name are nested under that name inside **Plugins**; plugins with no submenu appear as plain rows. If no plugin adds a global menu item, this group is hidden. See [Adding Functionality](<../PluginsOverview/AddingFunctionality.md>) for how plugins register menu entries.

When a record with its own plugin menus is open (for example a project with an **Export** or **Templates** submenu), those table-scoped submenus appear as additional groups at the end of the menu, alongside File/Plugins/Edit/View/Help — see [Plugin Settings](<../StandardPlugins/PluginSettings.md>).

## Edit

| **Item** | **What it does** |
| :--- | :--- |
| Find | Switches to the [Search Page](<SearchPage.md>). |
| Filter Data… | Opens the [Filter Tool](<FilterTool.md>) for whatever list is currently active. |

## View

| **Item** | **What it does** |
| :--- | :--- |
| Dark Mode | Toggles between light and dark theme. Shows a check mark when dark mode is on. The same three-way choice (System, Light, Dark) is also available from **Settings > Appearance** — see [Preferences](<Preferences.md>). |
| Show Internal / Budget Items | Shows or hides notes and tracker items marked **Internal**, and the earned value/budget figures on the [Project List Page](<ProjectListPage.md>) and [Project Page](<ProjectPage.md>). See [Presenting to Clients](<PresentingToClients.md>) for a typical use of this toggle. |
| Show Closed Projects | Shows or hides projects whose **Project Status** is **Closed** on the [Project List Page](<ProjectListPage.md>). |
| Show Resolved Items | Shows or hides tracker items whose **Status** is **Resolved** on the [Item Tracker Page](<ItemTrackerPage.md>). |
| Zoom controls | A single row with **−**, the current zoom percentage, and **+** to step [UI Zoom](<VectorZoom.md>) in and out, plus a fullscreen toggle. |
| Log Viewer | Opens the Log Viewer window — see [Error Log](<ErrorLog.md>). |

Each toggle in this group shows a live check mark reflecting its current state, and takes effect immediately — there is no separate Apply or OK step.

## Help

| **Item** | **What it does** |
| :--- | :--- |
| User Guide | Opens this User Guide to the topic for whatever section you're currently viewing. Also reachable by pressing **F1** anywhere in the app. |
| Check for Updates… | Checks GitHub for a newer release and always reports the result, even when you're already up to date. |
| Send Logs to Support… | Packages your log files and opens an email addressed to support — see [Error Log](<ErrorLog.md>). |
| About | Opens the **About** section of [Settings](<Preferences.md>), with the current version and links to documentation, release notes, and source code. |

## Related Documentation

- [Preferences](<Preferences.md>) — the Settings page opened from **File > Preferences**
- [Filter Tool](<FilterTool.md>) — the filter editor opened from **Edit > Filter Data…**
- [Cloud Sync](<RemoteHost.md>) — details on **Sync Now** and **Sync All**
- [UI Zoom](<VectorZoom.md>) — the zoom controls in the **View** group
