
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
pluginname = "Import IFS Projects"
plugindescription = "Import managed projects assocaiated with your Oracle Username."
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

OracleUsername = ""
ProjectsFolder = ""

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pnc = ProjectNotesCommon()
    pne = ProjectNotesExcelTools()

    def getclientsxml(adodb):
        # get all the customers on my open projects
        strsql = "select distinct i.customer_id, i.name from nort1app.customer_info i left join NORT1APP.project p on i.customer_id=p.customer_id right join NORT1APP.customer_info u on p.customer_id=u.customer_id where p.manager='" + OracleUsername.upper() + "' and p.state not in ('Closed', 'Completed')"

        clientsxml = "<table name=\"clients\">\n"
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
        projectsxml = "<table name=\"projects\">\n"
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

        projectsxml = projectsxml + "<table name=\"project_locations\">\n"
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

        projectsxml = "<table name=\"project_people\">\n"
        peoplexml = "<table name=\"people\">\n"
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
              companyxml = "<table name=\"clients\">\n"
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
    def event_menuclick(xmlstr):
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)
            return ""

        global OracleUsername
        global ProjectsFolder

        if not pnc.verify_global_settings():
            return ""

        # setup global variables
        OracleUsername = pnc.get_global_setting("OracleUsername")
        ProjectsFolder = pnc.get_global_setting("ProjectsFolder")

        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        QtWidgets.QApplication.processEvents()

        progbar = QProgressDialog()
        progbar.setWindowTitle("Importing...")
        progbar.setWindowFlags(
            QtCore.Qt.Window |
            QtCore.Qt.WindowCloseButtonHint |
            QtCore.Qt.WindowStaysOnTopHint
            )
        progbar.setCancelButton(None)
        progbar.setLabelText("Getting data...")
        progbar.setValue(0)
        QtWidgets.QApplication.processEvents() 
        progbar.show()
        progval = 20

        docxml = "<projectnotes>\n"

        adodb = pnc.connect()
        progval = progval + 20
        progbar.setValue(progval)
        progbar.setLabelText("Looking up clients...")
        QtWidgets.QApplication.processEvents() 

        docxml = docxml + getclientsxml(adodb)

        progval = progval + 20
        progbar.setValue(progval)
        progbar.setLabelText("Looking up projects...")
        QtWidgets.QApplication.processEvents() 

        docxml = docxml + getprojectsxml(adodb)

        progval = progval + 30
        progbar.setValue(progval)
        progbar.setLabelText("Looking up team members...")
        QtWidgets.QApplication.processEvents() 


        docxml = docxml + getteammembersxml(adodb)
        docxml = docxml + "</projectnotes>\n"
        pnc.close(adodb)

        progbar.setValue(100)
        progbar.setLabelText("Finalizing import files...")
        QtWidgets.QApplication.processEvents() 
        progbar.hide()
        progbar.close()


        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents() 

        return docxml

# call when testing outside of Project Notes
"""
import sys
print("Run Test")
app = QApplication(sys.argv)

str = event_menuclick(None)
print(str)
"""
# TODO: Right click menus should include icons from the main menu

