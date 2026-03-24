# File Collector

The **File Collector** runs as a background process. Every minute, the plugin scans all configured folders and looks for files related to your projects, automatically adding them to the **Artifacts** tab on the Project Page. This keeps your project file list up to date without manual maintenance.

## How It Works

The File Collector matches files to projects using the **project number** found in the file name or folder path. When a matching file is found that is not already listed in the project's Artifacts, the plugin adds it automatically with the appropriate file type and a description based on the file classification rules.

## Configuring File Collector Settings

**To open the File Collector settings:**

1. From the **Plugins** menu choose **Settings > File Finder**.

The settings dialog has two sections:

### Search Locations

A list of root folders for the File Collector to scan. Each entry specifies a folder path. The plugin will recursively search these locations for project-related files.

| Button | Description |
| :--- | :--- |
| **Add Location** | Add a new folder to scan. |
| **Edit Location** | Edit the selected folder path. |
| **Delete Location** | Remove the selected folder from the scan list. |

### File Classifications

Classification rules determine how files are categorized when added to the Artifacts list. Each rule maps a file extension or naming pattern to a description and file type.

| Button | Description |
| :--- | :--- |
| **Add Classification** | Add a new file classification rule. |
| **Edit Classification** | Edit the selected classification rule. |
| **Delete Classification** | Remove the selected classification rule. |

## Notes

- The File Collector only adds files; it does not remove or modify existing Artifact entries.
- Files are added with the **Project Folder** artifact type when a matching project folder is found.
- The plugin runs only while Project Notes is open.

## Related Documentation

- [Standard Plugins Overview](<PluginSettings.md>) — Complete list of all standard plugins and their configuration options
