# Preferences

The Project Notes Preferences dialog allows you to configure application-wide settings that affect how Project Notes operates and communicates with team members and clients.

## Accessing Preferences

**To open the Preferences dialog:**

1. From the **File** menu, choose **Preferences...** (or press **Preferences** button if available)
2. The Preferences dialog will open with all available settings

## Project Manager

The **Project Manager** setting identifies you as the primary project manager for your organization. This setting is used to:

- **Exclude you from communications** — When sending emails or generating reports, Project Notes may exclude the project manager from recipient lists to avoid sending information to yourself
- **Track meeting attendance** — Some meeting scheduling features use this setting to determine if you should be included in meeting invitations
- **Filter report recipients** — Ensures that internal communications don't accidentally include the wrong people

**To set the Project Manager:**

1. Open the **Preferences** dialog from the **File** menu
2. Click the **Project Manager** dropdown
3. Select your name from the list of people in your database
4. Click **OK**

**Note:** The project manager list is populated from the **People** data in your database. If you don't see your name in the list, add yourself as a person first (see [People List Page](PeopleListPage.md)).

## Managing Company

The **Managing Company** setting specifies your organization or company. This setting is used to:

- **Identify internal communications** — Project Notes distinguishes between internal-only communications and communications that include clients
- **Exclude your company from client communications** — When generating client-facing documents, your company information may be filtered differently than client information
- **Organize team members** — Helps Project Notes understand the structure of your organization vs. client organizations

**To set the Managing Company:**

1. Open the **Preferences** dialog from the **File** menu
2. Click the **Managing Company** dropdown
3. Select your company name from the list of clients in your database
4. Click **OK**

**Note:** The managing company list is populated from the **Clients** data in your database. If you don't see your company in the list, add it as a client first (see [Client List Page](ClientListPage.md)).

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

When you use the [Filter Tool](FilterTool.md) or [View Menu](ViewMenu.md) filtering options:

- **Client filtering** — You can filter data by the managing company to see only your organization's data
- **Internal item visibility** — The "Show Internal Items" option works in conjunction with your project manager setting to determine what should be hidden when presenting to clients

## Where Preferences Are Saved

The **Project Manager** and **Managing Company** settings are saved in the `application_settings` table inside the Project Notes database (`ProjectNotes.db`). This means they travel with the database — if you share a database or copy it to another machine, your preferences move with it. They are also included in cloud sync when synchronization is enabled.

See [Plugin Settings — Where Settings Are Stored](<../StandardPlugins/PluginSettings.md>) for a complete breakdown of what is stored in the database versus your local OS profile across all features and plugins.

## Related Documentation

- [File Menu](FileMenu.md) — Access Preferences from the File menu
- [People List Page](PeopleListPage.md) — Manage people in your database
- [Client List Page](ClientListPage.md) — Manage clients/companies in your database
- [Presenting to Clients](PresentingToClients.md) — How to use preferences when presenting to clients
