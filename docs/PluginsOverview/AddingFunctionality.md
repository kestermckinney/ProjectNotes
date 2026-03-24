# Adding Functionality to Project Notes

Project Notes is designed to be extended with custom functionality using Python plugins. The application includes a number of standard plugins that can be copied and modified for your organization's needs. It is recommended that you build your own installation that includes custom plugins tailored to your workflow.

## Plugin Basics

Functionality is extended using the [Python](<http://www.python.org>) scripting language to process input XML from Project Notes and return XML. Extensions are accessed from right-click menus, the Plugins menu, and can be triggered on startup, shutdown, or at regular intervals using timers.

Project Notes includes the Python runtime, the [Qt](<http://qt.io>) framework, and the [PyQt6](<https://www.riverbankcomputing.com/software/pyqt/>) module for Python. This allows plugins to access the full Qt framework for building user interfaces and leveraging cross-platform capabilities.

**For Qt framework documentation:** [https://doc.qt.io/qt-6/index.html](<https://doc.qt.io/qt-6/index.html>)

**For PyQt6 tutorials:** [https://build-system.fman.io/pyqt6-tutorial](<https://build-system.fman.io/pyqt6-tutorial>)

## Plugin Architecture

### Two Types of Plugins: Main Thread vs Background Threads

Project Notes supports two fundamentally different types of plugins:

**Main Thread Plugins (plugins folder)**
- Reside in the `plugins` folder
- Execute on the Qt main thread
- Can display user interface elements (dialogs, windows, message boxes)
- Have access to Qt GUI classes via PyQt6
- Used for interactive tools, settings dialogs, and user-facing features

**Background Thread Plugins (threads folder)**
- Reside in the `threads` folder
- Execute in a separate worker thread
- Cannot display any user interface elements
- Must not interact with Qt GUI classes from the background thread
- Used for long-running tasks, timers, and automated processes
- Can write results back to the database using the `projectnotes` module

### Why GUI Elements Cannot Be in Background Threads

The Qt framework is not thread-safe for GUI operations. All GUI element creation and manipulation must happen on the main Qt thread. If you attempt to create GUI elements (dialogs, buttons, windows) from a background thread, the application will crash or exhibit unpredictable behavior.

If a background thread plugin needs to display information or get user input, it should:
1. Prepare the data needed
2. Write the results to the database using `projectnotes.update_data()`
3. Let the user interact with the main thread UI to view or modify the results

### Plugin File Locations

When Project Notes starts, it automatically:
1. Scans the `plugins` folder for `.py` files and executes them
2. Scans the `threads` folder for `.py` files and executes them
3. Looks for event handler functions in each plugin (if defined)
4. Registers menu items defined in `pluginmenus` variables

Any changes made to `.py` files in these folders are automatically detected, and the plugin is reloaded. You do not need to restart Project Notes to test plugin changes.

### Auto-Reload on File Changes

Project Notes monitors the `plugins` and `threads` folders for changes. When you save a modified `.py` file:
1. The old plugin is unloaded (the `event_shutdown` function is called if defined)
2. The new version of the plugin is loaded
3. The `event_startup` function is called if defined
4. Menu items are re-registered with the new definitions

This allows rapid development and testing. You can edit a plugin, save it, and immediately test the changes in the UI without restarting the application.

**Important:** If your plugin has syntax errors, the reload will fail and an error message will appear. Fix the syntax error, save the file again, and the plugin will reload when the syntax is correct.

## Required Plugin Structure and Global Variables

Every plugin must define certain global variables that Project Notes uses to identify the plugin, configure its behavior, and register its menu items. These variables are read when the plugin is loaded.

### Essential Global Variables

Every plugin must include these variables:

```python
# Plugin identification (REQUIRED)
pluginname = "My Plugin Name"
plugindescription = "Brief description of what this plugin does. Supported platforms: Windows, Linux, MacOS"
pluginmenus = []  # List of menu items (can be empty)
```

**pluginname** — A unique name used to identify the plugin throughout Project Notes. This name is used as the configuration group in QSettings (see Settings Storage section below). It appears in the Plugins menu and in log output. Choose a descriptive name that won't conflict with other plugins.

**plugindescription** — A brief description of the plugin's purpose and which platforms it supports. This should clearly explain what the plugin does and any platform-specific behavior.

**pluginmenus** — A Python list of dictionaries that define menu items for the plugin (see Plugin Menu Customization section below). This can be an empty list if the plugin only uses events and doesn't add menu items.

### Optional Global Variables

For timer-based plugins in the `threads` folder:

```python
plugintimerevent = 1  # Timer interval in minutes (default: 1)
```

**plugintimerevent** — Specifies how frequently the `event_timer` function is called, in minutes. If not defined, the default is 1 minute. This is only relevant for plugins in the `threads` folder.

### Settings Storage with QSettings

Plugin settings are persisted to disk using Qt's QSettings mechanism. On Windows, settings are stored in the registry under `HKEY_CURRENT_USER\Software\Anthropic\ProjectNotes\<pluginname>`. On Linux and macOS, they are stored in configuration files in the user's home directory.

Settings are key-value pairs that survive application restarts. The typical workflow is:

1. Load settings when the plugin starts or when a settings dialog is opened
2. Allow the user to modify settings via a dialog
3. Save settings back to QSettings
4. Use the settings to control plugin behavior

### Storing Settings in Your Plugin

Use the `ProjectNotesCommon` class to interact with settings:

```python
from includes.common import ProjectNotesCommon

class MyPlugin:
    def __init__(self):
        self.pnc = ProjectNotesCommon()
        self.pluginname = "My Plugin Name"

        # Load a setting (returns empty string if not found)
        self.my_setting = self.pnc.get_plugin_setting("SettingName", self.pluginname)

        # Save a setting
        self.pnc.set_plugin_setting("SettingName", "value", self.pluginname)
```

**get_plugin_setting(key, pluginname)** — Retrieves a setting value from QSettings. Returns an empty string if the key doesn't exist.

**set_plugin_setting(key, value, pluginname)** — Stores a setting value in QSettings.

### Best Practice: Providing a Settings Dialog

The standard practice is to create a settings dialog that allows users to configure your plugin. This dialog should be accessible from the **Plugins > Settings > [Your Plugin Name]** menu item.

A typical settings dialog:

1. Loads current settings from QSettings when the dialog opens
2. Displays controls (text fields, dropdowns, checkboxes) for each setting
3. When the user clicks OK/Save, writes the settings back to QSettings
4. Updates the running plugin behavior with the new settings

Example menu definition:

```python
pluginmenus = [
    {
        "menutitle": "My Plugin Settings",
        "function": "show_settings_dialog",
        "tablefilter": "",
        "submenu": "Settings",
        "dataexport": "",
        "parameter": ""
    }
]

def show_settings_dialog(parameter=None):
    dialog = MyPluginSettingsDialog()
    dialog.exec()  # Shows the dialog and waits for user action
```

The settings dialog (using PyQt6) might look like:

```python
from PyQt6.QtWidgets import QDialog, QVBoxLayout, QLabel, QLineEdit, QPushButton

class MyPluginSettingsDialog(QDialog):
    def __init__(self):
        super().__init__()
        self.pnc = ProjectNotesCommon()
        self.pluginname = "My Plugin Name"
        self.init_ui()
        self.load_settings()

    def init_ui(self):
        layout = QVBoxLayout()

        layout.addWidget(QLabel("Setting 1:"))
        self.setting1_input = QLineEdit()
        layout.addWidget(self.setting1_input)

        save_button = QPushButton("Save")
        save_button.clicked.connect(self.save_settings)
        layout.addWidget(save_button)

        self.setLayout(layout)
        self.setWindowTitle("My Plugin Settings")

    def load_settings(self):
        """Load settings from QSettings and populate the dialog"""
        setting1 = self.pnc.get_plugin_setting("Setting1", self.pluginname)
        self.setting1_input.setText(setting1)

    def save_settings(self):
        """Save settings from the dialog to QSettings"""
        self.pnc.set_plugin_setting("Setting1", self.setting1_input.text(), self.pluginname)
        self.accept()  # Close the dialog
```

This pattern makes your plugin configurable and user-friendly, with settings that persist across application restarts.

## Embedded Python Functions

Project Notes exposes the following functions to plugins through the `projectnotes` module:

### projectnotes.update_data(xml_string)

Inserts, updates, or deletes records in the database by processing XML. This is the primary way plugins modify the database.

**Parameters:**
- `xml_string` — An XML string containing the data to import (see XML format examples below)

**Returns:** Nothing (but may trigger UI updates in Project Notes)

**How it works:**
1. Records are matched by their `id` attribute if present
2. If no `id` is present, records are matched by unique columns (like `name` or `project_number`)
3. Matching records are updated with the new values
4. Non-matching records are inserted as new records
5. Records with `delete="true"` are deleted from the database

**Example:**

```python
import projectnotes

# Update or insert a person
contact_xml = """<?xml version="1.0" encoding="UTF-8"?>
<projectnotes>
<table name="people">
    <row id="{3a5adf23-3af4-40c6-bb3c-3ed5baaec0a5}">
        <column name="people_id">{3a5adf23-3af4-40c6-bb3c-3ed5baaec0a5}</column>
        <column name="name">John Smith</column>
        <column name="email">john@company.com</column>
    </row>
</table>
</projectnotes>"""

projectnotes.update_data(contact_xml)
```

### projectnotes.get_data(xml_string)

Retrieves records from the database as XML. Used to fetch data for processing.

**Parameters:**
- `xml_string` — An XML string specifying which tables and filters to retrieve (see examples below)

**Returns:** An XML string containing the matching records

**How it works:**
1. Specify which tables to query using `<table name="...">` elements
2. Filter results using `filter_field_#` and `filter_value_#` attributes
3. Use `LIKE` syntax for partial matches (e.g., `name LIKE "A%"` finds names starting with A)
4. Use `skip` and `top` for pagination
5. The returned XML includes all related child records

**Example:**

```python
import projectnotes

# Fetch all people whose names start with "A"
query_xml = """<?xml version="1.0" encoding="UTF-8"?>
<projectnotes>
<table name="people" filter_field_1="name" filter_value_1="A%"/>
</projectnotes>"""

result_xml = projectnotes.get_data(query_xml)
print(result_xml)

# Fetch specific clients with pagination
query_xml = """<?xml version="1.0" encoding="UTF-8"?>
<projectnotes>
<table name="clients" filter_field_1="name" filter_value_1="%" skip="5" top="10"/>
</projectnotes>"""

result_xml = projectnotes.get_data(query_xml)
```

## Understanding the Delete Attribute: Dangers and Best Practices

The `delete="true"` attribute in XML allows plugins to delete records from the database. However, using this feature requires careful consideration of database foreign key relationships.

### The Danger of Using Delete Without Care

Every record in the database may have related records in other tables. For example:

- **A Person** may have related records in `meeting_attendees` table (meetings they attended)
- **A Project** may have related records in `project_notes`, `item_tracker`, `meeting_attendees`, `project_locations`, `project_people`, etc.
- **A Meeting Note** may have related `item_tracker` (action items) and `meeting_attendees` records

**If you delete a parent record without deleting its related child records, you create orphaned records with broken foreign key relationships.** This can cause:

- UI errors when trying to display the orphaned records
- Export errors when the orphaned records cannot find their parent
- Data integrity issues and inconsistent state

### Safe Deletion Practice

When deleting a record, you must either:

1. **Delete all related child records first** — Include all related records in your delete XML
2. **Use the Filter Tool to remove relationships first** — Update child records to remove the relationship before deleting the parent

**Example of safe deletion:**

```python
import projectnotes

# SAFE: Delete a person AND all their related meeting attendee records
delete_xml = """<?xml version="1.0" encoding="UTF-8"?>
<projectnotes>
<table name="meeting_attendees">
    <row delete="true">
        <column name="people_id">{person-id}</column>
    </row>
</table>
<table name="people">
    <row delete="true" id="{person-id}">
        <column name="people_id">{person-id}</column>
    </row>
</table>
</projectnotes>"""

projectnotes.update_data(delete_xml)
```

**Example of unsafe deletion:**

```python
# UNSAFE: Deleting a person without deleting their meeting attendee records
# This leaves orphaned records in meeting_attendees pointing to a non-existent person!
delete_xml = """<?xml version="1.0" encoding="UTF-8"?>
<projectnotes>
<table name="people">
    <row delete="true" id="{person-id}">
        <column name="people_id">{person-id}</column>
    </row>
</table>
</projectnotes>"""

projectnotes.update_data(delete_xml)  # DANGEROUS - orphans created!
```

### Best Practice Recommendation

Unless you have a specific reason to delete records, consider:
- **Marking records as inactive or archived** instead of deleting them
- **Creating a data cleanup script** that safely handles deletions with proper foreign key checks
- **Testing deletions thoroughly** with sample data before deploying to production

### Plugin Menu Customization

Project Notes plugin architecture provide the ability to add new menu items to the **Plugins** menu as well as right-click menus for specific types of data. Plugin menus are defined using a Python list of dictionaries.  The pluginmenus list variable can be populated statically as shown below, or built dynamically.

```python
pluginmenus = [
    {"menutitle" : "File Collector", "function" : "menuFileCollectorSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Editor", "function" : "menuEditorSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Outlook Integration", "function" : "menuOutlookIntegrationSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "My Shortcuts", "function" : "menuMyShortcutSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Meeting and Email Types", "function" : "menuMeetingEmailTypesSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Settings Migrator", "function" : "menuSettingsMigrator", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
]
```

The example below demonstrates how a plugin menu can be generated dynamically at load time.

```python
    pluginmenus.append({"menutitle" : "Open MS Project", "function" : "menuOpenMSProject", "tablefilter" : "projects/project_locations", "submenu" : "", "dataexport" : "projects"})
```

#### Dictionary Keys for Menus
| **Key** | **Description** |
| :--- | :--- |
| menutitle | The text that will show in the right-click or **Plugins** menu. |
| function | The python function that will be called when the menu is triggered. For functions called from the **Plugins** menu, only one parameter is provided.  For functions called from a right-click on a data type, the first parameter is an XML string, and the second is a string parameter. |
| tablefilter | The table filter is used to filter the xml sent to the menu event.  If the value is empty the entire data tree will be exported.  Applying the table filter reduces the cost of generating the xml.  For example when exporting a project record all data associated with the project will be exported if a filter is not applied.  If a filter of "projects/project_notes" were used only the project record and assocated notes records would be included. |
| submenu | If a submenu value is specified the menutitle will appear under the submenu text. |
| dataexport | The record type the menu applies to.  If this value is empty, the **Plugins** menu is used. |

<br>

### Plugin Options

When a plugin is called from the Plugins menu, Startup event, Shutdown event, or timer event, no XML export is sent to the event handler.  For all other events a dataexport table should be defined. The right click on a list view will show the plugin option if the dataexport table defined is for the same table that is displayed. The tablesfilter variable does not have to be defined. You should always try to filter the XML that is sent to the plugin to improve performance. When the tablesfilter is defined, Project Notes will only include related tables listed. Table names in the string should be separated by the forward slash (/).  See the example in the Python script above.

### Extendable Events

The events listed in the table below can be extended in Python plugins. In each event, the specified plugin table and related child records are converted to XML and passed to the plugin as a Python string. If the plugin is called from a right-click on a data item, the XML will only include that item and it's related children. For example, if you right-click a project in the Project List and choose **Export** from the **File** menu, the data exported is the same data that will be exported to a plugin. Using the XML Import and XML Export features of Project Notes are very useful in testing and debugging a Python script plugin. If the defined event returns an XML string, the XML is processed in the same was as the XML Import. Importing XML will update, insert, or delete records.

| **Event** | **Description** |
| :--- | :--- |
| event\_startup | The startup event executes when the the plugin is first loaded. |
| event\_shutdown | The shutdown executes just before the plugin is unloaded. |
| event\_timer | This event is only used in a thread.  The frequency the event is triggered is defined by the "plugintimerevent" variable defined in the script.  If the variable is not defined the default value is 1 for every one minute. |

<br>

### Tables

The tables defined below are options for the data to be passed to the [Python](<http://www.python.org>) script event function. When defining an event, it is important that the table corresponds with the item associated with the right-click. The XML structures are complex. An export will help you understand the structures betters.

Each exported `<row>` contains the record's primary key as its first `<column>` element. On import, you can optionally add an `id` attribute directly on the `<row>` element to target a specific existing record for update or delete — this overrides unique-key matching. To insert new data, omit both the `id` attribute and any primary key column value so Project Notes generates a new identifier.


| **Data View** | **Description** |
| :--- | :--- |
| clients | The exported XML contains the client names and associated people. |
| people | The exported XML contains the people and the associated clients. |
| projects | The exported XML is all inclusive of all project elements including people, meetings notes, meeting action items, meeting attendees, tracker items, locations, notes, project team members, and project locations. |
| project_people | The exported XML contains all the people associated with a project and their related companies. |
| status_report_items | The exported XML contains the status report items associated with a project. |
| project_locations | The exported XML contains the project locations associated with a project. |
| project_notes | The exported XML contains the project notes associated with a project. |
| meeting_attendees | The exported XML contains the meeting attendees for project notes associated with a project. |
| item_tracker_updates | The exported XML contains notes attached to tracker and action items. |
| item_tracker | The exported XML contains action or tracker items. |

<br>

### Basic XML formats

The example below show an XML export of a person. Notice child tables contain the **"*filter\_field"*** and **"*filter\_value"*** attributes to define the parent child relationship of the data. Many attributes such as file names and column numbers are provided to give the export further context, but are not used in the import. A more detailed description can be found in [Project Notes XML](ProjectNotesXML.md). 

```xml
<xml version="1.0" encoding="UTF-8">
<projectnotes filepath="C:\Users\joe\Sample.db" export_date="12/11/2020 01:01 PM" filter_field="people_id" project_manager_id="59709810500028597" managing_company_id="1597098105000493" managing_company_name="Sample Company, Inc." managing_manager_name="Jacob Smith" filter_values="{ba96fb89-6c2d-46db-864c-5be6292b10ef}">
<table name="ix_people" filter_field_1="people_id" filter_value_1="{ba96fb89-6c2d-46db-864c-5be6292b10ef}">
<row delete=true>
<column name="people_id">{ba96fb89-6c2d-46db-864c-5be6292b10ef}</column>
<column name="name">Aaron Brown</column>
<column name="email">Aaron.Brown@somecompany.com</column>
<column name="office_phone">(555) 555-2459</column>
<column name="cell_phone">(555) 555-1224</column>
<column name="client_id" lookupvalue="Simco, Inc.">{ba96fb89-6c2d-46db-864c-5be6292b10ef}</column>
<column name="role">Programming L4ead</column>
</row>
</table>
<projectnotes>
```

### Code Example

The plugin architecture calls event functions if they have been defined. Below is a common section of code used to respond to events. See other plugins installed with Project Notes for more examples.

```python
# make sure includes folder can be found
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../plugins')))

from includes.common import ProjectNotesCommon
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QDateTime, QElapsedTimer, QDir, QDirIterator, QFileInfo

# Project Notes Plugin Parameters
pluginname = "File Finder Thread" # name used in the menu
plugindescription = "This is test thread. Supported platforms: Windows, Linux, MacOS"
plugintimerevent = 1 # how many minutes between the timer event

pluginmenus = [
    {"menutitle" : "Find All Fiiles", "function" : "event_timer", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : "", "parameter" : "all"},
    {"menutitle" : "Find Project Files", "function" : "event_data_rightclick", "tablefilter" : "projects", "submenu" : "Utilities", "dataexport" : "projects", "parameter" : ""}
]

# all events return an xml string that can be processed by ProjectNotes
#
# Supported Events

# def event_startup(parameter):
#     return
#
# def event_shutdown(parameter):
#     return
#
# def event_timer(parameter):
#     return
#

class  FileFinder:
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "File Finder"
        self.search_locations = self.pnc.get_plugin_setting("SearchLocations", self.settings_pluginname)
        self.classifications = self.pnc.get_plugin_setting("Classifications", self.settings_pluginname)

    # ... code removed for simplicity

def event_timer(parameter):
    ff = FileFinder()
    ff.parse_by_project((parameter == "all"))

    return ""

def event_data_rightclick(xmlstr, parameter):
    ff = FileFinder()

    projectnumber = ff.get_projectnumber(xmlstr)

    if projectnumber is not None:
        ff.parse_by_project(False, projectnumber)

    return ""
```

### Updating Data

Records can be updated from embeded python using the *update_data* method found in the embeded *projectnotes* module.  Project Notes uses the built in XML import capabilities.  Records are identified first by their *id* if available, and then by unique column values such as *name* or *project_number*.  See the code below for an example.

```python
import projectnotes

contact = """<?xml version="1.0" encoding="UTF-8"?>
<projectnotes>
<table name="people">
        <row id="{3a5adf23-3af4-40c6-bb3c-3ed5baaec0a5}">
            <column name="people_id">{3a5adf23-3af4-40c6-bb3c-3ed5baaec0a5}</column>
            <column name="name">Adam Lester</column>
            <column name="email">adam@someplace.com</column>
            <column name="office_phone"></column>
            <column name="cell_phone"></column>
            <column lookupvalue="Some Company" name="client_id">16757088310005279</column>
            <column name="role"></column>
        </row>
</table>
</projectnotes>
"""

projectnotes.update_data(contact)

```

### Fetching Data

Records can be retrieved from embeded python using the *get_data* method found in the embeded *projectnotes* module.  Project Notes uses the built in XML export capabilities.  Records are filtered by the Sqlite LIKE operator based on the *filter_field_#* and *filter_value_#* attributes.  The *skip* attribute can be used to skip over the specified records. The *top* attribute must also be set to use the *skip* attribute.  While the *top* attribute returns the number for rows past the skip.  See the example code below.

```python

    contact = """<?xml version="1.0" encoding="UTF-8"?>
    <projectnotes>
    <table name="people" filter_field_1="name" filter_value_1="A%"/>
    <table name="clients" filter_field_1="name" filter_value_1="A%" skip=5 top=10/>
    </projectnotes>
    """

    print(projectnotes.get_data(contact))
``` 

