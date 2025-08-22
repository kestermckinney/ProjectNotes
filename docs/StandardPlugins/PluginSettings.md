# Plugin Settings

## Managing and Storing Settings

Each plugin will typically have a settings the user will want to change. The plugins provided with Project Notes store settings in the personal operating system profile under a Plugin Settings group with the application settings. In Windows, this is the registry and on MacOS and Linux systems this is a configuration file.  The "base_plugin_settings.py" plugin script contains all of the code to handle the user interface for modifying settings of the base plugins included with Project Notes.  The Qt Forms for these settings are located in the "forms" folder under the plugins folder.  Using Qt Designer the user interface elements can be created an modified.  Conventionally a plugin will add menus to the **Plugins** menu in Project Notes under a "Settings" submenu to provide access to specific settigns.

## Standard Plugins

- [Editor](Editor.md)
- [File Collector](FileCollector.md)
- Meeting And Email Types
- My Shortcuts
- Outlook Integration
- Settings Migrator

