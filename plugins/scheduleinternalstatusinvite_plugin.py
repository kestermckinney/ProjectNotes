from includes.common import ProjectNotesCommon
from includes.excel_tools import ProjectNotesExcelTools

from PySide6 import QtSql, QtGui, QtCore, QtUiTools
from PySide6.QtSql import QSqlDatabase
from PySide6.QtXml import QDomDocument, QDomNode
from PySide6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo
from PySide6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog, QInputDialog
from PySide6.QtGui import QDesktopServices
import sys
import win32com

# Project Notes Plugin Parameters
pluginname = "Schedule Internal Status"
plugindescription = "Using Outlook create an invite to the internal status meeting."

# events must have a data structure and data view specified
#
# Structures:
#      disabled        The event will not be enabled
#      wxxmldocument   The event will pass a wxLua wx.wxXmlDocument containing the spedified data view and expect the plugin to return a wx.wxXmlDocument
#      string          The event will pass a wxLua string containing XML and will expect the plugin to return an XML string
#      nodata          The event will pass a wxLua nil and will expect the plugin to return an XML string
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
PluginMenuClick="disabled"
RightClickProject="wxxmldocument:ix_projects"
RightClickPeople="disabled"
RightClickClient="disabled"
RightClickStatusReportItem="disabled"
RightClickLocationItem="disabled"
RightClickTeamMember="disabled"
RightClickMeeting="wxxmldocument:ix_project_notes"
RightClickAttendee="disabled"
RightCickTrackerItem="disabled"

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any functions

# Project Notes Parameters
parameters = {
}

pnc = ProjectNotesCommon()
pne = ProjectNotesExcelTools()

# processing main function
def main_process_project( xmlval ):
    outlook = win32com.client.Dispatch("Outlook.Application")
    message = outlook.CreateItem(1)

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
    pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()
    co = xmlroot.attributes().namedItem("managing_company_name").nodeValue()

    email = None
    nm = None
    pco = None

    teammember = pnc.find_node(xmlroot, "table", "name", "ix_project_people")
    if teammember:
        memberrow = teammember.firstChild()

        while not memberrow.isNull():
            nm = pnc.get_column_value(memberrow, "name")
            email = pnc.get_column_value(memberrow, "email")
            pco = pnc.get_column_value(memberrow, "client_name")
            if nm != pm:
                if (email is not None and email != "" and pco == co):
                    message.Recipients.Add(email)

            memberrow = memberrow.nextSibling()

    project = pnc.find_node(xmlroot, "table", "name", "ix_projects")
    if project:
        projectrow = project.firstChild()

        if not projectrow.isNull():
            message.Subject = pnc.get_column_value(projectrow, "project_number") + " " + pnc.get_column_value(projectrow, "project_name") + " - Internal Status"

    txt = get_text_invite()
    message.MeetingStatus = 1
    message.Duration = 60
    message.Location = pnc.get_global_setting("DefaultMeetingLocation")
    message.Body = txt
    outlook.ActiveExplorer().Activate()
    message.Display()

    outlook = None
    message = None

    return xmldoc

def main_process_meeting( xmlval ):
    outlook = win32com.client.Dispatch("Outlook.Application")
    message = outlook.CreateItem(1)

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
    pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()
    co = xmlroot.attributes().namedItem("managing_company_name").nodeValue()

    attendeetable = pnc.find_node(xmlroot, "table", "name", "ix_meeting_attendees")

    if attendeetable:
        attendeerow = attendeetable.firstChild()

    email = None
    nm = None

    while not attendeerow.isNull():
        nm = pnc.get_column_value(attendeerow, "name")
        email = pnc.get_column_value(attendeerow, "email")

        if nm != pm:
            if (not email is None and email != ""):
                message.Recipients.Add(email)

        attendeerow = attendeerow.nextSibling()

    project = pnc.find_node(xmlroot, "table", "name", "ix_project_notes")
    if project:
        projectrow = project.firstChild()

    if not projectrow.isNull():
        message.Subject = pnc.get_column_value(projectrow, "project_id") + " " + pnc.get_column_value(projectrow, "project_id_name") + " - Internal Status"

    txt = get_text_invite()
    message.MeetingStatus = 1
    message.Duration = 60
    message.Location = pnc.get_global_setting("DefaultMeetingLocation")
    message.Body = txt
    message.Display()

    outlook = None
    message = None

    return xmldoc

# Project Notes Plugin Events
def event_startup(xmlstr):
    return None

def event_shutdown(xmlstr):
    return None

def event_everyminute(xmlstr):
    return None

def event_every5minutes(xmlstr):
    return None

def event_every10minutes(xmlstr):
    return None

def event_every30Mmnutes(xmlstr):
    return None

def event_menuclick(xmlstr):
    return None

def event_projectrightclick(xmlstr):
    return main_process_project(xmlstr)

def event_peoplerightclick(xmlstr):
    return None

def event_clientrightclick(xmlstr):
    return None

def event_statusreportitemrightclick(xmlstr):
    return None

def event_teammemberrightclick(xmlstr):
    return None

def event_locationitemrightclick(xmlstr):
    return None

def event_meetingrightclick(xmlstr):
    return main_process_meeting(xmlstr)

def event_attendeerightclick(xmlstr):
    return None

def event_trackeritemrightclick(xmlstr):
    return None

def get_text_invite():
    txtdoc = """Internal Status Meeting Agenda
The purpose of this meeting is to make sure all work is progressing, everyone understands their assignments, and to identify any roadblocks that are holding up work or could hold up work.

     Review Action Items From Prior Meeting
     Provide Updates On Assigned Tasks
     Identify Any Roadblocks
     Review Tasks To Start
    """

    return txtdoc
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

#main_process_project(xmldoc)
main_process_meeting(xmldoc)
"""
