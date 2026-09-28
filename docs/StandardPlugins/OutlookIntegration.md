# Outlook Integration

The **Outlook Integration** plugin downloads project-related emails and synchronizes assigned items. Contact import and export have moved to native [Contact Synchronization](../InterfaceOverview/Preferences.md#contact-synchronization); use that preferences section for Outlook, Office 365, Thunderbird, or iCloud contacts. The plugin's remaining Microsoft 365 functions run on a background timer and offer manual menu actions.

It does **not** send project email and it does not schedule meetings. Those are built into Project Notes and are set up on the main **Settings** page. See [Email and Meeting Setup](<../InterfaceOverview/EmailAndMeetingSetup.md>).

## Two ways to connect

The **Integration Type** setting picks the route.

| Integration Type | Works on | What it can do |
| :--- | :--- | :--- |
| **Office 365 Application** | Windows, macOS, Linux | Everything below, through the Microsoft Graph API. Runs on a five-minute timer as well as from the menu. |
| **Outlook Automation** | Windows only, with Outlook (classic) installed | Download project email. Contact synchronization is configured in native Preferences. Menu items only; it does not sync assigned items. |

**Outlook Automation** is the default for a new installation. It has no effect on macOS or Linux, so choose **Office 365 Application** there.

## What it does

| Capability | Description |
| :--- | :--- |
| Import contacts | Copies contacts from Outlook into the Project Notes people list. |
| Export new contacts | Copies Project Notes people that do not already exist in your Outlook contacts. Existing contacts are skipped, never merged or overwritten. |
| Download project emails | Archives project-related email from configured mailbox folders into the project folder. |
| Sync assigned items | Publishes the project manager's active tracker and action items as Microsoft To Do tasks. **Office 365 Application** only. |

## Requirements for Office 365 Application

You need a Microsoft Entra app registration that permits these delegated permissions:

- `Contacts.Read` and `Contacts.ReadWrite` for importing and exporting contacts
- `Mail.ReadWrite` for archiving project email
- `Tasks.ReadWrite` for publishing assigned items
- `Mail.Send`, used only when the optional IFS Cloud report plugin emails a report with **Send Emails using Office 365** turned on

This registration is configured separately from the one used by the built-in email and meeting features. The two connections are independent, and signing in to one does not sign you in to the other.

## Configuring the settings

1. From the app menu choose **Plugins · Settings**, then **Outlook Integration**.

| Setting | Description |
| :--- | :--- |
| **Integration Type** | **Office 365 Application** or **Outlook Automation**. See [Two ways to connect](#two-ways-to-connect). |
| **Application ID** | The Microsoft Entra application (client) ID for your app registration. Office 365 Application only. |
| **Tenant ID** | Your Microsoft Entra tenant ID. Office 365 Application only. |
| **Import Contacts** | Import contacts from Outlook into the people list. |
| **Export New Contacts** | Export people who do not already exist as Outlook contacts. |
| **Sync ToDo Items with Due Dates** | Publish action items that have a due date as tasks. |
| **Sync ToDo Items without Due Dates** | Also publish action items without a due date. |
| **Backup Emails** | Archive project-related email into the project folder. |
| **Backup Inbox Folder** | The folder name or path inside the inbox to archive from. |
| **Backup Sent Folder** | The folder name or path inside sent items to archive from. |
| **Send Emails using Office 365** | Used only by the IFS Cloud report plugin. When on, and **Integration Type** is **Office 365 Application**, the plugin creates its email as a Microsoft 365 draft. When off, it opens the email in Outlook (classic), which works on Windows only. Project Notes' own email ignores this setting. |

With **Office 365 Application**, the first time the plugin needs to authenticate it opens a browser window for sign-in. The token is cached so you are not prompted on every action.

## Running it manually

With **Office 365 Application**, these entries appear under **Plugins · Utilities**:

- **Import Contacts from Office 365**
- **Export New Contacts to Office 365**
- **Sync Tracker Items**
- **Download All Emails**, available when you right-click a project

Between manual runs the plugin performs the same work on a five-minute timer.

With **Outlook Automation** on Windows, the entries are **Import Contacts from Outlook**, **Export Contacts to Outlook** and **Download Emails from Outlook**. They run only when you choose them.

## Archived email

Downloaded messages are written to the project's configured folder. See [Project Page](<../InterfaceOverview/ProjectPage.md>) for setting the **Project Folder** location. Messages are matched to a project by the project number appearing in the subject.

## Related Documentation

- [Email and Meeting Setup](<../InterfaceOverview/EmailAndMeetingSetup.md>) — Sending project email and scheduling meetings
- [iCloud Contacts](<iCloudContacts.md>) — The Apple equivalent for contact synchronization
- [Office 365 Integration](<../InterfaceOverview/Office365Integration.md>) — The separate native Microsoft connection
- [Plugin Settings](<PluginSettings.md>) — Where plugin settings are stored
