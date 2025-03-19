import sys
import platform
import threading
import time
import json
import projectnotes

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtCore import Qt, QRect
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QDialog, QFileDialog, QWidget, QTableWidgetItem, QStyledItemDelegate, QComboBox

# Project Notes Plugin Parameters
pluginname = "Base Plugins" # name used in the menu
plugindescription = "Base set of plugins. Supported platforms: Windows, Linux, MacOS"

# define menus to be added to ProjectNotes Plugins menu and data export/import right click.
# the menutitle is the menu name
# the function value is the the python function to be called
# the table filter filters the XML sent to the plugin to make the export more efficient
# the menu can be placed under a submenu
# the function wil only show on the right click if it matches the table specified in dataexport
# if a dataexport value exist the menu will not appear on the plugin menu
pluginmenus = [
    {"menutitle" : "Copy Email to Clipboard", "function" : "menuCopyContactEmail", "tablefilter" : "people", "submenu" : "Settings", "dataexport" : "people"}, #todo:
    {"menutitle" : "Copy Email to Clipboard", "function" : "menuCopyTeamEmail", "tablefilter" : "project_people", "submenu" : "Settings", "dataexport" : "project_people"}, #todo:
    {"menutitle" : "Copy Email to Clipboard", "function" : "menuCopyAttendeeEmail", "tablefilter" : "meeting_attendees", "submenu" : "Settings", "dataexport" : "meeting_attendees"}, #todo:
    {"menutitle" : "Copy Path to Clipboard", "function" : "menuCopyPath", "tablefilter" : "project_locations", "submenu" : "Settings", "dataexport" : "project_locations"}, #todo:
    {"menutitle" : "Export Meeting Notes", "function" : "menuExportMeetingNotes", "tablefilter" : "projects/project_notes/meeting_attendees/item_tracker/project_locations", "submenu" : "Settings", "dataexport" : "projects"},   #todo



    #todo: move editor here
    {"menutitle" : "Export Contacts to Outlook", "function" : "menuExportContactsToOutlook", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : ""},   #todo make dynamic for linux
    {"menutitle" : "Import Contacts from Outlook", "function" : "menuImportContactsFromOutlook", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : ""},   #todo make dynamic for linux
    {"menutitle" : "Email as Attachment", "function" : "menuEmailAttachment", "tablefilter" : "project_locations", "submenu" : "", "dataexport" : "project_locations"},   #todo make dynamic for linux
    {"menutitle" : "My Shortcuts", "function" : "menuMyShortcutSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
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

