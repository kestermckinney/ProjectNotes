# Getting Started

## Creating Your Main Database

Start Project Notes and do the following items:

**To create a new database:**

1. From the Project Notes **File** menu choose **New Database...** You can place your new database anywhere. Keeping your database file in a local file location is ideal, so you can access your project information on the go. Using a location such as iCloud or One drive will keep you database backed up to a remote location. The database is locked while Project Notes is open, so it will not sync using these services until you close the program.
2. Select the location to store your database file.
3. Type the name of your database without a file extension.
4. Click **Save**.

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
12. From the **Plugins** menu, choose **Settings > File Finder** and add the root folder where your project files are stored so the File Collector can find them automatically.

## Populating Your Database

Project Notes is capable of connecting information from various systems. In order to benefit from these capabilities, you will need to consistently name your clients, people, and project numbers. The first step in populating your database is importing clients and people.

There are three methods to import clients and people.

* **Importing From Outlook** - Project Notes includes Outlook contact sync utilities. See [Outlook Integration](<../StandardPlugins/OutlookIntegration.md>) for details. (Outlook COM sync requires Windows; Office 365 sync works on all platforms.)
* **Importing From XML** - All database items can be imported from a [Project Notes XML](<../PluginsOverview/ProjectNotesXML.md>) file. File import is accessible from the [File Menu](<../InterfaceOverview/FileMenu.md>).
* **Entering Manually** - Client information can be entered using the [Client List Page](<../InterfaceOverview/ClientListPage.md>), and contacts can be entered using the [People List Page](<../InterfaceOverview/PeopleListPage.md>).

## Project Notes Preferences

Once your clients and people are setup, you will need to tell Project Notes which contact is the Project Manager coordinating all projects, and what company he works for. This information can be setup in the [Project Notes Preferences](<../InterfaceOverview/Preferences.md>).

## Plugin Settings

Project Notes helps you find and generate artifacts related to your project. In order to do this you need to configure the plugins. Each plugin has its own settings accessible under **Plugins > Settings**. For example, the **Export Meeting Notes** and **Export Tracker Items** plugins each have a sub-folder setting that controls where generated reports are saved. See [Plugin Settings](<../StandardPlugins/PluginSettings.md>) for a full list of available settings.

## Populating Your Project Data And Plugins

Once you have configured Project Notes you are ready to start populating project data. Your edition of Project Notes may include custom plugins specific to your organization. These plugins may import project information from other enterprise systems within your organization.