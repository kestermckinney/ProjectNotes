# Plugins Menu

# Managing Python Scripts

The Plugins menu provides a way to view, execute, enable, and disable plugins.&nbsp; The Plugins menu as well as many other context menus can be customized by using a Python script.&nbsp; To learn how to customize Project Notes see [Adding Functionality](<AddingFunctionality.md>).&nbsp; The **View Console** option displays a windows that shows all console output from executing Python scripts.

&nbsp;

All menu items below the divider line in the Plugins menu have been added by a Python script.

&nbsp;

**Enabling and Disabling Plugins**

Plugins may be enabled or disabled.&nbsp; You may find a plugin that loads on startup takes too long.&nbsp; You may also find that there are plugins you don't use.&nbsp; You can enable and disable plugins from the **Plugin Settings** dialog.&nbsp; Note: Plugin scripts are loaded when Project Notes loads.&nbsp; Disabling a script only disables any Events from being called and corresponding context menus from being displayed.&nbsp; The plugin script will still be loaded into memory.

&nbsp;

**To enable or disable a plugin:**

1. From the **Plugins** menu choose **Plugin Settings...**.
1. Select the plugin from the list on the left side of the **Plugins Settings** dialog.
1. Check the **Enabled** box to enable the plugin, or unchecked the box to disable it.
1. Click **Apply**.

# Understanding Plugin Settings

Each plugin can have its own settings.&nbsp; When the plugin is called, the **Plugin Settings** name is a global variable that is passed to the Python script.&nbsp; The name must be a valid Python variable name.&nbsp; See Python Scripting to learn more about about to write Python scripts.

&nbsp;

**To change a plugin setting:**

1. From the Plugins menu choose **Plugin Settings...**.
1. Select the plugin from the list on the left side of the **Plugins Settings** dialog.
1. Click the property setting value box in the **Plugin Settings:** property grid.
1. Type in the new property value.
1. Click **Apply**.

&nbsp;


***
_Created with the Personal Edition of HelpNDoc: [Don't be left in the past: convert your WinHelp HLP help files to CHM with HelpNDoc](<https://www.helpndoc.com/step-by-step-guides/how-to-convert-a-hlp-winhelp-help-file-to-a-chm-html-help-help-file/>)_
