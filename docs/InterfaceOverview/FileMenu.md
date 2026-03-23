# File Menu

## Cloud Sync Settings

Project Notes stores its database you user profile. The database is created automatically on first launch.

If you want to sync your data with a cloud host, use **Cloud Sync Settings** to configure the connection.

**To configure cloud sync:**

1. From the Project Notes **File** menu choose **Cloud Sync Settings...**
2. Check **Sync data with cloud host** to enable syncing.
3. Choose a **Sync Host Type**:
   - **Self-Hosted PostgREST** — a PostgREST server you run yourself
   - **Supabase** — a hosted Supabase project
4. Enter the **Server URL**, **Username (Email)**, and **Password** for your host.
5. Optionally enter an **Encryption Phrase** to encrypt data at rest on the server. Leave blank to disable encryption.  Be sure to keep your encryption phrase in a safe place.  You cannot recover it.
6. If using Supabase, also enter the **Supabase Anon Key** from your Supabase Dashboard.
7. Click **OK**.

Your settings are saved immediately. If Project Notes cannot connect using the information provided, a warning dialog will appear. You can correct the settings by returning to **File > Cloud Sync Settings...**.

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
Project Notes can export data to an XML file. You can select individual items or entire tables of information to export. To learn more about the XML format go to [Project Notes XML](../PluginsOverview/ProjectNotesXML.md).

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
