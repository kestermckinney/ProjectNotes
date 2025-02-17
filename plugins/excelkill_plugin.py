import sys
import platform

#sys.path.append('c:/program files/python313/lib/site-packages/win32')

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com
    from win32com.client import GetObject

from includes.common import ProjectNotesCommon
from PyQt6 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt6.QtSql import QSqlDatabase
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Close Stranded Excel"
plugindescription = "Closes all of the abandon Excel automation object processes."

pluginmenus = [
    {"menutitle" : "Close Stranded Excel", "function" : "event_menuclick", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : ""},
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
    pne = ProjectNotesExcelTools()

    def event_menuclick():
        pne.killexcelautomation()
        return ""

# call when testing outside of Project Notes
#app = QApplication(sys.argv)
#print("Kill Excel Automations")
#event_menuclick("")
