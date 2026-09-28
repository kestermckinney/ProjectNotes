# Preferences

The Project Notes Settings page allows you to configure application-wide settings that affect how Project Notes operates and communicates with team members and clients.

## Accessing Preferences

**To open the Settings page:**

1. Click the **Settings** icon in the icon rail, or choose **Preferences** from the app menu (the menu icon at the top of the icon rail).
2. The Settings page opens with sections for Appearance, Cloud Sync, Office 365 Integration, Email Templates, Meeting Templates, File Finder, Project Folders, Preferences, View Options, and Data.

## Related Settings Pages

This page covers Appearance, Project Manager/Managing Company, View Options,
Data, and About. A few other Settings-page sections are substantial enough to
have their own documentation:

- [Cloud Sync](<RemoteHost.md>) — Sync your database with Project Notes Pro
- [Project Folders](<ProjectFolders.md>) — Configure per-project folder locations
- [File Finder](<FileFinder.md>) — Native automatic project file discovery
- [Office 365 Integration](<Office365Integration.md>) — Native Microsoft 365 sign-in used by File Finder

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

## View Report After Save

**View report after save**, at the top of the **Preferences** section, opens a saved report in its default application as soon as it is written. Leave it off if you only want the file on disk. It has no effect on emailed reports.

## Contact Synchronization

Contact synchronization is part of **Preferences → Email, Contact and Calendar Integration** and follows its **Integration** selection. Thunderbird uses Thunderbird contacts; Outlook (Classic) uses classic Outlook contacts; Outlook and Microsoft 365 drafts use the shared Office 365 account; MacOS Email and Calendar uses iCloud. Default Application does not provide contact synchronization. There is no separate contact-source selector.

- **Outlook** is offered only on Windows and reads classic Outlook's active profile. New Outlook and other platforms use Office 365 for Microsoft cloud contacts.
- **Office 365** uses the shared account configured under **Office 365 Integration**. Enable contact synchronization here, then sign in from that tab to grant Contacts.Read. After enabling export, sign in there again to grant Contacts.ReadWrite. Preferences has no separate Microsoft sign-in.
- **Thunderbird** requires Thunderbird 128 or newer, the bundled Project Notes Contacts add-on, and the native host. Use **Set up Thunderbird bridge**, then install `projectnotes-contacts.xpi` from the application folder through Thunderbird's Add-ons menu and restart Thunderbird. Only local writable books are export destinations. Sandboxed Thunderbird needs access to the native host inside its sandbox; ordinary desktop installations are supported directly.
- **iCloud** uses your Apple Account and an app-specific password saved in your operating system credential store. A blank password leaves the stored password unchanged.

Use **Test Connection** to discover books without importing or exporting. Select import books, or leave all unchecked to import all books. Enable **Export missing contacts** to create missing Project Notes contacts directly in the selected application—no destination selection is needed. Outlook and Office 365 use their default Contacts folder. Thunderbird and iCloud use a provider-designated default writable address book when available, otherwise the first writable book ordered by stable ID (not display name). Thunderbird exports only to local writable books. If no writable book is available, imports continue and export reports the problem. Duplicate checking covers all discovered books, including books not selected for import. New configurations start paused, with export disabled and a five-minute interval. The interval can be changed from 1 to 1,440 minutes; startup sync is optional.

Contact synchronization runs on a dedicated background thread, including matching and database writes. Settings and review actions remain in the main interface; progress and refreshed records are delivered back to it safely.

The source's supplied values replace corresponding Project Notes fields, including explicit empty values. Omitted fields are retained. Remote deletions never delete local records, and deleted linked local people are never restored. Optional export creates missing contacts; it does not update existing external contacts.

Names are compared using normalization, token reordering, edit distance, and company legal suffix variants. Saved source identities take precedence. Strong matches reuse existing IDs. **Review Contact Conflicts** enables when uncertain clients or contacts need attention and opens a separate review dialog. First resolve duplicate clients: choose **Use Selected Client** to associate the source company with an existing client, or **Keep as a Separate Client** to create a distinct client. Each company choice applies to all its source contacts and is saved for later synchronization. Once all client choices are resolved, contact matching resumes using those clients. Uncertain contacts then wait before people are created: choose **Use This One** under either contact, or **Use Both** to retain the existing person and import a separate person. Both sides show name, company, email, and phone details. The client association follows the saved client choice, and the display name comes from the chosen contact; when keeping both identical names, a unique display name is derived automatically from the source company. Existing duplicate records are not merged. Review includes the external fields that will be applied.

An export whose outcome is uncertain is not retried automatically. After a fresh sync, check the destination address book before choosing **I checked the destination; allow retry**. Local state and credentials are scoped to this computer and developer profile; moving a database may require matching its contacts again.

Legacy contact actions have moved out of Python plugins. Existing iCloud account settings and valid links are carried forward where available, but synchronization stays paused until enabled here. Other plugin functions remain available.

## Email, Contact and Calendar Integration

The **Email, Contact and Calendar Integration** section, below **Preferences**, decides how Project Notes hands finished emails and meeting invitations to you. One choice covers email, contacts, and calendars.

The **Integration** list only offers what your operating system supports:

| Platform | Choices in the list |
| :--- | :--- |
| **Windows** | Default Application, Outlook, Outlook (Classic), Thunderbird, Microsoft 365 drafts |
| **macOS** | Default Application, MacOS Email and Calendar, Thunderbird, Microsoft 365 drafts |
| **Linux** | Default Application, Thunderbird, Microsoft 365 drafts |

| Integration | What happens |
| :--- | :--- |
| **Default Application** | The email is written as a draft message file (`.eml`) and opened by whatever application handles that type. Meetings open as a calendar file (`.ics`) in your default calendar application. |
| **Outlook** (Windows) | The email and the meeting open in the new Outlook, whichever app holds the `.eml` and `.ics` associations. The new Outlook drops a meeting's attendees, so use **Copy Attendees** in the Schedule meeting window and paste them into the event. |
| **Outlook (Classic)** (Windows) | The email opens in Outlook (classic) as an unsent message. The meeting opens as an unsent meeting window with the attendees and agenda already filled in. |
| **MacOS Email and Calendar** (macOS) | The email opens in a Mail compose window without Bcc recipients or attachments. The meeting becomes an event in Calendar without attendees; use **Copy Attendees** in the Schedule meeting window and paste them into the event. |
| **Thunderbird** | The email opens in a Thunderbird compose window with its attachments. The meeting is passed to Thunderbird, which offers to import it into one of its calendars. |
| **Microsoft 365 drafts** | The email becomes a draft in your Microsoft 365 mailbox and the meeting a draft event in your calendar, both opened in Outlook on the web. Sign in under **Settings > Office 365 Integration** first. |

Until you choose, Project Notes uses **MacOS Email and Calendar** on macOS and **Default Application** everywhere else. A settings file copied from another computer can name a choice this platform lacks, such as **Outlook (Classic)** on a Mac. Project Notes ignores it and uses the default until you pick again.

Choosing **Thunderbird** reveals a **Thunderbird executable** field. Enter a path or a command, such as `flatpak run org.mozilla.Thunderbird`.

Project Notes never sends an email or an invitation. It prepares one and hands it over; you send it.

Temporary messages, attachments and calendar files live in the application cache and are removed when Project Notes closes.

For the full walk-through, including what to set up on Windows, macOS and Linux, see [Email and Meeting Setup](<EmailAndMeetingSetup.md>).

## View Options

The **View Options** section holds three checkboxes that control what the Projects, Items, and Search lists show. Each takes effect immediately:

- **Show closed projects** — Shows or hides projects whose **Project Status** is **Closed** on the [Project List Page](<ProjectListPage.md>).
- **Show internal items** — Shows or hides notes and tracker items marked **Internal**, and the earned value/budget figures on the Project List and Project Page. See [Presenting to Clients](<PresentingToClients.md>).
- **Show resolved items** — When cleared, hides tracker items whose **Status** is **Resolved**, **Deferred**, or **Canceled**, on the [Item Tracker Page](<ItemTrackerPage.md>), leaving only **New** and **Assigned** items.

These same three toggles are also reachable from the [Application Menu](<ApplicationMenu.md>)'s **View** group as **Show Closed Projects**, **Show Internal / Budget Items**, and **Show Resolved Items** — the two locations stay in sync.

## Data

The **Data** section imports records from a [Project Notes XML](<../PluginsOverview/ProjectNotesXML.md>) file.

**To import from XML:**

1. Open **Settings** and go to the **Data** section.
2. Click **Import from XML…**.
3. Choose the `.xml` file and click **Open**.

Export is handled separately, per record — open any project, note, item, person, or client and use its **Export** button (or **Export XML…** in the [Application Menu](<ApplicationMenu.md>)) to export just that record.

## About

Choosing **About** from the [Application Menu](<ApplicationMenu.md>)'s **Help** group shows the installed version and links to the online documentation, release notes, and source code.

## Checking for Updates

Project Notes can keep itself up to date by checking GitHub for newer releases. A quiet, background check runs automatically each time Project Notes starts. If a newer release is found, it offers to download and install it for you; the installer runs automatically and Project Notes relaunches itself when it finishes. When no update is available, the background check stays silent so it never interrupts your work.

You can also check at any time from the app menu by choosing **Check for Updates…**. A manual check always reports its result — including telling you when you are already running the latest version.

## How Preferences Are Used

### In Communications

When you send a report or schedule a meeting, Project Notes uses your preferences to:

- **Determine recipients** — The project manager is kept off recipient lists to avoid sending information to yourself
- **Name the organizer** — Meeting invitations list the project manager as the organizer, never as an attendee
- **Filter content** — Items and notes marked **Internal** are excluded from client-facing reports
- **Fill in templates** — `preferences.managerName` and `preferences.managingCompanyName` resolve from these two settings

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

The **Email, Contact and Calendar Integration** choice and the Thunderbird path are stored in your local OS profile instead, so each machine can use a different mail application. See [Plugin Settings — Where Settings Are Stored](<../StandardPlugins/PluginSettings.md>) for the full breakdown.

## Related Documentation

- [People List Page](PeopleListPage.md) — Manage people in your database
- [Client List Page](ClientListPage.md) — Manage clients/companies in your database
- [Presenting to Clients](PresentingToClients.md) — How to use preferences when presenting to clients
- [Email and Meeting Setup](<EmailAndMeetingSetup.md>) — Setting up mail and calendar handoff
- [Reports and Email](<ReportsAndEmail.md>) — Generating and sending project reports
