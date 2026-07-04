# File Menu

## Cloud Sync Settings

Project Notes stores its database in your user profile. The database is created automatically on first launch.

Cloud sync is provided by a **Project Notes Pro** subscription. With a subscription, your data is backed up to Project Notes Pro hosting and kept in sync across all of your devices — including the [Project Notes Mobile](../Mobile/ProjectNotesMobile.md) app. There is no server to set up and no host address to enter: you simply sign in with your subscription account. Manage your subscription at [www.projectnotespro.com](https://www.projectnotespro.com).

**To configure cloud sync:**

1. From the Project Notes **File** menu choose **Cloud Sync Settings...**
2. Check **Sync all your devices and backup your data** to enable syncing.
3. Enter the **Username (Email)** and **Password** for your Project Notes Pro account.
4. Optionally enter an **Encryption Phrase** to encrypt your data before it leaves your machine. Leave blank to disable encryption. Be sure to keep your encryption phrase in a safe place — it cannot be recovered, and every device that syncs this data must use the same phrase.
5. Click **OK**.

The dialog shows your current **subscription status** and the **Project ID** your data is associated with. Your settings are saved immediately. If Project Notes cannot sign in with the information provided, a warning dialog will appear. You can correct the settings by returning to **File > Cloud Sync Settings...**.

For a full reference of all cloud sync features, settings, and troubleshooting, see [Cloud Sync](RemoteHost.md).

## Sync All

**Sync All** immediately synchronizes all project data with Project Notes Pro hosting. This is useful when you want to push or pull the latest changes without waiting for an automatic sync cycle.  In some rare instances records may not get pushed to the server, or pulled.  This option resets all sync flags and starts over.  It can take some time for the full database to sync.  The synchronization progress bar at the bottom right of the status window will show the overall sync progress.

**To sync all data:**

1. From the Project Notes **File** menu choose **Sync All**.

Project Notes will connect to Project Notes Pro hosting and synchronize all records. If the connection fails or cloud sync is not configured, a warning dialog will appear. You can configure cloud sync settings under **File > Cloud Sync Settings...**.

## Sync Status Bar

When cloud sync is active, a progress bar appears in the bottom-right corner of the status bar. It shows how much of the database has been synchronized with the cloud host.

- **While syncing** — the bar fills from left to right as records are pushed and pulled. Hovering over the bar shows a tooltip with the exact percentage complete and the number of records pending push and pull.
- **When complete** — the bar disappears automatically once the database is fully synchronized (100%).
- **When sync is disabled or not configured** — the bar is hidden.

If the bar remains visible for an extended period, it may indicate a slow connection or a large number of pending records. You can use **File > Sync All** to reset all sync flags and start over if records appear to be stuck.

## Searching The Database

Project Notes provides a powerful search tool that searches the entire database. Project Notes can quickly search for any text, and provide quick navigation to found items by double clicking the results. When Show Internal items is selected from the View menu, internal data will show in the search results.

**To search the database:**

1. If the Search Panel is not showing, from the Project Notes **File** menu choose **Search** or click the Search icon on the toolbar.
2. Next to the magnifying glass icon type in the text to search for.
3. Press the **Return** key.

**To open the record found in the search result:**

1. Double click the item in the Search Panel.

## Importing and Exporting Information

### Exporting data
Project Notes can export data to an XML file. You can select individual items or entire tables of information to export. To learn more about the XML format go to [Project Notes XML](../PluginsOverview/ProjectNotesXML.md).  If you are building your own plugin, exporting data to review the XML is the best way to understand how to exchange data with Project Notes.

**To export all projects, people, clients, or tracker items to XML:**

1. From the Project Notes **File** menu choose **XML Export...**
2. Choose the information type option you want to export.
3. Click **Export**.
4. Select the location to save the file.
5. Type in the file name without the extension.
6. Click **Save**.

**To export an individual item:**

1. Right-click on the item, and choose **XML Export...**
2. Click **Export**.
3. Select the location to save the file.
4. Type in the file name without the extension.
5. Click **Save**.

### Importing data

Project Notes can import a properly constructed XML file.  To learn more about the XML format go to [Project Notes XML](../PluginsOverview/ProjectNotesXML.md).

**To import information:**

1. From the Project Notes **File** menu choose **XML Import...**
2. Type in the full file name and path or click the three dots to Select the the import file and click **Open**.
3. Click **Import**.

## Preferences

# Setting Yourself Up As The Project Manager

Project Notes excludes specific groups when constructing communications. Some communications are intended for internal use while others are for internal and customer. By specifying the Project Manager and the Managing Company, Project Notes can identify the correct recipients.

**To specify the Project Manager and Managing Company:**

1. From the **File** Menu in **Project Notes**, select **Preferences...**
2. Select the **Project Manager** from the drop down.
3. Select the **Managing Company** from the drop down.
4. Click the **OK** button to save the settings.

## Exiting Project Notes

You can exit Project Notes by clicking the close button on the window frame, or choosing **Exit** from the File menu.
