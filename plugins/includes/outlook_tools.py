from win32com.client import GetObject
import win32com
import sys

from PyQt6 import QtSql, QtGui, QtCore, QtWidgets
from PyQt6.QtCore import QDirIterator, QDir, QSettings, QFile
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication
from includes.common import ProjectNotesCommon

top_windows = []

def windowEnumerationHandler(hwnd, topwindows):
    topwindows.append((hwnd, win32gui.GetWindowText(hwnd)))

class ProjectNotesOutlookTools:
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()

    def bring_window_to_front(self, title):
        win32gui.EnumWindows(windowEnumerationHandler, top_windows)

        for i in top_windows:
            if title.lower() in i[1].lower():
                win32gui.ShowWindow(i[0],5)
                win32gui.SetForegroundWindow(i[0])
                break
        return


    def find_contact(self, list, fullname ):
        for contact in list:
            if contact[1] == fullname.strip().upper():
                return contact[0]

    # processing main function
    def export_contacts(self, xmlstr):
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""

        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()

        progbar = QProgressDialog()
        progbar.setWindowTitle("Exporting...")
        progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint 
            )
        progbar.setMinimumWidth(350)
        progbar.setCancelButton(None)
        progbar.show()


        outlook = win32com.client.Dispatch("Outlook.Application")
        mapi = outlook.GetNamespace("MAPI")
        contactsfold = mapi.GetDefaultFolder(10) # olFolderContacts

        # load all contacts into memory
        cont_enum = contactsfold.Items
        contactlist = []
        for contact in cont_enum:
            if hasattr(contact, "FullName"):
                cols = []
                cols.append(contact)
                cols.append(contact.FullName.strip().upper())
                contactlist.append(cols)

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node

        childnode = xmlroot.firstChild()
        tot_contacts = childnode.childNodes().count()

        cur_contacts = 0

        while not childnode.isNull():

            if childnode.attributes().namedItem("name").nodeValue() == "people":
                rownode = childnode.firstChild()

                while not rownode.isNull():
                    cur_contacts = cur_contacts + 1

                    progbar.setValue(int(cur_contacts / tot_contacts * 100))
                    progbar.setLabelText("Exporting Contacts...")
                    QtWidgets.QApplication.processEvents()

                    colnode = rownode.firstChild()

                    fullname = None
                    company = None
                    workphone = None
                    workemail = None
                    cellphone = None
                    jobtitle = None

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

                    #print("Exporting ..." + fullname)

                    #searchname = find_contact(outlook, fullname, mapi, contactsfold)
                    searchname = self.find_contact(contactlist, fullname)

                    if searchname == None:
                        searchname = contactsfold.Items.Add()

                    searchname.FullName = fullname
                    searchname.CompanyName = company
                    searchname.BusinessTelephoneNumber = workphone
                    searchname.MobileTelephoneNumber = cellphone
                    searchname.Email1Address = workemail
                    searchname.JobTitle = jobtitle
                    searchname.Save()

                    rownode = rownode.nextSibling()

            childnode = childnode.nextSibling()

        outlook = None
        mapi = None
        contactsfold = None

        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed

        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()  

        return ""

    def import_contacts(self, xmlstr):
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""

        outlook = win32com.client.Dispatch("Outlook.Application")
        mapi = outlook.GetNamespace("MAPI")
        contactsfold = mapi.GetDefaultFolder(10) # olFolderContacts

        xmlclients = ""
        xmldoc = ""

        #print("count of contacts : " + str(contactsfold.Items.Count))

        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()

        tot_contacts = contactsfold.Items.Count
        cur_contacts = 0

        progbar = QProgressDialog()
        progbar.setWindowTitle("Importing...")
        progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint 
            )
        progbar.setMinimumWidth(350)
        progbar.setCancelButton(None)
        progbar.show()


        cont_enum = contactsfold.Items
        for contact in cont_enum:
            cur_contacts = cur_contacts + 1

            progbar.setValue(int(cur_contacts / tot_contacts * 100))
            progbar.setLabelText("Importing Contacts...")
            QtWidgets.QApplication.processEvents()

            # olContactItem
            if contact is not None:
                if hasattr(contact, "FullName"):
                    #print("importing ... " + contact.FullName)
                    xmldoc = xmldoc + "<row>\n"

                    xmldoc = xmldoc + "<column name=\"name\">" + self.pnc.to_xml(contact.FullName.strip()) + "</column>\n"

                    if contact.Email1Address is not None:
                        # make sure the email address looks valid
                        if "@" in contact.Email1Address: 
                            xmldoc = xmldoc + "<column name=\"email\">" + self.pnc.to_xml(contact.Email1Address.strip()) + "</column>\n"
                        else:
                            print("email address is corrupt for " + contact.FullName)

                    if contact.BusinessTelephoneNumber is not None:
                        xmldoc = xmldoc + "<column name=\"office_phone\">" + self.pnc.to_xml(contact.BusinessTelephoneNumber.strip()) + "</column>\n"

                    if contact.MobileTelephoneNumber is not None:
                        xmldoc = xmldoc + "<column name=\"cell_phone\">" + self.pnc.to_xml(contact.MobileTelephoneNumber.strip()) + "</column>\n"

                    if contact.JobTitle is not None:
                        xmldoc = xmldoc + "<column name=\"role\">" + self.pnc.to_xml(contact.JobTitle.strip()) + "</column>\n"

                    # add the company name as a sub tablenode
                    if (contact.CompanyName is not None and contact.CompanyName != ''):
                        xmldoc = xmldoc + "<column name=\"client_id\" number=\"5\" lookupvalue=\"" + self.pnc.to_xml(contact.CompanyName.strip()) + "\"></column>\n"
                        xmlclients = xmlclients + "<row><column name=\"client_name\">" + self.pnc.to_xml(contact.CompanyName.strip()) + "</column></row>\n"

                    xmldoc = xmldoc + "</row>\n"
            else:
                print("Corrupt Contact Record Found")

        xmldoc = """
        <projectnotes>
        <table name="clients">
        """ + xmlclients + """
        </table>
        <table name="people">
        """ + xmldoc + """
        </table>
        </projectnotes>
        """

        outlook = None
        mapi = None
        contactsfold = None
        cont_enum = None

        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed

        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()   

        return xmldoc

    def makefilename(self, datetime, subject):
        id = re.sub(r"[-`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[ ]", "", datetime)
        cleanname = id + "-" + self.pnc.valid_filename(subject)
        # there is no guarantee this length will work.  It depends on system settigns and the base path length
        return cleanname[:70]

    def export_email(self, xmlstr):
        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()   

        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
            QtWidgets.QApplication.processEvents()
            return ""

        if not self.pnc.verify_global_settings():
            QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
            QtWidgets.QApplication.processEvents()
            return ""

        answer = QMessageBox.question(None,
            "WARNING: Long Process",
            "WARNING: This process can take some time.  Are you sure you want to continue?",
            QMessageBox.StandardButton.Yes,
            QMessageBox.StandardButton.No)

        if (answer != QMessageBox.StandardButton.Yes):
            QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
            QtWidgets.QApplication.processEvents()
            return ""

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = self.pnc.get_projectfolder(xmlroot)
        print("project folder " + projectfolder)

        projtab = self.pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = self.pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = self.pnc.get_column_value(projtab.firstChild(), "project_name")

        if (projectfolder is None or projectfolder ==""):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""

        progbar = QProgressDialog()
        progbar.setValue(0)
        progbar.setLabelText("Archiving project emails...")
        progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint 
            )
        progbar.setMinimumWidth(350)
        progbar.setCancelButton(None)
        progbar.show()

        progval = 0

        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()


        sentfolder = projectfolder + "\\Correspondence\\Sent Email\\"
        receivedfolder = projectfolder + "\\Correspondence\\Received Email\\"

        if not QDir(sentfolder).exists():
            os.mkdir(sentfolder)

        if not QDir(receivedfolder).exists():
            os.mkdir(receivedfolder)

        outlook = win32com.client.Dispatch("Outlook.Application")
        mapi = outlook.GetNamespace("MAPI")
        #mailfold = mapi.Folders.GetFirst()
        inbox = mapi.GetDefaultFolder(6)
        sent = mapi.GetDefaultFolder(5)

        progtot = inbox.Items.Count + sent.Items.Count
        for message in inbox.Items:
            progval = progval + 1
            progbar.setValue(int(min(progval / progtot * 100, 100)))
            progbar.setLabelText("Parsing Inbox items...")
            QtWidgets.QApplication.processEvents()
            #print("looking in inbox for project " + projnum)

            if message.Subject.find(projnum) >= 0:
                if hasattr(message, "SentOn"):
                    filename = receivedfolder + makefilename(str(message.SentOn), message.Subject) + ".msg"

                    #print (filename + "\n")

                    if not QFile.exists(filename):
                        message.SaveAs(filename, 3)
                else:
                    print("Message has no sent date: " + message.Subject)

        for message in sent.Items:
            progval = progval + 1
            progbar.setValue(int(min(progval / progtot * 100, 100)))
            progbar.setLabelText("Parsing Sent items...")
            QtWidgets.QApplication.processEvents()

            if message.Subject.find(projnum) >= 0:
                filename = sentfolder + makefilename(str(message.SentOn), message.Subject) + ".msg"

                if not QFile.exists(filename):
                    message.SaveAs(filename, 3)

        mail_enum = None
        message = None

        outlook = None
        mapi = None
        contactsfold = None
        cont_enum = None

        progbar.setValue(100)
        progbar.setLabelText("Complete ...")
        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed
        return ""

    def schedule_meeting(self, addresses, subject, body):
        outlook = win32com.client.Dispatch("Outlook.Application")
        message = outlook.CreateItem(1)

        for address in addresses:
            message.Recipients.Add(address["emailAddress"]["email"])

        message.Subject = subject

        message.MeetingStatus = 1
        message.Duration = 60
        message.Location = self.pnc.get_plugin_setting("DefaultMeetingLocation")
        message.Body = body
        outlook.ActiveExplorer().Activate()
        message.Display()

        outlook = None
        message = None

        self.bring_window_to_front(subject)

        return None

    def send_email(self, addresses, subject, content, attachments):
        outlook = win32com.client.Dispatch("Outlook.Application")

        message = outlook.CreateItem(0)
        message.To = ""

        for address in addresses:
            message.Recipients.Add(address["emailAddress"]["email"])

        message.Display()
        outlook.ActiveExplorer().Activate()

        DefaultSignature = message.HTMLBody

        message.Subject = subject
        message.HTMLBody = content + DefaultSignature

        for attachment in attachments:
            message.Attachments.Add(attachment, 1)

        outlook = None
        message = None

        self.bring_window_to_front(subject)

        return None
