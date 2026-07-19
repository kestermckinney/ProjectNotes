# Export Status Report

The **Export Status Report** plugin generates a formatted project status report as HTML and PDF, combining project activity, open issues, and (for internal users) earned value metrics onto a single page. The plugin is fully cross-platform — it works on Windows, macOS, and Linux and does not require Microsoft Excel or Microsoft Word.

## Report Contents

| Section | Source |
| :--- | :--- |
| Project Manager, Stakeholders, Description, Date, Review Period | The project's **Managing Manager**, team members with **Receive Status** checked, project description, and report date. |
| Activities in Progress / For Next Period / Completed | [Status Report Items](<../InterfaceOverview/ProjectPage.md>) on the **Status** tab of the Project Page, grouped by **Category**: **In Progress**, **Starting**, and **Completed**. |
| Issues | Open [Tracker Items](<../InterfaceOverview/ItemTrackerPage.md>) (status **New** or **Assigned**) — Action Items and closed items are never listed here. Sorted by **Priority**, then **Due Date**. |
| Earned Value Project Metrics | Calculated from the project's **Budget**, **Actual**, **BCWP**, **BCWS**, and **BAC** fields. Only shown when **Internal Items** is selected in the **View** menu — see [Generate Internal Report](#report-options) below. |
| Appendix | A glossary of the earned value terms used in the metrics table. See [Earned Value Terms](<../InterfaceOverview/EarnedValueTerms.md>). |

The **Review Period** shown on the report is calculated from the project's **Status Report Period** field:

| Status Report Period | Review Period |
| :--- | :--- |
| Monthly | One month before the report date |
| Bi-Weekly | 14 days before the report date |
| Weekly | 7 days before the report date |
| None | Left blank |

## Report Options

When you run the export, a dialog appears with the following options:

| Option | Description |
| :--- | :--- |
| **Reporting Date** | The date used for the report header and to calculate the review period. Defaults to today. |
| **Display Report when complete** | Opens the PDF after it is generated. |
| **Generate Internal Report** | Adds the **Earned Value Project Metrics** section and includes tracker items marked **Internal** in the Issues table. |
| **Generate HTML Report** | Saves the HTML file to the project folder in addition to the PDF. |
| **Email Options** | Choose how (or whether) to send the report by email: **Email as Inline HTML**, **Email as a PDF attachment**, **Email as an HTML attachment**, or **Do not email**. |

**To export a status report:**

1. Right-click on the project in the **Project List Page**.
2. Choose **Export > Status Report**.
3. Set the **Reporting Date**.
4. Check **Generate Internal Report** to include earned value metrics and internal issues.
5. Check **Generate HTML Report** if you want to keep the HTML source file.
6. Select an **Email Option**.
7. Click **OK** to generate the report.

## Report File Names and Locations

The plugin looks for a **File/Folder Location** with a description of ***"Project Folder"*** to determine where to save the generated reports. You can configure a sub-folder name where reports are saved by going to **Plugins > Settings > Export Status Report**. The default sub-folder is `Project Management/Status Reports`.

The output files are named using the project number:

- `[Project Number] Status Report.pdf`
- `[Project Number] Status Report Internal.pdf` (when **Generate Internal Report** is checked)

## Email Integration

The plugin can send the report by email immediately after generation, to team members with **Receive Status** checked on the project's **Team** tab. Email delivery requires either **Office 365** (Graph API, all platforms) or **Microsoft Outlook** (Windows with COM automation). See [Outlook Integration](<OutlookIntegration.md>) for setup instructions.

## Settings

**To configure the export sub-folder:**

1. From the **Plugins** menu, choose **Settings > Export Status Report**.
2. Enter the sub-folder name relative to the project folder where reports should be saved.
3. Click **OK**.

## Related Documentation

- [Project Page](<../InterfaceOverview/ProjectPage.md>) — Status Report Items, Status Report Period, and the Team tab's Receive Status flag
- [Earned Value Terms](<../InterfaceOverview/EarnedValueTerms.md>) — Definitions for the metrics in the Earned Value section
- [Export Tracker Items](<ExportTrackerItems.md>) — The full tracker/action item PDF report
