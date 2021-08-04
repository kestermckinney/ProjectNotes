from includes.common import ProjectNotesCommon
from includes.excel_tools import ProjectNotesExcelTools

from PySide6 import QtSql, QtGui, QtCore, QtUiTools
from PySide6.QtSql import QSqlDatabase
from PySide6.QtXml import QDomDocument, QDomNode
from PySide6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PySide6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PySide6.QtGui import QDesktopServices
import sys
import win32com

# Project Notes Plugin Parameters
pluginname = "Export Outlook Contact(s)"
plugindescription = "Import Outlook contacts and assocated companies."

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
PluginMenuClick="wxxmldocument:ix_people"
RightClickProject="disabled"
RightClickPeople="wxxmldocument:ix_people"
RightClickClient="disabled"
RightClickStatusReportItem="disabled"
RightClickLocationItem="disabled"
RightClickTeamMember="disabled"
RightClickMeeting="disabled"
RightClickAttendee="disabled"
RightCickTrackerItem="disabled"

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any functions

# Project Notes Parameters
parameters = {
}

OracleUsername = ""
ProjectsFolder = ""

pnc = ProjectNotesCommon()
pne = ProjectNotesExcelTools()

def find_contact( outlook, fullname ):
    mapi  = outlook.GetNamespace("MAPI")
    contactsfold = mapi.GetDefaultFolder(10) # olFolderContacts

    cont_enum = contactsfold.Items
    for contact in cont_enum:
        try:
            if contact.FullName.strip().upper() == fullname.strip().upper():
                mapi = None
                contactsfold = None
                cont_enum = None
                print("found: " + contact.FullName)
                return contact
        except:
            print("Group Name Found")

    mapi = None
    contactsfold = None
    cont_enum = None

    return None

# processing main function
def main_process( xmlval ):
    outlook = win32com.client.Dispatch("Outlook.Application")
    mapi = outlook.GetNamespace("MAPI")
    contactsfold = mapi.GetDefaultFolder(10) # olFolderContacts


    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
    childnode = xmlroot.firstChild()

    while not childnode.isNull():
        if childnode.attributes().namedItem("name").nodeValue() == "ix_people":
            rownode = childnode.firstChild()

            while not rownode.isNull():
                colnode = rownode.firstChild()

                fullname = None
                company = None
                workphone = None
                workemail = None
                cellphone = None
                jobtitle = None

                while not colnode.isNull():
                    #textnode = colnode.firstChild()
                    content = colnode.toElement().text()
                    #print(content)

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

                #print(fullname)

                searchname = find_contact(outlook, fullname)

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

    return xmldoc

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

# setup test data
"""
print("Buld up QDomDocument")
app = QApplication(sys.argv)


xmldoc = QDomDocument("TestDocument")
f = QFile("people.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

print("Run Test")
# call when testing outside of Project Notes
main_process(xmldoc)
"""
