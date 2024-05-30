import sys
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt6.QtSql import QSqlDatabase
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Email as Attachment"
plugindescription = "Emails the selected file/artifact as an attachment."
plugintable = "project_locations" # the table or view that the plugin applies to.  This will enable the right click
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

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    #
    pnc = ProjectNotesCommon()

    def event_data_rightclick(xmlstr):
        print("called event: " + __file__)

        print(xmlstr)

        app = QApplication(sys.argv)
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        

        if xmlroot:
            projlocation = pnc.find_node(xmlroot, "table", "name", "project_locations")

            if projlocation:
                locationrow = projlocation.firstChild()

            while not locationrow.isNull():
                fp = pnc.get_column_value(locationrow, "full_path")
                ld = pnc.get_column_value(locationrow, "location_description")
                pn = pnc.get_column_value(locationrow, "project_number")
                pe = pnc.get_column_value(locationrow, "project_name")

                if (fp is not None and fp != ""):
                    pnc.email_word_file_as_html(pn + " " + pe + " - ", "", fp, None)

                locationrow = locationrow.nextSibling()

        return ""

# setup test data
"""
print("Buld up QDomDocument")

xmldoc = QDomDocument("TestDocument")

f = QFile("exampleproject.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
    xmldoc.setContent(f)
    f.close()

event_data_rightclick(xmldoc.toString())
"""

