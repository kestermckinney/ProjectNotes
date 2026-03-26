# Export Meeting Notes

The **Export Meeting Notes** plugin generates a formatted PDF and optionally an HTML file containing all of the meeting notes for a project. The plugin is fully cross-platform — it works on Windows, macOS, and Linux and does not require Microsoft Excel or Microsoft Word.

## Report File Names and Locations

The plugin looks for a **File/Folder Location** with a description of ***"Project Folder"*** to determine where to save the generated reports. See [Project Information Page](<../InterfaceOverview/ProjectPage.md>) for information on **File/Folder Locations**. You can configure a sub-folder name where reports are saved by going to **Plugins > Settings > Export Notes**.

If a project folder is not configured, a folder selection dialog will appear when you run the export.

The output files are named using the project number:

- `[Project Number] Meeting Minutes.pdf`
- `[Project Number] Meeting Minutes Internal.pdf` (when **Generate Internal Report** is checked)

## Report Format

Reports are generated using clean HTML with embedded CSS styling. Each meeting is rendered as a bordered table including:

- Meeting title and date
- Attendees list
- Meeting notes body
- Action items assigned during the meeting (Item, Assigned To, Status, Due Date)

The PDF is rendered in **A4 portrait** layout via the embedded Qt WebEngine browser — no external PDF tools are required.

## Email Integration

The plugin can send the report by email immediately after generation. Email delivery requires either **Office 365** (Graph API, all platforms) or **Microsoft Outlook** (Windows with COM automation). See [Outlook Integration](<OutlookIntegration.md>) for setup instructions.

Available email options:

| Option | Description |
| :--- | :--- |
| **Email as Inline HTML** | The report is pasted directly into the body of the email as formatted HTML. |
| **Email as a PDF attachment** | The generated PDF is attached to the email. |
| **Email as an HTML attachment** | The generated HTML file is attached to the email. |
| **Do not email** | The report is saved only — no email is sent. |

The email subject is automatically populated with the **Project Number**, **Project Name**, and **Report Date**. The project manager specified in [Preferences](<../InterfaceOverview/Preferences.md>) is never included in the recipient list.

## Export Options Dialog

When you run the export, a dialog appears with the following options:

| Option | Description |
| :--- | :--- |
| **Reporting Date** | The date shown in the report footer. Defaults to today. |
| **Display Report when complete** | Opens the PDF after it is generated. |
| **Generate Internal Report** | Includes meeting notes and action items marked as **Internal**. Internal reports are never emailed. |
| **Generate HTML Report** | Saves the HTML file to the project folder in addition to the PDF. |
| **Email Options** | Choose how (or whether) to send the report by email. |

**To export meeting notes:**

1. Right-click on the project in the **Project List Page**.
2. Choose **Export > Meeting Notes**.
3. Set the **Reporting Date** if needed.
4. Check **Generate Internal Report** if you want internal items included.
5. Check **Generate HTML Report** if you want to keep the HTML source file.
6. Select an **Email Option**.
7. Click **OK** to generate the report.

## Settings

**To configure the export sub-folder:**

1. From the **Plugins** menu, choose **Settings > Export Notes**.
2. Enter the sub-folder name relative to the project folder where reports should be saved.
3. Click **OK**.

## Related Documentation

- [Meeting Notes Archive](<MeetingNotesArchive.md>) — Automatically archive meeting notes to a designated folder
