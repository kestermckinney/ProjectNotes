from includes.common import ProjectNotesCommon
from includes.excel_tools import ProjectNotesExcelTools

from PySide6 import QtSql, QtGui, QtCore, QtUiTools
from PySide6.QtSql import QSqlDatabase
from PySide6.QtXml import QDomDocument, QDomNode
from PySide6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PySide6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PySide6.QtGui import QDesktopServices
import sys
import win32com

# Project Notes Plugin Parameters
pluginname = "Open Editor"
plugindescription = "Open the specified editor."

# events must have a data structure and data view specified
#
# Structures:
#      disabled        The event will not be enabled
#      wxxmldocument   The event will pass a wxLua wx.wxXmlDocument containing the spedified data view and expect the plugin to return a wx.wxXmlDocument
#      string          The event will pass a wxLua string containing XML and will expect the plugin to return an XML string
#      nodata          The event will pass a wxLua None and will expect the plugin to return an XML string
#
# all tables in the database have corresponding import/export data views the views are prefixed by ix_
#
# Data Views:
#      ix_clients
#      ix_people
#      ix_projects
#      ix_project_people
#      ix_status_report_items
#      ix_project_locations
#      ix_project_notes
#      ix_meeting_attendees
#      ix_item_tracker_updates
#      ix_item_tracker

# Active Events
Startup="disabled"
Shutdown="disabled"
EveryMinute="disabled"
Every5Minutes="disabled"
Every10Minutes="disabled"
Every30Minutes="disabled"
PluginMenuClick="nodata"
RightClickProject="disabled"
RightClickPeople="disabled"
RightClickClient="disabled"
RightClickStatusReportItem="disabled"
RightClickLocationItem="disabled"
RightClickTeamMember="disabled"
RightClickMeeting="disabled"
RightClickAttendee="disabled"
RightCickTrackerItem="disabled"

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# Project Notes Parameters
parameters = {
    "EditorFullPath"
}

pnc = ProjectNotesCommon()

# Project Notes Plugin Events
def event_startup(xmlstr):
    return None

def event_shutdown(xmlstr):
    return None

def event_everyminute(xmlstr):
    return None

def event_every5minutes(xmlstr):
    return None

def event_every10minutes(xmlstr):
    return None

def event_every30Mmnutes(xmlstr):
    return None

def event_menuclick(xmlstr):
    if (EditorFullPath is None or EditorFullPath == ""):
        QMessageBox.critical(None, "Editor Not Specified",
        "You will need to specify an editor in the Open Editor plugin settings.",
        QMessageBox.Cancel)
    else:
        pnc.exec_program( EditorFullPath )
    return None

def event_projectrightclick(xmlstr):
    return None

def event_peoplerightclick(xmlstr):
    return None

def event_clientrightclick(xmlstr):
    return None

def event_statusreportitemrightclick(xmlstr):
    return None

def event_teammemberrightclick(xmlstr):
    return None

def event_locationitemrightclick(xmlstr):
    return None

def event_meetingrightclick(xmlstr):
    return None

def event_attendeerightclick(xmlstr):
    return None

def event_trackeritemrightclick(xmlstr):
    return None

"""
print("Testing Plugin")
EditorFullPath = "notepad.exe"
event_menuclick(None)
"""
