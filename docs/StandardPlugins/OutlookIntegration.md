# Outlook Integration

Project Notes can send emails and schedule meetings using one of two methods. You configure which method to use in **Plugins > Settings > Outlook Integration**.

## Integration Types

### Office 365 Application (Graph API) — All Platforms

When **Integration Type** is set to **Office 365 Application**, Project Notes uses the Microsoft Graph API to send email and schedule meetings. This method works on Windows, macOS, and Linux.

Requirements:
- An **Azure Active Directory** app registration with the following permissions:
  - `Mail.Send` — for sending email
  - `Calendars.ReadWrite` — for scheduling meetings
  - `Contacts.ReadWrite` — for importing and exporting contacts
- The **Application ID** and **Tenant ID** entered in the Outlook Integration settings

When Project Notes first needs to authenticate, it opens a browser window for you to sign in with your Microsoft 365 account. The token is cached so you are not prompted on every action.

### Microsoft Outlook COM (Windows Only)

When **Integration Type** is set to **Outlook Automation**, Project Notes uses COM automation to control the local Outlook desktop application. This method is Windows-only and requires Microsoft Outlook to be installed.

When sending an email or scheduling a meeting, Outlook opens with a pre-populated draft. You review and send (or send directly) from Outlook.

> **Note:** Email sending honors the **Integration Type** selection — when **Outlook Automation** is selected, email is delivered through the local Outlook application even if you have **Office 365 Application** credentials configured for other features. Earlier 5.0.x builds had a bug in which email was sent through Graph regardless of this setting; that has been fixed in 5.0.1.

## Configuring Outlook Integration Settings

**To open the Outlook Integration settings:**

1. From the **Plugins** menu choose **Settings > Outlook Integration**.

The settings dialog contains the following fields:

| Setting | Description |
| :--- | :--- |
| **Integration Type** | Choose **Outlook Automation** to use the local Outlook desktop app via COM (Windows only), or **Office 365 Application** to use the Microsoft Graph API. |
| **Application ID** | The Azure AD application (client) ID for your app registration. Required for Office 365 Application. |
| **Tenant ID** | Your Azure AD tenant ID. Required for Office 365 Application. |
| **Import Contacts** | Enable to allow importing contacts from Outlook. |
| **Export New Contacts** | Enable to allow exporting new contacts to Outlook. |
| **Sync ToDo Items with Due Dates** | Enable to sync action items that have a due date as Outlook tasks. |
| **Sync ToDo Items without Due Dates** | Enable to also sync action items without a due date. |
| **Backup Emails** | Enable to archive project-related emails to the project folder. |
| **Backup Inbox Folder** | Folder name (or path) inside the inbox to archive from. |
| **Backup Sent Folder** | Folder name (or path) inside the sent items to archive from. |

## Email Recipient Groups

When sending an email or scheduling a meeting, Project Notes automatically determines recipients based on the context:

| Recipient Group | Who is included |
| :--- | :--- |
| **Full Project Team** | All team members on the project. |
| **Internal Project Team** | Only team members whose company matches the Managing Company in Preferences. |
| **Exclude Client** | All team members except those belonging to the client company. |
| **Only Client** | Only team members belonging to the client company. |
| **Receives Status** | Team members with the **Receive Status** flag checked on the project team. |
| **Individual** | A single recipient based on the right-clicked record. |

The project manager specified in [Preferences](<../InterfaceOverview/Preferences.md>) is never included as a recipient.

## Configuring Meeting and Email Types

Meeting invitation templates and email templates are managed in **Plugins > Settings > Meeting and Email Types**. Each row in the settings table defines one item that appears in either the **Schedule Meeting** or **Send Email** right-click sub-menu.

**To open Meeting and Email Types settings:**

1. From the **Plugins** menu choose **Settings > Meeting and Email Types**.

### Fields

| Field | Description |
| :--- | :--- |
| **Type** | `Email` — adds the entry to the **Send Email** sub-menu. `Meeting` — adds the entry to the **Schedule Meeting** sub-menu. |
| **Name** | The label shown in the sub-menu. |
| **Invitees** | Controls who receives the email or meeting invitation. See [Invitees Options](#invitees-options) below. |
| **Subject** | The subject line template. Supports [variable substitution](<PluginSettings.md#variable-replacement-in-plugins>) (e.g., `[$projects.project_number.1] [$projects.project_name.1]`). |
| **Template** | The body template for the email or meeting invitation. Supports variable substitution. Edited as rich text. |
| **Data Type** | Determines which right-click context shows this item. See [Data Types](#data-types) below. |

### Invitees Options

| Option | Who receives the email or meeting invitation |
| :--- | :--- |
| **Full Project Team** | All team members on the project. |
| **Internal Project Team** | Only team members whose company matches the Managing Company in Preferences. |
| **Exclude Client** | All team members except those belonging to the client company. |
| **Only Client** | Only team members belonging to the client company. |
| **Receives Status** | Team members with the **Receive Status** flag checked on the project team. |
| **Individual** | The single person associated with the right-clicked record. |
| **Attachment Only** | No recipients — used for email types that send an attachment without addressing it to specific people. |

### Data Types

The **Data Type** controls which table's right-click menu shows the item:

| Data Type | Where it appears |
| :--- | :--- |
| `projects` | Right-click on a project in the Project List. |
| `people` | Right-click on a person in the People List. |
| `project_people` | Right-click on a team member in the project team. |
| `project_locations` | Right-click on a file/location entry in the project. |
| `project_notes` | Right-click on a meeting or note in the Project Notes panel. |
| `meeting_attendees` | Right-click on an attendee in a meeting. |
| `item_tracker` | Right-click on a tracker item. |
| `item_tracker_updates` | Right-click on a tracker item update. |

### Default Types

Project Notes ships with the following types pre-configured:

**Meetings** (appear in the **Schedule Meeting** sub-menu):

| Name | Invitees | Subject Template |
| :--- | :--- | :--- |
| Project Kickoff | Full Project Team | `[$projects.project_number.1] [$projects.project_name.1] - Project Kickoff` |
| Project Kickoff (Internal) | Exclude Client | `[$projects.project_number.1] [$projects.project_name.1] - Project Kickoff (Internal)` |
| Project Status | Full Project Team | `[$projects.project_number.1] [$projects.project_name.1] - Project Status` |
| Project Status (Internal) | Exclude Client | `[$projects.project_number.1] [$projects.project_name.1] - Project Status (Internal)` |
| Lessons Learned | Full Project Team | `[$projects.project_number.1] [$projects.project_name.1] - Lessons Learned` |
| Lessons Learned (Internal) | Exclude Client | `[$projects.project_number.1] [$projects.project_name.1] - Lessons Learned (Internal)` |
| Proposal Working | Exclude Client | `[$projects.project_number.1] [$projects.project_name.1] - Proposal Working Meeting` |

Each meeting type has a pre-built agenda in its **Template** field. You can edit any template to match your organization's standards.

**Emails** (appear in the **Send Email** sub-menu):

| Name | Invitees | Data Type |
| :--- | :--- | :--- |
| Individual | Individual | `people` |
| Attendee | Individual | `meeting_attendees` |
| Team Member | Individual | `project_people` |
| Non-Client | Exclude Client | `projects` |
| Full Team | Full Project Team | `projects` |
| Internal Project Team | Internal Project Team | `projects` |
| Attachment | Attachment Only | `project_locations` |

## Outlook Contact Sync (Windows Only)

On Windows using Outlook COM, two utilities are available in **Plugins > Utilities**:

- **Export Contacts to Outlook** — Copies all people in Project Notes to your Outlook contacts.
- **Import Contacts from Outlook** — Imports contacts from Outlook into the Project Notes people list.
- **Download Emails from Outlook** — Archives project-related emails from Outlook to the project folder.

## Contact Export Behavior (Office 365)

When using the **Office 365 Application** integration, contact export adds new contacts to your Microsoft 365 account but does **not** overwrite existing contacts. If a contact already exists in your Microsoft 365 contacts, it is skipped — no fields are updated or merged. To update an existing contact, edit it directly in Outlook or your Microsoft 365 account.
