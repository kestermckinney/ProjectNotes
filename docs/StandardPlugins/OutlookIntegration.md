# Outlook Integration

The **Outlook Integration** plugin keeps your Project Notes people, emails and assigned items in step with a Microsoft 365 mailbox. It runs on a timer in the background and also adds menu items you can trigger yourself.

It does **not** send project email and it does not schedule meetings. Those are built into Project Notes and are set up on the main **Settings** page. See [Email and Meeting Setup](<../InterfaceOverview/EmailAndMeetingSetup.md>).

## What it does

| Capability | Description |
| :--- | :--- |
| Import contacts | Copies contacts from your Microsoft 365 account into the Project Notes people list. |
| Export new contacts | Copies Project Notes people that do not already exist in your Microsoft 365 contacts. Existing contacts are skipped, never merged or overwritten. |
| Download project emails | Archives project-related email from configured mailbox folders into the project folder. |
| Sync assigned items | Publishes the project manager's active tracker and action items as Microsoft To Do tasks. |

## Requirements

Everything runs through the Microsoft Graph API, so it works on Windows, macOS and Linux. You need a Microsoft Entra app registration that permits these delegated permissions:

- `Contacts.Read` and `Contacts.ReadWrite` for importing and exporting contacts
- `Mail.ReadWrite` for archiving project email
- `Tasks.ReadWrite` for publishing assigned items
- `MailboxSettings.Read`

The plugin's own sign-in also requests `Mail.Send` and `Calendars.ReadWrite`, which are left over from earlier plugin features. Project email and meetings no longer go through it.

This registration is configured separately from the one used by the built-in email and meeting features. The two connections are independent, and signing in to one does not sign you in to the other.

## Configuring the settings

1. From the app menu choose **Plugins · Settings**, then **Outlook Integration**.

| Setting | Description |
| :--- | :--- |
| **Integration Type** | Set this to **Office 365 Application**. The plugin's menu items only appear when it is, because all of its work now goes through Microsoft Graph. |
| **Application ID** | The Microsoft Entra application (client) ID for your app registration. |
| **Tenant ID** | Your Microsoft Entra tenant ID. |
| **Import Contacts** | Import contacts from Microsoft 365 into the people list. |
| **Export New Contacts** | Export people who do not already exist as Microsoft 365 contacts. |
| **Sync ToDo Items with Due Dates** | Publish action items that have a due date as tasks. |
| **Sync ToDo Items without Due Dates** | Also publish action items without a due date. |
| **Backup Emails** | Archive project-related email into the project folder. |
| **Backup Inbox Folder** | The folder name or path inside the inbox to archive from. |
| **Backup Sent Folder** | The folder name or path inside sent items to archive from. |

The first time the plugin needs to authenticate it opens a browser window for sign-in. The token is cached so you are not prompted on every action.

## Running it manually

With the connection configured, these entries appear under **Plugins · Utilities**:

- **Import Contacts from Office 365**
- **Export New Contacts to Office 365**
- **Sync Tracker Items**
- **Download All Emails**, available when you right-click a project

Between manual runs the plugin performs the same work on a five-minute timer.

## Archived email

Downloaded messages are written to the project's configured folder. See [Project Page](<../InterfaceOverview/ProjectPage.md>) for setting the **Project Folder** location. Messages are matched to a project by the project number appearing in the subject.

## Related Documentation

- [Email and Meeting Setup](<../InterfaceOverview/EmailAndMeetingSetup.md>) — Sending project email and scheduling meetings
- [iCloud Contacts](<iCloudContacts.md>) — The Apple equivalent for contact synchronization
- [Office 365 Integration](<../InterfaceOverview/Office365Integration.md>) — The separate native Microsoft connection
- [Plugin Settings](<PluginSettings.md>) — Where plugin settings are stored
