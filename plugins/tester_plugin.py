import sys
import platform
import projectnotes

#print(sys.path)

#if (platform.system() == 'Windows'):
#    from includes.excel_tools import ProjectNotesExcelTools
#    import win32com

#import importlib.machinery
#print(importlib.machinery.all_suffixes())

from includes.common import ProjectNotesCommon
from PyQt6 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt6.QtSql import QSqlDatabase
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Testing Plugin" # name used in the menu
pluginsubmenu = "A Test Submenu" # the sub menu to display the plugin in
plugindescription = "This is test plugin. Supported platforms: Windows, Linux, MacOS"
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
    "TestParameter1",
    "TestParameter2"
]

# WARNING: The next line below can cause Project Notes to hang only use it for testing
pnc = ProjectNotesCommon()

# Project Notes Plugin Events

def disabled_event_startup(xmlstr):
    #app = QApplication(sys.argv)
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

    print("Tester: Event Startup called...")
    print(xmlstr)

    return ""

def disabled_event_shutdown(xmlstr):
    #app = QApplication(sys.argv)
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

    print("Tester: Event Startup called...")
    print(xmlstr)    

    return ""

def disabled_event_everyminute(xmlstr):
    #app = QApplication(sys.argv)
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

    print("Tester: Event Every Minute called...")
    print(xmlstr)

    return ""

def disabled_event_every5minutes(xmlstr):
    #app = QApplication(sys.argv)
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

    print("Tester: Event Every 5 Minutes called...")
    print(xmlstr)

    return ""

def disabled_event_every10minutes(xmlstr):
    #app = QApplication(sys.argv)
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

    print("Tester: Event Every 10 Minutes called...")
    print(xmlstr)

    return ""

def disabled_event_every30minutes(xmlstr):
    #app = QApplication(sys.argv)
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

    print("Tester: Event Every 30 Minutes called...")
    print(xmlstr)

    return ""

def event_menuclick(xmlstr):
    #app = QApplication(sys.argv)
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

    print("Tester: Event Menu click called...")
    print(xmlstr) 

    contact = """<?xml version="1.0" encoding="UTF-8"?>
    <projectnotes>
    <table name="people" filter_field_1="name" filter_value_1="A%" top="2" skip="1" />
    <table name="clients" filter_field_1="name" filter_value_1="A%"/>
    </projectnotes>
    """

    #projectnotes.update_data(contact)

    print(projectnotes.get_data(contact))

    QMessageBox.critical(None, "Test Plugin", "menu click called", QMessageBox.StandardButton.Cancel)

    return ""

def event_data_rightclick(xmlstr):
    #app = QApplication(sys.argv)    
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

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
print("Testing Plugin")
EditorFullPath = "notepad.exe"
#event_menuclick("")
event_data_rightclick("")
"""