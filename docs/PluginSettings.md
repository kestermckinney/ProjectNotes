# Plugin Settings

## Controlling Active Plugins

The Plugin Settings window lists all of the available plugins and the events that they are tied to. The Plugin Description text box contains an explanation of what the plugin does. When Project Notes starts it looks through the "plugins" folder and loads all of the ".py" files into the [Python](<http://www.python.org>) interpreter. The Python files contain the information displayed on the Plugin Settings window. By default any new plugins found are set as enabled and stored in your personal operating system profile. In Windows, this is the registry and on MacOS and Linux systems this is a configuration file.

**Events**

The Python script specifies which events it will handle. The table below describes the different events available.

| **Event** | **Description** |
| --- | --- |
| event\_startup | The startup event executes when the application is opened. |
| event\_shutdown | The shutdown executes when the application closed. |
| event\_everyminute | Once a database is open this event is called every minute. |
| event\_every5minutes | Once a database is open this event is called every 5 minutes. |
| event\_every10minutes | Once a database is open this event is called every 10 minutes. |
| event\_every30Mmnutes | Once a database is open this event is called every 30 minutes. |
| event\_menuclick | Makes the plugin appear in the Plugins menu, and is called when the menu option is chosen. |
| event\_data\_rightclick | Makes the plugin appear in the Right-Click on a list, and is called when the menu option is chosen. |

**Properties**
Each plugin can have it's own settings. Settings are stored in the registry on Windows computers and in a configuration file on MacOS and Linux computers. Settings values are stored as text. The settings in the [Global Settings](<Global Settings Plugin>) plugin are shared by multiple plugins.

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