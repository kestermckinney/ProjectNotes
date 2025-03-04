import sys
import platform

if (platform.system() == 'Windows'):
    from includes.word_tools import ProjectNotesWordTools
    import win32com
    from win32com.client import GetObject

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Close Stranded Word"
plugindescription = "Closes all of the abandon Word automation object processes."

pluginmenus = [
    {"menutitle" : "Close Stranded Word", "function" : "event_menuclick", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : ""},
]

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


# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pne = ProjectNotesWordTools()

    def event_menuclick():
        pne.killwordautomation()
        return ""  
 
# call when testing outside of Project Notes
"""
app = QApplication(sys.argv)

print("Kill Word Automations")
event_menuclick("") 
"""
 