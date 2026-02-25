import os
import sys
import platform
import threading
import time
import json
import projectnotes

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import Qt, QRect, QSettings
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QDialog, QWidget, QComboBox

# Project Notes Plugin Parameters
pluginname = "Team Member Quick Add" # name used in the menu
plugindescription = "This plugin allows you to quickly add a new team member that isn't already in the list of people. Supported platforms: Windows, Linux, MacOS"

# define menus to be added to ProjectNotes Plugins menu and data export/import right click.
# the menutitle is the menu name
# the function value is the the python function to be called
# the table filter filters the XML sent to the plugin to make the export more efficient
# the menu can be placed under a submenu
# the function wil only show on the right click if it matches the table specified in dataexport
# if a dataexport value exist the menu will not appear on the plugin menu
pluginmenus = [
    {"menutitle" : "Quick Add", "function" : "menuQuickAdd", "tablefilter" : "project_people", "submenu" : "", "dataexport" : "project_people"}
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

# Team member quick add
class TeamMemberQuickAdd(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "Team Member Quick Add"

        self.project_number = None

        self.ui = uic.loadUi("plugins/forms/dialogTeamMemberQuickAdd.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
        self.setModal(True)

        self.ui.buttonBox.accepted.connect(self.add_contact)
        self.ui.buttonBox.rejected.connect(self.reject_changes)

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        if (x != '' and y != '' and w != '' and h != ''):
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

    def setup_window(self, xmlstr):

        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return False

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node

        self.project_number = self.pnc.scrape_project_number(xmlroot)


        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table name="clients" />\n</projectnotes>\n'
        xmlresult = projectnotes.get_data(xmldoc)

        self.ui.comboBoxClientName.clear()
        self.ui.comboBoxClientName.addItem("")

        xmlval = QDomDocument()
        if (xmlval.setContent(xmlresult) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return False

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node

        childnode = xmlroot.firstChild()
        while not childnode.isNull():

            if childnode.attributes().namedItem("name").nodeValue() == "clients":
                rownode = childnode.firstChild()

                while not rownode.isNull():

                    client_name = self.pnc.get_column_value(rownode, "client_name")

                    self.ui.comboBoxClientName.addItem(client_name)

                    rownode = rownode.nextSibling()

            childnode = childnode.nextSibling()

        self.ui.lineEditName.setText("")
        self.ui.lineEditEmail.setText("")
        self.ui.lineEditOfficePhone.setText("")
        self.ui.lineEditMobilePhone.setText("")
        self.ui.lineEditRole.setText("")
        self.ui.comboBoxClientName.setCurrentText("")

        return True

    def save_window_state(self):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

    def add_contact(self):

        xmlstr = f"""<?xml version="1.0" encoding="UTF-8"?>
        <projectnotes>
        <table name="clients">
                <row>
                    <column name="client_name">{self.ui.comboBoxClientName.currentText()}</column>
                </row>
        </table>
        <table name="people">
                <row>
                    <column name="name">{self.ui.lineEditName.text()}</column>
                    <column name="email">{self.ui.lineEditEmail.text()}</column>
                    <column name="office_phone">{self.ui.lineEditOfficePhone.text()}</column>
                    <column name="cell_phone">{self.ui.lineEditMobilePhone.text()}</column>
                    <column name="client_id" lookupvalue="{self.ui.comboBoxClientName.currentText()}"></column>
                    <column name="role">{self.ui.lineEditRole.text()}</column>
                </row>
        </table>
        <table name="project_people">
            <row>
                <column lookupvalue="{self.project_number}"></column>
                <column name="people_id" lookupvalue="{self.ui.lineEditName.text()}"></column>
                <column name="receive_status_report"></column>
                <column name="role">{self.ui.lineEditRole.text()}</column>
            </row>
        </table>
        </projectnotes>
        """

        print(xmlstr)

        projectnotes.update_data(xmlstr)

        self.save_window_state()
        self.accept()

    def reject_changes(self):
        self.save_window_state()
        
        # Call the base class implementation
        self.reject()

    def closeEvent(self, event):
        self.save_window_state()
        # Call the base class implementation
        super().closeEvent(event)

def menuQuickAdd(xmlstr, parameter):
    tmqa.setup_window(xmlstr)
    tmqa.show()
    return ""


# Use code below for testing
if __name__ == '__main__':
    print("Entered __main__")
    app = QApplication(sys.argv)
    os.chdir("..")

    pnc = ProjectNotesCommon()
    tmqa = TeamMemberQuickAdd(pnc.get_main_window())
    menuQuickAdd("", "")
    sys.exit(app.exec())
else:
    pnc = ProjectNotesCommon()
    tmqa = TeamMemberQuickAdd(pnc.get_main_window())



