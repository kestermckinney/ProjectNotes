import sys
import platform

print(sys.path)

#if (platform.system() == 'Windows'):
#    from includes.excel_tools import ProjectNotesExcelTools
#    import win32com

import importlib.machinery
print(importlib.machinery.all_suffixes())

from includes.common import ProjectNotesCommon
from PyQt5 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt5.QtSql import QSqlDatabase
from PyQt5.QtXml import QDomDocument, QDomNode
from PyQt5.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt5.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt5.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Open Editor" # name used in the menu
plugindescription = "Open the specified editor. Supported platforms: Windows, Linux, MacOS"
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
    "EditorFullPath"
]

pnc = ProjectNotesCommon()

# Project Notes Plugin Events
"""
def event_startup():
    return None

def event_shutdown():
    return None

def event_everyminute():
    return None

def event_every5minutes():
    return None

def event_every10minutes():
    return None

def event_every30Mmnutes():
    return None
"""
def event_menuclick(xmlstr):
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)
        return ""

    print(xmlstr);
        
    if (EditorFullPath is None or EditorFullPath == ""):
        QMessageBox.critical(None, "Editor Not Specified",
        "You will need to specify an editor in the Open Editor plugin settings.",
        QMessageBox.Cancel)
    else:
        pnc.exec_program( EditorFullPath )
    return ""

"""
def event_data_rightclick(xmlstr):
    print("data right click")
    print("python val ", xmlstr)
    QMessageBox.critical(None, "Param", EditorFullPath, QMessageBox.Cancel)

    QMessageBox.critical(None, "String Output Test", "example" + xmlstr, QMessageBox.Cancel)


    #dom = QDomDocument()
    #dom.setContent(xmlstr)

    #print(dom.toString())

    return "<html/>"
"""

"""
print("Testing Plugin")
EditorFullPath = "notepad.exe"
event_menuclick("")
"""