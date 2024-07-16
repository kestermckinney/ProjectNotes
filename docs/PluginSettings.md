# Plugin Settings

# Controlling Active Plugins

The Plugin Settings window lists all of the available plugins and the events that they are tied to.&nbsp; The Plugin Description text box contains an explanation of what the plugin does.&nbsp; When Project Notes starts it looks through the "plugins" folder and loads all of the ".py" files into the [Python](<http://www.python.org>) interpreter.&nbsp; The Python files contain the information displayed on the Plugin Settings window.&nbsp; By default any new plugins found are set as enabled and stored in your personal operating system profile.&nbsp; In Windows, this is the registry and on MacOS and Linux systems this is a configuration file.

&nbsp;

**Events**

The Python script specifies which events it will handle.&nbsp; The table below describes the different events available.

&nbsp;

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


&nbsp;

&nbsp;

**Properties**

Each plugin can have it's own settings.&nbsp; Settings are stored in the registry on Windows computers and in a configuration file on MacOS and Linux computers.&nbsp; Settings values are stored as text. The settings in the [Global Settings](<GlobalSettings.md>) plugin are shared by multiple plugins.

&nbsp;

**To change a property:**

1. From Project Notes **Plugins** menu choose **Plugin Settings**.
1. Select the plugin from the list of **Loaded Plugins**.
1. Select the property text in the **Plugin Settings** grid.
1. Type in the property value.
1. Click **Close**.

&nbsp;

**To enable or disabled a plugin:**

1. From Project Notes **Plugins** menu choose **Loaded Plugin Settings**.
1. Select the plugin from the list of **Plugins**.
1. Check the **Enabled** check box to enable the plugin.&nbsp; Uncheck the **Enabled** check box to disable the plugin.
1. Type in the property value.
1. Click **Close**.

&nbsp;


***
_Created with the Personal Edition of HelpNDoc: [Easily create iPhone documentation](<https://www.helpndoc.com/feature-tour/iphone-website-generation>)_
