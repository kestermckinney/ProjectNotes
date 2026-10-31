# Office Document Templates

The **MS Office Templates** plugin (Windows) creates documents from Word, Excel and PowerPoint templates that you set up, and project schedules from MS Project templates, filled in from the record you right-click. It also opens a project's schedule and closes Excel or Word processes that automation left behind. It needs Microsoft Office, and Microsoft Project for schedules.

## Creating a document

Each template you configure adds a command to the right-click menu of one kind of record, such as projects or tracker items. Choose it, and MS Office Templates:

1. opens the template in Word, Excel, PowerPoint or MS Project, out of sight;
2. asks for any values the template needs typed in, such as a change request number;
3. fills in the placeholders from the record and its project;
4. saves the result in the project's folder and adds it to the project's **Files & Folders**;
5. opens the new document.

A document that already exists is never overwritten. You are asked whether to open it instead. When the project has no folder set on the [Project Page](<../InterfaceOverview/ProjectPage.md>) **Files & Folders** tab, or the record has no project at all, you choose where to save.

## Included templates

Four examples come with Project Notes and are set up the first time MS Office Templates starts.

| Menu | Right-click | Template | Saved as |
| :--- | :--- | :--- | :--- |
| **Templates > Change Request** | a project | `Change Request Template.docx` | `Project Management/PCR's/P-100 Change Request 03.docx` |
| **Templates > Tracker Items Status** | a project | `Tracker Items Status Template.xlsx` | `Project Management/Issues/P-100 Tracker Items 2026-10-30.xlsx` |
| **Templates > Project Kickoff** | a project | `Project Kickoff Template.pptx` | `Project Management/Meeting Minutes/P-100 Project Kickoff.pptx` |
| **Templates > Project Schedule** | a project | `Software Development Lifecycle Template.xml` | `Project Management/Schedule/P-100 Schedule.mpp` |

The change request asks for the change number and fills in the project, client, contact, project manager and current budget. Description, reason, impact and approval sections are left for you to complete.

The tracker items workbook lists every tracker item of the project, one row each, with its type, owner, priority, status and dates, plus a count of items by status.

The kickoff presentation is a nine-slide deck for the first meeting with the client. The title slide, project overview and communication slides fill in the project name and number, client, contact, project manager, budget and reporting periods. The team slide lists every member of the project team with their role, and the risks slide lists the project's tracker items. Objectives, scope and target dates are prompts for you to replace.

The software development schedule is a Microsoft Project plan with seven phases: initiation, requirements, design, development, testing, deployment and closure. Its 30 tasks and milestones are linked, so the plan starts on the day you create it and the dates follow from there. Adjust durations and add your own tasks once it opens. The schedule is saved as an `.mpp` file, so **Open MS Project** finds it. It needs Microsoft Project installed.

All four files live in the plugin's `templates` folder. Copy one somewhere else before you change it, then point the template's row at your copy, so an update of Project Notes can't replace your edits. Open the schedule template in Microsoft Project to change it, and save it as `.xml`, `.mpp` or `.mpt`.

## Settings

Open **Settings > MS Office Templates**. Each row of the **Templates** table is one menu command:

| Column | Meaning |
| :--- | :--- |
| **Menu** | The command's name. |
| **Submenu** | The submenu it appears under, such as `Templates`. Leave it empty to list the command directly. |
| **Entity** | The kind of record whose right-click menu shows the command, and whose values fill the document. |
| **Template** | The Word (`.docx`, `.dotx`, `.doc`), Excel (`.xlsx`, `.xltx`, `.xls`), PowerPoint (`.pptx`, `.potx`) or MS Project (`.mpp`, `.mpt`, `.xml`) file to start from. Use **…** to browse. A path that doesn't start with a drive or `\\` is inside the plugin's folder. |
| **Output Folder** | Where the document goes, relative to the project's folder. It can use placeholders: `Project Management/{{ projects.project_number }}`. |
| **File Name** | The new file's name, without the extension. It can use placeholders. When empty, it's the project number and the menu name. |

Right-click the Output Folder or File Name box and choose **Insert Field** to pick from the placeholders available for the row's Entity. Delete a row to remove its command. The two included templates are only added on first start, so deleting them is permanent until you add them back by hand.

## Placeholders

Templates use the same `{{ name }}` placeholders as email templates and My Shortcuts. See [Plugin Settings](<PluginSettings.md#placeholders>). Write them anywhere in the document: body text, tables, headers and footers in Word; any cell in Excel; text boxes and tables in PowerPoint; the title, subject, company, manager and comments, and task names and notes, in MS Project. A schedule also moves to start on the day it is created; tasks pinned to a date stay put.

| Placeholder | Becomes |
| :--- | :--- |
| `{{ projects.project_name }}` | A column of the record you right-clicked, or of its project |
| `{{ item_tracker.2.item_name }}` | Row 2 of a related list |
| `{{ project_team.*.role }}` | That column from every row, joined with commas |
| `{{ item_tracker.#.status }}` | In a table row, repeats the row once for each row of the list (see below) |
| `{{ input.change_number }}` | Asks for a value when the document is created; the label is the name with underscores as spaces |
| `{{ template.name }}` | The menu name |
| `{{ current.date }}`, `{{ current.isoDate }}` | Today as `10/30/2026` or `2026-10-30`; use the second in file names |
| `{{ preferences.managerName }}`, `{{ preferences.managingCompanyName }}` | The default project manager and managing company from Settings |

Dates read as `MM/dd/yyyy`, money as `$1,234.00`, and checkboxes as Yes or No. Related lists are the clicked record's own, or else its project's. From a tracker item, `item_tracker` therefore means all of the project's tracker items, in item number order.

Templates written for the earlier fixed commands keep working: `{{ project.number }}`, `{{ project.name }}` and `{{ company.name }}` still fill in.

### Repeating rows

Put `#` where the row number goes, and the whole row repeats once for each row of that list. In a Word table, that is the table row. In Excel, it is the worksheet row, copied with its formatting and conditional formatting. In a PowerPoint table, it is the table row. In MS Project, it is a task with `#` in its name or notes: each copy keeps the task's outline level, duration and predecessors. When the list is empty, the row or task is removed.

The included schedule ends with a **Tracker Items for** the project summary task. The one task under it is named `{{ item_tracker.#.item_number }} {{ item_tracker.#.item_name }}`, and its notes hold the item's type, status, priority, assignee, due date and description, so the schedule lists every tracker item of the project. Several other tasks, such as the kickoff meeting and go-live, carry the project's name, number or company in their names.

The included tracker workbook's row is:

```
{{ item_tracker.#.item_number }} | {{ item_tracker.#.item_name }} | {{ item_tracker.#.status }} | ...
```

`#` works with any related list of the record or its project, such as `project_team`, `project_notes` or `status_report_items`.

## Open MS Project

Right-click a project and choose **Open MS Project**. Project Notes opens the `.mpp` file listed in the project's **Files & Folders**, or asks which one when there are several.

## Stranded Excel and Word

When automation fails partway, Excel or Word can keep running in the background. **Plugins · Utilities > Close Stranded Excel** and **Close Stranded Word** close the copies that were started by automation and leave the ones you opened yourself alone.

## Upgrading from earlier versions

The fixed Change Order, PCR Register, PowerPoint, Risk Register and MS Project commands, and the Lessons Learned, Meeting and Risk Register templates, are gone. To keep using one of your own templates, add a row for it in **Settings > MS Office Templates**.
