# Project Page

# Keeping Key Project Information In One Place

The Project Page provides a place to keep key project information where you can quickly access it.&nbsp; You can maintain information manually or use a custom plugin to import the information automatically.&nbsp; Financial numbers will only show if **Internal Items** has been selected in the **View** menu.

&nbsp;

**To open a project:**

1. From the [Project List Page](<ProjectListPage.md>), double-click the project row number.

&nbsp;

**To view project status information:**

1. Click the **Status** tab from the Project Page.

&nbsp;

**Basic Project Information**

&nbsp;

| **Column** | **Description** |
| --- | --- |
| Number | The project number is often a unique identifier used across multiple systems.&nbsp; In many cases Lua scripts will use this number to find and relate project information. |
| Project Name | The project name should remain consistent across multiple systems.&nbsp; It should be a concise description of the project. |
| Client | The client the project work applies. |
| Primary Contact | The primary contact at the client. |
| Last Status | The last time the status report was ran. |
| Last Invoice | The last date an invoice was sent. |
| Budget | The overall budgeted cost of the project. |
| Actual | The actual cost of the project. |
| BAC | The budget at completion.&nbsp; The planned cost for all project work. |
| Project Status | The status of the project: **Active** or **Closed**. |
| Invoicing Period | The planned invoicing period for a project: Monthly or Milestone |
| Status Report Period | The planned status report period for a project: **None**, **Weekly**, **Bi-Weekly**, or **Monthly**.&nbsp; Once a project is complete and not closed, the None status will turn off indicators in the [Project List Page](<ProjectListPage.md>). |


&nbsp;

**Status Report Items**

Status report items are temporary items.&nbsp; Once an item is not longer needed on a report it should be removed.

&nbsp;

| **Column** | **Description** |
| --- | --- |
| Category | The category of the status report item: **In Progress** or **Complete** |
| Description | The status item description. |


&nbsp;

&nbsp;

**To add a status report item:**

1. Select **New Item** from the **Edit** menu.
1. Type the the **Description**.
1. Select the **Category** from the drop down.

&nbsp;

**To remove a status report item:**

1. Click a cell in the status item row to select it.
1. Select **Delete Item** from the **Edit** menu.
1. Click **Yes** to confirm the deletion.

&nbsp;

**To edit a status report item:**

1. Double-click the cell to activate the editor.
1. Type in or select the new value.
1. Click out of the cell or press **Enter**.

&nbsp;

**Team Members**

Team members are setup under the **Team** tab of the Project Page. Team members appear in drop downs for meeting attendees, Action/Tracker Item assignments, Identified By, and Updated By fields.&nbsp; You cannot delete team members that are in use on a project.

&nbsp;

| **Column** | **Description** |
| --- | --- |
| Team Member | The person selected from values found in the [People List Panel](<PeopleListPage.md>). |
| Role | The role is an open text field.&nbsp; If a Role has been specified in the [People List Panel](<PeopleListPage.md>), it will auto populate with a value you can change. |
| Receive Status | When checked the Lua script that generates the status report can indicate who should receive it and email it to them. |


&nbsp;

**To add a team member:**

1. Select **New Item**, from the **Edit** menu..
1. Select the **Team Member** from the drop down.
1. Type the new text in the **Role** column, if the preferred role doesn't auto fill.
1. Check the **Receive Status** if the team member will receive a status report.

&nbsp;

**To remove a team member:**

1. Click a cell in the team member row to select it.
1. Select **Delete Item**, From the **Edit** menu.
1. Click **Yes** to confirm the deletion. Note: You may not be able to delete the team member if it is associated with other project information.&nbsp; In this case clicking, **Yes** opens the Search Page.&nbsp; The [File Menu](<FileMenu.md>) topic explains how the Search Page works.

&nbsp;

**To edit a team member:**

1. Double-click the cell to activate the editor.
1. Type in or select the new value.
1. Click out of the cell or press **Enter**.

&nbsp;

**Project Artifacts**

Links to project documents and web locations are setup under the **Artifacts** tab.&nbsp; Keeping all of your commonly access file locations and documents in **Artifacts** can save you a lot of time compared to navigating file folders.&nbsp; The **Artifacts** section keeps links to all of your file and web locations.&nbsp; It is important to make sure locations of your file don't change in order to used the **Artifacts** section reliably.&nbsp; You can create a custom plugin to search your standard project folder structure and populate the **Artifacts** section or you can populate the area manually.&nbsp;

&nbsp;

| **Column** | **Description** |
| --- | --- |
| Type | The type of file location determines how Project Notes handles the Location column.&nbsp; Using the incorrect Type can cause the Location column value to get corrupted.&nbsp; Custom Python scripts can use the Type files for specific operations.&nbsp; The types available are File Folder, Web Link, Microsoft Project, Word Document, Excel Document, PDF File, and Generic File (System Identified) |
| Description | An open description file.&nbsp; In some cases a Python script can look for specific values.&nbsp; Once example is "Project Folder".&nbsp; Python scripts use this location to automatically generate project artifacts. |
| Location | The location referred to by the Type column.&nbsp; The field must be formatted correctly according to its type.&nbsp; For example, "C:\\MyFolder" would not work property for a Web Link, and "http://www.google.com" would not work correctly as a File Folder type. |


&nbsp;

**To add an artifact:**

1. Select **New Item**, from the **Edit** menu..
1. Select the **File** from the three dot button.&nbsp; Note: The three dot button will only allow you to select a file.&nbsp; For selecting folders or web address you will need to type them.
1. Type the **Description**, if the preferred description doesn't auto fill.
1. Type the **Type**, if the preferred description doesn't auto fill.

&nbsp;

**To remove an artifact:**

1. Click a cell in the artifacts row to select it.
1. Select **Delete Item**, From the **Edit** menu.
1. Click **Yes** to confirm the deletion.

&nbsp;

**To edit an artifact:**

1. Double-click the cell to activate the editor.
1. Type in or select the new value.
1. Click out of the cell or press **Enter**.

&nbsp;

**To open an artifact:**

1. Click a cell in the artifacts row to select it.
1. Select **Open Item**, From the **Edit** menu.

&nbsp;

**Project Notes**

Meeting notes associated with a project are kept under the **Notes** tab.&nbsp; All of your notes and their corresponding action items are kept with your projects.&nbsp; The note taking interface is designed to be quick for taking notes during a project meeting.

&nbsp;

**To add a note:**

1. Select **New Item**, from the **Edit** menu.&nbsp; A new note is created with the default project manager as an attendee for the current date.
1. Type the in the **Title**.
1. Choose a different **Date**, if it is not the current date.
1. Open the note to begin entering meeting notes.

&nbsp;

**To remove a note:**

1. Click a cell in the notes row to select it.
1. Select **Delete Item**, From the **Edit** menu.
1. Click **Yes** to confirm the deletion.&nbsp; Note: You may not be able to delete the note if it is associated with other project information.&nbsp; In this case clicking, **Yes** opens the Search Page.&nbsp; The [File Menu](<FileMenu.md>) topic explains how the Search Page works.

&nbsp;

**To edit a note:**

1. Double-click the cell to activate the editor.
1. Type in or select the new value.
1. Click out of the cell or press **Enter**.
1. Open the note to begin editing the meeting notes.

&nbsp;

**To open a note:**

1. Click a cell in the notes row to select it.
1. Select **Open Item**, From the **Edit** menu.

&nbsp;

Often you will meet with the same people.&nbsp; The copy function will copy an existing meeting to a new date keeping the attendees and meeting title.&nbsp; The new note will be empty, an no action items will be copied.

&nbsp;

**To copy a note:**

1. Click a cell in the notes row to select it.
1. Select **Copy Item**, From the **Edit** menu.

&nbsp;


***
_Created with the Personal Edition of HelpNDoc: [Qt Help documentation made easy](<https://www.helpndoc.com/feature-tour/create-help-files-for-the-qt-help-framework>)_
