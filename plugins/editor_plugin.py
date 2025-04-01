import sys
import platform

from includes.common import ProjectNotesCommon
from PyQt6.QtWidgets import QMessageBox, QApplication
from PyQt6.QtGui import QDesktopServices

# Project Notes Plugin Parameters
pluginname = "Open Editor" # name used in the menu
plugindescription = "Open the specified editor. Supported platforms: Windows, Linux, MacOS"

# define menus to be added to ProjectNotes Plugins menu and data export/import right click.
# the menutitle is the menu name
# the function value is the the python function to be called
# the table filter filters the XML sent to the plugin to make the export more efficient
# the menu can be placed under a submenu
# the function wil only show on the right click if it matches the table specified in dataexport
# if a dataexport value exist the menu will not appear on the plugin menu
pluginmenus = [
    {"menutitle" : "Editor", "function" : "event_menuclick", "tablefilter" : "", "submenu" : "", "dataexport" : ""},
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

def event_menuclick(parameter):
    pnc = ProjectNotesCommon()

    print("called event: " + __file__)

    EditorFullPath = pnc.get_plugin_setting("EditorPath", "Custom Editor")

    #print(f"Editor Path: {EditorFullPath}")
        
    if (EditorFullPath is None or EditorFullPath == ""):
        QMessageBox.critical(None, "Editor Not Specified", "You will need to specify an editor in the Open Editor plugin settings.", QMessageBox.StandardButton.Cancel)
    else:
        pnc.exec_program( EditorFullPath )
    return ""

"""
print("Testing Plugin")
EditorFullPath = "notepad.exe"
event_menuclick("")
"""