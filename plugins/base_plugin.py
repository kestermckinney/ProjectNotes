# Copyright (C) 2025, 2026 Paul McKinney
import sys
import platform
import threading
import time
import projectnotes

from includes.common import ProjectNotesCommon
from includes.graphapi_tools import GraphAPITools, TokenAPI
if (platform.system() == 'Windows'):
    from includes.outlook_tools import ProjectNotesOutlookTools
from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtCore import Qt, QRect, QDateTime, QTime
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QDialog, QFileDialog, QWidget, QTableWidgetItem, QStyledItemDelegate, QComboBox
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtGui import QDesktopServices, QClipboard


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
    {"menutitle" : "Copy Email to Clipboard", "function" : "menu_copy_contact_email", "tablefilter" : "people", "submenu" : "", "dataexport" : "people"}, 
    {"menutitle" : "Copy Email to Clipboard", "function" : "menu_copy_team_email", "tablefilter" : "project_people", "submenu" : "", "dataexport" : "project_people"},
    {"menutitle" : "Copy Email to Clipboard", "function" : "menu_copy_attendee_email", "tablefilter" : "meeting_attendees", "submenu" : "", "dataexport" : "meeting_attendees"}, 
    {"menutitle" : "Copy Path to Clipboard", "function" : "menu_copy_path", "tablefilter" : "project_locations", "submenu" : "", "dataexport" : "project_locations"}, 
    {"menutitle" : "Script Editor", "function" : "menu_open_editor", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : ""},
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

tapi = TokenAPI()

class BasePlugins:
    def __init__(self, tapi=None):
        super().__init__()
        self.pnc = ProjectNotesCommon()

    def copy_email_to_clipboard(self, xmlstr):

        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(QApplication.activeWindow(), "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return ""

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        

        if xmlroot:

            email = None
            nm = None

            teammember = self.pnc.find_node(xmlroot, "table", "name", "project_people")
            if not teammember.isNull():
                memberrow = teammember.firstChild()

                while not memberrow.isNull():
                    nm = self.pnc.get_column_value(memberrow, "name")
                    email = self.pnc.get_column_value(memberrow, "email")

                    if (email is not None and email != ""):
                        QtWidgets.QApplication.clipboard().setText(email)

                    memberrow = memberrow.nextSibling()

            teammember = self.pnc.find_node(xmlroot, "table", "name", "people")
            if not teammember.isNull():
                memberrow = teammember.firstChild()

                while not memberrow.isNull():
                    nm = self.pnc.get_column_value(memberrow, "name")
                    email = self.pnc.get_column_value(memberrow, "email")

                    if (email is not None and email != ""):
                        QtWidgets.QApplication.clipboard().setText(email)

                    memberrow = memberrow.nextSibling()

            teammember = self.pnc.find_node(xmlroot, "table", "name", "meeting_attendees")
            if not teammember.isNull():
                memberrow = teammember.firstChild()

                while not memberrow.isNull():
                    nm = self.pnc.get_column_value(memberrow, "name")
                    email = self.pnc.get_column_value(memberrow, "email")

                    if (email is not None and email != ""):
                        QtWidgets.QApplication.clipboard().setText(email)

                    memberrow = memberrow.nextSibling()

        return ""

    def copy_path_to_clipboard(self, xmlstr):
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(QApplication.activeWindow(), "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return ""

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        

        if xmlroot:
            projlocation = self.pnc.find_node(xmlroot, "table", "name", "project_locations")

            if not projlocation.isNull():
                locationrow = projlocation.firstChild()

            while not locationrow.isNull():
                fp = self.pnc.get_column_value(locationrow, "full_path")

                if (fp is not None and fp != ""):
                    QtWidgets.QApplication.clipboard().setText(fp)

                locationrow = locationrow.nextSibling()

        return ""

    def open_editor(self):
        EditorFullPath = self.pnc.get_plugin_setting("EditorPath", "Custom Editor")
            
        if (EditorFullPath is None or EditorFullPath == ""):
            msg = "You will need to specify an editor in the Open Editor plugin settings."
            print(msg)
            QMessageBox.critical(QApplication.activeWindow(), "Editor Not Specified", msg)
        else:
            self.pnc.exec_program( EditorFullPath )
        return ""

def populate_dynamic_menu():
    global pluginmenus

    use_graph_api = ((pnc.get_plugin_setting("IntegrationType", "Outlook Integration") or "") == "Office 365 Application")

    if (platform.system() == 'Windows' and not use_graph_api):
        pluginmenus.append({"menutitle" : "Download Emails from Outlook", "function" : "menu_download_emails_from_outlook", "tablefilter" : "projects/project_locations", "submenu" : "Utilities", "dataexport" : "projects", "parameter" : ""})


def menu_copy_contact_email(xmlstr, parameter):
    baseplugin = BasePlugins(tapi)
    baseplugin.copy_email_to_clipboard(xmlstr)

    return ""

def menu_copy_team_email(xmlstr, parameter):
    baseplugin = BasePlugins(tapi)
    baseplugin.copy_email_to_clipboard(xmlstr)

    return ""

def menu_copy_attendee_email(xmlstr, parameter):
    baseplugin = BasePlugins(tapi)
    return baseplugin.copy_email_to_clipboard(xmlstr)

def menu_copy_path(xmlstr, parameter):
    baseplugin = BasePlugins()
    baseplugin.copy_path_to_clipboard(xmlstr)

    return ""

def menu_open_editor(parameter):
    baseplugin = BasePlugins(tapi)
    baseplugin.open_editor()

    return ""


def menu_download_emails_from_outlook(xmlstr, parameter):
    pnot = ProjectNotesOutlookTools()
    pnot.download_emails(xmlstr)

    return ""

pnc = ProjectNotesCommon()
populate_dynamic_menu()
