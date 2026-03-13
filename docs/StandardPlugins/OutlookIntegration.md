# Outlook Integration

Project Notes can send emails and schedule meetings using one of two methods. You configure which method to use in **Plugins > Settings > Outlook Integration**.

## Integration Types

### Office 365 Application (Graph API) — All Platforms

When **Integration Type** is set to **Office 365 Application**, Project Notes uses the Microsoft Graph API to send email and schedule meetings. This method works on Windows, macOS, and Linux.

Requirements:
- An **Azure Active Directory** app registration with `Mail.Send` and `Calendars.ReadWrite` permissions
- The **Client ID**, **Tenant ID**, and optionally **Client Secret** entered in the Outlook Integration settings

When Project Notes first needs to authenticate, it opens a browser window for you to sign in with your Microsoft 365 account. The token is cached so you are not prompted on every action.

### Microsoft Outlook COM (Windows Only)

When **Integration Type** is not set to **Office 365 Application**, Project Notes uses COM automation to control the local Outlook desktop application. This method is Windows-only and requires Microsoft Outlook to be installed.

When sending an email or scheduling a meeting, Outlook opens with a pre-populated draft. You review and send (or send directly) from Outlook.

## Configuring Outlook Integration Settings

**To open the Outlook Integration settings:**

1. From the **Plugins** menu choose **Settings > Outlook Integration**.

The settings dialog contains the following fields:

| Setting | Description |
| :--- | :--- |
| **Integration Type** | Choose **Office 365 Application** to use Graph API, or leave blank/other for Outlook COM (Windows). |
| **Send Email via O365** | Enable to send emails through Office 365. |
| **Schedule Meetings via O365** | Enable to schedule meetings through Office 365. |
| **Client ID** | The Azure AD application (client) ID for your app registration. |
| **Tenant ID** | Your Azure AD tenant ID. |
| **Client Secret** | Optional client secret for app-only auth flows. |

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

## Outlook Contact Sync (Windows Only)

On Windows using Outlook COM, two utilities are available in **Plugins > Utilities**:

- **Export Contacts to Outlook** — Copies all people in Project Notes to your Outlook contacts.
- **Import Contacts from Outlook** — Imports contacts from Outlook into the Project Notes people list.
- **Download Emails from Outlook** — Archives project-related emails from Outlook to the project folder.
