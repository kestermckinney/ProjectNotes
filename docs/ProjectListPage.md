# Project List Page

# The Project List Page

The Project List Page provides a list of projects.&nbsp; You can move quickly between project information by double-clicking on the row counter of a project.&nbsp; The [Toolbar](<Toolbar.md>) and context menu provides functions for adding, deleting and filtering projects.&nbsp; More information on how to filter can be found in the [Filter Tool](<FilterTool.md>) section.

&nbsp;

**To add a project:**

1. From the **Edit** menu, select **New Item**.
1. A new project with a default number and name surrounded by brackets is created.

&nbsp;

**To delete a project:**

1. Select the project by clicking on a cell in the row.
1. From the **Edit** menu, select **Delete Item**.
1. Click **Yes** to delete the project.&nbsp; If related items are found, Yes will search the database for related project information.&nbsp; You can not delete a project if it contains related information.

&nbsp;

You can quickly make a copy of a project.&nbsp; A copy will copy all team members in the current project into a new project.&nbsp; Unique values such as project number and project name will have "Copy \[#############\]" prefixed to the value where number is a unique time stamp value.

&nbsp;

**To copy a project:**

1. Click a cell in the project row to select it.
1. Select **Copy Item**, From the **Edit** menu.

&nbsp;

**To view closed projects:**

1. From the **View** menu check **Closed Projects**.

# Viewing Your Entire Project Portfolio

The Project List Page also provides in depth earned value metrics based on the cost information entered in the [Project Information Page](<ProjectPage.md>).&nbsp; The portfolio information provides reminders for invoicing and status reports.&nbsp; The [Earned Value Terms](<EarnedValueTerms.md>) section explains the earned value calculations.&nbsp; Columns are explained below.&nbsp; Financial numbers will only show if **Internal Items** has been selected in the **View** menu.

&nbsp;

&nbsp;

| **Column** | **Description** |
| --- | --- |
| Number | The project number is often a unique identifier used across multiple systems.&nbsp; In many cases Lua scripts will use this number to find and relate project information. |
| Project Name | The project name should remain consistent across multiple systems.&nbsp; It should be a concise description of the project. |
| Client | The client the project work applies. |
| Last Status | The last time the status report was ran.&nbsp; The value will have different colors to warn you when the next status report should be sent to the client. | **Status Period** | **Indicator Description** |
| --- | --- |
| **Weekly:** | Red indicates it has been more than 7 days since the last report. Yellow indicates it has been exactly 7 days since the last report. |
| **Bi-Weekly:** | Red indicates it has been more than 14 days since the last report. Yellow indicates it has been more than 12 days since the last report. |
| **Monthly:** | Red indicates it has been more than 30 days since the last report. Yellow indicates it has been more than 25 days since the last report. |
 |
| Last Invoice | The last date an invoice was sent.&nbsp; The value will have different colors to warn you when the next invoice should be sent to the client. | **Invoice Period** | **Indicator Description** |
| --- | --- |
| **Milestone:** | Yellow indicates it has been more than 25 days since the last invoice. |
| **Monthly:** | Red indicates it has been more than 1 month since the last report. |
| **Complete:** | No color indicators will show. |
 |
| Budget | The overall budgeted cost of the project. |
| Actual | The actual cost of the project. |
| Consumed | The percentage of the budget consumed.&nbsp; It is the actual cost divided by the budgeted cost.&nbsp; This value will display yellow to indicate 90% or more of the budget has been consumed.&nbsp; The value will display red to indicate 95% or more of the budget has been consumed. |
| BCWS | The budgeted cost of work scheduled to date. |
| BCWP | The budgeted cost of work performed to date. |
| BAC | The budget at completion.&nbsp; The planned cost for all project work. |
| EAC | The estimated cost to complete the project based upon the current progress. |
| CV | Cost variance of the project comparing actual costs to budgeted costs to date.&nbsp; This value will display yellow for a variance 5% or greater.&nbsp; This value will display red for a variance of 10% or greater. |
| SV | Schedule variance of the project comparing actual work complete to the planned work complete to date.&nbsp; This value will display yellow for a variance 5% or greater.&nbsp; This value will display red for a variance of 10% or greater. |
| Complete | The percentage of overall planned work complete. This value will display yellow to indicate 90% or more of the work is complete.&nbsp; The value will display red to indicate 95% or more of the work is complete. |
| CPI | The cost performance index.&nbsp; This value will display Yellow for values less than 1.0.&nbsp; This value will display red for values of 0.8 or less. |
| Status | The status of the project: Active or Closed |


&nbsp;

**To hide earned value metrics:**

1. From the **View** menu uncheck **Internal Items**.&nbsp; Note: this will also hide **Notes** and and **Item Tracker** items marked as **Internal**.

&nbsp;


***
_Created with the Personal Edition of HelpNDoc: [Full-featured EBook editor](<https://www.helpndoc.com/create-epub-ebooks>)_
