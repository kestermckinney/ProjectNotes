import json
import platform

from PyQt6 import QtCore
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QElapsedTimer, QStandardPaths, QDir, QJsonDocument, QSettings
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QDialog, QFileDialog, QWidget, QTableWidgetItem, QStyledItemDelegate, QComboBox

from includes.graphapi_tools import GraphAPITools, TokenAPI
if (platform.system() == 'Windows'):
    from includes.outlook_tools import ProjectNotesOutlookTools
from includes.common import ProjectNotesCommon

class CollaborationTools:
    def __init__(self):
        super().__init__()
        self.pnc = ProjectNotesCommon()

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
  
    def send_an_email(self, xmlstr, subject, content, attachment, invitees, ignoreattachments=False):
        xmlval = QDomDocument()
        xmldoc = ""

        attachments = []
        project_number = None
        project_name = None
        
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to draft an email.",QMessageBox.StandardButton.Cancel)
            return ""
            
        xmlroot = xmlval.documentElement()

        email_body_filled_in = self.pnc.strip_html_body(self.pnc.replace_variables(content, xmlroot))

        addresses = self.list_builder(xmlroot, "email", invitees)

        email_subject = self.pnc.replace_variables(subject, xmlroot)

        if not ignoreattachments:
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
                gapi.set_token(token)

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
        meeting_template_filled_in = self.pnc.replace_variables(content, xmlroot)

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
                gapi.set_token(token)

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

