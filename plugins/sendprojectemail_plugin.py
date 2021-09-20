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
pluginname = "Send Project Email"
plugindescription = "Using Outlook create an a new email with project information"

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
PluginMenuClick="disabled"
RightClickProject="wxxmldocument:ix_projects"
RightClickPeople="disabled"
RightClickClient="disabled"
RightClickStatusReportItem="disabled:ix_status_report_items"
RightClickLocationItem="disabled"
RightClickTeamMember="wxxmldocument:ix_project_people"
RightClickMeeting="wxxmldocument:ix_project_notes"
RightClickAttendee="wxxmldocument:ix_meeting_attendees"
RightCickTrackerItem="disabled"

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# Project Notes Parameters
parameters = {
}

pnc = ProjectNotesCommon()
pne = ProjectNotesExcelTools()

# processing main def
def main_process( xmlval ):
    outlook = win32com.client.Dispatch("Outlook.Application")

    message = outlook.CreateItem(0)
    message.To = ""

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
    projectnumber = ""
    projectname = ""

    if xmlroot:
        col = pnc.find_node(xmlroot, "column", "name", "project_number")
        if col:
            lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
            #contents = col.GetChildren():GetContent()
            contents = col.toElement().text()

            if lookupvalue != "" and lookupvalue is not None:
                projectnumber = lookupvalue
            else:
                projectnumber = contents



        col = pnc.find_node(xmlroot, "column", "name", "project_name")
        if col:
            lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
            #contents = col.GetChildren():GetContent()
            contents = col.toElement().text()

            if lookupvalue != "" and lookupvalue is not None:
                projectname = lookupvalue
            else:
                projectname = contents

        if projectname == "" or projectname is None:
            col = pnc.find_node(xmlroot, "column", "name", "project_id_name")
            if col:
                lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
                #contents = col.GetChildren():GetContent()
                contents = col.toElement().text()

            if lookupvalue != "" and lookupvalue is not None:
                projectname = lookupvalue
            else:
                projectname = contents

        if projectnumber == "" or projectnumber is None:
            col = pnc.find_node(xmlroot, "column", "name", "project_id")
            if col:
                lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
                #contents = col.GetChildren():GetContent()
                contents = col.toElement().text()

            if lookupvalue != "" and lookupvalue != None:
                projectnumber = lookupvalue
            else:
                projectnumber = contents

        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()
        co = xmlroot.attributes().namedItem("managing_company_name").nodeValue()

        email = None
        nm = None

        teammember = pnc.find_node(xmlroot, "table", "name", "ix_project_people")
        if teammember:
            memberrow = teammember.firstChild()

        while not memberrow.isNull():
            nm = pnc.get_column_value(memberrow, "name")
            email = pnc.get_column_value(memberrow, "email")

            if nm != pm:
                if (email is not None and email != ""):
                    message.Recipients.Add(email)

            memberrow = memberrow.nextSibling()

        teammember = pnc.find_node(xmlroot, "table", "name", "ix_meeting_attendees")
        if teammember:
            memberrow = teammember.firstChild()

            while not memberrow.isNull():
                nm = pnc.get_column_value(memberrow, "name")
                email = pnc.get_column_value(memberrow, "email")

                if nm != pm:
                    if (email is not None and email != ""):
                        message.Recipients.Add(email)

                memberrow = memberrow.nextSibling()



        message.Display()
        outlook.ActiveExplorer().Activate()

        DefaultSignature = message.HTMLBody

        message.Subject = projectnumber + " " + projectname + " - "

    outlook = None
    message = None

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

# setup test data
"""
print("Buld up QDomDocument")
app = QApplication(sys.argv)

xmldoc = QDomDocument("TestDocument")
f = QFile("exampleproject.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

main_process(xmldoc)
"""
