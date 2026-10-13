# Outlook Integration

The **Outlook Integration** plugin archives project email into project folders and, with Office 365, keeps your open tracker items in Microsoft To Do. Contact synchronization and **Full Contact Export** are built into [Preferences](../InterfaceOverview/Preferences.md#contact-synchronization), not this plugin, and it neither sends project email nor schedules meetings. Set those up through [Email and Meeting Setup](../InterfaceOverview/EmailAndMeetingSetup.md).

## Connection choices

| Integration | Works on | What it does |
| :--- | :--- | :--- |
| **Office 365 Application** | Windows, macOS, Linux | Archives project email and syncs tracker items through Microsoft Graph, every five minutes and on request. |
| **Outlook Application** | Windows with Outlook (classic) | Archives project email from Outlook, on request. |

## Configure the plugin

Open **Settings > Outlook Integration** and choose the integration.

Office 365 uses the account you sign in with under [Office 365 Integration](../InterfaceOverview/Office365Integration.md); there is no separate application or tenant to register. The plugin asks for two permissions on top of what Project Notes already uses: reading mail, to archive it, and reading and writing tasks, for To Do. The first time it needs them, sign in again under **Settings > Office 365 Integration** and approve them.

| Setting | What it does |
| :--- | :--- |
| **Sync my open tracker items that have a due date** | Publishes your open items with a due date to the To Do "Tasks" list. |
| **Sync my open tracker items without a due date** | The same for items with no due date. |
| **Archive project emails automatically** | Every few minutes, saves messages whose subject contains a project number into that project's folder. |
| **Inbox subfolder**, **Sent items subfolder** | Where in the project folder received and sent messages go, for example `Correspondence/Received`. |

"Your" items are the ones assigned to the project manager chosen in [Preferences](../InterfaceOverview/Preferences.md). Each task's title starts with the project and item number, so the plugin can update or remove it when the item changes or closes.

## Run it yourself

- **Plugins · Utilities > Sync Tracker Items** publishes every open item now (Office 365).
- Right-click a project and choose **Utilities > Download All Emails** to archive all of that project's mail (Office 365).
- With Outlook, right-click a project and choose **Utilities > Download Emails from Outlook**.

Office 365 saves messages as `.eml` files and Outlook as `.msg` files, named by date and subject. A message already in the folder is not downloaded again. Set the project's folder on the [Project Page](../InterfaceOverview/ProjectPage.md) **Files & Folders** tab.

Settings from the Python version of this plugin carry over the first time it starts. Its own Application ID and Tenant ID are no longer used.

## Related documentation

- [Contact Synchronization](../InterfaceOverview/Preferences.md#contact-synchronization)
- [Office 365 Integration](../InterfaceOverview/Office365Integration.md)
- [Plugin Settings](PluginSettings.md)
