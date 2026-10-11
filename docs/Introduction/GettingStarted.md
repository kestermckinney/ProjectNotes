# Getting Started

## Your Database

Project Notes automatically creates and opens a local database when it first launches. The database is stored in your user profile.

If you are developing or testing plugins and need a separate environment that does not touch your production data, launch Project Notes with `--developer-profile PROFILENAME`. If you also need cloud sync to hit the test Project Notes Pro environment instead of production while testing, add `--test-supabase` at the same time. See [Command-Line Options](<DeveloperProfile.md>) for both flags and what each isolates.

If you want to back up your data and sync it across your devices, configure cloud sync from the **Cloud Sync** section of Settings (click the **Settings** icon in the icon rail). See [Cloud Sync](../InterfaceOverview/RemoteHost.md) for a full feature reference.

## Cloud Sync Setup

Cloud sync lets you back up your Project Notes data and keep it synchronized across all of your devices including your desktop installations and the [Project Notes Mobile](<../Mobile/ProjectNotesMobile.md>) application automatically and in the background.

Cloud sync is provided by a **Project Notes Pro** subscription. The hosting is fully managed. There is no server to set up and no host address or API key to enter. You simply sign in with your subscription account.

**To enable cloud sync:**

1. Subscribe at [www.projectnotespro.com](https://www.projectnotespro.com).
2. Click the **Settings** icon in the icon rail and open the **Cloud Sync** section.
3. Check **Enable cloud sync**.
4. Enter the **Sync Email** and **Sync Password** for your Project Notes Pro account.
5. Optionally set an **Encryption Phrase** to encrypt your data before it leaves your machine. Keep it somewhere safe — it cannot be recovered, and every device must use the same phrase.

Each field saves as soon as you enter it — there is no separate OK or Save step.

See [Cloud Sync](<../InterfaceOverview/RemoteHost.md>) for a complete reference of all sync features, settings, and troubleshooting steps.

## Setting Yourself as the Project Manager

You will need to setup yourself to manage all of the projects. First you need to add yourself to the [Contacts List](<../InterfaceOverview/ContactsListPage.md>) and [Company List](<../InterfaceOverview/CompaniesListPage.md>). Then setup the Project Notes [Preferences](<../InterfaceOverview/Preferences.md>).

**To setup Project Notes:**

1. Click the **Companies** icon in the icon rail.
2. Click **+ New** in the bar above the list.
3. Type in ***"Your Company Name"*** in the new row.
4. Click the **Contacts** icon in the icon rail.
5. Click **+ New** in the bar above the list.
6. Type your **Name, Email**, and **Role** in the new row.
7. Choose **Preferences** from the app menu (the menu icon at the top of the icon rail).
8. Select yourself as the **Project Manager**.
9. Select ***"Your Company Name"*** as the **Managing Company**.
10. Still in **Settings**, open **Email, Contacts, and Calendar** and choose your **Integration**.
11. Open **Settings > Meeting Templates** and review the standard meeting types, such as status meetings and kickoffs. Customize their subjects, agendas and durations, or add your own types.
12. Open **Settings > File Finder** and add the root folder where your project files are stored.

Steps 10 and 11 are the start of email and meeting setup. See [Email and Meeting Setup](<../InterfaceOverview/EmailAndMeetingSetup.md>) for the rest, including what to configure on Windows, macOS and Linux.

## Populating Your Database

Project Notes is capable of connecting information from various systems. In order to benefit from these capabilities, you will need to consistently name your companies, contacts, and project numbers. The first step in populating your database is importing companies and contacts.

There are three methods to import companies and contacts.

* **Importing From Outlook** - The Outlook Integration plugin imports and exports contacts through Microsoft 365 on any platform, or directly from Outlook (classic) on Windows. See [Outlook Integration](<../StandardPlugins/OutlookIntegration.md>) for details.
* **Importing From XML** - All database items can be imported from a [Project Notes XML](<../PluginsOverview/ProjectNotesXML.md>) file. Choose **Import XML…** from the app menu (the menu icon at the top of the icon rail) to import a file.
* **Entering Manually** - Company information can be entered using the [Companies List Page](<../InterfaceOverview/CompaniesListPage.md>), and contacts can be entered using the [Contacts List Page](<../InterfaceOverview/ContactsListPage.md>).

## Project Notes Settings

Once your companies and contacts are setup, you will need to tell Project Notes which contact is the Project Manager coordinating all projects, and what company they work for. This information can be set up by clicking the **Settings** icon in the icon rail — see [Project Notes Preferences](<../InterfaceOverview/Preferences.md>).

## Email, Meetings and Reports

Sending meeting notes, sending project reports and scheduling meetings are built into Project Notes. They are configured on the main **Settings** page, under **Preferences**, **Email Templates** and **Meeting Templates**. Work through [Email and Meeting Setup](<../InterfaceOverview/EmailAndMeetingSetup.md>) once, then see [Reports and Email](<../InterfaceOverview/ReportsAndEmail.md>) and [Scheduling Meetings](<../InterfaceOverview/ScheduleMeeting.md>).

## Plugin Settings

Project Notes plugins are configurable through dialogs under **Plugins > Settings**. Application services use the main **Settings** page instead: File Finder and Office 365 Integration have their own sections there. See [Plugin Settings](<../StandardPlugins/PluginSettings.md>), [File Finder](<../InterfaceOverview/FileFinder.md>), and [Office 365 Integration](<../InterfaceOverview/Office365Integration.md>) for configuration details.

Plugin settings are stored in your local OS profile (registry on Windows, plist on macOS, ini on Linux) and are not synced to the cloud host. If you move to a new machine, use the **Settings Migrator** (under **Plugins > Settings**) to transfer your configuration.

## Populating Your Project Data And Plugins

Once you have configured Project Notes you are ready to start populating project data. Your edition of Project Notes may include custom plugins specific to your organization. These plugins may import project information from other enterprise systems within your organization.
