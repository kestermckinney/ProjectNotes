# Meeting Notes Archive

## Exporting Meeting Notes to PDF

The **Export Meeting Notes** plugin automatically generates a PDF file containing all of the meeting notes taken for the project. The plugin is cross-platform and works on Windows, macOS, and Linux — it does not require Microsoft Excel.

For complete documentation see [Export Meeting Notes](<ExportMeetingNotes.md>).

## Report File Names and Locations

The plugin looks for a **File/Folder Location** with a description of ***"Project Folder"*** to determine where to save the generated reports. See [Project Information Page](<../InterfaceOverview/ProjectPage.md>) to learn about **File/Folder Locations**. You can configure the sub-folder name where reports are placed in **Plugins > Settings > Export Notes**.

By default all meeting notes not marked as internal are included in the report. You can generate a separate report that includes internal items — the report file will have "Internal" appended to the name. Internal reports are never automatically emailed.

## Email Options

The plugin can email the report as inline HTML, a PDF attachment, or an HTML attachment immediately after generation. It requires **Office 365** (all platforms) or **Outlook** (Windows only) to be configured. See [Outlook Integration](<OutlookIntegration.md>).

**To export meeting notes:**

1. Right-click on the project in the **Project List Page**.
2. Choose **Export > Meeting Notes**.
3. Select the **Reporting Date**. By default the current date appears.
4. Check **Display Report when complete** if you want to open the PDF after it is generated.
5. Check **Generate Internal Report** if you want the report to include internal items.
6. Check **Generate HTML Report** if you want to keep the HTML source file.
7. Choose the type of email to send in the **Email Options** section.
8. Click **OK** to generate the report.
