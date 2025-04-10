import sys
import platform
import threading
import time
import json
import projectnotes

from includes.common import ProjectNotesCommon
from includes.graphapi_tools import GraphAPITools, TokenAPI
from includes.outlook_tools import ProjectNotesOutlookTools
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

tapi = TokenAPI()

class BasePlugins:
    def __init__(self):
        super().__init__()
        self.pnc = ProjectNotesCommon()

    def send_an_email(self, xmlstr, subject, content, attachment, company_filter):
        xmlval = QDomDocument()
        xmldoc = ""

        addresses = []
        attachments = []
        
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to draft an email.",QMessageBox.StandardButton.Cancel)
            return ""
            
        xmlroot = xmlval.documentElement()
        pm = xmlroot.toElement().attribute("managing_manager_name")

        email = None
        nm = None
        pco = None

        teammember = self.pnc.find_node(xmlroot, "table", "name", "project_people")
        if not teammember.isNull():
            memberrow = teammember.firstChild()

            while not memberrow.isNull():
                nm = self.pnc.get_column_value(memberrow, "name")
                email = self.pnc.get_column_value(memberrow, "email")
                pco = self.pnc.get_column_value(memberrow, "client_name")

                # if filtering by company only includ matching client names
                # don't email to yourself, exclude the PM
                if (nm != pm):
                    if (email is not None and email != "" and (company_filter is None or pco == company_filter)):
                        addresses.append(
                        {
                            "emailAddress": {
                                "address": email
                            }
                        })

                memberrow = memberrow.nextSibling()

        teammember = self.pnc.find_node(xmlroot, "table", "name", "meeting_attendees")
        if not teammember.isNul():
            memberrow = teammember.firstChild()

            while not memberrow.isNull():
                nm = self.pnc.get_column_value(memberrow, "name")
                email = self.pnc.get_column_value(memberrow, "email")
                pco = self.pnc.get_column_value(memberrow, "client_name")

                if nm != pm:
                    if (email is not None and email != "" and (company_filter is None or  pco == company_filter)):
                        addresses.append(
                        {
                            "emailAddress": {
                                "address": email
                            }
                        })

                memberrow = memberrow.nextSibling()

        teammember = self.pnc.find_node(xmlroot, "table", "name", "people")
        if not teammember.isNull():
            memberrow = teammember.firstChild()

            while not memberrow.isNull():
                nm = self.pnc.get_column_value(memberrow, "name")
                email = self.pnc.get_column_value(memberrow, "email")

                colnode = self.pnc.find_node(memberrow, "column", "name", "client_id")
                if not colnode.isNull() and colnode.attributes().namedItem("lookupvalue").nodeValue() is not None and colnode.attributes().namedItem("lookupvalue").nodeValue() != '':
                    pco = colnode.attributes().namedItem("lookupvalue").nodeValue()

                if nm != pm:
                    if (email is not None and email != "" and (company_filter is None or pco == company_filter)):
                        addresses.append(
                        {
                            "emailAddress": {
                                "address": email
                            }
                        })

                memberrow = memberrow.nextSibling()


        email_subject = subject

        project = self.pnc.find_node(xmlroot, "table", "name", "projects")
        if not project.isNull():
            projectrow = project.firstChild()

            if not projectrow.isNull():
                email_subject = f"{self.pnc.get_column_value(projectrow, "project_number")} {self.pnc.get_column_value(projectrow, "project_name")} - {subject}"
                

        projlocation = self.pnc.find_node(xmlroot, "table", "name", "project_locations")

        if not projlocation.isNull():
            locationrow = projlocation.firstChild()

            while not locationrow.isNull():
                fp = self.pnc.get_column_value(locationrow, "full_path")

                attachments.append(fp)

                locationrow = locationrow.nextSibling()

        # determine what method to use to send emails
        use_graph_api = (self.pnc.get_plugin_setting("IntegrationType", self.settings_pluginname) == "Office 365 Application")
        use_o365 = self.pnc.get_plugin_setting("SendO365", self.settings_pluginname).lower() == "true"

        if use_o365 and use_graph_api:
            token = tapi.authenticate()

            if token is not None:
                gapi = GraphAPITools()
                gapi.setToken(token)

                msg = gapi.draft_an_email(addresses, email_subject, content, attachments)
                if msg is not None:
                    print(msg)
                    QMessageBox.critical(None, "Cannot Send Email", msg, QMessageBox.StandardButton.Ok)
                    return ""

            else:
                msg = "No token was returned.  Office 365 sync failed.  Make sure Outlook Integrations are configured correctly."
                print(msg)
                QMessageBox.critical(None, "Cannot Send Email", msg, QMessageBox.StandardButton.Ok)
        elif platform.system() == 'Windows':
            pnot = ProjectNotesOutlookTools()

            msg = pnot.send_an_mail(addresses, email_subject, content, attachments)
            if msg is not None:
                print(msg)
                QMessageBox.critical(None, "Cannot Send Email", msg, QMessageBox.StandardButton.Ok)
                return ""

        msg = "Sending email using Outlook is not supported on this operating system."
        print(msg)
        QMessageBox.critical(None, "Not Supported", msg,QMessageBox.StandardButton.Ok)

        return ""


def menuSendProjectEmail(xmlstr, parameter):
    baseplugin = BasePlugins()

    return baseplugin.send_an_email(xmlstr, "", "", None, False)
