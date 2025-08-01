# Plugin Settings

## Managing and Storing Settings

Each plugin will typically have a settings the user will want to change. The plugins provided with Project Notes store settings in the personal operating system profile under a Plugin Settings group with the application settings. In Windows, this is the registry and on MacOS and Linux systems this is a configuration file.  The "base_plugin_settings.py" plugin script contains all of the code to handle the user interface for modifying settings of the base plugins included with Project Notes.  The Qt Forms for these settings are located in the "forms" folder under the plugins folder.  Using Qt Designer the user interface elements can be created an modified.

### Events

Project notes calls standard events when they are defined within the Python script.  An event take one parameter called parameter.  When an event is called from the Plugins menu the parameter is not used.

| **Event** | **Description** |
| :--- | :--- |
| event\_startup | The startup event executes when the the plugin is first loaded. |
| event\_shutdown | The shutdown executes just before the plugin is unloaded. |
| event\_timer | This event is only used in a thread.  The frequency the event is triggered is defined by the "plugintimerevent" variable defined in the script.  If the variable is not defined the default value is 1 for every one minute. |

<br>
### Properties

Each plugin can have it's own settings. Settings are stored in the registry on Windows computers and in a configuration file on MacOS and Linux computers. Settings values are stored as text. The settings in the [Global Settings](<../StandardPlugins/GlobalSettings.md>) plugin are shared by multiple plugins.

**To change a property:**

1. From Project Notes **Plugins** menu choose **Plugin Settings**.
2. Select the plugin from the list of **Loaded Plugins**.
3. Select the property text in the **Plugin Settings** grid.
4. Type in the property value.
5. Click **Close**.

**To enable or disabled a plugin:**

1. From Project Notes **Plugins** menu choose **Loaded Plugin Settings**.
2. Select the plugin from the list of **Plugins**.
3. Check the **Enabled** check box to enable the plugin. Uncheck the **Enabled** check box to disable the plugin.
4. Type in the property value.
5. Click **Close**.