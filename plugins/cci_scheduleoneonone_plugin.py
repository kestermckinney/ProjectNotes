
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
pluginname = "Schedule One On One"
plugindescription = "Using Outlook create an invite to an employee review meeting."
plugintable = "people" # the table or view that the plugin applies to.  This will enable the right click
childtablesfilter = "" # a list of child tables that can be sent to the plugin.  This will be used to exclude items like notes or action items when they aren't used

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

OracleUsername = ""
ProjectsFolder = ""

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pnc = ProjectNotesCommon()
    pne = ProjectNotesExcelTools()

    # processing main function
    def event_data_rightclick(xmlstr):
        print("called event: " + __file__)
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)
            return ""
            
        outlook = win32com.client.Dispatch("Outlook.Application")
        message = outlook.CreateItem(1)
        email = ""
        nm = ""
        pm = ""

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = pnc.get_projectfolder(xmlroot)
        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()

        if xmlroot:
            teammember = pnc.find_node(xmlroot, "table", "name", "people")
            if teammember:
                memberrow = teammember.firstChild()

                while not memberrow.isNull():
                    nm = pnc.get_column_value(memberrow, "name")
                    email = pnc.get_column_value(memberrow, "email")
                    if nm != pm:
                        if (email != None and email != ""):
                            message.Recipients.Add(email)

                    memberrow = memberrow.nextSibling()

            message.Subject = "One On One with " + pm + " and " + nm

            txt = get_text_invite()
            message.MeetingStatus = 1
            message.Duration = 60
            message.Location = pnc.get_plugin_setting("DefaultMeetingLocation")
            message.Body = txt
            outlook.ActiveExplorer().Activate()
            message.Display()

            outlook = None
            message = None

        return("")

    def get_text_invite():
        txtdoc = """
        One On One Meeting Agenda
        The purpose of this meeting is to review personal performance, discuss goals, and overall wellbeing.  We will review and discuss the following items.

        Wellbeing
        - Utilization Numbers
        - Performance Factors
        - Associate Development Plan
        - Company Performance
        - Safety
        - Annual Review ( of Year)
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

print("Run Test")
# call when testing outside of Project Notes
main_process(xmldoc)
"""

# TESTED: Phase 1