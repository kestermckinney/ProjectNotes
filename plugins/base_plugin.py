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
    {"menutitle" : "Copy Email to Clipboard", "function" : "menuCopyContactEmail", "tablefilter" : "people", "submenu" : "", "dataexport" : "people"}, 
    {"menutitle" : "Copy Email to Clipboard", "function" : "menuCopyTeamEmail", "tablefilter" : "project_people", "submenu" : "", "dataexport" : "project_people"},
    {"menutitle" : "Copy Email to Clipboard", "function" : "menuCopyAttendeeEmail", "tablefilter" : "meeting_attendees", "submenu" : "", "dataexport" : "meeting_attendees"}, 
    {"menutitle" : "Copy Path to Clipboard", "function" : "menuCopyPath", "tablefilter" : "project_locations", "submenu" : "", "dataexport" : "project_locations"}, 
    {"menutitle" : "Script Editor", "function" : "menuOpenEditor", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : ""},
    {"menutitle" : "Send Meeting Notes", "function" : "menuSendNotes", "tablefilter" : "", "submenu" : "", "dataexport" : "project_notes"},
    #{"menutitle" : "My Shortcuts", "function" : "menuMyShortcutSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
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

    def list_builder(self, xmlroot, listtype, invitees):
        pm = xmlroot.toElement().attribute("managing_manager_name")
        co = xmlroot.toElement().attribute("managing_company_name")
        cli = None
        company_filter = None
        company_exclude = None

        addresses = []

        projects = self.pnc.find_node(xmlroot, "table", "name", "projects")
        if not projects.isNull():
            project = projects.firstChild()
            cli = self.pnc.get_column_value(project, "client_id")


        if invitees == "Internal Project Team":
            company_filter = co
        elif invitees == "Exclude Client":
            company_exclude = cli
        elif invitees == "Only Client":
            company_filter = cli
        elif invitees == "Full Project Team":
            company_filter = None
            company_exclude = None
        elif invitees == "Individual":
            company_filter = None
            company_exclude = None

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
                    if (email is not None and email != "" and (company_filter is None or pco == company_filter) and (company_exclude is None or pco != company_exclude)):
                        if listtype == "email": 
                            addresses.append(
                            {
                                "emailAddress": {
                                    "address": email
                                }
                            })
                        else:
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
                pco = self.pnc.get_column_value(memberrow, "client_name")

                if nm != pm:
                    if (email is not None and email != "" and (company_filter is None or pco == company_filter) and (company_exclude is None or pco != company_exclude)):
                        if listtype == "email": 
                            addresses.append(
                            {
                                "emailAddress": {
                                    "address": email
                                }
                            })
                        else:
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
                pco = self.pnc.get_column_value(memberrow, "client_id")

                colnode = self.pnc.find_node(memberrow, "column", "name", "client_id")
                if not colnode.isNull() and colnode.attributes().namedItem("lookupvalue").nodeValue() is not None and colnode.attributes().namedItem("lookupvalue").nodeValue() != '':
                    pco = colnode.attributes().namedItem("lookupvalue").nodeValue()

                if nm != pm:
                    if (email is not None and email != "" and (company_filter is None or pco == company_filter) and (company_exclude is None or pco != company_exclude)):
                        if listtype == "email": 
                            addresses.append(
                            {
                                "emailAddress": {
                                    "address": email
                                }
                            })
                        else:
                            addresses.append(
                            {
                                "emailAddress": {
                                    "address": email,
                                    "name": nm
                                },
                                "type": "required"
                            })

                memberrow = memberrow.nextSibling()

        return addresses


    def send_an_email(self, xmlstr, subject, content, attachment, invitees):
        xmlval = QDomDocument()
        xmldoc = ""

        attachments = []
        project_number = None
        project_name = None
        
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to draft an email.",QMessageBox.StandardButton.Cancel)
            return ""
            
        #print(xmlstr)

        xmlroot = xmlval.documentElement()

        email_body_filled_in = self.pnc.replace_variables(content, xmlroot)

        addresses = self.list_builder(xmlroot, "email", invitees)

        email_subject = self.pnc.replace_variables(subject, xmlroot)

        locations = self.pnc.find_node(xmlroot, "table", "name", "project_locations")

        if not locations.isNull():
            locationrow = locations.firstChild()

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

                msg = gapi.draft_an_email(addresses, email_subject, email_body_filled_in, attachments)
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

            msg = pnot.send_email(addresses, email_subject, email_body_filled_in, attachments)
            if msg is not None:
                print(msg)
                QMessageBox.critical(None, "Cannot Send Email", msg, QMessageBox.StandardButton.Ok)
                return ""

        msg = "Sending email using Outlook is not supported on this operating system."
        print(msg)
        QMessageBox.critical(None, "Not Supported", msg,QMessageBox.StandardButton.Ok)

        return ""

    def schedule_a_meeting(self, xmlstr, subject, content, attachment, invitees):
        xmlval = QDomDocument()
        xmldoc = ""

        attachments = []
        
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to draft an email.",QMessageBox.StandardButton.Cancel)
            return ""
            
        xmlroot = xmlval.documentElement()

        addresses = self.list_builder(xmlroot, "meeting", invitees)

        meeting_subject = self.pnc.replace_variables(subject, xmlroot)
        meeting_template_filled_in = pnc.replace_variables(content, xmlroot)

        locations = self.pnc.find_node(xmlroot, "table", "name", "project_locations")

        if not locations.isNull():
            locationrow = locations.firstChild()

            while not locationrow.isNull():
                fp = self.pnc.get_column_value(locationrow, "full_path")
                attachments.append(fp)
                locationrow = locationrow.nextSibling()

        # if a single attchment was specified, add it
        if not attachment is None:
            attachments.append(attachment)

        # determine what method to use to send emails
        use_graph_api = (self.pnc.get_plugin_setting("IntegrationType", "Outlook Integration") == "Office 365 Application")
        use_o365 = self.pnc.get_plugin_setting("ScheduleO365", "Outlook Integration").lower() == "true"

        if use_o365 and use_graph_api:
            token = tapi.authenticate()

            if token is not None:
                gapi = GraphAPITools()
                gapi.setToken(token)

                starttime = QDateTime.currentDateTime()
                starttime.setTime(QTime(starttime.time().hour() + 1, 0))
                endtime = starttime.addSecs(3600)

                msg = gapi.draft_a_meeting(addresses, meeting_subject, meeting_template_filled_in, starttime, endtime)
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

            msg = pnot.schedule_meeting(addresses, meeting_subject, meeting_template_filled_in)
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
            msg = "You will need to specify an editor in the Open Editor plugin settings."
            print(msg)
            QMessageBox.critical(None, "Editor Not Specified", msg, QMessageBox.StandardButton.Cancel)
        else:
            self.pnc.exec_program( EditorFullPath )
        return ""

def populate_dynamic_menu(json_string):
    global json_menu_data
    global pluginmenus

    # nothing was saved
    if (json_string is None or json_string == ""):
        return

    json_menu_data = json.loads(json_string)
    use_graph_api = (pnc.get_plugin_setting("IntegrationType", "Outlook Integration") == "Office 365 Application")

    if (len(json_menu_data) > 0):
        # Populate the table with data
        # make sure to filter the xml to the top level.  We doon't want to get the full projeect xml
        for row, row_data in enumerate(json_menu_data): 
            tablefilter = None
            dataexport = None

            inviteees = row_data["Invitees"]

            if row_data["Type"] == "Email":
                if (inviteees == "Individual"):
                    pluginmenus.append({"menutitle" : row_data["Name"], "function" : "menuSendEmail",  "tablefilter" : "people", "submenu" : "Send Email", "dataexport" : "people", "parameter" : row_data["Name"] })
                elif (inviteees == "Attachment Only"):
                    pluginmenus.append({"menutitle" : row_data["Name"], "function" : "menuSendEmail",  "tablefilter" : "project_locations", "submenu" : "Send Email", "dataexport" : "project_locations", "parameter" : row_data["Name"] })                    
                else:
                    pluginmenus.append({"menutitle" : row_data["Name"], "function" : "menuSendEmail",  "tablefilter" : "projects/project_people", "submenu" : "Send Email", "dataexport" : "projects", "parameter" : row_data["Name"] })
                    pluginmenus.append({"menutitle" : row_data["Name"], "function" : "menuSendEmail",  "tablefilter" : "project_notes/meeting_attendees", "submenu" : "Send Email", "dataexport" : "project_notes", "parameter" : row_data["Name"] })
            else:
                if (inviteees == "Individual"):
                    pluginmenus.append({"menutitle" : row_data["Name"], "function" : "menuScheduleMeeting",  "tablefilter" : "people", "submenu" : "Schedule Meeting", "dataexport" : "people", "parameter" : row_data["Name"] })
                elif (inviteees == "Attachment Only"):
                    pluginmenus.append({"menutitle" : row_data["Name"], "function" : "menuScheduleMeeting",  "tablefilter" : "project_locations", "submenu" : "Schedule Meeting", "dataexport" : "project_locations", "parameter" : row_data["Name"] })                    
                else:
                    pluginmenus.append({"menutitle" : row_data["Name"], "function" : "menuScheduleMeeting",  "tablefilter" : "projects/project_people", "submenu" : "Schedule Meeting", "dataexport" : "projects", "parameter" : row_data["Name"] })
                    pluginmenus.append({"menutitle" : row_data["Name"], "function" : "menuScheduleMeeting",  "tablefilter" : "project_notes/meeting_attendees", "submenu" : "Schedule Meeting", "dataexport" : "project_notes", "parameter" : row_data["Name"] })

    if (platform.system() == 'Windows' and not use_graph_api):
        pluginmenus.append({"menutitle" : "Export Contacts to Outlook", "function" : "menuExportContactsToOutlook", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : "", "parameter" : ""})
        pluginmenus.append({"menutitle" : "Import Contacts from Outlook", "function" : "menuImportContactsFromOutlook", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : "", "parameter" : ""})

def menuScheduleMeeting(xmlstr, parameter):
    global json_menu_data

    # find meeting type
    nam = None
    for d in json_menu_data:
        itemname = d.get('Name')
        if itemname == parameter:
            nam = d

    if nam is None or len(nam) == 0:
        QMessageBox.critical(None, "Meeting Type Error", "Unable schedule a meeting.  The meeting type is not configured correctly.",QMessageBox.StandardButton.Cancel)
        return ""

    template = nam.get('Template')
    invitees = nam.get('Invitees')
    subject = nam.get('Subject')

    baseplugin = BasePlugins()
    baseplugin.schedule_a_meeting(xmlstr, subject, template, None, invitees)

    return ""

def menuSendEmail(xmlstr, parameter):
    global json_menu_data

    # find meeting type
    nam = None
    for d in json_menu_data:
        itemname = d.get('Name')
        if itemname == parameter:
            nam = d

    if nam is None or len(nam) == 0:
        QMessageBox.critical(None, "Meeting Type Error", "Unable schedule a meeting.  The meeting type is not configured correctly.",QMessageBox.StandardButton.Cancel)
        return ""

    template = nam.get('Template')
    invitees = nam.get('Invitees')
    subject = nam.get('Subject')

    baseplugin = BasePlugins()
    baseplugin.send_an_email(xmlstr, subject, template, None, invitees)

    return ""

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

def menuSendNotes(xmlstr, parameter):
    baseplugin = BasePlugins()
    nf = NoteFormatter(xmlstr)
    return baseplugin.send_an_email(xmlstr, nf.getSubject(), nf.getHTML(), None, "Full Project Team")

def menuImportContactsFromOutlook(parameter):
    pnot = ProjectNotesOutlookTools()
    pnot.import_contacts("")

    return ""

def menuExportContactsFromOutlook(parameter):
    pnot = ProjectNotesOutlookTools()

    xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table name="people" />\n</projectnotes>\n'
    xmlresult = projectnotes.get_data(xmldoc)

    pnot.export_contacts(xmlresult)

    return ""

pnc = ProjectNotesCommon()
json_menu_data = None
menu_data = pnc.get_plugin_setting("MeetingEmailTypes", "Meeting And Email Types")
populate_dynamic_menu(menu_data)

