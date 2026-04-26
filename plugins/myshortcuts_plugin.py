# Copyright (C) 2025, 2026 Paul McKinney
import json

from includes.common import ProjectNotesCommon
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "My Shortcuts"
plugindescription = "My Shortcuts provides configurable URLs that can be called from specified menu locations.  The URLs can contain variables that are populated based upon XML passed to the plugion."

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
        QMessageBox.critical(QApplication.activeWindow(), "Cannot Parse XML", "Unable to parse XML sent to plugin.")
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



