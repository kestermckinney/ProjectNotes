# Adding Functionality

# Adding Functionality to Project Notes

Functionality can be extended using the [Python](<www.python.org>) scripting language to process input XML from Project Notes and return XML.&nbsp; Extensions are generally access from right-click menus and the Plugins menu.&nbsp; Some extensions can be added to startup and shutdown as well as regularly occurring timers.&nbsp; Project Notes is distributed with the Python run time.&nbsp; The [Qt](<http://qt.io>) framework is distributed is included as well.&nbsp; Project Notes is written based upon the Qt framework in order to make it a cross platform application.&nbsp; In order to take advantage of the Qt framework in plugins the [PyQt5](<http://https://www.riverbankcomputing.com/software/pyqt/>) module for Python is also distributed with Project Notes.

&nbsp;

For documentation on the Qt framework see [https://doc.qt.io/qt-5/index.html.](<https://doc.qt.io/qt-5/index.html>)

For examples on how to use PyQt5 see [https://build-system.fman.io/pyqt5-tutorial](<https://build-system.fman.io/pyqt5-tutorial>)

### Python Script File Locations

When Project Notes starts it looks into the "plugins" folder and executes all files ending in ".py".&nbsp; Events are customized when their corresponding functions are defined in a Python plugin.&nbsp; The variables also define Property values the plugin will use.

### Python Script File Structures

Python script files follow a general format.&nbsp; This helps to quickly find code when modifying or debugging the script.&nbsp; In most cases, you will copy an existing script in order to create a new plugin.

### Plugin Naming
### The global variables pluginname and plugindescription are the plugin name and description that appear in the Project Notes [Plugin Settings](<PluginSettings.md>) window.&nbsp;

&nbsp;

**Properties**

Properties are global variables used in Python script.&nbsp; To tell Project Notes to use a global variable as a property the name of the global variable needs to appear in the global parameters array as shown below.

\
\-- Project Notes Parameters\
parameters = \[\
&nbsp; &nbsp; "EditorFullPath"\
\]

\
**Plugin Options**\
When a plugin is called from the Plugins menu, Startup event, Shutdown event, or timer event, no filtering of rows is applied.&nbsp; If a plugintable is defined, all records in that table are passed to the event.&nbsp; This is a very time consuming option, and should rarely be used.&nbsp; For all other events a plugintable should be defined.&nbsp; The right click on a list view will show the plugin option if the plugintable defined is for the same table that is displayed.&nbsp; The childtablesfilter variable does not have to be defined.&nbsp; You should always try to filter the XML that is sent to the plugin to improve performance.&nbsp; When the childtablesfilter is defined, Project Notes will only include related tables listed.&nbsp; Table names in the string should be separated by the forward slash (/).

\
**Extendable Events**

The events listed in the table below can be extended in Python plugins.&nbsp; In each event, the specified plugin table and related child records are converted to XML and passed to the plugin as a Python string.&nbsp; If the plugin is called from a right-click on a data item, the XML will only include that item and it's related children.&nbsp; For example, if you right-click a project in the Project List and choose **Export** from the **File** menu, the data exported is the same data that will be exported to a plugin.&nbsp; Using the XML Import and XML Export features of Project Notes are very useful in testing and debugging a Python script plugin.&nbsp; If the defined event returns an XML string, the XML is processed in the same was as the XML Import.&nbsp; Importing XML will update and insert records.&nbsp; There is currently no feature to delete records.

&nbsp;

| **Events** | **Description** |
| --- | --- |
| def event\_startup(xmlstr): | Each time Project Notes is started this event is called if it is defined. |
| def event\_shutdown(xmlstr): | Each time Project Notes is shutdown this event is called if it is defined. |
| def event\_everyminute(xmlstr): | Every minute this event is called if it is defined. |
| def event\_every5minutes(xmlstr): | Every 5 minutes this event is called if it is defined. |
| def event\_every10minutes(xmlstr): | Every 10 minutes this event is called if it is defined. |
| def event\_every30Mmnutes(xmlstr): | Every 30 minutes this event is called if it is defined. |
| def event\_menuclick(xmlstr): | The pluginname will appear in the Plugins menu.&nbsp; When an user selects the menu option, the plugin is called. |
| def event\_data\_rightclick(xmlstr): | The pluginname will appear when the list is right clicked and the list contains data from the defined plugintable.&nbsp; When a list is right-clicked and the corresponding plugintable is specified, this event is called if it is defined. |


### Tables

The tables defined below are options for the data to be passed to the [Python](<www.python.org>) script event function.&nbsp; When defining an event, it is important that the table corresponds with the item associated with the right-click.&nbsp; The XML structures are complex.&nbsp; An export will help you understand the structures betters.

&nbsp;

All elements have an "id" attribute.&nbsp; On an import this attribute is used as a unique identifier and can cause existing data to be overridden.&nbsp; To insert new data, do not used the id identifier or other unique identifiers such as a name or project number when returning or importing XML.\
&nbsp;

| **Data View** | **Description** |
| --- | --- |
| clients | The exported XML contains the client names and associated people. |
| people | The exported XML contains the people and the associated clients. |
| projects | The exported XML is all inclusive of all project elements including people, meetings notes, meeting action items, meeting attendees, tracker items, locations, notes, project team members, and project locations. |
| project\_people | The exported XML contains all the people associated with a project and their related companies. |
| status\_report\_items | The exported XML contains the status report items associated with a project. |
| project\_locations | The exported XML contains the project locations associated with a project. |
| project\_notes | The exported XML contains the project notes associated with a project. |
| meeting\_attendees | The exported XML contains the meeting attendees for project notes associated with a project. |
| item\_tracker\_updates | The exported XML contains notes attached to tracker and action items. |
| item\_tracker | The exported XML contains action or tracker items. |


\
**Basic XML formats**

The example below show an XML export of a person.&nbsp; Notice child tables contain the **"*filter\_field"*** and **"*filter\_value"*** attributes to define the parent child relationship of the data.&nbsp; Many attributes such as file names and column numbers are provided to give the export further context, but are not used in the import.

\
\<?xml version="1.0" encoding="UTF-8"?\>\
\<**projectnotes** filepath="**C:\\Users\\joe\\Sample.db**" export\_date="**12/11/2020 01:01 PM**" filter\_field="**people\_id**" project\_manager\_id="**159709810500028597**" managing\_company\_id="**1597098105000493**" managing\_company\_name="**Sample Company, Inc.**" managing\_manager\_name="Jacob Smith" filter\_values="**{ba96fb89-6c2d-46db-864c-5be6292b10ef}**"\>\
**&nbsp;** \<**table** name="**ix\_people**" filter\_field="**people\_id**" filter\_value="**{ba96fb89-6c2d-46db-864c-5be6292b10ef}**"\>\
**&nbsp;** &nbsp; \<**row** id="**15970981060009492**"\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**name**"\>**Aaron Brown**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**email**"\>**Aaron.Brown@somecompany.com**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**office\_phone**"\>**(555) 555-2459**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**cell\_phone**"\>**(555) 555-1224**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**client\_id**" lookupvalue="**Simco, Inc.**"\>**{ba96fb89-6c2d-46db-864c-5be6292b10ef}**\</**column**\>\
**&nbsp;** &nbsp; &nbsp; \<**column** name="**role**"\>**Programming Lead**\</**column**\>\
**&nbsp;** &nbsp; \</**row**\>\
**&nbsp;** \</**table**\>\
\</**projectnotes**\>

&nbsp;

**Code Example**

The plugin architecture calls event functions if they have been defined.&nbsp; Below is a common section of code used to respond to events.&nbsp; See other plugins installed with Project Notes for more examples.

&nbsp;

\
import platform\
\
if (platform.system() == 'Windows'):\
&nbsp; &nbsp; from includes.excel\_tools import ProjectNotesExcelTools\
&nbsp; &nbsp; import win32com\
\
from includes.common import ProjectNotesCommon\
from PyQt5 import QtSql, QtGui, QtCore, QtWidgets, uic\
from PyQt5.QtSql import QSqlDatabase\
from PyQt5.QtXml import QDomDocument, QDomNode\
from PyQt5.QtCore import QFile, QIODevice, QDateTime, QUrl\
from PyQt5.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog\
from PyQt5.QtGui import QDesktopServices\
\
\
*\# Project Notes Plugin Parameters*\
pluginname = "Schedule Customer Lessons Learned"\
plugindescription = "Using Outlook create an invite to the customer lessons learned session."\
plugintable = "projects" *# the table or view that the plugin applies to.&nbsp; This will enable the right click*\
childtablesfilter = "projects/project\_people" *# a list of child tables that can be sent to the plugin.&nbsp; This will be used to exclude items like notes or action items when they aren't used*\
\
*\# events must have a data structure and data view specified*\
*\#*\
*\# Structures:*\
*\#&nbsp; &nbsp; &nbsp; string&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; The event will pass a python string containing XML and will expect the plugin to return an XML string*\
*\#*\
*\# Data Views:*\
*\#&nbsp; &nbsp; &nbsp; clients*\
*\#&nbsp; &nbsp; &nbsp; people*\
*\#&nbsp; &nbsp; &nbsp; projects*\
*\#&nbsp; &nbsp; &nbsp; project\_people*\
*\#&nbsp; &nbsp; &nbsp; status\_report\_items*\
*\#&nbsp; &nbsp; &nbsp; project\_locations*\
*\#&nbsp; &nbsp; &nbsp; project\_notes*\
*\#&nbsp; &nbsp; &nbsp; meeting\_attendees*\
*\#&nbsp; &nbsp; &nbsp; item\_tracker\_updates*\
*\#&nbsp; &nbsp; &nbsp; item\_tracker*\
\
*\# Supported Events*\
\
*\# def event\_startup(xmlstr):*\
*\# &nbsp; &nbsp; return ""*\
*\#*\
*\# def event\_shutdown(xmlstr):*\
*\# &nbsp; &nbsp; return ""*\
*\#*\
*\# def event\_everyminute(xmlstr):*\
*\# &nbsp; &nbsp; return ""*\
*\#*\
*\# def event\_every5minutes(xmlstr):*\
*\# &nbsp; &nbsp; return ""*\
*\#*\
*\# def event\_every10minutes(xmlstr):*\
*\# &nbsp; &nbsp; return ""*\
*\#*\
*\# def event\_every30Mmnutes(xmlstr):*\
*\# &nbsp; &nbsp; return ""*\
*\#*\
*\# def event\_menuclick(xmlstr):*\
*\# &nbsp; &nbsp; return ""*\
\
*\# Parameters specified here will show in the Project Notes plugin settings window*\
*\# the global variable name must be specified as a string value to be read by project notes*\
*\# Project Notes will set these values before calling any defs*\
\
*\# Project Notes Parameters*\
parameters = \[\
\]\
\
*\# this plugin is only supported on windows*\
if (platform.system() == 'Windows'):\
&nbsp; &nbsp; pnc = ProjectNotesCommon()\
&nbsp; &nbsp; pne = ProjectNotesExcelTools()\
\
&nbsp; &nbsp; **def** event\_data\_rightclick(xmlstr):\
&nbsp; &nbsp; &nbsp; &nbsp; print("called event: " + **\_\_file\_\_**)\
\
&nbsp; &nbsp; &nbsp; &nbsp; xmlval = QDomDocument()\
&nbsp; &nbsp; &nbsp; &nbsp; xmldoc = ""\
&nbsp; &nbsp; &nbsp; &nbsp; window\_title = ""\
&nbsp; &nbsp; &nbsp; &nbsp; \
&nbsp; &nbsp; &nbsp; &nbsp; if (xmlval.setContent(xmlstr) == **False**):\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; return ""\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; \
&nbsp; &nbsp; &nbsp; &nbsp; outlook = win32com.client.Dispatch("Outlook.Application")\
&nbsp; &nbsp; &nbsp; &nbsp; message = outlook.CreateItem(1)\
\
&nbsp; &nbsp; &nbsp; &nbsp; xmlroot = xmlval.elementsByTagName("projectnotes").at(0) *# get root node&nbsp; &nbsp; &nbsp; &nbsp; *\
&nbsp; &nbsp; &nbsp; &nbsp; pm = xmlroot.toElement().attribute("managing\_manager\_name")\
\
&nbsp; &nbsp; &nbsp; &nbsp; email = None\
&nbsp; &nbsp; &nbsp; &nbsp; nm = None\
\
&nbsp; &nbsp; &nbsp; &nbsp; teammember = pnc.find\_node(xmlroot, "table", "name", "project\_people")\
&nbsp; &nbsp; &nbsp; &nbsp; if teammember:\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; memberrow = teammember.firstChild()\
\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; while not memberrow.isNull():\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; nm = pnc.get\_column\_value(memberrow, "name")\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; email = pnc.get\_column\_value(memberrow, "email")\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; if nm \!= pm:\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; if (email is not None **and** email \!= ""):\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; message.Recipients.Add(email)\
\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; memberrow = memberrow.nextSibling()\
\
&nbsp; &nbsp; &nbsp; &nbsp; project = pnc.find\_node(xmlroot, "table", "name", "projects")\
&nbsp; &nbsp; &nbsp; &nbsp; if project:\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; projectrow = project.firstChild()\
\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; if not projectrow.isNull():\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; window\_title = pnc.get\_column\_value(projectrow, "project\_number") + " " + pnc.get\_column\_value(projectrow, "project\_name") + " - Lessons Learned"\
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; message.Subject = window\_title\
\
&nbsp; &nbsp; &nbsp; &nbsp; txt = get\_text\_invite()\
&nbsp; &nbsp; &nbsp; &nbsp; message.MeetingStatus = 1\
&nbsp; &nbsp; &nbsp; &nbsp; message.Duration = 60\
&nbsp; &nbsp; &nbsp; &nbsp; message.Location = pnc.get\_plugin\_setting("DefaultMeetingLocation")\
&nbsp; &nbsp; &nbsp; &nbsp; message.Body = txt\
&nbsp; &nbsp; &nbsp; &nbsp; outlook.ActiveExplorer().Activate()\
&nbsp; &nbsp; &nbsp; &nbsp; message.Display()\
\
&nbsp; &nbsp; &nbsp; &nbsp; outlook = None\
&nbsp; &nbsp; &nbsp; &nbsp; message = None\
\
&nbsp; &nbsp; &nbsp; &nbsp; pnc.bring\_window\_to\_front(window\_title)\
\
&nbsp; &nbsp; &nbsp; &nbsp; return xmldoc\
\
\
**def** get\_text\_invite():\
&nbsp; &nbsp; txtdoc = """Lessons Learned Meeting Agenda\
The purpose the of meeting is to have a discussion and collectively identify lessons learned during the project, so future projects may benefit from our experience.&nbsp; During our discussion we want to recognize and document what things went well, and why were they so successful.&nbsp; We also want to recognize areas that may need improvement, and how we might improve them on the next project.\
\
During our time we will cover the following areas:\
\
&nbsp;&nbsp; &nbsp; \*Project Planning\
&nbsp;&nbsp; &nbsp; \*Project Execution\
&nbsp;&nbsp; &nbsp; \*Testing\
&nbsp;&nbsp; &nbsp; \*Project Communication\
&nbsp;&nbsp; &nbsp; \*Obstacles\
&nbsp;&nbsp; &nbsp; \*On-Site Implementation\
&nbsp;&nbsp; &nbsp; \*Vendor Management\
&nbsp;&nbsp; &nbsp; \*Schedule And Budget\
&nbsp; &nbsp; """\
\
&nbsp; &nbsp; return txtdoc

&nbsp;


***
_Created with the Personal Edition of HelpNDoc: [Streamline Your Documentation Process with HelpNDoc's Intuitive Interface](<https://www.helpndoc.com/feature-tour/stunning-user-interface/>)_
