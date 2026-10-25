# Standard Plugins and Plugin Settings

## Plugins that come with Project Notes

Project Notes installs a small set of JavaScript plugins. They add menus to records and to the **Plugins** group of the app menu, and each one with options has its own tab under **Settings**.

| Plugin | What it does | Platforms |
| :--- | :--- | :--- |
| Base Plugins | Copy a contact's email or a file's path to the clipboard; open your script editor. See [Editor](<Editor.md>). | All |
| My Shortcuts | Menus that open web pages filled in from the record you right-click. See [My Shortcuts](<MyShortcuts.md>). | All |
| Team Member Quick Add | Add a new contact to a project's team in one step. See [Team Member Quick Add](<TeamMemberQuickAdd.md>). | All |
| Archive Mail | Archive project email into project folders. See [Archive Mail](<ArchiveMail.md>). | All (Outlook classic archiving on Windows) |
| Office Tools | New project documents from Word, Excel, PowerPoint and MS Project templates; Open MS Project; close stranded Excel and Word processes. See [Office Document Templates](<NewDocumentTemplates.md>). | Windows |

The IFS Cloud plugin is installed separately. See [IFS Cloud Integration](<IFSCloud.md>).

Email, meeting invitations and the project reports are built into Project Notes, not plugins. See [Email and Meeting Setup](<../InterfaceOverview/EmailAndMeetingSetup.md>), [Reports and Email](<../InterfaceOverview/ReportsAndEmail.md>) and [Scheduling Meetings](<../InterfaceOverview/ScheduleMeeting.md>). [File Finder](<../InterfaceOverview/FileFinder.md>), [Office 365 Integration](<../InterfaceOverview/Office365Integration.md>) and [contact synchronization](../InterfaceOverview/Preferences.md#contact-synchronization) are application services with their own Settings pages too.

## Managing plugins

Open **Settings > Plugins** to see every plugin, turn one on or off, install a `.pnplugin` package, or open your plugins folder. Plugins that come with Project Notes are on from the start. A plugin you install stays off until you review what it asks to do (read or change certain records, reach certain web sites, automate Office, and so on) and approve it. If a later version asks for more, you review it again.

Your own plugins live in the plugins folder in the application's data folder, never in the application's install folder, so upgrades leave them alone. **Open Plugins Folder** shows where it is. See [JavaScript Plugins](<../PluginsOverview/JavaScriptPlugins.md>) to write one.

Settings saved by the Python plugins of earlier releases carry over: each plugin copies its old settings the first time it starts. Python plugins themselves no longer run.

## Backing up and moving plugin settings

Plugin settings belong to one computer. **File > Export Settings…** saves them together with the application settings (see [Backing Up Settings](<../InterfaceOverview/Preferences.md#backing-up-settings>)). For plugins alone, scroll to the bottom of **Settings > Plugins**, below the plugin list, to the **Back Up Plugin Settings** section. It lists each installed plugin that has saved settings. Tick the ones you want and click **Export Settings…** to save them in a JSON file. On the other computer, or after a reinstall, click **Import Settings…** and pick the file. Imported values replace the same settings on that computer. Settings the file doesn't mention stay as they are.

Passwords are never written to the file. They stay in the operating system's credential store, so enter them again after an import.

Files saved by the **Settings Migrator** of the old Python plugins import the same way. Each old settings group goes to the plugin that replaced it, so a My Shortcuts or IFS Cloud backup from an earlier release still works. A group whose plugin isn't installed yet is kept, and the plugin picks it up when you install it.

## Where settings are kept

Settings in Project Notes are stored in two places depending on what they control.

### Settings stored in your operating system profile

Plugin settings, UI state, and connection details are stored in your **operating system's user profile**. They belong to this computer and user account and do not sync. Passwords and sign-in tokens go to the operating system's credential store instead.

| Category | Examples |
| :--- | :--- |
| Local application and plugin configuration | Document template subfolders, My Shortcuts, Archive Mail options, IFS Cloud addresses, File Finder search locations and classifications, email templates |
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

Email templates, meeting templates, My Shortcuts and Office document templates all fill in values the same way. A placeholder is a dotted name between double braces, such as `{{ project.name }}`. Spaces inside the braces are optional. To write the braces themselves, put a backslash in front: `\{{`.

Email and meeting templates have a fixed list of names, shown in the template editor: `project.number`, `project.name`, `company.name`, `preferences.managerName`, `preferences.managingCompanyName`, `current.date` and a few more for each kind of email. Office templates use `project.number`, `project.name`, `company.name`, `current.date` and, for change orders, `change.number`. See [Office Document Templates](<NewDocumentTemplates.md>).

My Shortcuts also reads the record you right-click. A name there has the form `entity.column`:

- `entity` is the kind of record, such as `projects` or `contacts` (see [Data Types](<../PluginsOverview/DataTypes.md>));
- `column` is a field of that record.

To reach a row of a related list, put the row number between them: `{{ project_team.2.role }}` is the role of the second team member.

To get a column from every row, use `*` as the row: `{{ project_team.*.email }}` joins the email address of each team member with a comma and a space, skipping empty ones. Without a row number, a related list gives its first row.

| Placeholder | Becomes |
| :--- | :--- |
| `{{ projects.project_number }}` | The project number |
| `{{ projects.project_name }}` | The project name |
| `{{ contacts.name }}` | The contact's name |
| `{{ contacts.email }}` | The contact's email address |
| `{{ project_notes.note_title }}` | A meeting's title |

A shortcut such as

```
https://outlook.office365.com/mail/?folderid=Inbox&searchText={{ projects.project_number }} {{ projects.project_name }}
```

opens `https://outlook.office365.com/mail/?folderid=Inbox&searchText=P-002 Website Redesign` for project P-002. Export a record to [XML](<../PluginsOverview/ProjectNotesXML.md>) to see every column you can use.

Older shortcuts and templates written as `[$projects.project_number.1]` or `<PROJECTNAME>` keep working. My Shortcuts rewrites its saved shortcuts to the new form the next time it starts.

Starting with 6.4.0, opening a database retires the old Widgets page-state and XML filter settings. Retired database settings are marked deleted so the cleanup syncs. Current QML filters, sort orders, project folders, templates, and shared preferences are preserved. The obsolete `project_risks` table, including any historical rows, is removed; current risks are stored in `item_tracker`. Desktop startup also removes the old Widgets window geometry, table layouts, and local XML filter settings while preserving current view options and plugin settings.
