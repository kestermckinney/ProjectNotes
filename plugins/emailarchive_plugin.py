from includes.common import ProjectNotesCommon
from includes.excel_tools import ProjectNotesExcelTools

from PySide6 import QtSql, QtGui, QtCore, QtUiTools
from PySide6.QtSql import QSqlDatabase
from PySide6.QtXml import QDomDocument, QDomNode
from PySide6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir
from PySide6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PySide6.QtGui import QDesktopServices
import sys
import win32com
import re
import os

pnc = ProjectNotesCommon()

# Project Notes Plugin Parameters
pluginname = "Archive Project Email"
plugindescription = "Archive all of the email related to the project into the corresponding project folder."

# events must have a data structure and data view specified
#
# Structures:
#      disabled        The event will not be enabled
#      wxxmldocument   The event will pass a wxLua wx.wxXmlDocument containing the spedified data view and expect the plugin to return a wx.wxXmlDocument
#      string          The event will pass a wxLua string containing XML and will expect the plugin to return an XML string
#      nodata          The event will pass a wxLua None and will expect the plugin to return an XML string
#
# all tables in the database have corresponding import/export data views the views are prefixed by ix_
#
# Data Views:
#      ix_clients
#      ix_people
#      ix_projects
#      ix_project_people
#      ix_status_report_items
#      ix_project_locations
#      ix_project_notes
#      ix_meeting_attendees
#      ix_item_tracker_updates
#      ix_item_tracker

# Active Events
Startup="disabled"
Shutdown="disabled"
EveryMinute="disabled"
Every5Minutes="disabled"
Every10Minutes="disabled"
Every30Minutes="disabled"
PluginMenuClick="nodata"
RightClickProject="wxxmldocument:ix_projects"
RightClickPeople="disabled"
RightClickClient="disabled"
RightClickStatusReportItem="disabled"
RightClickLocationItem="disabled"
RightClickTeamMember="disabled"
RightClickMeeting="disabled"
RightClickAttendee="disabled"
RightCickTrackerItem="disabled"

# Project Notes Parameters
parameters = {
}

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

def makefilename(datetime, subject):
    id = re.sub(r"[-`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[ ]", "", datetime)
    #id = datetime.toString("yyyy") + datetime.toString("MM") + datetime.toString("DD") + datetime.toString("hh") + datetime.toString("mm") + datetime.toString("ss")
    cleanname = id + "-" + pnc.valid_filename(subject)
    return cleanname[:100]

# processing main def
def main_process( xmlval ):
    if not pnc.verify_global_settings():
        return None

    answer = QMessageBox.question(None,
        "WARNING: Long Process",
        "WARNING: This process can take some time.  Are you sure you want to continue?",
        QMessageBox.Yes,
        QMessageBox.No)

    if (answer != QMessageBox.Yes):
        return None

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
    projectfolder = pnc.get_projectfolder(xmlroot)

    projtab = pnc.find_node(xmlroot, "table", "name", "ix_projects")
    projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
    projdes = pnc.get_column_value(projtab.firstChild(), "project_name")

    if (projectfolder is None or projectfolder ==""):
        projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QtCore.QDir.home().path())

        if projectfolder == "" or projectfolder is None:
            return(None)

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
    return None

# Project Notes Plugin Events
def event_startup(xmlstr):
    return main_process(xmlstr)

def event_shutdown(xmlstr):
    return main_process(xmlstr)

def event_everyminute(xmlstr):
    return main_process(xmlstr)

def event_every5minutes(xmlstr):
    return main_process(xmlstr)

def event_every10minutes(xmlstr):
    return main_process(xmlstr)

def event_every30Mmnutes(xmlstr):
    return main_process(xmlstr)

def event_menuclick(xmlstr):
    return main_process(xmlstr)

def event_projectrightclick(xmlstr):
    return main_process(xmlstr)

def event_peoplerightclick(xmlstr):
    return main_process(xmlstr)

def event_clientrightclick(xmlstr):
    return main_process(xmlstr)

def event_statusreportitemrightclick(xmlstr):
    return main_process(xmlstr)

def event_teammemberrightclick(xmlstr):
    return main_process(xmlstr)

def event_locationitemrightclick(xmlstr):
    return main_process(xmlstr)

def event_meetingrightclick(xmlstr):
    return main_process(xmlstr)

def event_attendeerightclick(xmlstr):
    return main_process(xmlstr)

def event_trackeritemrightclick(xmlstr):
    return main_process(xmlstr)
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
