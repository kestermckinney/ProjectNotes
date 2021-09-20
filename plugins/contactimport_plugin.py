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
pluginname = "Import Outlook Contacts"
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
PluginMenuClick="nodata"
RightClickProject="disabled"
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
# Project Notes will set these values before calling any functions

pnc = ProjectNotesCommon()

# processing main def
def main_process( xmlval ):
    outlook = win32com.client.Dispatch("Outlook.Application")
    mapi = outlook.GetNamespace("MAPI")
    contactsfold = mapi.GetDefaultFolder(10) # olFolderContacts

    xmlclients = ""
    xmldoc = ""

    cont_enum = contactsfold.Items
    for contact in cont_enum:
        # olContactItem
        #try:

        if (contact.FullName is not None and contact.FullName != ""):
            print(contact.FullName)
            xmldoc = xmldoc + "<row>\n"

            xmldoc = xmldoc + "<column name=\"name\" number=\"1\">" + pnc.to_xml(contact.FullName) + "</column>\n"

            if contact.Email1Address is not None:
                xmldoc = xmldoc + "<column name=\"email\" number=\"2\">" + pnc.to_xml(contact.Email1Address) + "</column>\n"

            if contact.BusinessTelephoneNumber is not None:
                xmldoc = xmldoc + "<column name=\"office_phone\" number=\"3\">" + pnc.to_xml(contact.BusinessTelephoneNumber) + "</column>\n"

            if contact.MobileTelephoneNumber is not None:
                xmldoc = xmldoc + "<column name=\"cell_phone\" number=\"4\">" + pnc.to_xml(contact.MobileTelephoneNumber) + "</column>\n"

            if contact.JobTitle is not None:
                xmldoc = xmldoc + "<column name=\"role\" number=\"4\">" + pnc.to_xml(contact.JobTitle) + "</column>\n"

            # add the company name as a sub tablenode
            if (contact.CompanyName is not None and contact.CompanyName != ''):
                xmldoc = xmldoc + "<column name=\"client_id\" number=\"5\" lookupvalue=\"" + pnc.to_xml(contact.CompanyName) + "\"></column>\n"
                xmlclients = xmlclients + "<row><column name=\"client_name\" number=\"1\">" + pnc.to_xml(contact.CompanyName) + "</column></row>\n"

            xmldoc = xmldoc + "</row>\n"
        #except:
        #    print("Group Name Found")

    xmldoc = """
    <?xml version="1.0" encoding="UTF-8"?>
    <projectnotes>
    <table name="ix_clients">
    """ + xmlclients + """
    </table>
    <table name="ix_people">
    """ + xmldoc + """
    </table>
    </projectnotes>
    """

    outlook = None
    mapi = None
    contactsfold = None
    cont_enum = None

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

#print("Run Test")
# call when testing outside of Project Notes
#print(main_process(None))
