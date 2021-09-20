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

# TODO:  Setup parameters for Connection
# TODO: check for successful connection and respond
# TODO: add error when not running in Windows_NT

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

# Project Notes Plugin Parameters
pluginname = "Generate Tracker Report"
plugindescription = "Generate a tracker report based on the options selected."

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
RightClickMeeting="wxxmldocument:ix_projects"
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
def main_process( xmlval ):
    emaillist = ""

    # setup global variable
    ProjectsFolder = pnc.get_global_setting("ProjectsFolder")

    executedate = QDateTime.currentDateTime()
    internalreport = False
    keepexcel = False
    emailashtml = False
    emailaspdf = False
    emailasexcel = False
    noemail = True

    loader = QtUiTools.QUiLoader()
    ui = loader.load("includes/dialogTrackerRptOptions.ui")

    ui.m_checkBoxDisplayTracker.setChecked(True)
    ui.m_checkBoxItemsTracker.setChecked(True)
    ui.m_checkBoxNewTracker.setChecked(True)
    ui.m_checkBoxAssignedTracker.setChecked(True)

    if ui.exec() == QDialog.Accepted:
        internalreport = ui.m_checkBoxInternalRptTracker.isChecked()
        keepexcel = ui.m_checkBoxExcelRptTracker.isChecked()

        emailashtml = ui.m_radioBoxEmailAsHTML.isChecked()
        emailaspdf = ui.m_radioBoxEmailAsPDF.isChecked()
        emailasexcel = ui.m_radioBoxEmailAsExcel.isChecked()
        noemail = ui.m_radioBoxDoNotEmail.isChecked()
    else:
        return None

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node

    check_tag = pnc.find_node_by2(xmlroot, "table", "name", "ix_item_tracker", "filter_field", "project_id")
    check_row = None
    if check_tag:
        check_row = check_tag.firstChild()

    if not check_row or not check_tag:
        QMessageBox.warning(None, "No Records", "No tracker or action items are available.", QMessageBox.Ok)
        return None

    projtab = pnc.find_node(xmlroot, "table", "name", "ix_projects")
    projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
    projdes = pnc.get_column_value(projtab.firstChild(), "project_name")

    email = None
    nm = None
    company = None
    receivers = ""

    projectfolder = pnc.get_projectfolder(xmlroot)

    if (projectfolder is None or projectfolder ==""):
        projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QtCore.QDir.home().path())

        if projectfolder == "" or projectfolder is None:
            return(None)
    else:
        projectfolder = projectfolder + "\\Issues List\\"

    pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()
    cm = xmlroot.attributes().namedItem("managing_company_name").nodeValue()

    progbar = QProgressDialog()
    progbar.setCancelButton(None)
    progbar.show()
    progval = 0
    progtot = 6


    progval = progval + 1
    progbar.setValue(min(progval / progtot * 100, 100))
    progbar.setLabelText("looking up recipients...")

    teammember = pnc.find_node(xmlroot, "table", "name", "ix_project_people")
    if teammember:
        memberrow = teammember.firstChild()

        while not memberrow.isNull():
            nm = pnc.get_column_value(memberrow, "name")
            email = pnc.get_column_value(memberrow, "email")
            company = pnc.get_column_value(memberrow, "client_name")

            if nm != pm:
                if (email != None and email != "" and  ( (internalreport and company == cm) or not internalreport )):
                    if receivers != "":
                        receivers = receivers + ", "
                        emaillist = emaillist + ";"

                    receivers = receivers + nm
                    emaillist = emaillist + email

            memberrow = memberrow.nextSibling()


        progval = progval + 1
        progbar.setValue(min(progval / progtot * 100, 100))
        progbar.setLabelText("Copying files...")

        excelreportname = ""
        pdfreportname = ""

        if ui.m_checkBoxInternalRptTracker.isChecked():
            excelreportname = projectfolder + projnum + " Tracker Report Internal.xlsx"
            pdfreportname = projectfolder + projnum + " Tracker Report Internal.pdf"
        else:
            excelreportname = projectfolder + projnum + " Tracker Report.xlsx"
            pdfreportname = projectfolder + projnum + " Tracker Report.pdf"

        QFile.copy("templates\\Tracker Items Template.xlsx", excelreportname)

        handle = pne.open_excel_document(excelreportname)
        sheet = handle['workbook'].Sheets("Item Tracker")

        progval = progval + 1
        progbar.setValue(min(progval / progtot * 100, 100))
        progbar.setLabelText("Gathering status items...")

        # count expand out excel rows for status report items
        repitem = pnc.find_node_by2(xmlroot, "table", "name", "ix_item_tracker", "filter_field", "project_id")
        itemcount = 0

        if repitem:
            repitemrow = repitem.firstChild()

        while not repitemrow.isNull():
            includeitemtype = False
            includeitemstatus = False
            includeiteminternal = False
            isinternal = pnc.get_column_value(repitemrow, "internal_item")
            itemtype = pnc.get_column_value(repitemrow, "item_type")
            status = pnc.get_column_value(repitemrow, "status")

            # determine internal inclusion
            if isinternal == "1" and ui.m_checkBoxInternalRptTracker.isChecked():
                includeiteminternal = True
            elif isinternal == "0" or isinternal == "":
                includeiteminternal = True

            # determine item type to include
            if itemtype == "Tracker" and ui.m_checkBoxItemsTracker.isChecked():
                includeitemtype = True

            if itemtype == "Action" and ui.m_checkBoxActionTracker.isChecked():
                includeitemtype = True

            # determine if it is an included status
            if status == "New" and ui.m_checkBoxNewTracker.isChecked():
                includeitemstatus = True

            if status == "Assigned" and ui.m_checkBoxAssignedTracker.isChecked():
                includeitemstatus = True

            if status == "Defered" and ui.m_checkBoxDeferedTracker.isChecked():
                includeitemstatus = True

            if status == "Resolved" and ui.m_checkBoxResolvedTracker.isChecked():
                includeitemstatus = True

            if status == "Cancelled" and ui.m_checkBoxCancelledTracker.isChecked():
                includeitemstatus = True

            if includeiteminternal and includeitemtype and includeitemstatus:
                itemcount = itemcount + 1

            repitemrow = repitemrow.nextSibling()

        #don't show the internal column on customer version
        if ui.m_checkBoxInternalRptTracker.isChecked() == False:
            intcol = pne.find_cell_tag(sheet, "<INTERNAL_ITEM>")
            if intcol:
                intcol.EntireColumn.Delete()

        pne.expand_row(sheet, "<ITEMID>", itemcount)
        progtot = progtot + itemcount

        progval = progval + 1
        progbar.setValue(min(progval / progtot * 100, 100))
        progbar.setLabelText("Gathering tracker items...")

        itemcount = 0
        repitemrow = repitem.firstChild()

        while not repitemrow.isNull():
            includeitemtype = False
            includeitemstatus = False
            includeiteminternal = False
            isinternal = pnc.get_column_value(repitemrow, "internal_item")
            itemtype = pnc.get_column_value(repitemrow, "item_type")
            status = pnc.get_column_value(repitemrow, "status")

            progval = progval + 1
            progbar:Update(math.min(progval / progtot * 100, 100), "Gathering tracker items...")

            # determine internal inclusion
            if isinternal == "1" and ui.m_checkBoxInternalRptTracker.isChecked():
                includeiteminternal = True
            elif isinternal == "0" or isinternal == "":
                includeiteminternal = True

            # determine item type to include
            if itemtype == "Tracker" and ui.m_checkBoxItemsTracker.isChecked():
                includeitemtype = True

            if itemtype == "Action" and ui.m_checkBoxActionTracker.isChecked():
                includeitemtype = True

            # determine if it is an included status
            if status == "New" and ui.m_checkBoxNewTracker.isChecked():
                includeitemstatus = True

            if status == "Assigned" and ui.m_checkBoxAssignedTracker.isChecked():
                includeitemstatus = True

            if status == "Defered" and ui.m_checkBoxDeferedTracker.isChecked():
                includeitemstatus = True

            if status == "Resolved" and ui.m_checkBoxResolvedTracker.isChecked():
                includeitemstatus = True

            if status == "Cancelled" and ui.m_checkBoxCancelledTracker.isChecked():
                includeitemstatus = True

            if includeiteminternal and includeitemtype and includeitemstatus:
                itemcount = itemcount + 1

                cell = pne.find_cell_tag(sheet, "<ITEMID" + str(itemcount) + ">")

                pne.replace_cell_tag(sheet, "<ITEMID" + str(itemcount) + ">", "'" + pnc.get_column_value(repitemrow, "item_number"))
                pne.replace_cell_tag(sheet, "<ITEMNAME" + str(itemcount) + ">", pnc.get_column_value(repitemrow, "item_name"))
                pne.replace_cell_tag(sheet, "<IDENTIFIEDBY" + str(itemcount) + ">", pnc.get_column_value(repitemrow, "identified_by"))
                pne.replace_cell_tag(sheet, "<DATEIDENTIFIED" + str(itemcount) + ">", pnc.get_column_value(repitemrow, "date_identified"))
                pne.replace_cell_tag(sheet, "<DESCRIPTION" + str(itemcount) + ">", pnc.get_column_value(repitemrow, "description"))
                pne.replace_cell_tag(sheet, "<ASSIGNEDTO" + str(itemcount) + ">", pnc.get_column_value(repitemrow, "assigned_to"))
                pne.replace_cell_tag(sheet, "<PRIORITY" + str(itemcount) + ">", pnc.get_column_value(repitemrow, "priority"))
                pne.replace_cell_tag(sheet, "<STATUS" + str(itemcount) + ">", status)
                pne.replace_cell_tag(sheet, "<DUEDATE" + str(itemcount) + ">", pnc.get_column_value(repitemrow, "date_due"))
                pne.replace_cell_tag(sheet, "<LASTUPDATED" + str(itemcount) + ">", pnc.get_column_value(repitemrow, "last_update"))
                pne.replace_cell_tag(sheet, "<DATERESOLVED" + str(itemcount) + ">", pnc.get_column_value(repitemrow, "date_resolved"))

                if isinternal == "1":
                    pne.replace_cell_tag(sheet, "<INTERNAL_ITEM" + str(itemcount) + ">", "Y")
                else:
                    pne.replace_cell_tag(sheet, "<INTERNAL_ITEM" + str(itemcount) + ">", "N")

                # pull together comments in a readable fashion
                # count expand out excell rows for status report items
                comment = ""
                trackerupdates = pnc.find_node(repitemrow, "table", "name", "ix_item_tracker_updates")

                if trackerupdates:
                    updaterow = trackerupdates.firstChild()

                    while not updaterow.isNull():
                        if comment != "":
                            comment = comment + "\n"

                        comment = comment + pnc.get_column_value(updaterow, "updated_by") + " - " + pnc.get_column_value(updaterow, "lastupdated_date") + ": "  + pnc.get_column_value(updaterow, "update_note")

                        updaterow = updaterow.nextSibling()

                    pne.set_cell_by_tag(sheet, "<COMMENTS" + str(itemcount) + ">", "'" + comment)

                    # expand the row so comments show
                    if cell:
                        cell.EntireRow.AutoFit()

            repitemrow = repitemrow.nextSibling()

    pne.replace_cell_tag(sheet, "<REPORTDATE>", executedate.toString("MM/dd/yyyy"))
    pne.replace_cell_tag(sheet, "<PROJECTNAME>", projnum + " " + projdes)

    # just in case nothing displayed
    pne.replace_cell_tag(sheet, "<ITEMID>", "")
    pne.replace_cell_tag(sheet, "<ITEMNAME>", "")
    pne.replace_cell_tag(sheet, "<IDENTIFIEDBY>", "")
    pne.replace_cell_tag(sheet, "<DATEIDENTIFIED>", "")
    pne.replace_cell_tag(sheet, "<DESCRIPTION>", "")
    pne.replace_cell_tag(sheet, "<ASSIGNEDTO>", "")
    pne.replace_cell_tag(sheet, "<PRIORITY>", "")
    pne.replace_cell_tag(sheet, "<STATUS>", "")
    pne.replace_cell_tag(sheet, "<DUEDATE>", "")
    pne.replace_cell_tag(sheet, "<LASTUPDATED>", "")
    pne.replace_cell_tag(sheet, "<DATERESOLVED>", "")
    pne.replace_cell_tag(sheet, "<COMMENTS>", "")

    if isinternal == "1":
        pne.replace_cell_tag(sheet, "<INTERNAL_ITEM>", "Y")
    else:
        pne.replace_cell_tag(sheet, "<INTERNAL_ITEM>", "N")


    handle['workbook'].Save()

    progval = progval + 1
    progbar.setValue(min(progval / progtot * 100, 100))
    progbar.setLabelText("Finalizing Excel files +.")

    # generate PDFs
    pne.save_excel_as_pdf(handle, sheet, pdfreportname)

    # should we email?
    if noemail == False:
        subject = projnum + " " + projdes + " - " + executedate.toString("MM/dd/yyyy")

        if emailashtml:
            pne.email_excel_html(sheet, subject, emaillist, None)
        elif emailasexcel:
            pne.email_excel_html(sheet, subject, emaillist, excelreportname)
        elif emailaspdf:
            pne.email_excel_html(sheet, subject, emaillist, pdfreportname)

        pne.close_excel_document(handle)

    if ui.m_checkBoxDisplayTracker.isChecked():
        QDesktopServices.openUrl(QUrl("file:///" + pdfreportname, QUrl.TolerantMode))

    progbar.setValue(100)
    progbar.setLabelText("Finalizing Excel files...")
    progbar.hide()
    progbar.close()
    progbar = None # must be destroyed

    pne.killexcelautomation()

    if keepexcel == False:
        QFile.remove(excelreportname)

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
