# Sending Meeting Notes

The **Send Meeting Notes** plugin emails formatted meeting notes to all attendees of a meeting immediately from the right-click context menu. It works on all platforms when **Office 365** is configured, or on Windows using **Outlook COM** automation. See [Outlook Integration](<OutlookIntegration.md>) for setup.

The email subject is automatically populated with the **Project Number**, **Project Name**, **Meeting Title**, and **Meeting Date**. This makes your communication consistent and allows recipients to quickly identify which meeting is being referenced. Because the subject always includes the **Project Number**, emails can be easily searched by project or archived using the [Project Email Archive](<ProjectEmailArchive.md>) plugin.

The email body is formatted in HTML and includes the **Title**, **Date**, **Attendees**, **Notes**, and **Action Items** from the meeting.

*Note: The Project Manager specified in [Preferences](<../InterfaceOverview/Preferences.md>) is never included in the recipient list.*

**To send meeting notes to all attendees:**

1. Right-click on the **Meeting** in the meeting list from the **Project Notes Panel**.
2. Choose **Send Meeting Notes**.

When using **Office 365**, the email is sent automatically via the Graph API. When using **Outlook COM** (Windows only):

3. Make **Outlook** the active application with Alt+Tab or by clicking the **Outlook** icon in the task bar.
4. Review and edit the email contents in **Outlook**.
5. Click **Send**.
