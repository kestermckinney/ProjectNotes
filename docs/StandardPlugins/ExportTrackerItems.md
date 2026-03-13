# Export Tracker Items

The **Export Tracker Items** plugin generates a formatted PDF report containing tracker and action items for a project. The plugin is fully cross-platform — it works on Windows, macOS, and Linux and does not require Microsoft Excel or Microsoft Word.

## Report File Names and Locations

The plugin looks for a **File/Folder Location** with a description of ***"Project Folder"*** to determine where to save the generated reports. See [Project Information Page](<../InterfaceOverview/ProjectPage.md>) for information on **File/Folder Locations**. You can configure a sub-folder name where reports are saved by going to **Plugins > Settings > Export Tracker Items**.

If a project folder is not configured, a folder selection dialog will appear when you run the export.

The output files are named using the project number:

- `[Project Number] Tracker Items.pdf`
- `[Project Number] Tracker Items Internal.pdf` (when **Generate Internal Report** is checked)

## Report Format

Reports are generated using clean HTML with embedded CSS styling and rendered to PDF via the embedded Qt WebEngine browser. The report is formatted as a **landscape Letter** page and contains the following columns:

| Column | Description |
| :--- | :--- |
| ID | Item number |
| Item | Item name |
| Identified By | Person who identified the item |
| Date Identified | When the item was identified |
| Description | Detailed description of the item |
| Assigned To | Person assigned to resolve the item |
| Priority | High (red), Medium (orange), or Low (green) |
| Status | New, Assigned, Resolved, Deferred, or Cancelled |
| Due Date | Target resolution date |
| Last Update | Date of the most recent change |
| Date Resolved | Date the item was resolved |
| Comments/Resolution | All update notes concatenated |
| Int | *(Internal report only)* Whether the item is marked internal |

## Filtering Options

The export dialog lets you filter which items appear in the report:

**Include Status** — check the statuses to include:
- New *(checked by default)*
- Assigned *(checked by default)*
- Resolved
- Deferred
- Cancelled

**Include Types** — check the item types to include:
- Tracker Items *(checked by default)*
- Action Items

**Generate Internal Report** — includes items marked as internal and adds the **Int** column.

## Email Integration

The plugin can send the report by email immediately after generation. Email delivery requires either **Office 365** (Graph API, all platforms) or **Microsoft Outlook** (Windows with COM automation). See [Outlook Integration](<OutlookIntegration.md>) for setup instructions.

Available email options:

| Option | Description |
| :--- | :--- |
| **Email as Inline HTML** | The report is pasted directly into the body of the email as formatted HTML. |
| **Email as a PDF attachment** | The generated PDF is attached to the email. |
| **Email as an HTML attachment** | The generated HTML file is attached to the email. |
| **Do not email** | The report is saved only — no email is sent. |

The email subject is automatically populated with the **Project Number**, **Project Name**, and current date. The project manager specified in [Preferences](<../InterfaceOverview/Preferences.md>) is never included in the recipient list.

## Export Options Dialog

When you run the export, a dialog appears with the following options:

| Option | Description |
| :--- | :--- |
| **Display Report when complete** | Opens the PDF after it is generated. |
| **Generate Internal Report** | Includes items marked as **Internal** and adds the **Int** column to the report. |
| **Generate HTML Report** | Saves the HTML file to the project folder in addition to the PDF. |
| **Include Status** | Filter items by status (New, Assigned, Resolved, Deferred, Cancelled). |
| **Include Types** | Filter by item type (Tracker Items, Action Items). |
| **Email Options** | Choose how (or whether) to send the report by email. |

**To export tracker items:**

1. Right-click on the project in the **Project List Page**.
2. Choose **Export > Tracker Items**.
3. Check **Generate Internal Report** if you want internal items included.
4. Check **Generate HTML Report** if you want to keep the HTML source file.
5. Check the **Include Status** items you want on the report.
6. Check the **Include Types** you want on the report.
7. Select an **Email Option**.
8. Click **OK** to generate the report.

## Settings

**To configure the export sub-folder:**

1. From the **Plugins** menu, choose **Settings > Export Tracker Items**.
2. Enter the sub-folder name relative to the project folder where reports should be saved.
3. Click **OK**.
