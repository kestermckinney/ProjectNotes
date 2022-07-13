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
pluginname = "Archive Meeting Notes"
plugindescription = "Generate meeting notes archive based on the options selected."

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
#      ix_meeting_attees
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
RightClickProject="wxxmldocument:ix_project_notes"
RightClickPeople="disabled"
RightClickClient="disabled"
RightClickStatusReportItem="disabled"
RightClickLocationItem="disabled"
RightClickTeamMember="disabled"
RightClickMeeting="disabled"
RightClickAttee="disabled"
RightCickTrackerItem="disabled"

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# Project Notes Parameters
parameters = {

}

OracleUsername = ""
ProjectsFolder = ""

pnc = ProjectNotesCommon()
pne = ProjectNotesExcelTools()

# make global to pass between sections

emaillist = ""

# processing main def
def main_process( xmlval ):
    if not pnc.verify_global_settings():
        return None

    # setup global variables
    ProjectsFolder = pnc.get_global_setting("ProjectsFolder")

    executedate = QDateTime.currentDateTime()
    internalreport = False
    keepexcel = False
    emailashtml = False
    emailaspdf = False
    emailasexcel = False
    noemail = True

    loader = QtUiTools.QUiLoader()
    ui = loader.load("includes/dialogNotesArchiveOptions.ui")

    if ui.exec() == QDialog.Accepted:
        internalreport = ui.m_checkBoxInternalRptNotes.isChecked()
        keepexcel = ui.m_checkBoxExcelRptNotes.isChecked()
        emailashtml = ui.m_radioBoxEmailAsHTML.isChecked()
        emailaspdf = ui.m_radioBoxEmailAsPDF.isChecked()
        emailasexcel = ui.m_radioBoxEmailAsExcel.isChecked()
        noemail = ui.m_radioBoxDoNotEmail.isChecked()
    else:
        return None

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
    projectfolder = pnc.get_projectfolder(xmlroot)
    pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()


    projtab = pnc.find_node(xmlroot, "table", "name", "ix_projects")
    projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
    projdes = pnc.get_column_value(projtab.firstChild(), "project_name")

    email = False
    nm = None
    stat = None
    receivers = ""

    if (projectfolder is None or projectfolder ==""):
        projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QtCore.QDir.home().path())

        if projectfolder == "" or projectfolder is None:
            return(None)
    else:
        projectfolder = projectfolder + "\\Meeting Minutes\\"

    progbar = QProgressDialog()
    progbar.setCancelButton(None)
    progbar.show()

    progval = 0
    progtot = 6
    progbar.setValue(min(progval / progtot * 100, 100))
    progbar.setLabelText("Generating Report...")

    progval = progval + 1
    progbar.setValue(min(progval / progtot * 100, 100))
    progbar.setLabelText("Copying files...")

    excelreportname = ""
    pdfreportname = ""

    if internalreport:
        excelreportname = projectfolder + projnum + " Meeting Minutes Internal.xlsx"
        pdfreportname = projectfolder + projnum + " Meeting Minutes Internal.pdf"
    else:
        excelreportname = projectfolder + projnum + " Meeting Minutes.xlsx"
        pdfreportname = projectfolder + projnum + " Meeting Minutes.pdf"


    QFile.copy("templates\\Meeting Template.xlsx", excelreportname)

    handle = pne.open_excel_document(excelreportname)
    sheet = handle['workbook'].Sheets("Meeting Notes")

    progval = progval + 1
    progbar.setValue(min(progval / progtot * 100, 100))
    progbar.setLabelText("Gathering notes...")


    # count expand out excell rows for status report items
    notes = pnc.find_node(xmlroot, "table", "name", "ix_project_notes")
    itemcount = 0

    if notes:
        notesrow = notes.firstChild()

        while not notesrow.isNull():
            isinternal = pnc.get_column_value(notesrow, "internal_item")

            if isinternal == "1" and internalreport:
                itemcount = itemcount + 1
            elif isinternal != "1":
                itemcount = itemcount + 1

            notesrow = notesrow.nextSibling()

        pne.expand_section(sheet, "<MEETINGTOP>", "<MEETINGBOTTOM>", itemcount)
        progtot = progtot + itemcount

        progval = progval + 1
        progbar.setValue(min(progval / progtot * 100, 100))
        progbar.setLabelText("Gathering notes...")

    itemcount = 0
    notesrow = notes.firstChild()

    while not notesrow.isNull():
        isinternal = pnc.get_column_value(notesrow, "internal_item")
        includeitem = False

        if isinternal == "1" and internalreport:
            includeitem = True
        elif isinternal != "1":
            includeitem = True

        if includeitem:
            itemcount = itemcount + 1

        progval = progval + 1
        progbar.setValue(min(progval / progtot * 100, 100))
        progbar.setLabelText("Gathering notes...")

        pne.replace_cell_tag(sheet, "<MEETING_TITLE" + str(itemcount) + ">", "'" + pnc.get_column_value(notesrow, "note_title"))
        pne.replace_cell_tag(sheet, "<MEETING_DATE" + str(itemcount) + ">", pnc.get_column_value(notesrow, "note_date"))
        # replace feature doesn't like long text values
        # expand the row so notes show

        # EXCEL WON'T AUTOFIT THIS FOR SOME REASON I BELIEVE IT IS BECAUSE THE CELLS ARE MERGED

        note = pnc.get_column_value(notesrow, "note")
        pne.set_cell_by_tag(sheet, "<MEETING_NOTES" + str(itemcount) + ">", "'" + note )
        """
        if cell :
          cell.EntireRow:AutoFit()

        """

        attendees = pnc.find_node(notesrow, "table", "name", "ix_meeting_attendees")
        attendeelist = ""
        if attendees:
            attendeerow = attendees.firstChild()

            while not attendeerow.isNull():
                if attendeelist != "" :
                    attendeelist = attendeelist + ", "

                attendeelist = attendeelist + pnc.get_column_value(attendeerow, "name")
                attendeerow = attendeerow.nextSibling()

        # expand the row so attees show
        cell = pne.find_cell_tag(sheet, "<ATTENDEE_NAME" + str(itemcount) + ">")
        pne.replace_cell_tag(sheet, "<ATTENDEE_NAME" + str(itemcount) + ">", attendeelist)
        if cell:
            cell.EntireRow:AutoFit()

        # count expand out excel rows for status report items
        trackeritems = pnc.find_node(notesrow, "table", "name", "ix_item_tracker")
        trackercount = 0
        if trackeritems:
            trackerrow = trackeritems.firstChild()

            while trackerrow:
                trackercount = trackercount + 1
                trackerrow = trackerrow.nextSibling()

        pne.replace_cell_tag(sheet, "<ITEM" + str(itemcount) + ">", "<ITEM>")
        pne.replace_cell_tag(sheet, "<ASSIGNED_TO" + str(itemcount) + ">", "<ASSIGNED_TO>")
        pne.replace_cell_tag(sheet, "<STATUS" + str(itemcount) + ">", "<STATUS>")
        pne.replace_cell_tag(sheet, "<DUE_DATE" + str(itemcount) + ">", "<DUE_DATE>")

        pne.expand_row(sheet, "<ITEM>", trackercount)

        trackercount = 0
        if trackeritems:
            trackerrow = trackeritems.firstChild()

            while trackerrow:
                trackercount = trackercount + 1
                cell = pne.find_cell_tag(sheet, "<ITEM" + str(trackercount) + ">")
                pne.replace_cell_tag(sheet, "<ITEM" + str(trackercount) + ">", pnc.get_column_value(trackerrow, "item_name"))
                if cell:
                    cell.EntireRow.AutoFit()

                pne.replace_cell_tag(sheet, "<ASSIGNED_TO" + str(trackercount) + ">", pnc.get_column_value(trackerrow, "assigned_to"))
                pne.replace_cell_tag(sheet, "<STATUS" + str(trackercount) + ">", pnc.get_column_value(trackerrow, "status"))
                pne.replace_cell_tag(sheet, "<DUE_DATE" + str(trackercount) + ">", pnc.get_column_value(trackerrow, "date_due"))

                trackerrow = trackerrow.nextSibling()

        notesrow = notesrow.nextSibling()


    pne.replace_cell_tag(sheet, "<REPORTDATE>", executedate.toString("MM/dd/yyyy"))
    pne.replace_cell_tag(sheet, "<PROJECTNAME>", projnum + " " + projdes)

    # just in case nothing displayed
    pne.replace_cell_tag(sheet, "<ITEM>", "")
    pne.replace_cell_tag(sheet, "<ASSIGNED_TO>", "")
    pne.replace_cell_tag(sheet, "<STATUS>", "")
    pne.replace_cell_tag(sheet, "<DUE_DATE>", "")

    handle['workbook'].Save()

    progval = progval + 1
    progbar.setValue(min(progval / progtot * 100, 100))
    progbar.setLabelText("Finalizing Excel files...")

    # generate PDFs
    pne.save_excel_as_pdf(handle, sheet, pdfreportname)

    # should we email?
    if noemail == False :
        subject = projnum + " " + projdes + " - " + executedate.toString("MM/dd/yyyy")

    if emailashtml:
        pne.email_excel_html(sheet, subject, "", None)
    elif emailasexcel:
        pne.email_excel_html(sheet, subject, "", excelreportname)
    elif emailaspdf:
        pne.email_excel_html(sheet, subject, "", pdfreportname)

    pne.close_excel_document(handle)

    if keepexcel == False:
        QFile.remove(excelreportname)

    if ui.m_checkBoxDisplayNotes.isChecked():
        QDesktopServices.openUrl(QUrl("file:///" + pdfreportname, QUrl.TolerantMode))

    progbar.setValue(100)
    progbar.setLabelText("Finalizing Excel files...")

    progbar.hide()
    progbar.close()
    progbar = None # must be destroyed


    pne.killexcelautomation()

    return None


# Project Notes Plugin Events
def event_startup(xmlstr):
    return(main_process(xmlstr))

def event_shutdown(xmlstr):
    return(main_process(xmlstr))

def event_everyminute(xmlstr):
    return(main_process(xmlstr))

def event_every5minutes(xmlstr):
    return(main_process(xmlstr))

def event_every10minutes(xmlstr):
    return(main_process(xmlstr))

def event_every30Mmnutes(xmlstr):
    return(main_process(xmlstr))

def event_menuclick(xmlstr):
    return(main_process(xmlstr))

def event_projectrightclick(xmlstr):
    return(main_process(xmlstr))

def event_peoplerightclick(xmlstr):
    return main_process(xmlstr)

def event_clientrightclick(xmlstr):
    return(main_process(xmlstr))

def event_statusreportitemrightclick(xmlstr):
    return(main_process(xmlstr))

def event_teammemberrightclick(xmlstr):
    return(main_process(xmlstr))

def event_locationitemrightclick(xmlstr):
    return(main_process(xmlstr))

def event_meetingrightclick(xmlstr):
    return(main_process(xmlstr))

def event_atteerightclick(xmlstr):
    return(main_process(xmlstr))

def event_trackeritemrightclick(xmlstr):
    return(main_process(xmlstr))

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

#TODO:  Some large XML fields from ProjectNotes 2 break the parser.  For examle an email was pasted into description.  Maybe CDATA tags are needed there.