# Project List Page

The Project List Page shows every project as a card. Click a **Projects** icon in the icon rail to get here, and click a card to open that project. The bar above the list and each card's right-click (or **⋮**) menu provide functions for adding, deleting, and filtering projects. More information on how to filter can be found in the [Filter Tool](<FilterTool.md>) section.

**To add a project:**

1. Click **+ New** in the bar above the list, or choose **New Record** from the app menu (the menu icon at the top of the icon rail).
2. A new project opens immediately with a default number and name surrounded by brackets. Edit the fields — changes save automatically.

**To delete a project:**

1. Right-click the project's card (or click its **⋮** button) to open its menu.
2. Choose **Delete**.
3. Click **Yes** to delete the project. If related items are found, Project Notes searches the database for related project information and opens the Search Page instead — you cannot delete a project that still has related records.

**To view closed projects:**

1. From the app menu, check **Show Closed Projects** (or turn on **Show closed projects** in Settings — see [Preferences](<Preferences.md>)).

## Viewing Your Entire Project Portfolio

The Project List Page also provides in-depth earned value metrics based on the cost information entered on the [Project Page](<ProjectPage.md>). Each card provides reminders for invoicing and status reports. The [Earned Value Terms](<EarnedValueTerms.md>) section explains the earned value calculations. Fields are explained below. Financial figures only show when **Show Internal / Budget Items** is turned on — from the app menu, the **Show Internal** toggle at the top of this page, or **Show internal items** in Settings. In most cases you will want to use a plugin that pulls your earned value numbers into Project Notes. Integrating with Microsoft Project would be an example of this.

| **Field** | **Description** |
| :--- | :---- |
| Number | The project number is often a unique identifier used across multiple systems. In many cases Python scripts will use this number to find and relate project information. |
| Project Name | The project name should remain consistent across multiple systems. It should be a concise description of the project. |
| Client | The client the project work applies. |
| Status Date | The last time the status report was run. The value is shown in different colors to warn you when the next status report should be sent to the client. |
| Report Period | **Weekly:** Red indicates it has been more than 7 days since the last report. Yellow indicates it has been exactly 7 days since the last report.  <br>**Bi-Weekly:** Red indicates it has been more than 14 days since the last report. Yellow indicates it has been more than 12 days since the last report.  <br>**Monthly:** Red indicates it has been more than 30 days since the last report. Yellow indicates it has been more than 25 days since the last report. |
| Invoice Date | The last date an invoice was sent. The value is shown in different colors to warn you when the next invoice should be sent to the client. |
| Invoice Period | **Milestone:** Yellow indicates it has been more than 25 days since the last invoice. <br>**Monthly:** Red indicates it has been more than 1 month since the last report. <br>**Complete:** No color indicators will show. |
| Budget | The overall budgeted cost of the project. |
| Actual | The actual cost of the project. |
| Consumed | The percentage of the budget consumed. It is the actual cost divided by the budgeted cost. This value will display yellow to indicate 90% or more of the budget has been consumed. The value will display red to indicate 95% or more of the budget has been consumed. |
| BCWS | The budgeted cost of work scheduled to date. |
| BCWP | The budgeted cost of work performed to date. |
| BAC | The budget at completion. The planned cost for all project work. |
| EAC | The estimated cost to complete the project based upon the current progress. |
| CV | Cost variance of the project comparing actual costs to budgeted costs to date. This value will display yellow for a variance 5% or greater. This value will display red for a variance of 10% or greater. |
| SV | Schedule variance of the project comparing actual work complete to the planned work complete to date. This value will display yellow for a variance 5% or greater. This value will display red for a variance of 10% or greater. |
| Complete | The percentage of overall planned work complete. This value will display yellow to indicate 90% or more of the work is complete. The value will display red to indicate 95% or more of the work is complete. |
| CPI | The cost performance index. This value will display yellow for values less than 1.0. This value will display red for values of 0.8 or less. |
| Status | The status of the project: Active or Closed. |

<br>
**To hide earned value metrics:**

1. From the app menu, uncheck **Show Internal / Budget Items** (or use the **Show Internal** toggle at the top of this page). Note: this will also hide **Notes** and **Item Tracker** items marked as **Internal**.
