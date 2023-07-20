
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt5 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt5.QtSql import QSqlDatabase
from PyQt5.QtXml import QDomDocument, QDomNode
from PyQt5.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt5.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt5.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Archive Project Email"
plugindescription = "Archive all of the email related to the project into the corresponding project folder."
plugintable = "" # the table or view that the plugin applies to.  This will enable the right click
childtablesfilter = "" # a list of child tables that can be sent to the plugin.  This will be used to exclude items like notes or action items when they aren't used

# events must have a data structure and data view specified
#
# Structures:
#      string          The event will pass a python string containing XML and will expect the plugin to return an XML string
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

# Supported Events

# def event_startup(xmlstr):
#     return ""
#
# def event_shutdown(xmlstr):
#     return ""
#
# def event_everyminute(xmlstr):
#     return ""
#
# def event_every5minutes(xmlstr):
#     return ""
#
# def event_every10minutes(xmlstr):
#     return ""
#
# def event_every30Mmnutes(xmlstr):
#     return ""
#
# def event_menuclick(xmlstr):
#     return ""

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# Project Notes Parameters
parameters = [
]

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs


# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pnc = ProjectNotesCommon()

    def makefilename(datetime, subject):
        id = re.sub(r"[-`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[ ]", "", datetime)
        #id = datetime.toString("yyyy") + datetime.toString("MM") + datetime.toString("DD") + datetime.toString("hh") + datetime.toString("mm") + datetime.toString("ss")
        cleanname = id + "-" + pnc.valid_filename(subject)
        return cleanname[:100]

    def event_data_rightclick(xmlstr):
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)
            return ""

        if not pnc.verify_global_settings():
            return ""

        answer = QMessageBox.question(None,
            "WARNING: Long Process",
            "WARNING: This process can take some time.  Are you sure you want to continue?",
            QMessageBox.Yes,
            QMessageBox.No)

        if (answer != QMessageBox.Yes):
            return ""

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = pnc.get_projectfolder(xmlroot)

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = pnc.get_column_value(projtab.firstChild(), "project_name")

        if (projectfolder is None or projectfolder ==""):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QtCore.QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""

        progbar = QProgressDialog()
        progbar.setCancelButton(None)
        progbar.show()
        progval = 0
        progbar.setValue(0)
        progbar.setLabelText("Archiving project emails...")

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
            progbar.setValue(min(progval / progtot * 100, 100))
            progbar.setLabelText("Parsing Inbox items...")

            if message.Subject.find(projnum) >= 0:
                try:
                    filename = receivedfolder + makefilename(str(message.SentOn), message.Subject) + ".msg"

                    print (filename + "\n")

                    if not QFile.exists(filename):
                        message.SaveAs(filename, 3)
                except:
                    print("Not an email record")

        for message in sent.Items:
            progval = progval + 1
            progbar.setValue(min(progval / progtot * 100, 100))
            progbar.setLabelText("Parsing Sent items...")

            if message.Subject.find(projnum) >= 0:
                try:
                    filename = sentfolder + makefilename(str(message.SentOn), message.Subject) + ".msg"

                    print (filename + "\n")

                    if not QFile.exists(filename):
                        message.SaveAs(filename, 3)
                except:
                    print("Not an email record")


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

"""
# setup test data
print("Buld up QDomDocument")
app = QApplication(sys.argv)


xmldoc = QDomDocument("TestDocument")
f = QFile("exampleproject.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

print("Run Test")
# call when testing outside of Project Notes
main_process(xmldoc)
"""
