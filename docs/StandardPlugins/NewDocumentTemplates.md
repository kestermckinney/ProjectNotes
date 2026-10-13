# Office Document Templates

The **Office Tools** plugin (Windows) creates project documents from Word, Excel, PowerPoint and Microsoft Project templates, opens a project's schedule, and closes Excel or Word processes that automation left behind. It needs Microsoft Office, and Microsoft Project for the schedule commands.

## New documents

Right-click a project and choose **Templates**, then one of:

| Command | Template | Placeholders filled in | Default subfolder |
| :--- | :--- | :--- | :--- |
| **Change Order** | `PCR Template.docx` (asks for the change number) | `<PROJECTNUMBER>`, `<CLIENTNAME>`, `<PROJECTNAME>`, `<CRDATE>`, `<CRNUMBER>` | `Project Management/PCR's` |
| **PCR Register** | `PCR Registry Template.xlsx` | none; the copy opens in Excel | `Project Management/PCR's` |
| **PowerPoint** | one you choose | `<PROJECTNUMBER>`, `<PROJECTNAME>` | `Project Management/Meeting Minutes` |
| **Risk Register** | `Risk Register Template.xlsx` | `<PROJECTNAME>`, with the project number in front | `Project Management/Risk Management` |
| **MS Project** | one you choose (`.mpp`) | tasks whose name contains `PROJECTNAME` | `Project Management/Schedule` |

Each command copies the template into the project folder's subfolder, names the copy after the project (for example `P-100 Risk Register.xlsx`), fills in the placeholders, and adds the file to the project's **Files & Folders**. A file that already exists is opened as it is, not overwritten. When the project has no folder, you are asked where to save.

The Change Order and PCR Register templates are not included with Project Notes; the first time, you are asked to choose your own. The Risk Register, Lessons Learned and Meeting templates come in the plugin's `templates` folder.

## Subfolders

Open **Settings > Office Tools** to change the subfolder each command saves to. Each is relative to the project's folder, which is set on the [Project Page](<../InterfaceOverview/ProjectPage.md>) **Files & Folders** tab.

```
[Project Folder]/
└── Project Management/
    ├── Meeting Minutes/      ← PowerPoint
    ├── Schedule/             ← MS Project
    ├── PCR's/                ← Change Order, PCR Register
    └── Risk Management/      ← Risk Register
```

## Open MS Project

Right-click a project and choose **Open MS Project**. Project Notes opens the `.mpp` file listed in the project's **Files & Folders**, or asks which one when there are several.

## Stranded Excel and Word

When automation fails partway, Excel or Word can keep running in the background. **Plugins · Utilities > Close Stranded Excel** and **Close Stranded Word** close the copies that were started by automation and leave the ones you opened yourself alone.

Subfolder settings from the Python template plugins carry over the first time Office Tools starts.
