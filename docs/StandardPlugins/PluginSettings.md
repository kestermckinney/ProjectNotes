# Standard Plugins and Plugin Settings

## Plugins that come with Project Notes

Project Notes installs a small set of JavaScript plugins. They add menus to records and to the **Plugins** group of the app menu, and each one with options has its own tab under **Settings**.

| Plugin | What it does | Platforms |
| :--- | :--- | :--- |
| Base Plugins | Copy a contact's email or a file's path to the clipboard; open your script editor. See [Editor](<Editor.md>). | All |
| My Shortcuts | Menus that open web pages filled in from the record you right-click. See [My Shortcuts](<MyShortcuts.md>). | All |
| Team Member Quick Add | Add a new contact to a project's team in one step. See [Team Member Quick Add](<TeamMemberQuickAdd.md>). | All |
| Outlook Integration | Archive project email and keep your tracker items in Microsoft To Do. See [Outlook Integration](<OutlookIntegration.md>). | All (Outlook classic archiving on Windows) |
| Office Tools | New project documents from Word, Excel, PowerPoint and MS Project templates; Open MS Project; close stranded Excel and Word processes. See [Office Document Templates](<NewDocumentTemplates.md>). | Windows |

The IFS Cloud plugin is installed separately. See [IFS Cloud Integration](<IFSCloud.md>).

Email, meeting invitations and the project reports are built into Project Notes, not plugins. See [Email and Meeting Setup](<../InterfaceOverview/EmailAndMeetingSetup.md>), [Reports and Email](<../InterfaceOverview/ReportsAndEmail.md>) and [Scheduling Meetings](<../InterfaceOverview/ScheduleMeeting.md>). [File Finder](<../InterfaceOverview/FileFinder.md>), [Office 365 Integration](<../InterfaceOverview/Office365Integration.md>) and [contact synchronization](../InterfaceOverview/Preferences.md#contact-synchronization) are application services with their own Settings pages too.

## Managing plugins

Open **Settings > Plugins** to see every plugin, turn one on or off, install a `.pnplugin` package, or open your plugins folder. Plugins that come with Project Notes are on from the start. A plugin you install stays off until you review what it asks to do (read or change certain records, reach certain web sites, automate Office, and so on) and approve it. If a later version asks for more, you review it again.

Your own plugins live in the **Project Notes/plugins** folder in your home folder, never in the application's folder, so upgrades leave them alone. See [JavaScript Plugins](<../PluginsOverview/JavaScriptPlugins.md>) to write one.

Settings saved by the Python plugins of earlier releases carry over: each plugin copies its old settings the first time it starts. Python plugins themselves no longer run.

## Where settings are kept

Settings in Project Notes are stored in two places depending on what they control.

### Settings stored in your operating system profile

Plugin settings, UI state, and connection details are stored in your **operating system's user profile**. They belong to this computer and user account and do not sync. Passwords and sign-in tokens go to the operating system's credential store instead.

| Category | Examples |
| :--- | :--- |
| Local application and plugin configuration | Document template subfolders, My Shortcuts, Outlook Integration options, IFS Cloud addresses, File Finder search locations and classifications, email templates |
| Cloud sync connection | Sync enabled flag and Project Notes Pro account |
| Window position and size | Restored on the next launch |
| UI zoom level | Set with **Ctrl +/-/0** or the zoom row of the [Application Menu](<../InterfaceOverview/ApplicationMenu.md>)'s **View** group. See [UI Zoom](<../InterfaceOverview/VectorZoom.md>). |
| Spell check dictionary | Selected dictionary and personal word list |

| Platform | Location |
| :--- | :--- |
| **Windows** | Registry under `HKEY_CURRENT_USER\Software\ProjectNotes` |
| **macOS** | `~/Library/Preferences/com.projectnotes.*.plist` |
| **Linux** | `~/.config/ProjectNotes/` |

When using the `--developer-profile` option, the profile name is appended to the organization key so each profile has its own isolated settings.

### Settings stored in the database

Preferences and view state that should be the same for anyone using the same database are stored in the `application_settings` table inside `ProjectNotes.db`. They are included in cloud sync and move with the database when it is copied or shared.

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

## Placeholders

Several plugins fill placeholders with values from the record you right-click. A placeholder has the form `[$entity.column.row]`:

- `entity` is the kind of record, such as `projects` or `contacts` (see [Data Types](<../PluginsOverview/DataTypes.md>));
- `column` is a field of that record;
- `row` is 1 for the record you right-clicked.

| Placeholder | Becomes |
| :--- | :--- |
| `[$projects.project_number.1]` | The project number |
| `[$projects.project_name.1]` | The project name |
| `[$contacts.name.1]` | The contact's name |
| `[$contacts.email.1]` | The contact's email address |
| `[$project_notes.note_title.1]` | A meeting's title |

A shortcut such as

```
https://outlook.office365.com/mail/?folderid=Inbox&searchText=[$projects.project_number.1] [$projects.project_name.1]
```

opens `https://outlook.office365.com/mail/?folderid=Inbox&searchText=P-002 Website Redesign` for project P-002. Export a record to [XML](<../PluginsOverview/ProjectNotesXML.md>) to see every column you can use.

Office templates use tags instead, such as `<PROJECTNUMBER>` and `<PROJECTNAME>`. See [Office Document Templates](<NewDocumentTemplates.md>).

Starting with 6.4.0, opening a database retires the old Widgets page-state and XML filter settings. Retired database settings are marked deleted so the cleanup syncs. Current QML filters, sort orders, project folders, templates, and shared preferences are preserved. The obsolete `project_risks` table, including any historical rows, is removed; current risks are stored in `item_tracker`. Desktop startup also removes the old Widgets window geometry, table layouts, and local XML filter settings while preserving current view options and plugin settings.
