# Outlook Integration

The **Outlook Integration** plugin archives project email and, with Microsoft 365, publishes assigned items as Microsoft To Do tasks. Contact synchronization and **Full Contact Export** are built into [Preferences](../InterfaceOverview/Preferences.md#contact-synchronization), not this plugin.

The plugin does not send project email or schedule meetings. Set those up through [Email and Meeting Setup](../InterfaceOverview/EmailAndMeetingSetup.md).

## Connection choices

| Integration Type | Works on | Plugin features |
| :--- | :--- | :--- |
| **Office 365 Application** | Windows, macOS, Linux | Archive project email and sync assigned items through Microsoft Graph, on a five-minute timer or by menu action. |
| **Outlook Automation** | Windows with Outlook (classic) | Archive project email through Outlook. Menu action only. |

**Outlook Automation** is the default for a new installation. On macOS and Linux, choose **Office 365 Application** to use this plugin.

## Configure the plugin

Open **Plugins · Settings → Outlook Integration**. Choose the integration type, then set the **Application ID** and **Tenant ID** if you use Office 365 Application. The Microsoft Entra registration for that connection needs `Mail.ReadWrite` to archive email and `Tasks.ReadWrite` to publish assigned items. The plugin's account is separate from the built-in [Office 365 Integration](../InterfaceOverview/Office365Integration.md) account used for mail and calendar drafts, File Finder, and native contacts.

Set **Backup Emails** and its inbox and sent folder settings to choose which project messages to archive. Use **Sync ToDo Items with Due Dates** and **Sync ToDo Items without Due Dates** to control task publishing. **Send Emails using Office 365** applies only to the optional IFS Cloud report plugin; it does not change Project Notes' built-in email handoff.

Older plugin settings or dialogs may still show **Import Contacts** or **Export New Contacts**. Those controls no longer run contact actions. Use **Settings → Email, Contacts, and Calendar** instead. The native **Full Contact Export** checks for possible existing contacts and lets you update, skip, or deliberately create a new contact.

## Run manually

With **Office 365 Application**, choose **Plugins · Utilities → Sync Tracker Items** or right-click a project and choose **Download All Emails**. With **Outlook Automation**, use **Download Emails from Outlook** on a project. The Office 365 Application also runs its configured work every five minutes.

Archived messages go into the project's configured folder. Project Notes matches them by a project number in the subject. See [Project Page](../InterfaceOverview/ProjectPage.md) to set the **Project Folder**.

## Related documentation

- [Contact Synchronization](../InterfaceOverview/Preferences.md#contact-synchronization)
- [Office 365 Integration](../InterfaceOverview/Office365Integration.md)
- [Plugin Settings](PluginSettings.md)
