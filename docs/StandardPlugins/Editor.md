# Editor

## Editing Script Plugins

Customizing a plugin, or writing your own, is easier in an editor with Python syntax highlighting. The **Editor** setting tells Project Notes which editor to start, and **Plugins · Utilities > Script Editor** opens it.

Until you choose one, Project Notes uses the editor that ships with your operating system:

| Platform | Default editor |
| :--- | :--- |
| **Windows** | Notepad (`C:/Windows/System32/notepad.exe`) |
| **macOS** | TextEdit (`/Applications/TextEdit.app/Contents/MacOS/TextEdit`) |
| **Linux** | vi (`/usr/bin/vi`) |

**To choose a different editor:**

1. From the app menu choose **Plugins · Settings**, then **Editor**.
2. Click the **...** button and choose the editor's executable.
3. Click **OK**.

Project Notes reloads a plugin as soon as you save it. Keep your own plugins in `~/Project Notes/plugins/` rather than the application's `plugins` folder: installers replace the bundled plugins on every upgrade and remove the ones a release retires. See [Adding Functionality](<../PluginsOverview/AddingFunctionality.md>) for the plugin API.
