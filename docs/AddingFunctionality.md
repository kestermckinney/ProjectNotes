# Adding Functionality to Project Notes

Functionality can be extended using the [Python](<www.python.org>) scripting language to process input XML from Project Notes and return XML. Extensions are generally access from right-click menus and the Plugins menu. Some extensions can be added to startup and shutdown as well as regularly occurring timers. Project Notes is distributed with the Python run time. The [Qt](<http://qt.io>) framework is distributed is included as well. Project Notes is written based upon the Qt framework in order to make it a cross platform application. In order to take advantage of the Qt framework in plugins the [PyQt6](<http://https://www.riverbankcomputing.com/software/pyqt/>) module for Python is also distributed with Project Notes.

For documentation on the Qt framework see [https://doc.qt.io/qt-6/index.html.](<https://doc.qt.io/qt-6/index.html>)

For examples on how to use PyQt6 see [https://build-system.fman.io/pyqt6-tutorial](<https://build-system.fman.io/pyqt6-tutorial>)

### Python Script File Locations

When Project Notes starts it looks into the "plugins" folder and executes all files ending in ".py". Events are customized when their corresponding functions are defined in a Python plugin. The variables also define Property values the plugin will use.

### Python Script File Structures

Python script files follow a general format. This helps to quickly find code when modifying or debugging the script. In most cases, you will copy an existing script in order to create a new plugin.

### Plugin Naming
The global variables pluginname and plugindescription are the plugin name and description that appear in the Project Notes [Plugin Settings](<PluginSettings.md>) window.

**Properties**

Properties are global variables used in Python script. To tell Project Notes to use a global variable as a property the name of the global variable needs to appear in the global parameters array as shown below.

```python
#Project Notes Parameters
parameters = [
  "EditorFullPath"
]
```

### Plugin Options

When a plugin is called from the Plugins menu, Startup event, Shutdown event, or timer event, no filtering of rows is applied. If a plugintable is defined, all records in that table are passed to the event. This is a very time consuming option, and should rarely be used. For all other events a plugintable should be defined. The right click on a list view will show the plugin option if the plugintable defined is for the same table that is displayed. The childtablesfilter variable does not have to be defined. You should always try to filter the XML that is sent to the plugin to improve performance. When the childtablesfilter is defined, Project Notes will only include related tables listed. Table names in the string should be separated by the forward slash (/).

### Extendable Events

The events listed in the table below can be extended in Python plugins. In each event, the specified plugin table and related child records are converted to XML and passed to the plugin as a Python string. If the plugin is called from a right-click on a data item, the XML will only include that item and it's related children. For example, if you right-click a project in the Project List and choose **Export** from the **File** menu, the data exported is the same data that will be exported to a plugin. Using the XML Import and XML Export features of Project Notes are very useful in testing and debugging a Python script plugin. If the defined event returns an XML string, the XML is processed in the same was as the XML Import. Importing XML will update and insert records. There is currently no feature to delete records.

| **Events** | **Description** |
| :--- | :--- |
| def event_startup(xmlstr): | Each time Project Notes is started this event is called if it is defined. |
| def event_shutdown(xmlstr): | Each time Project Notes is shutdown this event is called if it is defined. |
| def event_everyminute(xmlstr): | Every minute this event is called if it is defined. |
| def event_every5minutes(xmlstr): | Every 5 minutes this event is called if it is defined. |
| def event_every10minutes(xmlstr): | Every 10 minutes this event is called if it is defined. |
| def event_every30Mmnutes(xmlstr): | Every 30 minutes this event is called if it is defined. |
| def event_menuclick(xmlstr): | The pluginname will appear in the Plugins menu. When an user selects the menu option, the plugin is called. |
| def event_data_rightclick(xmlstr): | The pluginname will appear when the list is right clicked and the list contains data from the defined plugintable. When a list is right-clicked and the corresponding plugintable is specified, this event is called if it is defined. |  

<br>

### Tables

The tables defined below are options for the data to be passed to the [Python](<www.python.org>) script event function. When defining an event, it is important that the table corresponds with the item associated with the right-click. The XML structures are complex. An export will help you understand the structures betters.

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

The example below show an XML export of a person. Notice child tables contain the **"*filter\_field"*** and **"*filter\_value"*** attributes to define the parent child relationship of the data. Many attributes such as file names and column numbers are provided to give the export further context, but are not used in the import.

```xml
<xml version="1.0" encoding="UTF-8">
<projectnotes filepath="C:\Users\joe\Sample.db" export_date="12/11/2020 01:01 PM" filter_field="people_id" project_manager_id="59709810500028597" managing_company_id="1597098105000493" managing_company_name="Sample Company, Inc." managing_manager_name="Jacob Smith" filter_values="{ba96fb89-6c2d-46db-864c-5be6292b10ef}">
<table name="ix_people" filter_field="people_id" filter_value="{ba96fb89-6c2d-46db-864c-5be6292b10ef}">
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
from PyQt5 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt5.QtSql import QSqlDatabase
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