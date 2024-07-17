# Getting Started

# Creating Your Main Database

Start Project Notes do the following items:

**To create a new database:**
1. From the Project Notes **File** menu choose **New Database...** You can place your new database anywhere. Keeping your database file in a local file location is ideal, so you can access your project information on the go. Using a location such as iCloud or One drive will keep you database backed up to a remote location. The database is locked while Project Notes is open, so it will not sync using these services until you close the program.
2. Select the location to store your database file.
3. Type the name of your database without a file extension.
4. Click **Save**.

# Setting Yourself as the Project Manager
You will need to setup yourself to manage all of the projects. First you need to add yourself to the [People List](<PeopleListPage.md>) and [Client List](<ClientListPage.md>). Then setup the Project Notes [Preferences](<Preferences.md>).

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
10. From the **Plugins**, menu choose **Plugin Settings..**.
11. Choose the **Global Settings** plugin.
12. Type *the base folder location where all your projects are kept* for the **ProjectsFolder**.
13. Click **Apply**.

# Populating Your Database
Project Notes is capable of connecting information from various systems. In order to benefit from these capabilities, you will need to consistently name your clients, people, and project numbers. The first step in populating your database is importing clients and people.

There are three methods to import clients and people.

* **Importing From Outlook** - A Python plugin is included with Project Notes for[ Importing and Exporting Outlook Contacts](<ImportingandExportingOutlookCont.md>). (The plugin does not support Outlook Express.)
* **Importing From XML** - All database items can be imported from a [Project Notes XML](<ProjectNotesXML.md>) file. File import is accessible from the [File Menu](<FileMenu.md>).
* **Entering Manually** - Client information can be entered using the [Client List Page](<ClientListPage.md>), and contacts can be entered using the [People List Page](<PeopleListPage.md>).

# Project Notes Preferences
Once your clients and people are setup, you will need to tell Project Notes which contact is the Project Manager coordinating all projects, and what company he works for. This information can be setup in the [Project Notes Preferences](<Preferences.md>).

# Plugin Settings
Project notes helps you find and generate artifacts related to your project. In order to do this you need to configure the plugins. The [Global Settings](<GlobalSettings.md>) plugin sets up the project location and any other specific information related to your implementation of Project Notes. Custom plugins can overwrite the [Global Settings](<GlobalSettings.md>) Python plugin. For more information, plugin providers should add a help item to the help menu.

# Populating Your Project Data And Plugins

Once you have configured Project Notes you are ready to start populating project data. Your edition of Project Notes may include custom plugins specific to your organization. These plugins may import project information from other enterprise systems within your organization.