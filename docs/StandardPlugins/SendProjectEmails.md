# Sending Project Related Emails

The **Send Email** plugin automatically populates the subject of your email with the **Project Number** and **Project Name**. This makes your communication consistent and allows recipients to quickly identify which project is being referenced. Because every email subject includes the **Project Number**, emails can be searched by project or archived using the [Project Email Archive](<ProjectEmailArchive.md>) plugin.

The plugin works on all platforms when **Office 365** is configured, or on Windows using **Outlook COM** automation. See [Outlook Integration](<OutlookIntegration.md>) for setup.

Email recipients are automatically populated based on the item you right-clicked when calling the plugin. *Note: The Project Manager specified in [Preferences](<../InterfaceOverview/Preferences.md>) is never included in the recipient list.*

## Configuring Email Types

Send Email menu items are configured in **Plugins > Settings > Meeting and Email Types**. Each email type defines a name, subject template, body template, recipient group, and the data context it appears on (projects, project_people, meeting_attendees, etc.). This allows you to create customized email templates for different situations.

## Sending an Email

The right-click menu will display the email types applicable to the item you right-clicked. For example, right-clicking a project shows project-level email types; right-clicking a meeting attendee shows attendee-level types.

**To send a project email:**

1. Right-click on the item (project, team member, meeting, or attendee).
2. Choose the email type from the **Send Email** sub-menu.

When using **Office 365**, the email is drafted via the Graph API. When using **Outlook COM** (Windows only):

3. Make **Outlook** the active application with Alt+Tab or by clicking the **Outlook** icon in the task bar.
4. Review and edit the email in **Outlook**.
5. Click **Send**.
