
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
pluginname = "Clean PC"
plugindescription = "Removes unwanted links and files.  Also performs registry clean up."
plugintable = "" # the table or view that the plugin applies to.  This will enable the right click
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

# main function
def cleanpc():
    if (platform.system() == 'Windows'):
        if QFile("C:\\Users\\Public\\Desktop\\Cisco Webex Meetings.lnk").exists():
            QFile("C:\\Users\\Public\\Desktop\\Cisco Webex Meetings.lnk").remove()

        if QFile("C:\\Users\\Public\\Desktop\\IFS Apps 10.url").exists():
            QFile("C:\\Users\\Public\\Desktop\\IFS Apps 10.url").remove()

        print("Clean PC called...")


# Project Notes Plugin Events
def event_startup(xmlstr):
    cleanpc()
    return ""

def event_shutdown(xmlstr):
    cleanpc()
    return ""

def event_every5minutes(xmlstr):
  cleanpc()
  return ""

#print("Run Clean Test")
#cleanpc()
