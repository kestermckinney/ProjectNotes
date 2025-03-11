import platform
import msal
import json
import requests
import inspect

import projectnotes


if (platform.system() == 'Windows'):
    from win32com.client import GetObject
    import win32com
    import win32api
    import win32gui

top_windows = []

def windowEnumerationHandler(hwnd, tpwindows):
    if (platform.system() == 'Windows'):
        tpwindows.append((hwnd, win32gui.GetWindowText(hwnd)))

from PyQt6 import QtGui, QtCore, QtWidgets, uic
from common import ProjectNotesCommon
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QElapsedTimer, QStandardPaths, QDir, QJsonDocument
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices

class TokenAPI:
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()

        #self.tenant_id = 'cornerstonecontrols.com'
        #self.application_id = "a1786502-3fc6-4e15-94e9-b10b24d1c668"  # this if for MIGR Use "common" for multi-tenant or app-specific tenant id

        self.settings_pluginname = "Outlook Integration"
        # self.use_graph_api = (self.pnc.get_plugin_setting("IntegrationType", self.settings_pluginname) == "Office 365 Application")
        self.application_id = self.pnc.get_plugin_setting("ApplicationID", self.settings_pluginname)
        self.tenant_id = self.pnc.get_plugin_setting("TenantID", self.settings_pluginname)
        # self.sync_contacts = self.pnc.get_plugin_setting("SyncContacts", self.settings_pluginname).lower() == "true")
        # self.sync_todo_with_due = self.pnc.get_plugin_setting("SyncToDoWithDue", self.settings_pluginname).lower() == "true")
        # self.sync_todo_without_due = self.pnc.get_plugin_setting("SyncToDoDoWithoutDue", self.settings_pluginname).lower() == "true")
        # self.backup_emails = self.pnc.get_plugin_setting("BackupEmails", self.settings_pluginname).lower() == "true")
        # self.backup_inbox_folder = self.pnc.get_plugin_setting("BackupInBoxFolder", self.settings_pluginname)
        # self.backup_sent_folder = self.pnc.get_plugin_setting("BackupSentFolder", self.settings_pluginname)

        self.temporary_folder = QStandardPaths.writableLocation(QStandardPaths.StandardLocation.TempLocation)
        self.token_cache_file = self.temporary_folder + '/token_cache.json'
        self.scopes = ["Mail.Send", "Mail.ReadWrite", "Contacts.Read", "Contacts.ReadWrite","Calendars.ReadWrite", "Tasks.ReadWrite"]
        self.token_expires = None
        self.token_response = None
        self.access_token = None

    def authenticate(self):
        timer = QElapsedTimer()
        timer.start()

        if not self.access_token is None:
            # Check if token has not expired
            if (self.token_expires > QDateTime.currentDateTime()):
                execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
                print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")
                return(self.access_token)

        cache = msal.SerializableTokenCache()

        file = QFile(self.token_cache_file)
        if file.exists():
            if file.size() > 0:
                if file.open(QIODevice.OpenModeFlag.ReadOnly):
                    json_data = file.readAll().data().decode("utf-8")
                    file.close()
                    cache.deserialize(json.loads(json_data))
                else:
                    print(f"Could not open {self.token_cache_file}.")
            else:
                print(f"File {self.token_cache_file} was empty().")

        # Initialize the MSAL public client app (no client secret or client id needed)
        app = msal.PublicClientApplication(
            self.application_id,  
            authority=f"https://login.microsoftonline.com/{self.tenant_id}",
            token_cache=cache
        )

        # Initiating the device code flow
        flow = app.initiate_device_flow(scopes=self.scopes)

        if "user_code" not in flow:
            print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")
            return(None)

        self.token_response = None
        result = None

        accounts = app.get_accounts()
        
        if accounts:
            self.token_response = app.acquire_token_silent(self.scopes, account=accounts[0])
 
        if not self.token_response:
            # Poll for the access token (after user completes the login)
            self.token_response = app.acquire_token_interactive(self.scopes)

        
        file = QFile(self.token_cache_file)
        if file.open(QIODevice.OpenModeFlag.WriteOnly):
            jobj = cache.serialize()
            file.write(json.dumps(jobj).encode("utf-8"))
            file.close()
        else:
            print(f"Failed to open {self.token_cache_file}.")


        if "access_token" in self.token_response:

            expires_in = self.token_response.get("expires_in")
            expires_on = self.token_response.get("expires_on")

            if expires_in:
                # Calculate expiration time from expires_in
                self.token_expires = QDateTime.currentDateTime().addSecs(expires_in)
            elif expires_on:
                # Use expires_on directly
                self.token_expires = QDateTime.fromSecsSinceEpoch(expires_on)
            else:
                print("Token response missing expiration information.")
                self.token_expires = timer.elapsed() / 1000  # Convert milliseconds to seconds
                print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")
                return(None)

            self.access_token = self.token_response["access_token"]

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")
        return(self.access_token)


class GraphAPITools:
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.GRAPH_API_ENDPOINT = 'https://graph.microsoft.com'

        self.settings_pluginname = "Outlook Integration"
        self.use_graph_api = (self.pnc.get_plugin_setting("IntegrationType", self.settings_pluginname) == "Office 365 Application")
        self.sync_contacts = self.pnc.get_plugin_setting("SyncContacts", self.settings_pluginname).lower() == "true")
        self.sync_todo_with_due = self.pnc.get_plugin_setting("SyncToDoWithDue", self.settings_pluginname).lower() == "true")
        self.sync_todo_without_due = self.pnc.get_plugin_setting("SyncToDoDoWithoutDue", self.settings_pluginname).lower() == "true")
        self.backup_emails = self.pnc.get_plugin_setting("BackupEmails", self.settings_pluginname).lower() == "true")
        self.backup_inbox_folder = self.pnc.get_plugin_setting("BackupInBoxFolder", self.settings_pluginname)
        self.backup_sent_folder = self.pnc.get_plugin_setting("BackupSentFolder", self.settings_pluginname)

        self.headers = None
        self.access_token = None


    def setToken(self, token):
        timer = QElapsedTimer()
        timer.start()

        self.access_token = token

        # HTTP headers with the access token
        self.headers = {
            'Authorization': f'Bearer {self.access_token}',
            'Content-Type': 'application/json'
        }

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")
        return(False)

    def bring_window_to_front(self, title):
        if (platform.system() != 'Windows'):
            return

        win32gui.EnumWindows(windowEnumerationHandler, top_windows)

        for i in top_windows:
            if title.lower() in i[1].lower():
                # sometimes a window of a duplicate name gets closed
                if win32gui.IsWindow(i[0]):
                    win32gui.ShowWindow(i[0],5)

                # sometimes a window of a duplicate name gets closed
                if win32gui.IsWindow(i[0]):
                    win32gui.SetForegroundWindow(i[0])
                break
        return

    def makefilename(self, datetime, subject):
        id = re.sub(r"[-`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[ ]", "", datetime)
        cleanname = id + "-" + re.sub(r"[`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[]", "_", subject)
        # there is no guarantee this length will work.  It depends on system settigns and the base path length
        return cleanname[:70]

    def upload_attachment(self, draft_email_id, file_path):

        file = QFile(file_path)
        file_content = None
        if file.open(QIODevice.OpenModeFlag.ReadOnly):
            file_content = file.readAll().data().decode("utf-8")
            file.close()

        # File attachment structure
        attachment = {
            "@odata.type": "#microsoft.graph.fileAttachment",
            "name": file_path.split("/")[-1],  # Extract the file name from the path
            "contentBytes": file_content,
            "contentType": "application/octet-stream"
        }

        # Upload the attachment to the draft email
        response = requests.post(f"{self.GRAPH_API_ENDPOINT}/v1.0/me/messages/{draft_email_id}/attachments", headers=self.headers, json=attachment)

        if response.status_code != 201:
            print(f"Response Code: {response.status_code} Failed to upload attachment: {file_path}.")
            print(response.json())

    def import_batch_of_contacts(self):
        timer = QElapsedTimer()
        timer.start()
        
        saved_state = None
        statename = "people_import"
        skip = 0
        top = 500

        skip = self.pnc.get_save_state(statename)

        # Endpoint to get the list of contacts
        contacts_endpoint = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/contacts"

        if (skip > 0 or top > 0):
            contacts_endpoint = contacts_endpoint + "?"

        if (skip > 0):
            contacts_endpoint = contacts_endpoint + f"$skip={skip}"

        if (skip > 0 and top > 0):
            contacts_endpoint = contacts_endpoint + "&"

        if (top > 0):
            contacts_endpoint = contacts_endpoint + f"$top={top}"

        if (skip > 0 or top > 0):
            contacts_endpoint = contacts_endpoint + "&"

        contacts_endpoint = contacts_endpoint + "$orderby=displayName"
        
        response = requests.get(contacts_endpoint, headers=self.headers)

        xmldoc = ""
        xmlclients = ""

        contactcount = 0

        if response.status_code == 200:
            contacts = response.json().get('value', [])
            for contact in contacts:
                contactcount = contactcount + 1
                xmldoc += '<row>\n'
                xmldoc += f'<column name="name">{self.pnc.to_xml(contact.get("displayName", ""))}</column>\n'

                emails = contact.get("emailAddresses", [{"address": ""}])

                if len(emails) > 0:
                    xmldoc += f'<column name="email">{self.pnc.to_xml(emails[0]["address"])}</column>\n'
                # else:
                #     xmldoc += f'<column name="email"></column>\n'

                phones = contact.get("businessPhones", [])

                if len(phones) > 0:
                    xmldoc += f'<column name="office_phone">{self.pnc.to_xml(phones[0])}</column>\n'
                # else:
                #     xmldoc += f'<column name="office_phone"></column>\n'

                xmldoc += f'<column name="cell_phone">{self.pnc.to_xml(contact.get("mobilePhone", ""))}</column>\n'
                xmldoc += f'<column name="client_id" lookupvalue="{self.pnc.to_xml(contact.get("companyName", ""))}"></column>\n'
                xmldoc += f'<column name="role">{self.pnc.to_xml(contact.get("jobTitle", ""))}</column>\n'
                xmldoc += '</row>\n'

                xmlclients = xmlclients + f'<row><column name="client_name">{self.pnc.to_xml(contact.get("companyName", ""))}</column></row>\n'

            if (xmldoc != ""):
                xmldoc = '<table name="people">\n' + xmldoc + '</table>\n'

            if (xmlclients != ""):
                xmlclients = '<table name="clients">\n' + xmlclients + '</table>\n'

        else:
            print(f"Response Code: {response.status_code} Failed to retrieve contacts.")
            print(response.json())

        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n{xmlclients}{xmldoc}</projectnotes>\n'

        projectnotes.update_data(xmldoc)

        self.pnc.set_save_state(statename, skip, top, contactcount)

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")

        return

    def export_batch_of_contacts(self):

        if not self.sync_contacts:  #  sync contacts is disabled
            return

        timer = QElapsedTimer()
        timer.start()

        # need to get all contacts first to search for existing ones
        # Endpoint to get the list of contacts
        contacts = None
        contacts_endpoint = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/contacts?$top=2000"
        response = requests.get(contacts_endpoint, headers=self.headers)

        if response.status_code == 200:
            contacts = response.json().get('value', [])
        else:
            print(f"Failed to retrieve contacts: {response.status_code}")
            print(response.json())
            return

        saved_state = None
        statename = "people_export"

        skip = 0
        top = 500

        skip = self.pnc.get_save_state(statename)

        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table name="people" {self.pnc.state_range_attrib(top, skip)} />\n</projectnotes>\n'

        xmlresult = projectnotes.get_data(xmldoc)
        
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlresult) == False):
            print("Unable to parse XML returned from Project Notes in contacts export.")
            return

        xmlroot = xmlval.documentElement()
        
        childnode = xmlroot.firstChild()
        
        contactcount = 0

        while not childnode.isNull():
            if childnode.attributes().namedItem("name").nodeValue() == "people":
                rownode = childnode.firstChild()

                while not rownode.isNull():
                    contactcount = contactcount + 1

                    colnode = rownode.firstChild()

                    fullname = None
                    company = None
                    workphone = None
                    workemail = None
                    cellphone = None
                    jobtitle = None
                    contact_id = None

                    while not colnode.isNull():

                        content = colnode.toElement().text()

                        if colnode.attributes().namedItem("name").nodeValue() == "name":
                            fullname = content

                        if colnode.attributes().namedItem("name").nodeValue() == "email":
                            workemail = content

                        if colnode.attributes().namedItem("name").nodeValue() == "office_phone":
                            workphone = content

                        if colnode.attributes().namedItem("name").nodeValue() == "cell_phone":
                            cellphone = content

                        if colnode.attributes().namedItem("name").nodeValue() == "client_id":
                            company = colnode.attributes().namedItem("lookupvalue").nodeValue()

                        if colnode.attributes().namedItem("name").nodeValue() == "role":
                            jobtitle = content

                        colnode = colnode.nextSibling()

                    matching_contacts = [contact for contact in contacts if contact["displayName"].lower() == fullname.lower()]

                    if len(matching_contacts) == 0:
                        # Body of the PATCH request to update personalNotes
                        contact_details = {
                            "displayName": fullname,
                            "emailAddresses" : [ { "address" : workemail, "name" : fullname} ],
                            "businessPhones" : [workphone],
                            "mobilePhone" : cellphone,
                            "companyName" : company,
                            "jobTitle" : jobtitle
                        }

                        # Endpoint to add a new contact
                        contacts_endpoint = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/contacts"

                        # Make a POST request to create a new contact
                        response = requests.post(contacts_endpoint, headers=self.headers, json=contact_details)

                        if response.status_code != 201:
                            print(f"Response Code: {response.status_code} Failed to add contact: {fullname}.")
                            print(response.json())

                    rownode = rownode.nextSibling()

            childnode = childnode.nextSibling()

        self.pnc.set_save_state(statename, skip, top, contactcount)

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")

        return

    def draft_a_meeting(self, xmlstr, subject, content, datetime_start, datetime_end, company_filter):

        xmlval = QDomDocument()
        xmldoc = ""

        addresses = []
        
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to draft a meeting.",QMessageBox.StandardButton.Cancel)
            return ""
            
        xmlroot = xmlval.documentElement()
        pm = xmlroot.toElement().attribute("managing_manager_name")
        co = xmlroot.toElement().attribute("managing_company_name")

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
                    if (email is not None and email != "" and (company_filter == False or pco == co)):
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
                    if (email is not None and email != "" and (company_filter == False or pco == co)):
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
                    if (email is not None and email != "" and (company_filter == False or pco == co)):
                        addresses.append(
                            {
                                "emailAddress": {
                                    "address": email,
                                    "name": nm
                                },
                                "type": "required"
                            })

                memberrow = memberrow.nextSibling()


        meeting_title = subject

        project = self.pnc.find_node(xmlroot, "table", "name", "projects")
        if not project.isNull():
            projectrow = project.firstChild()

            if not projectrow.isNull():
                meeting_title = self.pnc.get_column_value(projectrow, "project_number") + " " + self.pnc.get_column_value(projectrow, "project_name") + f" - {subject}"

        # Endpoint to create an event (in the draft state)
        events_endpoint = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/events"

        formatted_datetime_start = datetime_start.toUTC().toString("yyyy-MM-ddThh:mm:ss")
        formatted_datetime_end = datetime_end.toUTC().toString("yyyy-MM-ddThh:mm:ss")

        event_details = {
            "subject": meeting_title,
            "body": {
                "contentType": "HTML",
                "content": content,
            },
            "start": {
                "dateTime": formatted_datetime_start,
                "timeZone": "UTC"
            },
            "end": {
                "dateTime": formatted_datetime_end,
                "timeZone": "UTC"
            },
            "attendees": addresses,
            "isDraft": True,  # This ensures the meeting is drafted and not sent
            "allowNewTimeProposals": True
        }

        # Make a POST request to create the draft event
        response = requests.post(events_endpoint, headers=self.headers, json=event_details)

        if response.status_code != 201:
            print(f"Response Code: {response.status_code} Failed to draft the meeting: {meeting_title}.")
            print(response.json())


        # bring outlook to the forefront, I don't think we can select the drafted meetig though
        self.bring_window_to_front("Outlook")

        return ""

    def draft_an_email(self, xmlstr, subject, content, attachment, company_filter):

        xmlval = QDomDocument()
        xmldoc = ""

        addresses = []
        
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
                email_subject = self.pnc.get_column_value(projectrow, "project_number") + " " + self.pnc.get_column_value(projectrow, "project_name") + f" - {subject}"
        
        # Graph API endpoint to send an email
        graph_api_endpoint = f'{self.GRAPH_API_ENDPOINT}/v1.0/me/messages'
        
        # Email data structure
        email_data = {
                "subject": email_subject,
                "body": {
                    "contentType": "HTML",
                    "content": content
                },
                "toRecipients": addresses
            }

        # Send email via Graph API
        response = requests.post(graph_api_endpoint, headers=self.headers, json=email_data)

        if response.status_code == 201:
            if attachment is not None:
                self.upload_attachment(response.json()['id'], attachment)
        else:
            print(f"Error: {response.status_code} Failed to draft email: {email_subject}.")
            print(response.json())

        return ""

    def download_project_emails(self, projectnumber, box, top, destination_folder):

        statename = destination_folder + "_" + box + "_downloadtracker"

        skip = self.pnc.get_save_state(statename)

        # Graph API endpoint to send an email
        graph_api_endpoint = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/mailFolders/{box}/messages?$filter=contains(subject,'{projectnumber}')&$select=id&$top={top}&$skip={skip}"  
            
        # Send email via Graph API
        response = requests.get(graph_api_endpoint, headers=self.headers)

        # Check response status code
        if response.status_code == 200:
            # Parse JSON response
            emails = response.json()["value"]

            # Download emails
            for email in emails:
                skip = skip + 1 # don't download past the end

                # Construct MSG download URL
                msg_url = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/messages/{email['id']}"
                msg_response = requests.get(msg_url, headers=self.headers)

                message_data = json.loads(msg_response.text)

                # Extract the subject and sent time
                subject = message_data["subject"]
                sent_time = message_data["sentDateTime"]

                if msg_response.status_code == 200:
                    msg_file = destination_folder + "/" + self.makefilename(sent_time, subject) + ".eml"

                    # only write it if it doesn't exist
                    if not QFile(msg_file).exists():
                        # Construct MSG download URL
                        msg_url = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/messages/{email['id']}/$value?$format=MessageFormat MSG"

                        # MSG download headers
                        msg_headers = {
                            "Authorization": f"Bearer {self.access_token}",
                            "Accept": "application/msg"
                        }

                        # Download MSG file
                        msg_response = requests.get(msg_url, headers=msg_headers, stream=True)

                        # Check response status code
                        if msg_response.status_code == 200:

                            # Save MSG file
                            file = QFile(msg_file)
                            if file.open(QIODevice.OpenModeFlag.WriteOnly):
                                file.write(msg_response)
                                file.close()
                            else:
                                print(f"Failed to write file {msg_file}.")


                            #print(f"Email saved: {email['id']}.msg")
                        else:
                            print(f"Resonse Code: {msg_response.status_code} Error downloading MSG: {msg_response.text}.")
                    else:
                        print(f"{msg_file} email message file already exists.")

                else:
                    print(f"Response Code: {msg_response.status_code} Error getting message details {msg_response.text}.")
        else:
            print(f"Response Code: {response.status_code} Error downloading emails {response.text}.")

        self.pnc.set_save_state(statename, skip, top, 0) # never redownload emails


    def download_batch_of_emails(self):
        timer = QElapsedTimer()
        timer.start()

        if not self.use_graph_api:  # office 365 integration is disabled
            return

        if not self.backup_emails:  #backup emails disabled
            return

        if self.backup_sent_folder == "":
            print("No backup folder was specified for Sent Items.")
            return

        if self.backup_inbox_folder == "":
            print("No backup folder was specified for InBox Items.")
            return

        saved_state = None
        statename = "emails_export"

        skip = 0
        top = 50 # writing a bunch of email files could be slow

        skip = self.pnc.get_save_state(statename)

        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table  filter_field_1="location_description" filter_value_1="Project Folder" name="project_locations" {self.pnc.state_range_attrib(top, skip)} />\n</projectnotes>\n'
        
        xmlresult = projectnotes.get_data(xmldoc)
        
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlresult) == False):
            print("Unable to parse XML returned from Project Notes in download batch of emails.")
            return

        xmlroot = xmlval.documentElement()

        childnode = xmlroot.firstChild()

        locationcount = 0

        while not childnode.isNull():
            if childnode.attributes().namedItem("name").nodeValue() == "project_locations":

                rownode = childnode.firstChild()

                while not rownode.isNull():
                    locationcount = locationcount + 1
                    
                    projectnumber = self.pnc.get_column_value(rownode, "project_number")
                    projectfolder = self.pnc.get_column_value(rownode, "full_path")

                    if QDir(projectfolder).exists():
                        emailfolder = projectfolder + "/Correspondence"
                        sentfolder = emailfolder + "/Sent Email/"
                        receivedfolder = emailfolder + "/Received Email/"

                        if not QDir(emailfolder).exists():
                            QDir.mkpath(emailfolder)

                        if not QDir(sentfolder).exists():
                            QDir.mkpath(sentfolder)

                        if not QDir(receivedfolder).exists():
                            QDir.mkpath(receivedfolder)

                        self.download_project_emails(projectnumber, "inbox", 8, receivedfolder)
                        self.download_project_emails(projectnumber, "sentitems", 8, sentfolder)

                    rownode = rownode.nextSibling()

            childnode = childnode.nextSibling()

        self.pnc.set_save_state(statename, skip, top, locationcount)

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")

        return

    def sync_tracker_to_tasks(self):

        if not self.sync_todo_with_due and not self.sync_todo_without_due:  # task sync is disabled
            return

        timer = QElapsedTimer()
        timer.start()

        saved_state = None
        statename = "tracker_export"

        skip = 0
        top = 100

        skip = self.pnc.get_save_state(statename)

        # just get the project manager people id
        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table name="clients" top="1" skip="0"/>\n</projectnotes>\n'

        timer2 = QElapsedTimer()
        timer2.start()
        
        xmlresult = projectnotes.get_data(xmldoc)

        execution_time = timer2.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function get_data in '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")


        xmlval = QDomDocument()
        if (xmlval.setContent(xmlresult) == False):
            print("Unable to parse XML returned from Project Notes in sync tracker items to tasks.")
            return

        xmlroot = xmlval.documentElement()
        if xmlroot.isNull():
            print("Failed to identify a root node in the xml in sync tracker items to tasks.")
            return 

        pmid = xmlroot.toElement().attribute("project_manager_id").strip()

        if pmid is None or pmid == '':
            print("Failed to identify the project manager sync tracker items to tasks.")
            return 

        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table name="item_tracker" filter_name_1="assigned_to" filter_value_1="{pmid}" {self.pnc.state_range_attrib(top, skip)} />\n</projectnotes>\n'

        timer2 = QElapsedTimer()
        timer2.start()

        xmlresult = projectnotes.get_data(xmldoc)

        execution_time = timer2.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function get_data in '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")

        xmlval = QDomDocument()
        if (xmlval.setContent(xmlresult) == False):
            print("Unable to parse XML returned from Project Notes sync tracker items to tasks.")
            return

        # find the to-do list
        url = "https://graph.microsoft.com/v1.0/me/todo/lists"
        response = requests.get(url, headers=self.headers)

        lists = None
        list_id = None

        if response.status_code == 200:
            lists = response.json().get("value", [])
            list_id = next((lst["id"] for lst in lists if lst["displayName"] == "Tasks"), None)
        else:
            print("Failed to retrieve todo lists.")
            print(response.text)
            return

        # get all of the to-do items
        url = f"https://graph.microsoft.com/v1.0/me/todo/lists/{list_id}/tasks?$top=10000"

        response = requests.get(url, headers=self.headers)
        todos = None

        if response.status_code == 200:
            todos = response.json().get("value", [])
        else:
            print("Failed to retrieve todo lists sync tracker items to tasks.")
            print(response.text)
            return

        xmlroot = xmlval.documentElement()

        if xmlroot.isNull():
            print("Failed to identify a root node in the xml sync tracker items to tasks.")
            return 
        
        childnode = xmlroot.firstChild()
        
        rowcount = 0

        while not childnode.isNull():
            if childnode.attributes().namedItem("name").nodeValue() == "item_tracker":
                rownode = childnode.firstChild()

                while not rownode.isNull():
                    colnode = rownode.firstChild()

                    rowcount = rowcount + 1

                    itemnumber = None
                    projectnumber = None
                    projectname = None
                    itemname = None
                    itemdescription = None
                    datedue = ""
                    createddate = ""
                    updateddate = ""
                    status = None
                    priority = None
                    comments = None
                    projectstatus = None
                    assignedto = None

                    while not colnode.isNull():

                        content = colnode.toElement().text()

                        if colnode.attributes().namedItem("name").nodeValue() == "project_id":
                            projectnumber = colnode.attributes().namedItem("lookupvalue").nodeValue().strip()

                        if colnode.attributes().namedItem("name").nodeValue() == "project_id_name":
                            projectname = content.strip()

                        if colnode.attributes().namedItem("name").nodeValue() == "item_number":
                            itemnumber = content.strip()

                        if colnode.attributes().namedItem("name").nodeValue() == "item_name":
                            itemname = content.strip()

                        if colnode.attributes().namedItem("name").nodeValue() == "description":
                            itemdescription = content

                        if colnode.attributes().namedItem("name").nodeValue() == "comments":
                            comments = content

                        if colnode.attributes().namedItem("name").nodeValue() == "date_due":
                            datedue = QDateTime.fromString(content, "MM/dd/yyyy") 

                        if colnode.attributes().namedItem("name").nodeValue() == "date_identified":
                            createddate = QDateTime.fromString(content, "MM/dd/yyyy") 

                        if colnode.attributes().namedItem("name").nodeValue() == "last_update":
                            updateddate = QDateTime.fromString(content, "MM/dd/yyyy") 

                        if colnode.attributes().namedItem("name").nodeValue() == "status":
                            status = content.strip()

                        if colnode.attributes().namedItem("name").nodeValue() == "project_status":
                            projectstatus = content.strip()

                        if colnode.attributes().namedItem("name").nodeValue() == "assigned_to":
                            assignedto = content.strip()

                        if colnode.attributes().namedItem("name").nodeValue() == "priority":
                            priority = "1"
                            if content == "High":
                                priority = "2"
                            if content == "Low":
                                priority = "0"

                        colnode = colnode.nextSibling()


                    # look for to-do task
                    prefix = f"{projectnumber}-{itemnumber}"

                    task_id = None
                    for lst in todos:
                        if lst["title"].startswith(prefix):
                            task_id = lst["id"]
                            break


                    if ((status == "New" or status == "Assigned") and projectstatus == "Active" and assignedto == pmid) :
                        #TODO need an option to set time of day for alarms
                        #TODO need to be able to turn on and off sync options
                        url = f"https://graph.microsoft.com/v1.0/me/todo/lists/{list_id}/tasks"

                        if task_id is not None:
                            url = f"{url}/{task_id}"

                        task_data = {
                            "title": f"{prefix} {projectname} {itemname}",
                            "importance" : priority,
                            "body" : 
                            {
                                "content" : f"{itemdescription}\n{comments}",
                                "contentType" : "text",
                            }
                        }

                        if createddate is not None and createddate.isValid():
                            task_data["createdDateTime"] = createddate.toUTC().toString("yyyy-MM-ddThh:mm:ss.ssZ")

                        if updateddate  is not None and  updateddate.isValid():
                            task_data["lastModifiedDateTime"] = updateddate.toUTC().toString("yyyy-MM-ddThh:mm:ssZ")

                        if datedue  is not None and datedue.isValid():
                            task_data["isReminderOn"] = True
                            task_data["dueDateTime"] = { "dateTime" : datedue.toUTC().toString("yyyy-MM-ddThh:mm:ss.ss"), "timeZone" : "UTC" }
                            task_data["reminderDateTime"] = { "dateTime" : datedue.toUTC().toString("yyyy-MM-ddThh:mm:ss.ss"), "timeZone" : "UTC" }

                        response = None
                        if task_id is None:
                            response = requests.post(url, headers=self.headers, json=task_data)

                            if response.status_code != 201:
                                print(f"\nFailed to add task '{prefix} {projectname} {itemname}': {response.status_code}\n\n {response.text}\n\n")
                        else:
                            response = requests.patch(url, headers=self.headers, json=task_data)

                            if response.status_code != 200:
                                print(f"\nFailed to update task '{prefix} {projectname} {itemname}': {response.status_code}\n\n {response.text}\n\n")
                    elif task_id is not None:
                        delete_url = f"https://graph.microsoft.com/v1.0/me/todo/lists/{list_id}/tasks/{task_id}"
                        delete_response = requests.delete(delete_url, headers=self.headers)

                        if delete_response.status_code != 204:
                            print("Failed to delete todo item sync tracker items to tasks.")
                            print(response.text)

                    rownode = rownode.nextSibling()

            childnode = childnode.nextSibling()

        self.pnc.set_save_state(statename, skip, top, rowcount)

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")

        return
