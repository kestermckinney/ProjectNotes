# Plugin Settings

## Managing and Storing Settings

Each plugin will typically have settings the user will want to change. Plugins provided with Project Notes store settings in the personal operating system profile: in the registry on Windows, and in a configuration file on macOS and Linux.

The **Base Plugins Settings** script (`base_plugin_settings.py`) contains all of the code to handle the user interface for modifying settings of the base plugins included with Project Notes. Qt Designer forms for these settings are located in the `plugins/forms/` folder.

Plugins conventionally add menu items to the **Plugins** menu under a **Settings** sub-menu to provide access to their configuration.

## Standard Plugin Settings

The following settings dialogs are available under **Plugins > Settings**:

| Settings Entry | Plugin | Description |
| :--- | :--- | :--- |
| **File Finder** | Base Plugins Settings | Configure folders to scan and file classification rules for the File Collector background process. |
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

## Enabling and Disabling Plugins

See [Plugins Menu](<../InterfaceOverview/PluginsMenu.md>) for information on enabling and disabling individual plugins.
