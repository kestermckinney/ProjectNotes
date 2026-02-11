import platform
import msal
import json
import requests
import base64
import inspect
import re
import projectnotes

if (platform.system() == 'Windows'):
    from win32com.client import GetObject
    import win32com
    import win32api
    import win32gui

top_windows = []

def windowEnumerationHandler(hwnd, tpwindows):
    if (platform.system() == 'Windows'):
        tpwindows.append((hwnd, win32gui.GetWindowText(hwnd))
)
        
from includes.common import ProjectNotesCommon
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import Qt, QFile, QIODevice, QDateTime, QUrl, QElapsedTimer, QStandardPaths, QDir, QJsonDocument, QTextStream, QStringConverter
from PyQt6.QtGui import QDesktopServices

class TokenAPI:
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()

        self.settings_pluginname = "Outlook Integration"
        self.application_id = self.pnc.get_plugin_setting("ApplicationID", self.settings_pluginname)
        self.tenant_id = self.pnc.get_plugin_setting("TenantID", self.settings_pluginname)

        self.temporary_folder = QStandardPaths.writableLocation(QStandardPaths.StandardLocation.TempLocation)
        self.token_cache_file = self.temporary_folder + '/token_cache.json'
        self.scopes = ["Mail.Send", "Mail.ReadWrite", "Contacts.Read", "Contacts.ReadWrite","Calendars.ReadWrite", "Tasks.ReadWrite", "MailboxSettings.Read"]
        self.token_expires = None
        self.token_response = None
        self.access_token = None

    def authenticate(self):
        # return nothing if not configured
        if self.tenant_id == "":
            return None

        if self.application_id == "":
            return None


        # timer = QElapsedTimer()
        # timer.start()

        if not self.access_token is None:
            # Check if token has not expired
            if (self.token_expires > QDateTime.currentDateTime()):
                # execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
                # print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")
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
                # self.token_expires = timer.elapsed() / 1000  # Convert milliseconds to seconds
                # print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")
                return(None)

            self.access_token = self.token_response["access_token"]

        # execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        # print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")
        return(self.access_token)


class GraphAPITools:
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.GRAPH_API_ENDPOINT = 'https://graph.microsoft.com'

        self.settings_pluginname = "Outlook Integration"
        self.use_graph_api = (self.pnc.get_plugin_setting("IntegrationType", self.settings_pluginname) == "Office 365 Application")
        self.import_contacts = (self.pnc.get_plugin_setting("ImportContacts", self.settings_pluginname).lower() == "true")
        self.export_contacts = (self.pnc.get_plugin_setting("ExportContacts", self.settings_pluginname).lower() == "true")
        self.sync_todo_with_due = (self.pnc.get_plugin_setting("SyncToDoWithDue", self.settings_pluginname).lower() == "true")
        self.sync_todo_without_due = (self.pnc.get_plugin_setting("SyncToDoDoWithoutDue", self.settings_pluginname).lower() == "true")
        self.backup_emails = (self.pnc.get_plugin_setting("BackupEmails", self.settings_pluginname).lower() == "true")
        self.backup_inbox_folder = self.pnc.get_plugin_setting("BackupInBoxFolder", self.settings_pluginname)
        self.backup_sent_folder = self.pnc.get_plugin_setting("BackupSentFolder", self.settings_pluginname)

        self.headers = None
        self.access_token = None

    def set_token(self, token):
        # timer = QElapsedTimer()
        # timer.start()

        self.access_token = token

        # HTTP headers with the access token
        self.headers = {
            'Authorization': f'Bearer {self.access_token}',
            'Content-Type': 'application/json'
        }

        # execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        # print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")
        return(False)

    def bring_window_to_front(self, title):
        if (platform.system() != 'Windows'):
            return

        try:
            win32gui.EnumWindows(handler_window_enumerator, top_windows)

            for i in top_windows:
                if title.lower() in i[1].lower():
                    win32gui.ShowWindow(i[0],5)
                    win32gui.SetForegroundWindow(i[0])
                    break

        except Exception as e:  # catches any Python exception
            # If it’s a COM error, the details are in e.args
            print(f'Error: {e} Attempting to bring window {title} to the foreground.')
            if hasattr(e, 'args') and e.args:
                print(f'COM error code: {e.args[0]}')   # usually a HRESULT like -2147352567

        return

    def make_filename(self, datetime, subject):
        qdt = QDateTime.fromString(datetime, Qt.DateFormat.ISODate)
        iso_string = qdt.toString(Qt.DateFormat.ISODate)  # UTC we want it to be the same format as the GraphAPI

        id = re.sub(r"[-`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[ ]", "", iso_string)
        cleanname = id + "-" + self.pnc.valid_filename(subject)
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

        #print(f"uploading {file_path} as an attachment")

        if response.status_code != 201:
            return f"Response Code: {response.status_code} Failed to upload attachment: {file_path}.  Server Responded: {response.json()}"

        return None

    def import_batch_of_contacts(self, parameter):
        if not self.use_graph_api:  # office 365 integration is disabled
            return

        if not self.import_contacts:  #  import contacts is disabled
            return

        # timer = QElapsedTimer()
        # timer.start()
        
        saved_state = None
        statename = "people_import"
        skip = 0
        top = 40

        if parameter == "all":
            top = 10000
        else:
            skip = self.pnc.get_save_state(statename)

            if skip is None:
                print("Failed to get save state.  Will try to import contacts again.")
                return

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

        if contactcount > 0:
            if self.pnc.set_save_state(statename, skip, top, contactcount) is None:
                print("Failed to set save state.  Will try to import the same contacts again.")

        # execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        # print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")

        return

    def export_batch_of_contacts(self, parameter):
        if not self.use_graph_api:  # office 365 integration is disabled
            return
        if not self.export_contacts:  #  sync contacts is disabled
            return

        # timer = QElapsedTimer()
        # timer.start()

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
        top = 40

        if parameter == "all":
            top = 10000
        else:
            skip = self.pnc.get_save_state(statename)

            if skip is None:
                print("Failed to get save state.  Will try to export contacts again.")
                return

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
                        #print(f"adding: {contact_details}")

                        if response.status_code != 201:
                            print(f"Response Code: {response.status_code} Failed to add contact: {fullname}.")
                            print(response.json())

                    rownode = rownode.nextSibling()

            childnode = childnode.nextSibling()

        if contactcount > 0:
            if self.pnc.set_save_state(statename, skip, top, contactcount) is None:
                print("Failed to set save state.  Will try to export contacts again.")

        # execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        # print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")

        return

    def draft_a_meeting(self, addresses, subject, content, datetime_start, datetime_end):
        if not self.use_graph_api:  # office 365 integration is disabled
            return

        tzone = self.get_user_timezone()

        # Endpoint to create an event (in the draft state)
        events_endpoint = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/events"

        formatted_datetime_start = datetime_start.toString("yyyy-MM-ddThh:mm:ss")
        formatted_datetime_end = datetime_end.toString("yyyy-MM-ddThh:mm:ss")

        event_details = {
            "subject": subject,
            "body": {
                "contentType": "HTML",
                "content": content,
            },
            "start": {
                "dateTime": formatted_datetime_start,
                "timeZone": tzone
            },
            "end": {
                "dateTime": formatted_datetime_end,
                "timeZone": tzone
            },
            "attendees": addresses,
            "isDraft": True,  # This ensures the meeting is drafted and not sent
            "allowNewTimeProposals": True
        }

        # Make a POST request to create the draft event
        response = requests.post(events_endpoint, headers=self.headers, json=event_details)

        msg = None
        if response.status_code != 201:
            msg = f"Response Code: {response.status_code} Failed to draft the meeting: {subject}."
            print(msg)
            print(response.json())

        return msg

    def draft_an_email(self, addresses, subject, content, attachments):
        # Graph API endpoint to send an email
        graph_api_endpoint = f'{self.GRAPH_API_ENDPOINT}/v1.0/me/messages'
        
        # Email data structure
        email_data = {
                "subject": subject,
                "body": {
                    "contentType": "HTML",
                    "content": content
                },
                "toRecipients": addresses
            }

        # Send email via Graph API
        response = requests.post(graph_api_endpoint, headers=self.headers, json=email_data)

        if response.status_code == 201:
            for attachment in attachments:
                msg = self.upload_attachment(response.json()['id'], attachment)
                if msg is not None:
                    return msg # error 
        else:
            return f"Error: {response.status_code} Failed to draft email: {email_subject}.  Server Responded: {response.json()}"

        return None

    def get_user_timezone(self):
        # Graph API endpoint to send an email
        graph_api_endpoint = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/mailboxSettings"  
            
        # Send email via Graph API
        response = requests.get(graph_api_endpoint, headers=self.headers)

        # Check response status code
        if response.status_code == 200:
            # Parse JSON response
            return response.json()["timeZone"]

        return "UTC"


    def download_project_emails(self, projectnumber, box, top, destination_folder):

        statename = projectnumber + "_" + box + "_downloadtracker"
        statefile = f"{destination_folder}/download_state.json"


        skip = self.pnc.get_save_state(statename, statefile)
        emailcount = 0

        if skip is None:
            print("Failed to get save state.  Will try to download email again.")
            return

        #print(f"Checking for new email for project {projectnumber} starting at {skip} and {top} items.")

        # Graph API get list of messages
        graph_api_endpoint = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/mailFolders/{box}/messages?$filter=contains(subject,'{projectnumber}')&$select=id&$top={top}&$skip={skip}" 
        
        # get email list via Graph API
        response = requests.get(graph_api_endpoint, headers=self.headers)

        # Check response status code
        if response.status_code == 200:
            # Parse JSON response
            data = response.json()
             # Parse JSON response
            emails = response.json()["value"]

            for email in emails:
                emailcount += 1 # don't download past the end

                # Construct MSG download URL
                msg_url = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/messages/{email['id']}"
                msg_response = requests.get(msg_url, headers=self.headers)

                if msg_response.status_code == 200:
                    # print(f'parsing response text from {msg_url}')
                    message_data = json.loads(msg_response.text)

                    #print(f"Currently at email count {emailcount}")
                    subject = message_data["subject"]
                    sent_time = message_data["sentDateTime"]

                    # if msg_response.status_code == 200:
                    eml_file = destination_folder + "/" + self.make_filename(sent_time, subject) + ".eml"
                    msg_file = destination_folder + "/" + self.make_filename(sent_time, subject) + ".msg"

                    #print(f"saving email to file: {msg_file}")

                    # only write it if it doesn't exist
                    if not QFile(msg_file).exists() and not QFile(eml_file).exists(): # don't save both versions
                        self.download_email(self.access_token, email['id'], eml_file)
                    else:
                        print(f"{eml_file} email message file already exists.")
                else:
                    print(f"Response Code: {msg_response.status_code} Error getting message details {msg_response.text}.")
        else:
            print(f"Response Code: {response.status_code} Error downloading emails {response.text}.")

        if emailcount > 0:
            if self.pnc.set_save_state(statename, skip, emailcount, emailcount, statefile) is None: # never redownload emails
                print("Failed to set save state.  Will try to download the same email again.")

    # provide a eml formated base64 string
    def format_base64_76(self, text):
        return '\n'.join(text[i:i+76] for i in range(0, len(text), 76))


    def download_email(self, graph_token, message_id, email_file):
        # Set API endpoint and headers
        graph_api_endpoint = f"{self.GRAPH_API_ENDPOINT}/v1.0/me/messages/{message_id}?$expand=attachments"
        response = requests.get(graph_api_endpoint, headers=self.headers)

        # Check if successful
        if response.status_code == 200:
            # Parse JSON response
            data = response.json()

            #print(f'Email JSON:\n{json.dumps(data, indent=4)}\nEnd Email JSON')

            has_attach = (data.get('attachments') is not None) and (len(data.get('attachments')) > 0)
            boundary = data.get('internetMessageId').replace('@','').replace('.','').replace('<','').replace('>', '').replace('-','').upper()
            msg_boundary = f'_MSG_{boundary}_'
            att_boundary = f'_ATT_{boundary}_'

            eml = ""
            val = data.get('internetMessageId', None)
            eml += f'Received: from {val}\n' if val else ''

            val = data.get('from', None)
            val = val.get('emailAddress', None) if val else None
            name = val.get('name', '') if val else ''
            address = val.get('address', None) if val else None
            eml += f'From: "{name}" <{address}>\n' if address else ""

            alist = ''
            for too in data.get('toRecipients'):
                val = too.get('emailAddress', None) if val else None
                name = val.get('name', '') if val else ''
                address = val.get('address', None) if val else None                
                alist += ", " if alist != '' else ''
                alist += f'"{name}" <{address}>' if address else ""

            eml += f'To: {alist}\n' if alist != '' else ''

            alist = ''
            for too in data.get('ccRecipients'):
                val = too.get('emailAddress', None) if val else None
                name = val.get('name', '') if val else ''
                address = val.get('address', None) if val else None                
                alist += ", " if alist != '' else ''
                alist += f'"{name}" <{address}>' if address else ""

            eml += f'CC: {alist}\n' if alist != '' else ''

            alist = ''
            for too in data.get('bccRecipients'):
                val = too.get('emailAddress', None) if val else None
                name = val.get('name', '') if val else ''
                address = val.get('address', None) if val else None                
                alist += ", " if alist != '' else ''
                alist += f'"{name}" <{address}>' if address else ""

            eml += f'BCC: {alist}\n' if alist != '' else ''

            val = data.get('subject')
            eml += f'Subject: {val}\n' if val else ''

            val = data.get('internetMessageId')
            eml += f'Message-ID: {val}\n' if val else ''

            alist = ''
            for too in data.get('replyTo'):
                val = too.get('emailAddress', None) if val else None
                name = val.get('name', '') if val else ''
                address = val.get('address', None) if val else None                
                alist += ", " if alist != '' else ''
                alist += f'"{name}" <{address}>' if address else ''

            eml += f'In-Reply-To: {alist}\n' if alist != '' else ''

            val = data.get('conversationId')
            eml += f'Thread-Index: {val}\n' if val else ''

            eml += f'Date: {QDateTime.currentDateTime().toString("ddd, dd MMM yyyy HH:mm:ss +0000")}\n'

            if has_attach:
                eml += 'X-MS-Has-Attach: yes\n'
                eml += 'Content-Type: multipart/related;\n'
                eml += f'\tboundary="{att_boundary}";\n'
                eml += '\ttype="multipart/alternative"\n'
                eml += 'MIME-Version: 1.0\n\n'

                eml += f'--{att_boundary}\n'
                eml += 'Content-Type: multipart/alternative;\n'
                eml += f'\tboundary="{msg_boundary}"\n\n'
            else:
                eml += 'X-MS-Has-Attach: no\n'
                eml += 'Content-Type: multipart/mixed;\n'
                eml += f'\tboundary="{msg_boundary}"\n'
                eml += 'MIME-Version: 1.0\n\n'

            eml += f'--{msg_boundary}\n'
            eml += 'Content-Type: text/html; charset="us-ascii"\n\n'

            val = data.get('body', None)
            content = val.get('content', '') if val else ''
            content = content.replace('’','&#8217;')
            content = content.replace('…', '&#8230;')
            content = content.replace('–', '&#8212;')
            content = content.replace('⋅','&#183;')
            content = content.replace('“', '&#8220;')
            content = content.replace('”', '&#8221;')
            content = content.replace('•', '&#183;')

            eml += content

            eml += f'\n\n--{msg_boundary}--\n\n'

            alist = ''
            for fi in data.get('attachments'):
                #print("Attachments Listed:")

                alist += f'--{att_boundary}\n'
                val = fi.get('contentType', '')
                name = fi.get('name', '')
                alist += f'Content-Type: {val}; \n\tname="{name}"\n'
                alist += f'Content-Description: {name}\n'
                
                inline = 'attachment;'
                if fi.get('isInline', False):
                    inline = "inline;"

                alist += f'Content-Disposition: {inline} filename="{name}";\n'
                size = fi.get('size')
                alist += f'\tsize={size};' if size else ''

                val = fi.get('lastModifiedDateTime', None)
                if val:
                    dt = QDateTime.fromString(val, Qt.DateFormat.ISODate)
                    alist += f' creation-date: "{dt.toString("ddd, dd MMM yyyy HH:mm:ss +0000")}";\n'
                    alist += f'\tmodification-date: "{dt.toString("ddd, dd MMM yyyy HH:mm:ss +0000")}"\n'

                val = fi.get('contentId', None)
                alist += f'Content-ID: <{val}>\n' if val else ''

                alist += 'Content-Transfer-Encoding: base64\n\n'

                alist += self.format_base64_76(fi.get('contentBytes', ''))

                alist += '\n\n'

            eml += alist

            if alist != '':
                eml += f'\n\n--{att_boundary}--\n\n'

            """
            EML from Mapping
            Received:
                internetMessageId
                #Note a lot of header information missing
            From:
                from: emailAddress:
            To:
                toRecipients: emailAddress:

            CC:
                toRecipients: emailAddress:
            Subject:
                subject
            Thread-Topic:
                ????
            Message-ID:
                internetMessageId:
            References:
                ????
            In-Reply-To:
                replyTo: emailAddress:
            Thread-Topic:
                ????
            Thread-Index:
                conversationId
            Date:
                current utc date and time
            Accept-Language: ???
            Content-Language: ???
            Content-Type: multipart/mixed;
                boundary="_005_BL0PR18MB2227DE7E1CD2FFBDF7E44498E485ABL0PR18MB2227nampPNOTES_"
            MIME-Version: 1.0
            
            --_005_BL0PR18MB2227DE7E1CD2FFBDF7E44498E485ABL0PR18MB2227nampPNOTES_
            Content-Type: text/plain; charset="us-ascii"
                contentType:
                content: ??? can we convert html??

            --_005_BL0PR18MB2227DE7E1CD2FFBDF7E44498E485ABL0PR18MB2227nampPNOTES_
            Content-Type: text/html; charset="us-ascii"

                contentType:
                content:

            --_005_BL0PR18MB2227DE7E1CD2FFBDF7E44498E485ABL0PR18MB2227nampPNOTES_
            """

            """
            File Attachments are as follows
            from json attachments array

            --_005_BL0PR18MB2227DE7E1CD2FFBDF7E44498E485ABL0PR18MB2227nampPNOTES_
            Content-Type: contentType: application/pdf; name="name:"
            Content-Description: name:
            Content-Disposition: attachment; filename="name:";
                size=size:; creation-date=lastModifiedDateTime: but use this format??"Thu, 08 Jan 2026 14:49:29 GMT";
                modification-date=lastModifiedDateTime: "Thu, 08 Jan 2026 14:49:39 GMT"
            Content-Transfer-Encoding: base64


            --_005_BL0PR18MB2227DE7E1CD2FFBDF7E44498E485ABL0PR18MB2227nampPNOTES_
            """


            # Save MSG file
            file = QFile(email_file)
            if file.open(QIODevice.OpenModeFlag.WriteOnly | QIODevice.OpenModeFlag.Text):
                stream = QTextStream(file)
                stream.setEncoding(QStringConverter.Encoding.Utf8)  # Set the codec to UTF-8
                stream << eml
                file.close()
            else:
                print(f"Failed to write file {msg_file}.")

            return True
        else:
            print(f"Error: {response.status_code} {response.text}")
            return False

    def download_batch_of_emails(self, xmlstr):
        # timer = QElapsedTimer()
        # timer.start()

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
        statename = "project_emails_export"

        skip = 0
        top = 5 # writing a bunch of email files could be slow only do 5 projects at a time
        top_emails = 20 # however download up to 20 emails from each box at a time

        if xmlstr is not None:
            top = 10000 # downloading all email for a project

        skip = self.pnc.get_save_state(statename)

        if skip is None:
            print("Failed to get save state.  Will try to download emails again.")
            return

        projectfilter = ""

        if xmlstr is not None:
            pxmlval = QDomDocument()
            if (pxmlval.setContent(xmlstr) == False):
                print("Unable to parse XML returned from Project Notes in emails download.")
                return

            pxmlroot = pxmlval.documentElement()

            projectnumber = self.pnc.scrape_project_number(pxmlroot)
            print(f"Downloading all emails for project {projectnumber}")

            projectfilter = f' filter_field_2="project_number" filter_value_2="{projectnumber}" '

        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table  filter_field_1="location_description" filter_value_1="Project Folder" {projectfilter} name="project_locations" {self.pnc.state_range_attrib(top, skip)} />\n</projectnotes>\n'
        #TODO: This gets projects that are closed too.  However, it won't write if folder doesn't exist.
        xmlresult = projectnotes.get_data(xmldoc)

        #print(xmlresult)
        
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
                        sentfolder = projectfolder + "/" + self.backup_sent_folder
                        receivedfolder = projectfolder + "/" + self.backup_inbox_folder

                        if not QDir(sentfolder).exists():
                            QDir().mkpath(sentfolder)

                        if not QDir(receivedfolder).exists():
                            QDir().mkpath(receivedfolder)

                        self.download_project_emails(projectnumber, "inbox", top_emails, receivedfolder)
                        self.download_project_emails(projectnumber, "sentitems", top_emails, sentfolder)

                    rownode = rownode.nextSibling()

            childnode = childnode.nextSibling()

        if locationcount > 0:
            if self.pnc.set_save_state(statename, skip, top, locationcount) is None:
                print("Failed to set save state.  Will try to import download the same email again.")

        # execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        # print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")

        return

    def sync_tracker_to_tasks(self, parameter):
        if not self.use_graph_api:  # office 365 integration is disabled
            return

        # timer = QElapsedTimer()
        # timer.start()

        saved_state = None
        statename = "tracker_export"

        skip = 0
        top = 20

        # if syncing all items
        if parameter == "all":
            top = 10000
        else:
            skip = self.pnc.get_save_state(statename)

        if skip is None:
            print("Failed to get save state.  Will try to tracker items again.")
            return

        # just get the project manager people id
        xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table name="clients" top="1" skip="0"/>\n</projectnotes>\n'

        # timer2 = QElapsedTimer()
        # timer2.start()
        
        xmlresult = projectnotes.get_data(xmldoc)

        # execution_time = timer2.elapsed() / 1000  # Convert milliseconds to seconds
        # print(f"Function get_data in '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")

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

        # timer2 = QElapsedTimer()
        # timer2.start()

        xmlresult = projectnotes.get_data(xmldoc)

        # execution_time = timer2.elapsed() / 1000  # Convert milliseconds to seconds
        # print(f"Function get_data in '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")

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
                    createddate = QDateTime()
                    updateddate = QDateTime()
                    status = None
                    priority = None
                    comments = None
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

                        if colnode.attributes().namedItem("name").nodeValue() == "assigned_to":
                            assignedto = content.strip()

                        if colnode.attributes().namedItem("name").nodeValue() == "priority":
                            priority = "normal"
                            if content == "High":
                                priority = "high"
                            if content == "Low":
                                priority = "low"

                        colnode = colnode.nextSibling()


                    # look for to-do task
                    prefix = f"{projectnumber}-{itemnumber}"


                    task_id = None
                    old_item = None
                    for lst in todos:
                        #print(f'looking through todos: {lst["title"]}')
                        if lst["title"].startswith(prefix):
                            task_id = lst["id"]
                            
                            old_item = {
                                "title": lst["title"],
                                "importance" : lst["importance"],
                                "body" : 
                                {
                                    "content" : lst["body"]["content"],
                                    "contentType" : "text",
                                }
                            }
                            break


                    if ((status == "New" or status == "Assigned") and assignedto == pmid) and ( (datedue.isValid() and self.sync_todo_with_due) or (not datedue.isValid() and self.sync_todo_without_due)):
                        #TODO: need an option to set time of day for alarms
                        #TODO: only changes to body text or description trigger an update Reminder date changes don't

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

                        jsonisdiff = (not (old_item == task_data))

                        if datedue  is not None and datedue.isValid():
                            task_data["isReminderOn"] = True
                            task_data["dueDateTime"] = { "dateTime" : datedue.toUTC().toString("yyyy-MM-ddThh:mm:ss.ss"), "timeZone" : "UTC" }
                            task_data["reminderDateTime"] = { "dateTime" : datedue.toUTC().toString("yyyy-MM-ddThh:mm:ss.ss"), "timeZone" : "UTC" }

                        if createddate is not None and createddate.isValid():
                            task_data["createdDateTime"] = createddate.toUTC().toString("yyyy-MM-ddThh:mm:ss.ssZ")

                        if updateddate  is not None and  updateddate.isValid():
                            task_data["lastModifiedDateTime"] = updateddate.toUTC().toString("yyyy-MM-ddThh:mm:ssZ")
                        else:
                            task_data["lastModifiedDateTime"] = QDateTime.currentDateTime().toUTC().toString("yyyy-MM-ddThh:mm:ssZ")

                        response = None
                        if task_id is None:
                            response = requests.post(url, headers=self.headers, json=task_data)

                            if response.status_code != 201:
                                print(f"\nFailed to add task '{prefix} {projectname} {itemname}': {response.status_code}\n\n {response.text}\n\n")

                        elif jsonisdiff: # don't update it everytiome
                            # print(task_data)
                            # print(old_item)

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

        if rowcount > 0:
            if self.pnc.set_save_state(statename, skip, top, rowcount) is None:
                print("Failed to set save state.  Will try to import download the same email again.")

        # execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        # print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")

        return


#TODO: VER 4.1 should i export contacts when using o365 integration??? maybe just as a manual export
