# Find Project Email

The **Find Project Email** plugin searches Microsoft Outlook for emails related to a project and a specific person. It is available on **Windows only** and requires Microsoft Outlook to be installed.

## How It Works

When invoked, the plugin reads the selected team member's or meeting attendee's email address and the project number from the current project. It then runs an Outlook search combining both values:

- **From:** the person's email address
- **Subject:** contains the project number

Outlook opens its search results view showing all matching emails. The plugin brings the Outlook window to the front automatically.

## Accessing the Plugin

The **Find Project Email** menu item appears in two right-click context menus:

| Where to right-click | Menu location |
| :--- | :--- |
| A row in the **Team** tab of a project | **Utilities > Find Project Email** |
| A row in the **Attendees** list of a meeting note | **Utilities > Find Project Email** |

## Requirements

- **Windows only** — the plugin uses Microsoft Outlook COM automation and is not available on macOS or Linux.
- Microsoft Outlook must be installed and configured with an active account.
- The selected person must have an **Email** address on record in Project Notes.
- The project must have a **Project Number** set.

## Related Documentation

- [Outlook Integration](<OutlookIntegration.md>) — Configure Office 365 and Outlook for email sending and contact sync
- [Project Page](<../InterfaceOverview/ProjectPage.md>) — Set up team members and Files & Folders
