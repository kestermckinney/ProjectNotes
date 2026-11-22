# Archive Mail

The **Archive Mail** plugin saves project email into project folders. Contact synchronization and **Full Contact Export** are built into [Preferences](../InterfaceOverview/Preferences.md#contact-synchronization), not this plugin, and it neither sends project email nor schedules meetings. Set those up through [Email and Meeting Setup](../InterfaceOverview/EmailAndMeetingSetup.md).

Enable the plugin to archive automatically; disable it to stop.

## Connection choices

| Integration | Works on | What it does |
| :--- | :--- | :--- |
| **Office 365 Application** | Windows, macOS, Linux | Archives project email through Microsoft Graph, every five minutes and on request. |
| **Outlook Application** | Windows with Outlook (classic) | Archives project email from Outlook, on request. |

## Configure the plugin

Open **Settings > Archive Mail** and choose the integration.

Office 365 uses the account you sign in with under [Office 365 Integration](../InterfaceOverview/Office365Integration.md); there is no separate application or tenant to register. The plugin needs `Mail.Read`, which the `Mail.ReadWrite` permission from that sign-in already covers, so the app registration needs no extra permission. If you signed in before Project Notes asked for `Mail.ReadWrite`, the plugin asks you to sign in again under **Settings > Office 365 Integration**.

| Setting | What it does |
| :--- | :--- |
| **Inbox subfolder**, **Sent items subfolder** | Where in the project folder received and sent messages go, for example `Correspondence/Received`. |

With Office 365, the plugin saves messages whose subject contains a project number into that project's folder every few minutes.

## Run it yourself

- Right-click a project and choose **Utilities > Download All Emails** to archive all of that project's mail (Office 365).
- With Outlook, right-click a project and choose **Utilities > Download Emails from Outlook**.

Office 365 saves messages as `.eml` files and Outlook as `.msg` files, named by date and subject. A message already in the folder is not downloaded again. Set the project's folder on the [Project Page](../InterfaceOverview/ProjectPage.md) **Files & Folders** tab.

Settings from the Python version of this plugin carry over the first time it starts. Its own Application ID and Tenant ID are no longer used.

## Related documentation

- [Contact Synchronization](../InterfaceOverview/Preferences.md#contact-synchronization)
- [Office 365 Integration](../InterfaceOverview/Office365Integration.md)
- [Plugin Settings](PluginSettings.md)
