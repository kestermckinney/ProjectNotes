# New Document Templates

Project Notes includes several plugins that copy pre-configured Microsoft Office and MS Project templates into your project folder, optionally replacing placeholder tags with project-specific data. These plugins are available on all platforms, though opening the created files may require the associated application.

## Available Template Plugins

| Plugin | Template File | Default Output Sub-folder |
| :--- | :--- | :--- |
| **New Change Order** | `plugins/templates/PCR Template.docx` (Word) | `Project Management/PCR's` |
| **New MS Project** | User-selected `.mpp` from `plugins/templates/` | `Project Management/Schedule` |
| **PCR Register** | `plugins/templates/PCR Registry Template.xlsx` (Excel) | `Project Management/PCR's` |
| **New PowerPoint** | User-selected `.pptx` from `plugins/templates/` | `Project Management/Meeting Minutes` |
| **New Risk Register** | `plugins/templates/Risk Register Template.xlsx` (Excel) | `Project Management/Risk Management` |

## Default Project Folder Structure

All template and export plugins look for a **Files & Folders** entry on the project with the description **"Project Folder"** to determine the project's root folder on disk. From that root, each plugin writes output to a configurable sub-folder. The defaults assume the following structure:

```
[Project Folder]/
└── Project Management/
    ├── Meeting Minutes/      ← New PowerPoint
    ├── Schedule/             ← New MS Project
    ├── PCR's/                ← New Change Order, PCR Register
    ├── Risk Management/      ← New Risk Register
    ├── Issues List/          ← Export Tracker Items, Generate Tracker Report
    └── Status Reports/       ← SSRS Report Capture (IFS)
```

If no **"Project Folder"** entry is configured for a project, the plugin will prompt you to choose a folder when it runs.

## How Template Plugins Work

When you invoke a template plugin from the right-click menu on a project:

1. The plugin locates the configured template file in `plugins/templates/`.
2. It copies the template to the configured sub-folder inside your **Project Folder**.
3. For PowerPoint and Word templates, supported placeholder tags in the document are replaced with project values such as the project number and project name.

Common placeholder tags used in templates:

| Tag | Replaced With |
| :--- | :--- |
| `<PROJECTNUMBER>` | The project number |
| `<PROJECTNAME>` | The project name |
| `<DATE>` | Today's date |

## Configuring Template Settings

Each template plugin has its own settings entry under **Plugins > Settings** where you can configure the sub-folder within the project folder where the generated file will be placed. The default sub-folder for each plugin is shown in the table above.

**To configure a template plugin:**

1. From the **Plugins** menu choose **Settings > [Template Name]** (e.g., **Settings > New Change Order**).
2. Enter the sub-folder name relative to the project folder.
3. Click **OK**.

## Open MS Project (Windows Only)

The **Open MS Project** plugin (Windows only) scans the project Files & Folders for `.mpp` files and opens the selected file in Microsoft Project. If multiple `.mpp` files are associated with the project, a selection dialog appears.

**To open an MS Project file:**

1. Right-click on a project.
2. Choose **Open MS Project**.
3. If prompted, select the `.mpp` file to open.

## Stranded Process Cleanup

If Microsoft Excel or Word automation fails mid-execution, the application process may remain open in the background. Use the utilities in the **Plugins > Utilities** menu to clean up these stranded processes:

- **Close Stranded Excel** — Terminates any Excel processes left running by automation.
- **Close Stranded Word** — Terminates any Word processes left running by automation.
