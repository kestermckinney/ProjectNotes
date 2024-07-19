# Plugins Menu

## Managing Python Scripts

The Plugins menu provides a way to view, execute, enable, and disable plugins. The Plugins menu as well as many other context menus can be customized by using a Python script. To learn how to customize Project Notes see [Adding Functionality](<../PluginsOverview/AddingFunctionality.md>). The **View Console** option displays a windows that shows all console output from executing Python scripts.

All menu items below the divider line in the Plugins menu have been added by a Python script.

## Enabling and Disabling Plugins

Plugins may be enabled or disabled. You may find a plugin that loads on startup takes too long. You may also find that there are plugins you don't use. You can enable and disable plugins from the **Plugin Settings** dialog. Note: Plugin scripts are loaded when Project Notes loads. Disabling a script only disables any Events from being called and corresponding context menus from being displayed. The plugin script will still be loaded into memory.

**To enable or disable a plugin:**

1. From the **Plugins** menu choose **Plugin Settings...**.
2. Select the plugin from the list on the left side of the **Plugins Settings** dialog.
3. Check the **Enabled** box to enable the plugin, or unchecked the box to disable it.
4. Click **Apply**.

## Understanding Plugin Settings

Each plugin can have its own settings. When the plugin is called, the **Plugin Settings** name is a global variable that is passed to the Python script. The name must be a valid Python variable name. See Python Scripting to learn more about about to write Python scripts.

**To change a plugin setting:**

1. From the Plugins menu choose **Plugin Settings...**.
2. Select the plugin from the list on the left side of the **Plugins Settings** dialog.
3. Click the property setting value box in the **Plugin Settings:** property grid.
4. Type in the new property value.
5. Click **Apply**.