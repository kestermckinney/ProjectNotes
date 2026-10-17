# Editor

## Editing plugins

The **Script Editor** setting tells Project Notes which program to open for editing plugins, and **Plugins · Utilities > Script Editor** starts it. Any editor that understands JavaScript works well.

Until you choose one, Project Notes uses the editor that ships with your operating system:

| Platform | Default editor |
| :--- | :--- |
| **Windows** | Notepad (`C:/Windows/System32/notepad.exe`) |
| **macOS** | TextEdit (`/Applications/TextEdit.app/Contents/MacOS/TextEdit`) |
| **Linux** | vi (`/usr/bin/vi`) |

**To choose a different editor**, open **Settings > Script Editor**, choose the editor program, and save.

Project Notes reloads a plugin when you save one of its files. Keep your own plugins in the plugins folder in the application's data folder (**Settings > Plugins > Open Plugins Folder** opens it): upgrades replace the plugins in the application's folder. See [JavaScript Plugins](<../PluginsOverview/JavaScriptPlugins.md>) for the plugin API.

Builds that cannot start other programs, such as the Mac App Store version, leave the Script Editor menu out.
