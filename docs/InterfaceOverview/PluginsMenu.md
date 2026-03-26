# Plugins Menu

## Managing Python Scripts

The Plugins menu provides a way to view, execute, enable, and disable plugins. The Plugins menu as well as many other context menus can be customized by using a Python script. To learn how to customize Project Notes see [Adding Functionality](<../PluginsOverview/AddingFunctionality.md>).

All menu items below the divider line in the Plugins menu have been added by a Python script.

## Viewing Log Output

The **View Logs** option opens the Log Viewer window, which displays all log output written by Python plugins.

The Log Viewer displays one tab per log file. Each log file corresponds to a plugin or a logging category. When a plugin writes to the log, the Log Viewer updates automatically — you do not need to close and reopen it to see new output. If a new log file is created while the window is open, a new tab appears automatically.

The log text scrolls to the bottom as new output arrives. If you scroll up to review earlier output, the view stays at your scroll position and does not jump back to the bottom while you are reading.

**To clear the currently visible log:**

1. Click the tab for the log you want to clear.
2. Click **Clear Log**.

Clearing a log permanently deletes that log file. The tab is removed from the window. The log file will be recreated the next time the plugin writes output.

**To close the Log Viewer:**

1. Click **Close**, or close the window using the title bar.

## Understanding Plugin Settings

Each plugin can have its own settings. The typical method for a plugin to provide settings is through a submenu call **Settings** under the **Plugins** menu. See Python Scripting to learn more about about to write Python scripts.

