
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


# Project Notes Plugin Parameters
pluginname = "Schedule Customer Lessons Learned"
plugindescription = "Using Outlook create an invite to the customer lessons learned session."
plugintable = "projects" # the table or view that the plugin applies to.  This will enable the right click
childtablesfilter = "projects/project_people" # a list of child tables that can be sent to the plugin.  This will be used to exclude items like notes or action items when they aren't used

# events must have a data structure and data view specified
#
# Structures:
#      string          The event will pass a python string containing XML and will expect the plugin to return an XML string
#
# Data Views:
#      clients
#      people
#      projects
#      project_people
#      status_report_items
#      project_locations
#      project_notes
#      meeting_attendees
#      item_tracker_updates
#      item_tracker

# Supported Events

# def event_startup(xmlstr):
#     return ""
#
# def event_shutdown(xmlstr):
#     return ""
#
# def event_everyminute(xmlstr):
#     return ""
#
# def event_every5minutes(xmlstr):
#     return ""
#
# def event_every10minutes(xmlstr):
#     return ""
#
# def event_every30Mmnutes(xmlstr):
#     return ""
#
# def event_menuclick(xmlstr):
#     return ""

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# Project Notes Parameters
parameters = [
]

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pnc = ProjectNotesCommon()
    pne = ProjectNotesExcelTools()

    def event_data_rightclick(xmlstr):
        print("called event: " + __file__)

        xmlval = QDomDocument()
        xmldoc = ""
        
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)
            return ""
            
        outlook = win32com.client.Dispatch("Outlook.Application")
        message = outlook.CreateItem(1)

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        
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
                    if (email is not None and email != ""):
                        message.Recipients.Add(email)

                memberrow = memberrow.nextSibling()

        project = pnc.find_node(xmlroot, "table", "name", "projects")
        if project:
            projectrow = project.firstChild()

            if not projectrow.isNull():
                message.Subject = pnc.get_column_value(projectrow, "project_number") + " " + pnc.get_column_value(projectrow, "project_name") + " - Lessons Learned"

        txt = get_text_invite()
        message.MeetingStatus = 1
        message.Duration = 60
        message.Location = pnc.get_plugin_setting("DefaultMeetingLocation")
        message.Body = txt
        outlook.ActiveExplorer().Activate()
        message.Display()

        outlook = None
        message = None

        return xmldoc

    def main_process_meeting( xmlval ):
        outlook = win32com.client.Dispatch("Outlook.Application")
        message = outlook.CreateItem(1)

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()

        attendeetable = pnc.find_node(xmlroot, "table", "name", "meeting_attendees")

        if attendeetable:
            attendeerow = attendeetable.firstChild()

        email = None
        nm = None

        while not attendeerow.isNull():
            nm = pnc.get_column_value(attendeerow, "name")
            email = pnc.get_column_value(attendeerow, "email")
            if nm != pm:
                if (not email is None and email != ""):
                    message.Recipients.Add(email)

            attendeerow = attendeerow.nextSibling()

        project = pnc.find_node(xmlroot, "table", "name", "project_notes")
        if project:
            projectrow = project.firstChild()

        if not projectrow.isNull():
            message.Subject = pnc.get_column_value(projectrow, "project_id") + " " + pnc.get_column_value(projectrow, "project_id_name") + " - Lessons Learned"

        txt = get_text_invite()
        message.MeetingStatus = 1
        message.Duration = 60
        message.Location = pnc.get_plugin_setting("DefaultMeetingLocation")
        message.Body = txt
        message.Display()

        outlook = None
        message = None

        return xmldoc


def get_text_invite():
    txtdoc = """Lessons Learned Meeting Agenda
The purpose the of meeting is to have a discussion and collectively identify lessons learned during the project, so future projects may benefit from our experience.  During our discussion we want to recognize and document what things went well, and why were they so successful.  We also want to recognize areas that may need improvement, and how we might improve them on the next project.

During our time we will cover the following areas:

     *Project Planning
     *Project Execution
     *Testing
     *Project Communication
     *Obstacles
     *On-Site Implementation
     *Vendor Management
     *Schedule And Budget
    """

    return txtdoc

"""
# setup test data
print("Buld up QDomDocument")
app = QApplication(sys.argv)

xmldoc = QDomDocument("TestDocument")
f = QFile("exampleproject.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

main_process_project(xmldoc)
#main_process_meeting(xmldoc)
"""

# TESTED: Phase 1
