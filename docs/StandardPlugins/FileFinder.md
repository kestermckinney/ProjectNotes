# File Finder

The **File Finder** runs as a background process. Every minute, the plugin scans all configured folders and looks for files related to your projects, automatically adding them to the **Files & Folders** tab on the Project Page. This keeps your project file list up to date without manual maintenance.

## How It Works

The File Finder matches files to projects using the **project number** found in the file name or folder path. When a matching file is found that is not already listed in the project's Artifacts, the plugin adds it automatically with the appropriate file type based on the **Classifications** matching rules.

### Project Number Matching

Files are matched to projects by extracting the project number from their path or filename. For example, if you have a project with number `ABC-001`, the File Finder will locate and associate files like:
- `C:\Projects\ABC-001\Schedule.mpp`
- `C:\Projects\ABC-001_Quotes\Proposal.pdf`
- Any file in a folder containing `ABC-001`

## Configuring File Finder Settings

**To open the File Finder settings:**

1. From the **Plugins** menu choose **Settings > File Finder**.

The settings dialog has two sections:

### Search Locations

Specifies the base folder(s) where File Finder searches for project-related files. The plugin will **recursively search** all subfolders within each location.

#### How Base Locations Work

When you add a search location, File Finder scans:
- The root folder itself and all its subfolders
- All files found in any subdirectory tree
- Project folders and shared file repositories

**Example:** If you add `C:\Users\user\Documents\Projects` as a search location, File Finder will find files in:
- `C:\Users\user\Documents\Projects\ABC-001\Schedule.mpp` ✓
- `C:\Users\user\Documents\Projects\Shared Files\ABC-001.pdf` ✓
- `C:\Users\user\Documents\Projects\Archive\Old Projects\XYZ-005\Proposal.docx` ✓

#### Default Location

By default, File Finder is configured to search your user's Documents folder under `Projects`:
- **Windows**: `C:\Users\[YourUsername]\Documents\Projects`
- **macOS**: `/Users/[YourUsername]/Documents/Projects`
- **Linux**: `/home/[YourUsername]/Documents/Projects`

If you store projects in multiple locations, you can add multiple search locations.

#### Managing Search Locations

| Button | Description |
| :--- | :--- |
| **Add Location** | Add a new folder to scan. Choose any folder where you store project files. |
| **Edit Location** | Edit the selected folder path. Useful if you relocate your project directories. |
| **Delete Location** | Remove the selected folder from the scan list. The folder itself is not deleted, just removed from File Finder's search scope. |

### File Classifications

Classification rules determine how files are categorized when added to the Files & Folders list. Each rule uses **pattern matching** (regular expressions) to identify files by name and automatically assign them a file type and description.

#### Understanding Pattern Matching

Pattern matching uses **regular expressions** to match file paths and names. This allows flexible rules that can match multiple related files with a single pattern.

**Common Pattern Elements:**

| Pattern | Means | Example Match |
| :--- | :--- | :--- |
| `.` | Any single character | `file.txt` matches `a.txt` |
| `.*` | Any characters (zero or more) | `.*Project.*` matches `MyProject`, `Project_2024`, `ProjectNotes` |
| `\.mpp$` | File extension `.mpp` at end of filename | `Schedule.mpp` (matches), `schedule.mpp.bak` (doesn't match) |
| `$` | End of string (anchors pattern) | `\.xlsx$` only matches Excel files, not `file.xlsx.tmp` |
| `\|` | OR (match one pattern or another) | `\.(pdf\|docx)$` matches both PDF and Word files |
| `[0-9]` | Any digit | `Project[0-9].pdf` matches `Project1.pdf`, `Project5.pdf` |
| `Schedule` | Literal text | `Schedule.mpp` (matches), `SCHEDULE.MPP` (case-sensitive, doesn't match by default) |

#### Pattern Matching Examples

**Example 1: Match all project schedules**
```
Pattern: .*Project Management/Schedule.*\.mpp$
Matches:
  ✓ C:\Projects\ABC-001\Project Management\Schedule.mpp
  ✓ C:\Projects\ABC-001\Project Management\Schedule_Final.mpp
  ✓ C:\Archive\Project Management\Schedule_2024.mpp
```

**Example 2: Match quotes (PDF or Excel)**
```
Pattern: .*Project Management/Quotes.*\.(pdf|xlsx)$
Matches:
  ✓ C:\Projects\ABC-001\Project Management\Quotes\Proposal.pdf
  ✓ C:\Projects\ABC-001\Project Management\Quotes\Budget_Estimate.xlsx
  ✗ C:\Projects\ABC-001\Quotes.docx (doesn't match — not PDF or Excel)
```

**Example 3: Match meeting minutes by name**
```
Pattern: .*Project Management/Meeting Minutes/.*\.(pptx|ppt|docx)$
Matches:
  ✓ C:\Projects\ABC-001\Project Management\Meeting Minutes\Kickoff.pptx
  ✓ C:\Projects\ABC-001\Project Management\Meeting Minutes\Status_Jan2024.docx
  ✓ C:\Projects\ABC-001\PM\Meeting Minutes\Design Review.ppt
```

**Example 4: Match PCR (change request) documents**
```
Pattern: .*Project Management/PCR.*\.(pdf|docx|xlsx)$
Matches:
  ✓ C:\Projects\ABC-001\Project Management\PCRs\PCR_001.pdf
  ✓ C:\Projects\ABC-001\Project Management\PCR Management\PCR_Tracking.xlsx
  ✗ C:\Projects\ABC-001\PCR_Backup.pdf.old (doesn't match — .old extension)
```

**Example 5: Match risk registers**
```
Pattern: .*Project Management/Risk Management.*\.(xlsx|docx)$
Matches:
  ✓ C:\Projects\ABC-001\Project Management\Risk Management\Risk Register.xlsx
  ✓ C:\Projects\ABC-001\Project Management\Risk_Analysis\Risks.docx
  ✗ C:\Projects\ABC-001\Risk.txt (doesn't match — not in Risk Management folder, wrong type)
```

#### Managing Classifications

| Button | Description |
| :--- | :--- |
| **Add Classification** | Create a new file classification rule with a pattern and file type. |
| **Edit Classification** | Modify an existing rule's pattern or file type. |
| **Delete Classification** | Remove a classification rule. Files previously added with this rule are not affected. |

#### Tips for Pattern Matching

- **Be specific**: More specific patterns prevent false matches. `.*Risk.*\.xlsx$` is better than `.*Risk.*`
- **Use folder structure**: Include folder names in your patterns to improve accuracy
- **Test your patterns**: Add test files and run File Finder to verify patterns work as expected
- **Order matters**: File Finder applies the first matching pattern, so order similar rules appropriately
- **Case sensitivity**: Patterns are case-sensitive by default. Use lowercase file extensions or adjust patterns as needed

## Notes

- The File Finder only adds files; it does not remove or modify existing Files & Folders entries.
- Files are added with the **Project Folder** file type when a matching project folder is found.
- The plugin runs only while Project Notes is open.
- Pattern matching uses regular expressions; refer to standard regex syntax for complex patterns.

## Related Documentation

- [Standard Plugins Overview](<PluginSettings.md>) — Complete list of all standard plugins and their configuration options
