# Adding Functionality to Project Notes

Project Notes includes a number of standard Python plugins. These plugins can be copied and modified in order to suit the needs of your organization. It is recommended that you build your own installation that includes your custom plugins as well as additional instructions.

Functionality can be extended using the [Python](<http://www.python.org>) scripting language to process input XML from Project Notes and return XML. Extensions are generally access from right-click menus and the Plugins menu. Some extensions can be added to startup and shutdown as well as regularly occurring timers. Project Notes is distributed with the Python run time. The [Qt](<http://qt.io>) framework is distributed is included as well. Project Notes is written based upon the Qt framework in order to make it a cross platform application. In order to take advantage of the Qt framework in plugins the [PyQt6](<http://https://www.riverbankcomputing.com/software/pyqt/>) module for Python is also distributed with Project Notes.

For documentation on the Qt framework see [https://doc.qt.io/qt-6/index.html.](<https://doc.qt.io/qt-6/index.html>)

For examples on how to use PyQt6 see [https://build-system.fman.io/pyqt6-tutorial](<https://build-system.fman.io/pyqt6-tutorial>)

### Muiltithreading

There are two fundamental types of plugins main thread and background threads. The main thread plugins reside in the plugins folder and can display user interface elements.  Background threads reside in the threads folder and should not display user interface items. The Qt framework does not support messaging UI elements in background threads.  Both plugin types are based upon Python scripting, however each require different elements to be present within the script.

### Python Script File Locations

When Project Notes starts it looks into the "plugins" and "threads" folder and executes all files ending in ".py". Events are customized when their corresponding functions are defined in a Python plugin.

### Python Script File Structures

Python script files follow a general format. This helps to quickly find code when modifying or debugging the script. In most cases, an existing script can be copied to create a new plugin.

### Plugin Naming
The global variables pluginname and plugindescription are the plugin name and description are used to identify plugins internally.  The configuration group in the Windows registy or Linux and MacOS configuation files is where a plugins settings are stored.  Below is an example of how these variables are defined in Python.

```python
# Project Notes Plugin Parameters
pluginname = "Base Plugins Settings" # name used in the menu
plugindescription = "This plugin provide settigns input for the base install set of plugins. Supported platforms: Windows, Linux, MacOS"
```

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

When a plugin is called from the Plugins menu, Startup event, Shutdown event, or timer event, no filtering of rows is applied. If a plugintable is defined, all records in that table are passed to the event. This is a very time consuming option, and should rarely be used. For all other events a plugintable should be defined. The right click on a list view will show the plugin option if the plugintable defined is for the same table that is displayed. The childtablesfilter variable does not have to be defined. You should always try to filter the XML that is sent to the plugin to improve performance. When the childtablesfilter is defined, Project Notes will only include related tables listed. Table names in the string should be separated by the forward slash (/).

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

All elements have an "id" attribute. On an import this attribute is used as a unique identifier and can cause existing data to be overridden. To insert new data, do not used the id identifier or other unique identifiers such as a name or project number when returning or importing XML.


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
<row id="15970981060009492" delete=true>
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
import platform

if (platform.system() == 'Windows'):
  from includes.excel_tools import ProjectNotesExcelTools
  import win32com

from includes.common import ProjectNotesCommon
from PyQt5 import QtGui, QtCore, QtWidgets, uic
from PyQt5.QtXml import QDomDocument, QDomNode
from PyQt5.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt5.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt5.QtGui import QDesktopServices


# Project Notes Plugin Parameters*
pluginname = "Schedule Customer Lessons Learned"
plugindescription = "Using Outlook create an invite to the customer lessons learned session."
plugintable = "projects" # the table or view that the plugin applies to. This will enable the right click*
childtablesfilter = "projects/project_people" # a list of child tables that can be sent to the plugin. This will be used to exclude items like notes or action items when they aren't used*

# events must have a data structure and data view specified*
#*
# Structures:*
#   string     The event will pass a python string containing XML and will expect the plugin to return an XML string*
#*
# Data Views:*
#   clients*
#   people*
#   projects*
#   project_people*
#   status_report_items*
#   project_locations*
#   project_notes*
#   meeting_attendees*
#   item_tracker_updates*
#   item_tracker*

# Supported Events*

# def event_startup(xmlstr):*
#   return ""*
#*
# def event_shutdown(xmlstr):*
#   return ""*
#*
# def event_everyminute(xmlstr):*
#   return ""*
#*
# def event_every5minutes(xmlstr):*
#   return ""*
#*
# def event_every10minutes(xmlstr):*
#   return ""*
#*
# def event_every30Mmnutes(xmlstr):*
#   return ""*
#*
# def event_menuclick(xmlstr):*
#   return ""*

# Parameters specified here will show in the Project Notes plugin settings window*
# the global variable name must be specified as a string value to be read by project notes*
# Project Notes will set these values before calling any defs*

# Project Notes Parameters*
parameters = [
]

# this plugin is only supported on windows*
if (platform.system() == 'Windows'):
  pnc = ProjectNotesCommon()
  pne = ProjectNotesExcelTools()

  **def** event_data_rightclick(xmlstr):
    print("called event: " + **__file__**)

    xmlval = QDomDocument()
    xmldoc = ""
    window_title = ""
    
    if (xmlval.setContent(xmlstr) == **False**):
      QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)
      return ""
      
    outlook = win32com.client.Dispatch("Outlook.Application")
    message = outlook.CreateItem(1)

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node    *
    pm = xmlroot.toElement().attribute("managing_manager_name")

    email = None
    nm = None

    teammember = pnc.find_node(xmlroot, "table", "name", "project_people")
    if teammember:
      memberrow = teammember.firstChild()

      while not memberrow.isNull():
        nm = pnc.get_column_value(memberrow, "name")
        email = pnc.get_column_value(memberrow, "email")
        if nm != pm:
          if (email is not None **and** email != ""):
            message.Recipients.Add(email)

        memberrow = memberrow.nextSibling()

    project = pnc.find_node(xmlroot, "table", "name", "projects")
    if project:
      projectrow = project.firstChild()

      if not projectrow.isNull():
        window_title = pnc.get_column_value(projectrow, "project_number") + " " + pnc.get_column_value(projectrow, "project_name") + " - Lessons Learned"
        message.Subject = window_title

    txt = get_text_invite()
    message.MeetingStatus = 1
    message.Duration = 60
    message.Location = pnc.get_plugin_setting("DefaultMeetingLocation")
    message.Body = txt
    outlook.ActiveExplorer().Activate()
    message.Display()

    outlook = None
    message = None

    pnc.bring_window_to_front(window_title)

    return xmldoc
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

