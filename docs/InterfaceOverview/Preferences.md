# Preferences

The Project Notes Settings page allows you to configure application-wide settings that affect how Project Notes operates and communicates with team members and clients.

## Accessing Preferences

**To open the Settings page:**

1. Click the **Settings** icon in the icon rail, or choose **Preferences** from the app menu (the menu icon at the top of the icon rail).
2. The Settings page opens with sections for Appearance, Cloud Sync, Project Folders, Preferences, View Options, Data, and About.

## Appearance

The **Appearance** section at the top of Settings controls the app's theme: **System**, **Light**, or **Dark**. **System** follows your operating system's current theme and switches automatically if it changes. Click a button to switch immediately — there is nothing to save. The same choice, as a quick two-way toggle, is also available as **Dark Mode** in the [Application Menu](<ApplicationMenu.md>)'s **View** group.

## Project Manager

The **Project Manager** setting, under the **Preferences** section of Settings, identifies you as the primary project manager for your organization. This setting is used to:

- **Exclude you from communications** — When sending emails or generating reports, Project Notes may exclude the project manager from recipient lists to avoid sending information to yourself
- **Track meeting attendance** — Some meeting scheduling features use this setting to determine if you should be included in meeting invitations
- **Filter report recipients** — Ensures that internal communications don't accidentally include the wrong people

**To set the Project Manager:**

1. Open **Settings** and find the **Preferences** section.
2. Click the **Project Manager** drop down.
3. Select your name from the list of people in your database — the setting saves immediately.

**Note:** The project manager list is populated from the **People** data in your database. If you don't see your name in the list, add yourself as a person first (see [People List Page](PeopleListPage.md)).

## Managing Company

The **Managing Company** setting, also under the **Preferences** section of Settings, specifies your organization or company. This setting is used to:

- **Identify internal communications** — Project Notes distinguishes between internal-only communications and communications that include clients
- **Exclude your company from client communications** — When generating client-facing documents, your company information may be filtered differently than client information
- **Organize team members** — Helps Project Notes understand the structure of your organization vs. client organizations

**To set the Managing Company:**

1. Open **Settings** and find the **Preferences** section.
2. Click the **Managing Company** drop down.
3. Select your company name from the list of clients in your database — the setting saves immediately.

**Note:** The managing company list is populated from the **Clients** data in your database. If you don't see your company in the list, add it as a client first (see [Client List Page](ClientListPage.md)).

## View Options

The **View Options** section holds three checkboxes that control what the Projects, Items, and Search lists show. Each takes effect immediately:

- **Show closed projects** — Shows or hides projects whose **Project Status** is **Closed** on the [Project List Page](<ProjectListPage.md>).
- **Show internal items** — Shows or hides notes and tracker items marked **Internal**, and the earned value/budget figures on the Project List and Project Page. See [Presenting to Clients](<PresentingToClients.md>).
- **Only New and Assigned tracker items** — When checked, hides tracker items whose **Status** is **Resolved**, **Deferred**, or **Canceled**, on the [Item Tracker Page](<ItemTrackerPage.md>).

These same three toggles are also reachable from the [Application Menu](<ApplicationMenu.md>)'s **View** group as **Show Closed Projects**, **Show Internal / Budget Items**, and **Show Resolved Items** — the two locations stay in sync.

## Data

The **Data** section imports records from a [Project Notes XML](<../PluginsOverview/ProjectNotesXML.md>) file.

**To import from XML:**

1. Open **Settings** and go to the **Data** section.
2. Click **Import from XML…**.
3. Choose the `.xml` file and click **Open**.

Export is handled separately, per record — open any project, note, item, person, or client and use its **Export** button (or **Export XML…** in the [Application Menu](<ApplicationMenu.md>)) to export just that record.

## About

The **About** section shows the installed version and links to the online documentation, release notes, and source code. It is also reachable by choosing **About** from the [Application Menu](<ApplicationMenu.md>)'s **Help** group.

## Checking for Updates

Project Notes can keep itself up to date by checking GitHub for newer releases. A quiet, background check runs automatically each time Project Notes starts. If a newer release is found, it offers to download and install it for you; the installer runs automatically and Project Notes relaunches itself when it finishes. When no update is available, the background check stays silent so it never interrupts your work.

You can also check at any time from the app menu by choosing **Check for Updates…**. A manual check always reports its result — including telling you when you are already running the latest version.

## How Preferences Are Used

### In Communications

When you use plugins to send emails or schedule meetings, Project Notes uses your preferences to:

- **Determine recipients** — The project manager is often excluded from internal emails to avoid duplicating communication
- **Filter content** — Internal items and notes marked as "Internal" may be excluded from client-facing communications based on your preferences
- **Personalize messages** — Your project manager setting may be used to automatically populate your name in email signatures or meeting invitations

### In Reports

When generating reports (such as meeting minutes or tracker item reports), Project Notes:

- **Filters recipients** — Excludes the project manager from the "Send to" list by default
- **Marks internal items** — Uses the managing company setting to identify which notes and items are internal vs. client-facing

### In Filtering

When you use the [Filter Tool](FilterTool.md) or the **Show Internal / Budget Items** toggle:

- **Client filtering** — You can filter data by the managing company to see only your organization's data
- **Internal item visibility** — The "Show Internal / Budget Items" option works in conjunction with your project manager setting to determine what should be hidden when presenting to clients

## Where Preferences Are Saved

The **Project Manager** and **Managing Company** settings are saved in the `application_settings` table inside the Project Notes database (`ProjectNotes.db`). This means they travel with the database — if you share a database or copy it to another machine, your preferences move with it. They are also included in cloud sync when synchronization is enabled.

See [Plugin Settings — Where Settings Are Stored](<../StandardPlugins/PluginSettings.md>) for a complete breakdown of what is stored in the database versus your local OS profile across all features and plugins.

## Related Documentation

- [People List Page](PeopleListPage.md) — Manage people in your database
- [Client List Page](ClientListPage.md) — Manage clients/companies in your database
- [Presenting to Clients](PresentingToClients.md) — How to use preferences when presenting to clients
