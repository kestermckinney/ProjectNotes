# Project Page

## Keeping Key Project Information In One Place

The Project Page provides a place to keep key project information where you can quickly access it. You can maintain information manually or use a custom plugin to import the information automatically. Financial numbers only show if **Show Internal / Budget Items** is turned on — from the [Application Menu](<ApplicationMenu.md>) or **Show internal items** in [Preferences](<Preferences.md>).

**To open a project:**

1. From the [Project List Page](<ProjectListPage.md>), click the project's card.



**To view project status information:**

1. Click the **Status Report** tab from the Project Page.



## Basic Project Information

All of the fields below sit above the tabs and are editable inline — changes save automatically as you move to the next field.

| **Column** | **Description** |
| :--- | :--- |
| Number | The project number is often a unique identifier used across multiple systems. In many cases Python scripts will use this number to find and relate project information. |
| Project Name | The project name should remain consistent across multiple systems. It should be a concise description of the project. |
| Company | The company the project work applies. |
| Primary Contact | The primary contact at the company. |
| Last Status | The last time the status report was ran. |
| Last Invoice | The last date an invoice was sent. |
| Budget | The overall budgeted cost of the project. |
| Actual | The actual cost of the project. |
| BAC | The budget at completion. The planned cost for all project work. |
| Project Status | The status of the project: **Active** or **Closed**. |
| Invoicing Period | The planned invoicing period for a project: Monthly or Milestone |
| Status Report Period | The planned status report period for a project: **None**, **Weekly**, **Bi-Weekly**, or **Monthly**. Once a project is complete and not closed, the None status will turn off indicators in the [Project List Page](<ProjectListPage.md>). |


## Status Report Items

Status report items are temporary items, shown under the **Status Report** tab. Once an item is no longer needed on a report it should be removed.

| **Column** | **Description** |
| :--- | :--- |
| Category | The category of the status report item: **In Progress**, **Starting**, or **Completed**. These map to the **Activities in Progress**, **Activities For Next Period**, and **Activities Completed** sections of the [Status Report](<ReportsAndEmail.md>). |
| Description | The status item description. |


**To add a status report item:**

1. From the **Status Report** tab, click **+ Add Status Item**.
2. Choose the **Category** from the drop down.
3. Type the **Description**.



**To remove a status report item:**

1. Click the **×** button on the status item row.



**To edit a status report item:**

1. Click into the **Category** drop down or the **Description** field.
2. Type in or select the new value.
3. Click out of the field or press **Enter** — the change saves automatically.



## Team Members

Team members are set up under the **Team** tab of the Project Page. Team members appear in drop downs for meeting attendees, Action/Tracker Item assignments, Identified By, and Updated By fields. You cannot delete team members that are in use on a project.

| **Column** | **Description** |
| :--- | :--- |
| Team Member | The contact selected from values found in the [Contacts List Page](<ContactsListPage.md>). |
| Role | The role is an open text field. If a Role has been specified in the [Contacts List Page](<ContactsListPage.md>), it will auto populate with a value you can change. |
| Receive Status | When checked, the [Status Report](<ReportsAndEmail.md>) includes this team member as a stakeholder, and they are selected by default when the report is emailed. |


**To add a team member:**

1. From the **Team** tab, click **+ Add Member**.
2. Search for and click the contact you want to add — they only need to already exist in the [Contacts List Page](<ContactsListPage.md>).
3. Type a **Role**, if the auto-filled value isn't the one you want.
4. Check **Receive Status** if the team member will receive a status report.



**To remove a team member:**

1. Click the **×** button on the team member row. Note: You may not be able to remove the team member if they are associated with other project information (for example an assigned tracker item). In that case Project Notes opens the Search Page so you can find and clear the reference first.



**To edit a team member:**

1. Click into the **Role** field or the **Receive Status** checkbox.
2. Type in or select the new value.
3. Click out of the field — the change saves automatically.

**To navigate to a team member's contact record:**

1. Right-click on a team member's row (or click its **⋮** button) to open its menu.
2. Choose **Go To Contact** from the menu.
3. The application will automatically switch to the Contacts page and open that contact's full record.



## Reports and Meetings

Right-click the project, either in the Projects list or on the project's own page, to reach two actions built on the data above.

**Reports** opens a submenu with **Meeting Notes…**, **Status…** and **Tracker Items…**. Each one prepares the report, then lets you save it or email it to recipients you pick from the team. See [Reports and Email](<ReportsAndEmail.md>).

**Schedule Meeting…** builds an invitation from a meeting type and the project team, then opens it in your calendar application. See [Scheduling Meetings](<ScheduleMeeting.md>).

Both need a one-time setup before first use. See [Email and Meeting Setup](<EmailAndMeetingSetup.md>).

## Locations

Links to project documents and web locations are set up under the **Locations** tab. Keeping all of your commonly accessed file locations and documents in **Locations** can save you a lot of time compared to navigating file folders. It is important to make sure the locations of your files don't change in order to use the **Locations** tab reliably. You can create a custom plugin to search your standard project folder structure and populate the **Locations** tab, or you can populate it manually.

| **Column** | **Description** |
| :--- | :--- |
| Type | The type of file location determines how Project Notes handles the Location field. Using the incorrect Type can cause the Location value to get corrupted. Custom Python scripts can use the Type field for specific operations. The types available are File Folder, Web Link, Microsoft Project, Word Document, Excel Document, PDF File, and Generic File (System Identified) |
| Description | An open description field. In some cases a Python script can look for specific values. One example is "Project Folder". Python scripts use this location to automatically generate project files and folders. |
| Location | The file path or web address referred to by the Type field. The field must be formatted correctly according to its type. For example, `C:\MyFolder` would not work properly for a Web Link, and `http://www.google.com` would not work correctly as a File Folder type. |


**To add a location:**

1. From the **Locations** tab, click **+ Add Location** to add a blank row, or click **Browse File** to pick a file directly and add it as a new location.
2. Choose the **Type** from the drop down, if the preferred value doesn't auto fill.
3. Type the **Description** and **Location** (path or web link).



**To add locations by dragging:**

You can drag one or more files or folders from Finder (macOS) or File Explorer (Windows) and drop them anywhere onto the **Locations** tab. A new row is created automatically for each dropped item with its full path pre-filled in the **Location** column. You can then set the **Description** and **Type** for each new entry. Web links can also be dragged from a browser's address bar or a bookmark and dropped onto the tab in the same way.



**To remove a location:**

1. Click the **×** button on the location row.



**To edit a location:**

1. Click into the **Type** drop down, or the **Description** or **Location** field.
2. Type in or select the new value.
3. Click out of the field — the change saves automatically.



**To open a location:**

1. Click the open-in-new icon on the location row. Project Notes opens the file or web link with your system's default handler.



## Project Notes

Meeting notes associated with a project are kept under the **Notes** tab. All of your notes and their corresponding action items are kept with your projects. The note taking interface is designed to be quick for taking notes during a project meeting.

**To add a note:**

1. From the **Notes** tab, click **+ Add Note**. A new note opens immediately with the default project manager as an attendee for the current date.
2. Type in the **Title**.
3. Choose a different **Date**, if it is not the current date.
4. Enter the meeting notes in the note editor.



**To remove a note:**

1. Click the note's card to open it.
2. Click **Delete** in the bar above the note.
3. Click **Yes** to confirm the deletion. Note: You may not be able to delete the note if it is associated with other project information. In this case, clicking **Yes** opens the Search Page — see [Search Page](<SearchPage.md>) for how to work with search results.



**To edit a note:**

1. Click the note's card to open it.
2. Update the **Title**, **Date**, or the meeting notes — changes save automatically.

See [Notes Page](<NotesPage.md>) for how to take notes and assign action items once a note is open.
