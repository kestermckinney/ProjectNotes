import json

from includes.common import ProjectNotesCommon
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "My Shortcuts"
plugindescription = "My Shortcuts provides configurable URLs that can be called from specified menu locations.  The URLs can contain variables that are populated based upon XML passed to the plugion."

# define menus to be added to ProjectNotes Plugins menu and data export/import right click.
# the menutitle is the menu name
# the function value is the the python function to be called
# the table filter filters the XML sent to the plugin to make the export more efficient
# the menu can be placed under a submenu
# the function wil only show on the right click if it matches the table specified in dataexport
# if a dataexport value exist the menu will not appear on the plugin menu
pluginmenus = [ ] # menu is dynamically created when module loads

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

def populate_menu_from_json(json_string):
    menu_array = []

    # nothing was saved
    if (json_string is None or json_string == ""):
        return ""

    data = json.loads(json_string)

    if (len(data) > 0):
        # Populate the table with data
        # make sure to filter the xml to the top level.  We doon't want to get the full projeect xml
        for row, row_data in enumerate(data): 
            if "Data Type" in row_data and row_data["Data Type"] != "":
                menu_array.append({"menutitle" : row_data["Menu"], "function" : "event_data_rightclick",  "tablefilter" : row_data["Data Type"], "submenu" : row_data["Submenu"], "dataexport" : row_data["Data Type"], "parameter" : row_data["URL"] })
            else:
                menu_array.append({"menutitle" : row_data["Menu"], "function" : "menu_click",  "tablefilter" : "", "submenu" : row_data["Submenu"], "dataexport" : "", "parameter" : row_data["URL"] })

    return menu_array

def menu_click(parameter):
    return event_data_rightclick("", parameter)

def event_data_rightclick(xmlstr, parameter):

    xmlval = QDomDocument()

    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        

    url_link = pnc.replace_variables(parameter, xmlroot)

    QDesktopServices.openUrl(QUrl(url_link))

    return ""

pnc = ProjectNotesCommon()

menu_data = pnc.get_plugin_setting("MyShortcuts", "My Shortcuts")
pluginmenus = populate_menu_from_json(menu_data)

# setup test data
"""
print("Buld up QDomDocument")

xmldoc = QDomDocument("TestDocument")

f = QFile("exampleproject.xml")

if f.open(QIODevice.OpenModeFlag.ReadOnly):
    print("example project opened")
    xmldoc.setContent(f)
    f.close()

event_data_rightclick(xmldoc.toString())
"""



