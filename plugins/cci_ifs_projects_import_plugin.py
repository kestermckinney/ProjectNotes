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
pluginname = "Import IFS Projects"
plugindescription = "Import managed projects assocaiated with your Oracle Username."

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

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any functions

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

OracleUsername = ""
ProjectsFolder = ""

pnc = ProjectNotesCommon()
pne = ProjectNotesExcelTools()

def getclientsxml(adodb):
    # get all the customers on my open projects
    strsql = "select distinct i.customer_id, i.name from nort1app.customer_info i left join NORT1APP.project p on i.customer_id=p.customer_id right join NORT1APP.customer_info u on p.customer_id=u.customer_id where p.manager='" + OracleUsername.upper() + "' and p.state not in ('Closed', 'Completed')"

    clientsxml = "<table name=\"ix_clients\">\n"
    recordset = win32com.client.Dispatch("ADODB.Recordset")

    recordset.Open(strsql, adodb)

    while not recordset.EOF:
        clientsxml = clientsxml + "  <row>\n   <column name=\"client_name\" number=\"1\">" + pnc.to_xml(recordset.Fields.Item("name").Value) + "</column>\n </row>\n"
        recordset.MoveNext()

    recordset.Close()
    recordset = None

    clientsxml = clientsxml + "</table>\n"
    return clientsxml


def getprojectsxml(adodb):
    global OracleUsername
    global ProjectsFolder

    strsql = """select p.PROJECT_ID, p.DESCRIPTION, u.customer_id, u.name, CONTROL_CATEGORY, COST_ELEMENT_DESCRIPTION, ESTIMATED, PLANNED, BASELINE, EARNED_VALUE, SCHEDULED_WORK, PLANNED_COMMITTED, COMMITTED, USED, ACTUAL,
    TO_CHAR((select max(invoice_date) from NORT1APP.INVOICE i where i.project_id=d.project_id),'MM/DD/YYYY') as last_invoice,
    ( ACTUAL + (BASELINE - EARNED_VALUE) / (EARNED_VALUE / ACTUAL) * (EARNED_VALUE / SCHEDULED_WORK) ) AS EAC,
    (  (ACTUAL - EARNED_VALUE)  / EARNED_VALUE ) AS CV,
    ( (EARNED_VALUE - SCHEDULED_WORK) / SCHEDULED_WORK ) AS SV,
    ( EARNED_VALUE / BASELINE ) AS PCT,
    ( EARNED_VALUE / ACTUAL ) AS CPI
    from NORT1APP.PROJ_CON_DET_SUM_COST_PROJECT d right join
    NORT1APP.project p on d.project_id=p.project_id right join
    NORT1APP.customer_info u on p.customer_id=u.customer_id
    where (d.CONTROL_CATEGORY='LABOR-INT' or p.STATE='Initialized' or p.STATE='Approved') and  p.manager='"""
    strsql += OracleUsername.upper() + "' and p.state not in ('Closed', 'Completed')"

    # print(strsql + "\n")
    locationsxml = ""
    projectsxml = "<table name=\"ix_projects\">\n"
    recordset = win32com.client.Dispatch("ADODB.Recordset")

    recordset.Open(strsql, adodb)

    while not recordset.EOF:

        projectsxml = projectsxml + "  <row>\n"

        projectsxml = projectsxml + "    <column name=\"project_number\" number=\"1\">" + pnc.to_xml(recordset.Fields.Item("project_id").Value) + "</column>\n"
        projectsxml = projectsxml + "    <column name=\"project_name\" number=\"2\">" + pnc.to_xml(recordset.Fields.Item("description").Value) + "</column>\n"
        #projectsxml = projectsxml + "    <column name=\"last_status_date\" number=\"3\">" + pnc.to_xml(recordset.Fields.Item("name").Value) + "</column>\n"
        projectsxml = projectsxml + "    <column name=\"last_invoice_date\" number=\"4\">" + pnc.to_xml(recordset.Fields.Item("last_invoice").Value) + "</column>\n"
        #projectsxml = projectsxml + "<column name=\"primary_contact\" number=\"5\"></column>\n"
        projectsxml = projectsxml + "    <column name=\"budget\" number=\"6\">" + pnc.to_xml(recordset.Fields.Item("estimated").Value) + "</column>\n"
        projectsxml = projectsxml + "    <column name=\"actual\" number=\"7\">" + pnc.to_xml(recordset.Fields.Item("actual").Value) + "</column>\n"
        projectsxml = projectsxml + "    <column name=\"bcwp\" number=\"8\">" + pnc.to_xml(recordset.Fields.Item("earned_value").Value) + "</column>\n"
        projectsxml = projectsxml + "    <column name=\"bcws\" number=\"9\">" + pnc.to_xml(recordset.Fields.Item("scheduled_work").Value) + "</column>\n"
        projectsxml = projectsxml + "    <column name=\"bac\" number=\"10\">" + pnc.to_xml(recordset.Fields.Item("baseline").Value) + "</column>\n"
        #projectsxml = projectsxml + "    <column name=\"invoicing_period\" number=\"11\"></column>\n"
        #projectsxml = projectsxml + "    <column name=\"status_report_period\" number=\"12\">Bi-Weekly</column>\n"
        projectsxml = projectsxml + "    <column name=\"client_id\" number=\"13\" lookupvalue=\"" + pnc.to_xml(recordset.Fields.Item("name").Value) + "\"></column>\n"
        projectsxml = projectsxml + "    <column name=\"project_status\" number=\"14\">Active</column>\n"

        locationsxml = locationsxml + pnc.find_projectlocations( recordset.Fields.Item("project_id").Value, ProjectsFolder)

        projectsxml = projectsxml + "  </row>\n"

        recordset.MoveNext()

    recordset.Close()
    recordset = None

    #print (projectsxml + "\n")

    projectsxml = projectsxml + "</table>\n"

    projectsxml = projectsxml + "<table name=\"ix_project_locations\">\n"
    projectsxml = projectsxml + locationsxml
    projectsxml = projectsxml + "</table>\n"

    return projectsxml


def getteammembersxml(adodb):
    global OracleUsername
    global ProjectsFolder
    strsql =  """Select distinct p.project_id, al.employee_id, em.name, ci.name company_name, tp.description from NORT1APP.proj_employee_allocations al
    left join NORT1APP.company_emp em on al.employee_id=em.employee_id
    left join NORT1APP.project p on al.project_id=p.project_id
    left join NORT1APP.company ci on em.company=ci.company
    left join NORT1APP.proj_res_groups_with_comp_conn tp on tp.resource_id=al.resource_id
    where p.manager='"""
    strsql += OracleUsername.upper() + "' and p.state not in ('Closed', 'Completed') "

    companyxml = ""

    projectsxml = "<table name=\"ix_project_people\">\n"
    peoplexml = "<table name=\"ix_people\">\n"
    recordset = win32com.client.Dispatch("ADODB.Recordset")

    recordset.Open(strsql, adodb)

    while not recordset.EOF:

        projectsxml = projectsxml + "  <row>\n"

        projectsxml = projectsxml + "    <column name=\"project_id\" number=\"1\" lookupvalue=\"" + pnc.to_xml(recordset.Fields.Item("project_id").Value) + "\"></column>\n"
        projectsxml = projectsxml + "    <column name=\"people_id\" number=\"2\" lookupvalue=\"" + pnc.to_xml(recordset.Fields.Item("name").Value) + "\"></column>\n"
        projectsxml = projectsxml + "    <column name=\"role\" number=\"4\">" + pnc.to_xml(recordset.Fields.Item("description").Value) + "</column>\n"
        projectsxml = projectsxml + "  </row>\n"

        peoplexml = peoplexml + "  <row>\n"

        peoplexml = peoplexml + "    <column name=\"name\" number=\"1\">" + pnc.to_xml(recordset.Fields.Item("name").Value) + "</column>\n"
        peoplexml = peoplexml + "    <column name=\"client_id\" number=\"5\" lookupvalue=\"" + pnc.to_xml(recordset.Fields.Item("company_name").Value) + "\"></column>\n"

        peoplexml = peoplexml + "  </row>\n"

        # only add the company name once to the client list
        if companyxml == "":
          companyxml = "<table name=\"ix_clients\">\n"
          companyxml = companyxml + "  <row>\n   <column name=\"client_name\" number=\"1\">" + pnc.to_xml(recordset.Fields.Item("company_name").Value) + "</column>\n </row>\n"
          companyxml = companyxml + "</table>\n"

        recordset.MoveNext()

    recordset.Close()

    peoplexml = peoplexml + "</table>\n"
    projectsxml = projectsxml + "</table>\n"

    # import the people before the projects team member records
    projectsxml = companyxml + peoplexml + projectsxml

    return projectsxml


# processing main function
def main_process( xmlval ):
    global OracleUsername
    global ProjectsFolder

    if not pnc.verify_global_settings():
        return None

    # setup global variables
    OracleUsername = pnc.get_global_setting("OracleUsername")
    ProjectsFolder = pnc.get_global_setting("ProjectsFolder")

    progbar = QProgressDialog()
    progbar.setCancelButton(None)
    progbar.setLabelText("Getting data...")
    progbar.setValue(0)
    progbar.show()
    progval = 20

    docxml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<projectnotes>\n"

    adodb = pnc.connect()
    progval = progval + 20
    progbar.setValue(progval)
    progbar.setLabelText("Looking up clients...")

    docxml = docxml + getclientsxml(adodb)

    progval = progval + 20
    progbar.setValue(progval)
    progbar.setLabelText("Looking up projects...")

    docxml = docxml + getprojectsxml(adodb)

    progval = progval + 30
    progbar.setValue(progval)
    progbar.setLabelText("Looking up team members...")


    docxml = docxml + getteammembersxml(adodb)
    docxml = docxml + "</projectnotes>\n"
    pnc.close(adodb)

    progbar.setValue(100)
    progbar.setLabelText("Finalizing import files...")
    progbar.hide()
    progbar.close()

    return docxml

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

# call when testing outside of Project Notes
"""
print("Run Test")
app = QApplication(sys.argv)

str = main_process(None)
print(str)
"""
