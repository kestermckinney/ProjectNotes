# Project List Page

The Project List Page provides a list of projects. You can move quickly between project information by double-clicking on the row counter of a project. The [Toolbar](<Toolbar.md>) and context menu provides functions for adding, deleting and filtering projects. More information on how to filter can be found in the [Filter Tool](<FilterTool.md>) section.

**To add a project:**

1. From the **Edit** menu, select **New Item**.
2. A new project with a default number and name surrounded by brackets is created.

**To delete a project:**

1. Select the project by clicking on a cell in the row.
2. From the **Edit** menu, select **Delete Item**.
3. Click **Yes** to delete the project. If related items are found, Yes will search the database for related project information. You can not delete a project if it contains related information.

You can quickly make a copy of a project. A copy will copy all team members in the current project into a new project. Unique values such as project number and project name will have "Copy \[#############\]" prefixed to the value where number is a unique time stamp value.

**To copy a project:**

1. Click a cell in the project row to select it.
2. Select **Copy Item**, From the **Edit** menu.

**To view closed projects:**

1. From the **View** menu check **Closed Projects**.

## Viewing Your Entire Project Portfolio

The Project List Page also provides in depth earned value metrics based on the cost information entered in the [Project Information Page](<ProjectPage.md>). The portfolio information provides reminders for invoicing and status reports. The [Earned Value Terms](<EarnedValueTerms.md>) section explains the earned value calculations. Columns are explained below. Financial numbers will only show if **Internal Items** has been selected in the **View** menu.

| **Column** | **Description** |
| :--- | :---- |
| Number | The project number is often a unique identifier used across multiple systems. In many cases Python scripts will use this number to find and relate project information. |
| Project Name | The project name should remain consistent across multiple systems. It should be a concise description of the project. |
| Client | The client the project work applies. |
| Last Status | The last time the status report was ran. The value will have different colors to warn you when the next status report should be sent to the client. | 
| Status Period | **Weekly:** Red indicates it has been more than 7 days since the last report. Yellow indicates it has been exactly 7 days since the last report.  <br>**Bi-Weekly:** Red indicates it has been more than 14 days since the last report. Yellow indicates it has been more than 12 days since the last report.  <br>**Monthly:** Red indicates it has been more than 30 days since the last report. Yellow indicates it has been more than 25 days since the last report. |
| Last Invoice | The last date an invoice was sent. The value will have different colors to warn you when the next invoice should be sent to the client. |
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
| CPI | The cost performance index. This value will display Yellow for values less than 1.0. This value will display red for values of 0.8 or less. |
| Status | The status of the project: Active or Closed |

<br>
**To hide earned value metrics:**

1. From the **View** menu uncheck **Internal Items**. Note: this will also hide **Notes** and and **Item Tracker** items marked as **Internal**.