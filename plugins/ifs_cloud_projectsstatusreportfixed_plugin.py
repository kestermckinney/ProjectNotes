
import platform

if (platform.system() == 'Windows'):
    import win32com

from includes.common import ProjectNotesCommon
from includes.ifs_common import IFSCommon
from PyQt5 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt5.QtSql import QSqlDatabase
from PyQt5.QtXml import QDomDocument, QDomNode
from PyQt5.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo
from PyQt5.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt5.QtGui import QDesktopServices
from urllib3.exceptions import InsecureRequestWarning 

import requests
import json

requests.packages.urllib3.disable_warnings(category=InsecureRequestWarning)

# Project Notes Plugin Parameters
pluginname = "Generate Project Status Report Fixed"
plugindescription = "Execute the SSRS Project Status Report for fixed priced projects.  Update the ISSUES Activity in IFS with items from the Tracker list.  Completed Tasks are deleted from IFS."
plugintable = "projects" # the table or view that the plugin applies to.  This will enable the right click
childtablesfilter = "projects/item_tracker/project_locations/project_people" # a list of child tables that can be sent to the plugin.  This will be used to exclude items like notes or action items when they aren't used

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


# make global to pass between sections
mtotalcost = 0
mtotalinvoiced = 0
mtotalpoline = 0
mtotalinvoicable = 0

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pnc = ProjectNotesCommon()
    ifsc = IFSCommon()

    def populate_financials(xmldoc, projectid, financialsdate):
        global ProjectsFolder
        global IFSUsername
        global IFSUrl
        global IFSPassword
        global IFSPersonId

        request_url = IFSUrl + '/main/ifsapplications/projection/v1/ProjectsHandling.svc/Projects?$filter=(ProjectId%20eq%20%27' + projectid + '%27)&$select=State,ProjectId,Objstate,Objgrants,Name,Company,CustomerCategory,CustomerId,Description,CompanyName,Manager,AccessOnOff,ProgramId,Category1Id,Category2Id,PlanStart,PlanFinish,ActualStart,ActualFinish,ApprovedDate,CloseDate,CancelDate,FrozenDate,luname,keyref,Cf_Lastinvoiced&$expand=AccountingProjectRef($select=ProjectGroup,Objgrants,luname,keyref),ManagerRef($select=Name,luname,keyref),CustomerIdRef($select=Name,Objgrants,luname,keyref),ProgramIdRef($select=Description,luname,keyref),Category1IdRef($select=Description,luname,keyref),Category2IdRef($select=Description,luname,keyref)'

        result = requests.get(request_url, verify=False, auth=(IFSUsername, IFSPassword),headers = {"Prefer": "odata.maxpagesize=500","Prefer": "odata.track-changes"})

        if (result.status_code != 200):
            QMessageBox.critical(None, "ODATA Request Failed", result.reason + ": " + result.text + " " + request_url, QMessageBox.Ok)
            print("ODATA Request Failed", result.reason + ": " + result.text + " " + request_url)
            return ""

        json_result = result.json()

        # print("Debug JSON:")
        # print(json.dumps(json_result, indent=4))

        table = pnc.xml_table(xmldoc, "projects")

        for rowval in json_result['value']:
            #print("processing projects row....")

            row = pnc.xml_row(xmldoc)
            table.appendChild(row)

            row.appendChild(pnc.xml_col(xmldoc, "project_number", rowval['ProjectId'], None))
            row.appendChild(pnc.xml_col(xmldoc, "project_name", pnc.to_xml(rowval['Description']), None))
            row.appendChild(pnc.xml_col(xmldoc, "last_status_date", pnc.to_xml(financialsdate), None))
            row.appendChild(pnc.xml_col(xmldoc, "last_invoice_date", rowval['Cf_Lastinvoiced'], None))

            if rowval['CustomerIdRef'] is not None:
                row.appendChild(pnc.xml_col(xmldoc, "client_id", None,pnc.to_xml(rowval['CustomerIdRef']['Name'])))

            metrics = ifsc.getearnedvaluemetrics(projectid)
            costmetrics = ifsc.getcostmetrics(projectid)

            row.appendChild(pnc.xml_col(xmldoc, "budget",pnc.to_xml(str(costmetrics['Baseline_aggr_'])) , None))
            row.appendChild(pnc.xml_col(xmldoc, "actual",pnc.to_xml(str(costmetrics['Used_aggr_'])) , None))
            row.appendChild(pnc.xml_col(xmldoc, "bcwp" ,pnc.to_xml(str(costmetrics['EarnedValue_aggr_'])) , None))
            row.appendChild(pnc.xml_col(xmldoc, "bcws",pnc.to_xml(str(costmetrics['ScheduledWork_aggr_'])), None))
            row.appendChild(pnc.xml_col(xmldoc, "bac",pnc.to_xml(str(metrics['Bac_aggr_'])), None))

            QtWidgets.QApplication.processEvents()

        return(table)

    # processing main function
    def event_data_rightclick(xmlstr):
        print("called event: " + __file__)

        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)
            return ""

        global ProjectsFolder
        global IFSUsername
        global IFSUrl
        global IFSPassword
        global IFSPersonId

        if not pnc.verify_global_settings():
            return ""

        # setup global variables
        ProjectsFolder = pnc.get_plugin_setting("ProjectsFolder")
        IFSUsername = pnc.get_plugin_setting("IFSUsername", "IFS Cloud Settings")
        IFSUrl = pnc.get_plugin_setting("IFSUrl", "IFS Cloud Settings")
        IFSPassword = pnc.get_plugin_setting("IFSPassword", "IFS Cloud Settings")
        IFSPersonId = pnc.get_plugin_setting("IFSPersonId", "IFS Cloud Settings")
        ReportServer = pnc.get_plugin_setting("ReportServer", "IFS Cloud Settings")
        DomainUser = pnc.get_plugin_setting("DomainUser", "IFS Cloud Settings")
        DomainPassword = pnc.get_plugin_setting("DomainPassword", "IFS Cloud Settings")

        emaillist = ""

        ProjectsFolder = pnc.get_plugin_setting("ProjectsFolder")

        statusdate = QDateTime.currentDateTime()
        executedate = statusdate
        keepword = False
        emailashtml = False
        emailaspdf = False
        emailasword = False
        noemail = False
        updateifstasks = True

        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()   

        ui = uic.loadUi("plugins/includes/dialogSSRSStatusRptOptions.ui")
        ui.m_datePickerRptDateStatus.setCalendarPopup(True)
        ui.setWindowFlags(
            QtCore.Qt.Window |
            QtCore.Qt.WindowCloseButtonHint 
            )

        ui.m_datePickerRptDateStatus.setDateTime(statusdate)
        ui.m_checkBoxDisplayStatus.setChecked(False)
        ui.m_radioBoxEmailAsHTML.setChecked(True)

        if ui.exec() == QDialog.Accepted:
            statusdate = ui.m_datePickerRptDateStatus.dateTime()
            keepword = ui.m_checkBoxWordRptStatus.isChecked()

            emailashtml = ui.m_radioBoxEmailAsHTML.isChecked()
            emailaspdf = ui.m_radioBoxEmailAsPDF.isChecked()
            emailasword = ui.m_radioBoxEmailAsWord.isChecked()
            noemail = ui.m_radioBoxDoNotEmail.isChecked()
            updateifstasks = ui.m_checkUpdateIFSTasks.isChecked()

            QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            QtWidgets.QApplication.processEvents()        
        else:
            QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            QtWidgets.QApplication.processEvents()
            return("")

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = pnc.get_projectfolder(xmlroot)
        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()

        if (projectfolder is None or projectfolder ==""):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return("")
        else:
            projectfolder = projectfolder + "\\Status Reports\\"

        progbar = QProgressDialog()
        progbar.setWindowTitle("Generating Report...")
        progbar.setWindowFlags(
            QtCore.Qt.Window |
            QtCore.Qt.WindowCloseButtonHint 
            )

        progbar.setMinimumWidth(350)
        progbar.setCancelButton(None)
        progbar.show()
        progval = 0
        progtot = 11

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = pnc.get_column_value(projtab.firstChild(), "project_name")
        period = pnc.get_column_value(projtab.firstChild(), "status_report_period")

        email = None
        nm = None
        stat = None
        receivers = ""

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("looking up recipients...")
        QtWidgets.QApplication.processEvents()   

        teammember = pnc.find_node(xmlroot, "table", "name", "project_people")
        if not teammember.isNull():
            memberrow = teammember.firstChild()

            while not memberrow.isNull():
                nm = pnc.get_column_value(memberrow, "name")
                email = pnc.get_column_value(memberrow, "email")
                stat = pnc.get_column_value(memberrow, "receive_status_report")
                if not nm == pm:
                    if (not email == None and not email == "" and stat == "1"):
                        if not receivers == "":
                            receivers = receivers + ", "
                            emaillist = emaillist + ";"

                        receivers = receivers + nm
                        emaillist = emaillist + email

                memberrow = memberrow.nextSibling()
                QtWidgets.QApplication.processEvents()

            progval = progval + 1
            progbar.setValue(int(min(progval / progtot * 100, 100)))
            progbar.setLabelText("Copying files...")
            QtWidgets.QApplication.processEvents()   

        basereportpdf = projectfolder + statusdate.toString("yyyyMMdd ") + projnum + " Status Report.pdf"
        basereportdocx = projectfolder + statusdate.toString("yyyyMMdd ") + projnum + " Status Report.docx"

        QFile.remove(basereportpdf)
        QFile.remove(basereportdocx)

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Gathering status items...")
        QtWidgets.QApplication.processEvents()   

        # show all tracker items
        trackeritems = pnc.find_node_by2(xmlroot, "table", "name", "item_tracker", "filter_field_1", "project_id")

        itemcount = 0
        isinternal = 0
        itemstatus = ""
        itemtype = ""

        if not trackeritems is None and updateifstasks:
            itemrow = trackeritems.firstChild()

            while not itemrow.isNull():
                itemcount = itemcount + 1

                itemrow = itemrow.nextSibling()
                QtWidgets.QApplication.processEvents()

            itemcount = 0

            # find the ISSUES Activity
            jsact = ifsc.getopenactivities(projnum)
            issuesseq = ifsc.getissuesactivity(jsact)

            if not issuesseq is None:           
                itemrow = trackeritems.firstChild()

                while not itemrow.isNull():
                    isinternal = pnc.get_column_value(itemrow, "internal_item")
                    itemstatus = pnc.get_column_value(itemrow, "status")
                    itemtype = pnc.get_column_value(itemrow, "item_type")
                    ifsitemid = projnum + "-" + pnc.get_column_value(itemrow, "item_number")

                    duedate = ''

                    if pnc.get_column_value(itemrow, "date_due") is not None:
                        dudate = " Due: " + pnc.get_column_value(itemrow, "date_due")

                    desc = pnc.get_column_value(itemrow, "description") + " Priority: " + pnc.get_column_value(itemrow, "priority") + duedate

                    progbar.setLabelText("Gathering status items... " + pnc.get_column_value(itemrow, "item_number"))

                    if (isinternal != "1" and itemtype == "Tracker" and (itemstatus == "Assigned" or itemstatus == "New")):
                        if not ifsc.updateactivitytask(issuesseq, projnum, ifsitemid, pnc.get_column_value(itemrow, "item_name"), desc):
                            ifsc.createactivitytask(issuesseq, projnum, ifsitemid, pnc.get_column_value(itemrow, "item_name"), desc)
                    else:
                        ifsc.deleteactivitytask(issuesseq, projnum, ifsitemid)    
                    
                    itemcount = itemcount + 1

                    itemrow = itemrow.nextSibling()
                    QtWidgets.QApplication.processEvents()


        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Pulling IFS project summary...")
        QtWidgets.QApplication.processEvents()   

        docxml = QDomDocument()
        docroot = docxml.createElement("projectnotes");
        docxml.appendChild(docroot);

        node = populate_financials(docxml, projnum, statusdate.toString("MM/dd/yyyy"))
        docroot.appendChild(node)

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Pulling IFS milestones...")
        QtWidgets.QApplication.processEvents()   

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Pulling IFS PO information...")
        QtWidgets.QApplication.processEvents()   

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Pulling IFS project summary...")
        QtWidgets.QApplication.processEvents()   

        periodstart = 14
        if period == "Bi-Weekly":
            periodstart = 14
        elif period == "Weekly":
            periodstart = 7
        elif period == "Monthly":
            periodstart = 30

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Finalizing files...")
        QtWidgets.QApplication.processEvents()   

        # project status report fixed
        if emailashtml or emailasword or keepword:
            ifsc.downloadreport("http://" + ReportServer + "/ReportServer/Pages/ReportViewer.aspx?%2fProd%2fProjects%2fProject+Status+Report+Fixed&ProjectId=" + projnum + "&StatusDate=" + statusdate.toString("MM/dd/yyyy") + "&ReportPeriod=" + str(periodstart) + "&rs:Command=Render&rs:Format=WORDOPENXML", basereportdocx)
        ifsc.downloadreport("http://" + ReportServer + "/ReportServer/Pages/ReportViewer.aspx?%2fProd%2fProjects%2fProject+Status+Report+Fixed&ProjectId=" + projnum + "&StatusDate=" + statusdate.toString("MM/dd/yyyy") + "&ReportPeriod=" + str(periodstart) + "&rs:Command=Render&rs:Format=PDF", basereportpdf)

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Finalizing files...")
        QtWidgets.QApplication.processEvents()   

        # should we email?
        if noemail == False:
            subject = projnum + " " + projdes + " Status Report " + statusdate.toString("MM/dd/yyyy")

            if emailashtml:
                pnc.email_word_file_as_html(subject, emaillist, None, basereportdocx)
            elif emailasword:
                pnc.email_word_file_as_html(subject, emaillist, basereportdocx, None)
            elif emailaspdf:
                pnc.email_word_file_as_html(subject, emaillist, basereportpdf, None)

        if keepword == False:
            QFile.remove(basereportdocx)

        if ui.m_checkBoxDisplayStatus.isChecked():
            QDesktopServices.openUrl(QUrl("file:///" + basereportpdf, QUrl.TolerantMode))

        progbar.setValue(100)
        progbar.setLabelText("Finalizing files...")
        QtWidgets.QApplication.processEvents()   
        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed
        #print("closing progress bar...")

        return(docxml.toString())

# setup test data
"""
import sys
print("Buld up QDomDocument")
app = QApplication(sys.argv)

xmldoc = QDomDocument("TestDocument")
f = QFile("C:/Users/pamcki/Desktop/project.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

print(event_data_rightclick(xmldoc.toString()))
"""