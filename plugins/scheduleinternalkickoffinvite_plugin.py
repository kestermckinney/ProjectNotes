import sys
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt6.QtSql import QSqlDatabase
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Schedule Internal Kickoff"
plugindescription = "Using Outlook create an invite to the internal Kickoff."
plugintable = "projects" # the table or view that the plugin applies to.  This will enable the right click
childtablesfilter = "projects/project_people" # a list of child tables that can be sent to the plugin.  This will be used to exclude items like notes or action items when they aren't used

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


# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    #
    pnc = ProjectNotesCommon()
    pne = ProjectNotesExcelTools()
    
    def event_data_rightclick(xmlstr):
        print("called event: " + __file__)

        app = QApplication(sys.argv)
        xmlval = QDomDocument()
        xmldoc = ""

        window_title = ""
        
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""

        outlook = win32com.client.Dispatch("Outlook.Application")
        message = outlook.CreateItem(1)

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        
        pm = xmlroot.toElement().attribute("managing_manager_name")
        co = xmlroot.toElement().attribute("managing_company_name")

        print("main pm", pm)
        print("main company", co)

        email = None
        nm = None
        pco = None

        teammember = pnc.find_node(xmlroot, "table", "name", "project_people")
        if teammember:
            memberrow = teammember.firstChild()

            while not memberrow.isNull():
                nm = pnc.get_column_value(memberrow, "name")
                email = pnc.get_column_value(memberrow, "email")
                pco = pnc.get_column_value(memberrow, "client_name")

                print(pco, ",", co, ",", email, ",", pm, ",", nm)

                if nm != pm:
                    if (email is not None and email != "" and pco == co):
                        message.Recipients.Add(email)

                memberrow = memberrow.nextSibling()

        project = pnc.find_node(xmlroot, "table", "name", "projects")
        if project:
            projectrow = project.firstChild()

            if not projectrow.isNull():
                window_title = pnc.get_column_value(projectrow, "project_number") + " " + pnc.get_column_value(projectrow, "project_name") + " - Internal Kickoff"
                message.Subject = window_title

        txt = get_text_invite()
        message.MeetingStatus = 1
        message.Duration = 60
        message.Location = pnc.get_plugin_setting("DefaultMeetingLocation")
        message.Body = txt
        outlook.ActiveExplorer().Activate()
        message.Display()


        outlook = None
        message = None

        pnc.bring_window_to_front(window_title)

        return xmldoc


def get_text_invite():
    txtdoc = """Internal Kick Off Agenda
-The purpose of this meeting is to get an initial understanding of who the stakeholders are and their roles.
-We then want to understand the business drivers that lead to the project.  We want to hear insight from sales on business drivers and continued sales strategy.
-We also want to review the current proposal to make sure everyone has an overall understanding of the project and it's deliverables.  As we review the proposal, let's highlight any risks we see.
-Anyone who has any experience with the client will want to provide insight on what it is like to work with the client.
-Depending upon the client, some may have very formal standards and some no standards.  We want to get an idea of where this client falls on the spectrum.
-We'll conclude our time with our next steps.

Review Team Roles
Sales Overview of Client & Estimate
  * Project Business Purpose
  * Key Business Drivers
  * Stakeholders Involved : People, Partners & Vendors
  * Future Sales Items/Sales Strategy
Review Proposal
  * Scope Of Work
  * Deliverables
  * Invoicing
  * Assumptions & Constraints
Review Lessons Learned From Prior Projects
Identify Risks
Review Client Standards
  * Drafting
  * Programming
  * Testing
  * Other Corporate
Additional Items?
Next Steps
"""

    return txtdoc
"""
# setup test data
import sys
print("Buld up QDomDocument")
#

xmldoc = QDomDocument("TestDocument")
f = QFile("C:/Users/pamcki/Desktop/project.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

#main_process_project(xmldoc)
event_data_rightclick(xmldoc.toString())
"""

# TESTED: Phase 1
