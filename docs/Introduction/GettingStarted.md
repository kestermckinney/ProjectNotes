# Getting Started

## Your Database

Project Notes automatically creates and opens a local database when it first launches. The database is stored in your user profile.

If you are developing or testing plugins and need a separate environment that does not touch your production data, see [Developer Profile](<DeveloperProfile.md>).

If you want to sync your data with a cloud host, configure the connection from **File > Cloud Sync Settings...**. See [File Menu](../InterfaceOverview/FileMenu.md) for details and [Remote Host](../InterfaceOverview/RemoteHost.md) for a full feature reference.

## Cloud Sync Setup

Cloud sync lets you access your Project Notes data from multiple machines and keep everything in sync automatically. Project Notes uses a [PostgREST](https://postgrest.org) API layer over a PostgreSQL database for all cloud sync operations. You have three hosting options:

| Option | Best For |
| :--- | :--- |
| **Supabase** | Easiest setup — managed PostgreSQL + PostgREST with a free tier |
| **Neon + PostgREST** | Serverless Postgres backend with a self-managed PostgREST layer |
| **Self-Hosted PostgREST** | Full control — run PostgreSQL and PostgREST on your own server or VM |

All three options use the same connection settings in Project Notes (**File > Cloud Sync Settings...**). Choose the option that matches your infrastructure preferences:

- [Setting Up Supabase](<CloudSync_Supabase.md>)
- [Setting Up Neon with PostgREST](<CloudSync_Neon.md>)
- [Setting Up a Self-Hosted PostgREST Server](<CloudSync_SelfHosted.md>)

Once your server is running, see [Remote Host](<../InterfaceOverview/RemoteHost.md>) for a complete reference of all sync features, settings, and troubleshooting steps.

## Setting Yourself as the Project Manager

You will need to setup yourself to manage all of the projects. First you need to add yourself to the [People List](<../InterfaceOverview/PeopleListPage.md>) and [Client List](<../InterfaceOverview/ClientListPage.md>). Then setup the Project Notes [Preferences](<../InterfaceOverview/Preferences.md>).

**To setup Project Notes:**

1. Choose **Clients** from the **View** menu.
2. Choose **New Item** from the **Edit** menu.
3. Type in ***"Your Company Name.*"** in the new row.
4. Choose **People** from the **View** menu.
5. Choose **New Item** from the **Edit** menu.
6. Type your **Name, Email**, and **Role** in the new row.
7. From the **File** menu, choose **Preferences**.
8. Select yourself as the **Project Manager**.
9. Select ***"Your Company Name"*** as the **Client**.
10. From the **Plugins** menu, choose **Settings > Export Notes** and set the sub-folder where meeting note exports will be saved.
11. From the **Plugins** menu, choose **Settings > Export Tracker Items** and set the sub-folder where tracker item exports will be saved.
12. From the **Plugins** menu, choose **Settings > File Finder** and add the root folder where your project files are stored so the File Finder can find them automatically.

## Populating Your Database

Project Notes is capable of connecting information from various systems. In order to benefit from these capabilities, you will need to consistently name your clients, people, and project numbers. The first step in populating your database is importing clients and people.

There are three methods to import clients and people.

* **Importing From Outlook** - Project Notes includes Outlook contact sync utilities. See [Outlook Integration](<../StandardPlugins/OutlookIntegration.md>) for details. (Outlook COM sync requires Windows; Office 365 sync works on all platforms.)
* **Importing From XML** - All database items can be imported from a [Project Notes XML](<../PluginsOverview/ProjectNotesXML.md>) file. File import is accessible from the [File Menu](<../InterfaceOverview/FileMenu.md>).
* **Entering Manually** - Client information can be entered using the [Client List Page](<../InterfaceOverview/ClientListPage.md>), and contacts can be entered using the [People List Page](<../InterfaceOverview/PeopleListPage.md>).

## Project Notes Preferences

Once your clients and people are setup, you will need to tell Project Notes which contact is the Project Manager coordinating all projects, and what company he works for. This information can be setup in the [Project Notes Preferences](<../InterfaceOverview/Preferences.md>).

## Plugin Settings

Project Notes plugins are highly configurable. Each plugin exposes its own settings dialog under **Plugins > Settings**. Settings control things like where exported files are saved, which folders the File Finder scans, and how integrations with Outlook or Office 365 are authenticated. See [Plugin Settings](<../StandardPlugins/PluginSettings.md>) for a full list of available settings and configuration options.

Plugin settings are stored in your local OS profile (registry on Windows, plist on macOS, ini on Linux) and are not synced to the cloud host. If you move to a new machine, use the **Settings Migrator** (under **Plugins > Settings**) to transfer your configuration.

## Populating Your Project Data And Plugins

Once you have configured Project Notes you are ready to start populating project data. Your edition of Project Notes may include custom plugins specific to your organization. These plugins may import project information from other enterprise systems within your organization.