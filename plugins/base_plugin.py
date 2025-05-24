import sys
import platform
import threading
import time
import json
import projectnotes

from includes.common import ProjectNotesCommon
from includes.noteformatter import NoteFormatter
from includes.graphapi_tools import GraphAPITools, TokenAPI
if (platform.system() == 'Windows'):
    from includes.outlook_tools import ProjectNotesOutlookTools
from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtCore import Qt, QRect 
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
    {"menutitle" : "Copy Email to Clipboard", "function" : "menuCopyContactEmail", "tablefilter" : "people", "submenu" : "", "dataexport" : "people"}, 
    {"menutitle" : "Copy Email to Clipboard", "function" : "menuCopyTeamEmail", "tablefilter" : "project_people", "submenu" : "", "dataexport" : "project_people"},
    {"menutitle" : "Copy Email to Clipboard", "function" : "menuCopyAttendeeEmail", "tablefilter" : "meeting_attendees", "submenu" : "", "dataexport" : "meeting_attendees"}, 
    {"menutitle" : "Copy Path to Clipboard", "function" : "menuCopyPath", "tablefilter" : "project_locations", "submenu" : "", "dataexport" : "project_locations"}, 
    {"menutitle" : "Script Editor", "function" : "menuOpenEditor", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : ""},
    {"menutitle" : "Send Meeting Notes", "function" : "menuSendNotes", "tablefilter" : "", "submenu" : "", "dataexport" : "project_notes"},
    {"menutitle" : "Send Email", "function" : "menuSendProjectEmail", "tablefilter" : "projects/project_people", "submenu" : "", "dataexport" : "projects"},
    {"menutitle" : "Send Internal Email", "function" : "menuSendInternalProjectEmail", "tablefilter" : "projects/project_people", "submenu" : "", "dataexport" : "projects"},
    {"menutitle" : "Send Email", "function" : "menuSendProjectEmail", "tablefilter" : "project_people", "submenu" : "", "dataexport" : "project_people"},
    {"menutitle" : "Send Email", "function" : "menuSendProjectEmail", "tablefilter" : "", "submenu" : "", "dataexport" : "people"},
    {"menutitle" : "Email as Attachment", "function" : "menuSendProjectEmail", "tablefilter" : "project_locations", "submenu" : "", "dataexport" : "project_locations"},

    {"menutitle" : "Export Meeting Notes", "function" : "menuExportMeetingNotes", "tablefilter" : "projects/project_notes/meeting_attendees/item_tracker/project_locations", "submenu" : "Utilities", "dataexport" : "projects"}, #todo
    {"menutitle" : "Export Contacts to Outlook", "function" : "menuExportContactsToOutlook", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : ""},   #todo make dynamic for linux
    {"menutitle" : "Import Contacts from Outlook", "function" : "menuImportContactsFromOutlook", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : ""},   #todo make dynamic for linux
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

    def copy_email_to_clipboard(self, xmlstr):

        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
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
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
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

    def send_an_email(self, xmlstr, subject, content, attachment, company_filter):
        xmlval = QDomDocument()
        xmldoc = ""

        addresses = []
        attachments = []
        project_number = None
        project_name = None
        
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
                project_number = self.pnc.get_column_value(memberrow, "project_number")
                project_name = self.pnc.get_column_value(memberrow, "project_name")

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
        if not teammember.isNull():
            memberrow = teammember.firstChild()

            while not memberrow.isNull():

                nm = self.pnc.get_column_value(memberrow, "name")
                email = self.pnc.get_column_value(memberrow, "email")

                if nm != pm:
                    if (email is not None and email != "" and (company_filter is None or pco == company_filter)):
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
        elif not project_number is None:
            email_subject = f"{project_number} {project_name} - {subject}"


        projlocation = self.pnc.find_node(xmlroot, "table", "name", "project_locations")

        if not projlocation.isNull():
            locationrow = projlocation.firstChild()

            while not locationrow.isNull():
                fp = self.pnc.get_column_value(locationrow, "full_path")

                attachments.append(fp)

                locationrow = locationrow.nextSibling()

        # if a single attchment was specified, add it
        if not attachment is None:
            attachments.append(attachment)

        # determine what method to use to send emails
        use_graph_api = (self.pnc.get_plugin_setting("IntegrationType", "Outlook Integration") == "Office 365 Application")
        use_o365 = self.pnc.get_plugin_setting("SendO365", "Outlook Integration").lower() == "true"

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

                return ""

            else:
                msg = "No token was returned.  Office 365 sync failed.  Make sure Outlook Integrations are configured correctly."
                print(msg)
                QMessageBox.critical(None, "Cannot Send Email", msg, QMessageBox.StandardButton.Ok)
                return ""

        elif platform.system() == 'Windows':
            pnot = ProjectNotesOutlookTools()

            msg = pnot.send_email(addresses, email_subject, content, attachments)
            if msg is not None:
                print(msg)
                QMessageBox.critical(None, "Cannot Send Email", msg, QMessageBox.StandardButton.Ok)
                return ""

        msg = "Sending email using Outlook is not supported on this operating system."
        print(msg)
        QMessageBox.critical(None, "Not Supported", msg,QMessageBox.StandardButton.Ok)

        return ""

    def schedule_a_meeting(self, xmlstr, subject, content, attachment, company_filter):
        xmlval = QDomDocument()
        xmldoc = ""

        addresses = []
        attachments = []
        project_number = None
        project_name = None
        
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
                project_number = self.pnc.get_column_value(memberrow, "project_number")
                project_name = self.pnc.get_column_value(memberrow, "project_name")

                # if filtering by company only includ matching client names
                # don't email to yourself, exclude the PM
                if (nm != pm):
                    if (email is not None and email != "" and (company_filter is None or pco == company_filter)):
                        addresses.append(
                        {
                            "emailAddress": {
                                "address": email,
                                "name": nm
                            },
                            "type": "required"
                        })

                memberrow = memberrow.nextSibling()

        teammember = self.pnc.find_node(xmlroot, "table", "name", "meeting_attendees")
        if not teammember.isNull():
            memberrow = teammember.firstChild()

            while not memberrow.isNull():

                nm = self.pnc.get_column_value(memberrow, "name")
                email = self.pnc.get_column_value(memberrow, "email")

                if nm != pm:
                    if (email is not None and email != "" and (company_filter is None or pco == company_filter)):
                        addresses.append(
                        {
                            "emailAddress": {
                                "address": email,
                                "name": nm
                            },
                            "type": "required"
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
                                "address": email,
                                "name": nm
                            },
                            "type": "required"
                        })

                memberrow = memberrow.nextSibling()


        meeting_subject = subject

        project = self.pnc.find_node(xmlroot, "table", "name", "projects")
        if not project.isNull():
            projectrow = project.firstChild()

            if not projectrow.isNull():
                meeting_subject = f"{self.pnc.get_column_value(projectrow, "project_number")} {self.pnc.get_column_value(projectrow, "project_name")} - {subject}"
        elif not project_number is None:
            meeting_subject = f"{project_number} {project_name} - {subject}"


        projlocation = self.pnc.find_node(xmlroot, "table", "name", "project_locations")

        if not projlocation.isNull():
            locationrow = projlocation.firstChild()

            while not locationrow.isNull():
                fp = self.pnc.get_column_value(locationrow, "full_path")

                attachments.append(fp)

                locationrow = locationrow.nextSibling()

        # if a single attchment was specified, add it
        if not attachment is None:
            attachments.append(attachment)

        # determine what method to use to send emails
        use_graph_api = (self.pnc.get_plugin_setting("IntegrationType", "Outlook Integration") == "Office 365 Application")
        use_o365 = self.pnc.get_plugin_setting("SendO365", "Outlook Integration").lower() == "true"

        if use_o365 and use_graph_api:
            token = tapi.authenticate()

            if token is not None:
                gapi = GraphAPITools()
                gapi.setToken(token)

                starttime = QDateTime.currentDateTime()
                starttime.setTime(QTime(starttime.time().hour() + 1, 0))
                endtime = starttime.addSecs(3600)

                msg = gapi.draft_a_meeting(addresses, meeting_subject, content, starttime, endtime)
                if msg is not None:
                    print(msg)
                    QMessageBox.critical(None, "Cannot Draft a Meeting", msg, QMessageBox.StandardButton.Ok)
                    return ""

                return ""

            else:
                msg = "No token was returned.  Office 365 sync failed.  Make sure Outlook Integrations are configured correctly."
                print(msg)
                QMessageBox.critical(None, "Cannot Draft a Meeting", msg, QMessageBox.StandardButton.Ok)
                return ""

        elif platform.system() == 'Windows':
            pnot = ProjectNotesOutlookTools()

            msg = pnot.schedule_meeting(addresses, email_subject, content)
            if msg is not None:
                print(msg)
                QMessageBox.critical(None, "Cannot Send Email", msg, QMessageBox.StandardButton.Ok)
                return ""

        msg = "Sending email using Outlook is not supported on this operating system."
        print(msg)
        QMessageBox.critical(None, "Not Supported", msg,QMessageBox.StandardButton.Ok)

        return ""

    def open_editor(self):
        EditorFullPath = self.pnc.get_plugin_setting("EditorPath", "Custom Editor")
            
        if (EditorFullPath is None or EditorFullPath == ""):
            QMessageBox.critical(None, "Editor Not Specified", "You will need to specify an editor in the Open Editor plugin settings.", QMessageBox.StandardButton.Cancel)
        else:
            self.pnc.exec_program( EditorFullPath )
        return ""

def populate_menu_from_json(json_string):
    global json_menu_data

    menu_array = []

    # nothing was saved
    if (json_string is None or json_string == ""):
        return ""

    json_menu_data = json.loads(json_string)

    if (len(json_menu_data) > 0):
        # Populate the table with data
        # make sure to filter the xml to the top level.  We doon't want to get the full projeect xml
        for row, row_data in enumerate(json_menu_data): 
            tablefilter = None
            dataexport = None

            inviteees = row_data["Invitees"]

            if (inviteees == "Individual"):
                menu_array.append({"menutitle" : row_data["Meeting Type"], "function" : "menuScheduleMeeting",  "tablefilter" : "people", "submenu" : "Schedule Meeting", "dataexport" : "people", "parameter" : row_data["Meeting Type"] })
            else:
                menu_array.append({"menutitle" : row_data["Meeting Type"], "function" : "menuScheduleMeeting",  "tablefilter" : "projects/project_people", "submenu" : "Schedule Meeting", "dataexport" : "projects", "parameter" : row_data["Meeting Type"] })
                menu_array.append({"menutitle" : row_data["Meeting Type"], "function" : "menuScheduleMeeting",  "tablefilter" : "project_notes/meeting_attendees", "submenu" : "Schedule Meeting", "dataexport" : "project_notes", "parameter" : row_data["Meeting Type"] })

    return menu_array

def menuScheduleMeeting(xmlstr, parameter):
    global json_menu_data

    xmlval = QDomDocument()

    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        

    # find meeting type
    meeting_type = None
    for d in json_menu_data:
        meet = d.get('Meeting Type')
        if meet == parameter:
            meeting_type = d

    if meeting_type is None or len(meeting_type) == 0:
        QMessageBox.critical(None, "Meeting Type Error", "Unable schedule a meeting.  The meeting type is not configured correctly.",QMessageBox.StandardButton.Cancel)
        return ""

    meeting_template = meeting_type.get('Template')
    invitees = meeting_type.get('Invitees')
    subject = meeting_type.get('Subject')
    company_filter = None #TODO add this


    meeting_template_filled_in = self.pnc.replace_variables(meeting_template, xmlroot)
    subject_filled_in = self.pnc.replace_variables(subject, xmlroot)

    baseplugin = BasePlugins()
    baseplugin.schedule_a_meeting(xmlstr, subject_filled_in, meeting_template_filled_in, None, company_filter)

    return ""

pnc = ProjectNotesCommon()
json_menu_data = None
menu_data = pnc.get_plugin_setting("MeetingTypes", "Meeting Types")
pluginmenus.append(populate_menu_from_json(menu_data))

def menuCopyContactEmail(xmlstr, parameter):
    baseplugin = BasePlugins()
    return baseplugin.copy_email_to_clipboard(xmlstr)

def menuCopyTeamEmail(xmlstr, parameter):
    baseplugin = BasePlugins()
    return baseplugin.copy_email_to_clipboard(xmlstr)

def menuCopyAttendeeEmail(xmlstr, parameter):
    baseplugin = BasePlugins()
    return baseplugin.copy_email_to_clipboard(xmlstr)

def menuCopyPath(xmlstr, parameter):
    baseplugin = BasePlugins()
    return baseplugin.copy_path_to_clipboard(xmlstr)

def menuOpenEditor(parameter):
    baseplugin = BasePlugins()
    return baseplugin.open_editor()

def menuSendInternalProjectEmail(xmlstr, parameter):
    xmlval = QDomDocument()
    
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to draft an email.",QMessageBox.StandardButton.Cancel)
        return ""
        
    xmlroot = xmlval.documentElement()

    co = xmlroot.toElement().attribute("managing_company_name")

    baseplugin = BasePlugins()
    return baseplugin.send_an_email(xmlstr, "", "", None, co)

def menuSendProjectEmail(xmlstr, parameter):
    baseplugin = BasePlugins()
    return baseplugin.send_an_email(xmlstr, "", "", None, None)

def menuSendNotes(xmlstr, parameter):
    baseplugin = BasePlugins()
    nf = NoteFormatter(xmlstr)
    return baseplugin.send_an_email(xmlstr, nf.getSubject(), nf.getHTML(), None, None)

