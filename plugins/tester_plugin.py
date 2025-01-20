import sys
import platform
#import projectnotes
import threading
import time

#from includes.common import ProjectNotesCommon
from PyQt6 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt6.QtSql import QSqlDatabase
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QThread
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices

# Project Notes Plugin Parameters
pluginname = "Testing Plugin" # name used in the menu
plugindescription = "This is test plugin. Supported platforms: Windows, Linux, MacOS"

# define menus to be added to ProjectNotes Plugins menu and data export/import right click.
# the dictionary key is the menu name
# the function value is the the python function to be called
# the table filter filters the XML sent to the plugin to make the export more efficient
# the menu can be placed under a submenu
# the function wil only show on the right click if it matches the table specified in dataexport
pluginmenus = {
    "Menu 1" : {"function" : "event_menuclick", "tablefilter" : "", "submenu" : "Test Submenu", "dataexport" : ""},
    "Menu 2" : {"function" : "event_menuclick", "tablefilter" : "", "submenu" : "Test Submenu", "dataexport" : ""},
    "Export Project" : {"function" : "event_data_rightclick", "tablefilter" : "", "submenu" : "Test Submenu", "dataexport" : "projects"},
}

# events must have a data structure and data view specified
#
# Structures:
#      string          The event will pass a python string when dataexport is defined containing XML. 
#                      The plugin can return an XML string to be processed by ProjectNotes.
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

# def event_menuclick(xmlstr):
#     return

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# WARNING: The next line below can cause Project Notes to hang only use it for testing
#pnc = ProjectNotesCommon()

# Project Notes Plugin Events

def get_global_setting(settingname):
    cfg = QSettings("ProjectNotes","PluginSettings")
    cfg.setFallbacksEnabled(False)

    val = cfg.value("Global Settings/" + settingname, "")

    return val

def get_setting(settingname):
    cfg = QSettings("ProjectNotes","PluginSettings")
    cfg.setFallbacksEnabled(False)

    val = cfg.value(pluginname + "/" + settingname, "")

    return val

def event_menuclick():
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return

    print("Tester: Event Menu click called...")

    contact = """<?xml version="1.0" encoding="UTF-8"?>
    <projectnotes>
    <table name="people" filter_field_1="name" filter_value_1="C%" top="2" skip="1" />
    <table name="clients" filter_field_1="name" filter_value_1="C%" top="5" />
    </projectnotes>
    """

    #projectnotes.update_data(contact)
    #print(projectnotes.get_data(contact))

    QMessageBox.critical(None, "Test Plugin", "menu click called", QMessageBox.StandardButton.Cancel)

    return ""

def event_data_rightclick(xmlstr):
    #app = QApplication(sys.argv)    
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return

    print("Tester: Right Click Data Event called...")
    print(xmlstr)

    QMessageBox.critical(None, "how", "Will this work", QMessageBox.StandardButton.Cancel)

    # simple test will always change the name back
    retval = """<projectnotes>
         <table name="clients">
          <row id="161357299500029810">
           <column name="client_id">161357299500029810</column>
           <column name="client_name">E-Cubed """

    retval = retval + "X"

    retval = retval + """</column>
          </row>
         </table>
        </projectnotes>"""


    return retval  

"""

app = QApplication(sys.argv)
print("Testing Plugin")
EditorFullPath = "notepad.exe"
event_menuclick("")
"""
#event_data_rightclick("")
