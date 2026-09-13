# File Finder

File Finder is a native Project Notes service that discovers files for active projects and adds or updates them on the project's **Files & Folders** tab. It runs every five minutes while Project Notes is open and File Finder is enabled. You can also start a scan immediately.

File Finder is no longer a Python plugin and is not configured from **Plugins > Settings**. Open the app menu, choose **Settings**, and select **File Finder**.

## How File Finder Matches Projects

For each active project, File Finder looks for a folder whose name contains the complete project number. Matching is case-insensitive, and the project number must be separated from surrounding letters and numbers. For example, project `ABC-001` can match `ABC-001 Project Name`, but not `XABC-0012`.

Each configured local search location is traversed once per scan. After File Finder finds a project's folder, it adds the folder as **Project Folder** and scans its files and subfolders. If **Include Office 365 locations** is enabled, it can also find a Microsoft Teams channel named for the project and scan the channel's SharePoint files.

File classification rules are evaluated in order. The first regular expression that matches a file name or normalized path supplies the classification. The resulting description has the form `Classification : filename.ext`.

When local and Office 365 discovery find a file with the same project, classification, and filename, they reconcile to one row instead of creating source-specific duplicates. The local (system) discovery wins: if a row already points at a local path and that path still exists, a Teams/SharePoint match for the same file does not replace it with a web link. This matters if you work with a file through an application that isn't integrated with Microsoft Teams — a CAD tool, an old Office install, anything that only understands a filesystem path. As long as your local search location still finds the file, **Files & Folders** keeps opening it locally instead of routing you through a browser link. Office 365 discovery only supplies the row when no live local copy was found, and it still adds any remaining unmatched files from either source. File Finder does not delete entries when a file disappears from a source.

## File Finder Settings

### File Finder Controls

| Setting or action | Description |
| :--- | :--- |
| **Enable File Finder** | Starts scheduled reconciliation. Disable it to stop background and manual scans without deleting settings or existing file entries. |
| **Include Office 365 locations** | Adds Microsoft Teams and SharePoint discovery to local discovery. Configure and sign in on the separate **Office 365 Integration** settings page first. |
| **Scan Now** | Starts an immediate scan using the saved settings. The current status and previous scan summary appear beside the button. |
| **Reconsider All Files** | Clears File Finder's discovery cache and performs a fresh scan. It keeps search locations, exclusions, classification rules, and existing **Files & Folders** rows. Use it after remote folders change in a way an ordinary scan does not detect. |

Microsoft 365 account configuration is intentionally kept on its own settings page so it can support more Project Notes features over time. See [Office 365 Integration](<Office365Integration.md>).

### Search Locations

Search locations are local root folders. File Finder recursively examines each root until it finds folders for the active project numbers.

- Enter a path and select **Add**, or use the folder button to browse.
- Select the delete action to stop scanning a root. The folder and existing Project Notes entries are not deleted.
- Add multiple roots when project folders are stored in more than one place.
- `~` means the current user's home folder and is the default for a new profile.

Avoid adding overlapping roots because the same directory tree will be visited from each root.

### Excluded Folders

Folder exclusions prevent File Finder from entering unwanted directory trees. Each entry is a case-insensitive regular expression checked against the folder name and its normalized path, with and without a trailing `/`. When an expression matches, that folder and its entire subtree are skipped for both local and Office 365 discovery.

New profiles exclude `Engineering/.*` by default. Add, edit, or delete expressions directly in the grid. For example:

```regex
(^|/)(node_modules|[.]git)(/|$)
```

This skips folders named `node_modules` or `.git` anywhere below a search root. Changing exclusions invalidates the Office 365 folder cache so remote content is reconsidered on the next scan.

### File Classifications

Each row contains a classification and a regular expression. Expressions are case-insensitive and are matched against both the file name and normalized path. Rules run from top to bottom, and only the first match is used.

| Pattern | Example use |
| :--- | :--- |
| `.*[.]mpp$` | Microsoft Project schedules |
| `.*Quote.*[.]pdf$` | PDF files containing `Quote` in the name or path |
| `.*[.](pdf\|docx)$` | PDF or Word files |
| `^(?!.*\bTemplate\b).*Risk.*[.]xlsx$` | Risk workbooks except files containing `Template` |

Use the blank row to add a rule, edit existing cells in place, or use the delete action to remove a rule. **Reset Defaults** replaces all classification rules with the maintained defaults; it does not change search locations or exclusions.

Changing classification rules invalidates the Office 365 folder cache. Run **Scan Now** if you want the new rules applied immediately.

## First Run and Upgrades

New profiles start with File Finder enabled, `~` as the local search root, the default `Engineering/.*` exclusion, and the maintained classification rules.

When upgrading from the retired Python File Finder, Project Notes imports existing search locations and classification rules once. The retired plugin, settings dialog, and background thread are not used after migration.

## Related Documentation

- [Office 365 Integration](<Office365Integration.md>) — Configure Microsoft identity and sign-in independently of File Finder.
- [Plugin Settings](<PluginSettings.md>) — Configure the remaining Python-based plugins.
