import sys
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com
    from win32 import win32api

from includes.common import ProjectNotesCommon
from PyQt6 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt6.QtSql import QSqlDatabase
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDate, QUrl, QDir
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices, QTextDocument

# Project Notes Plugin Parameters
pluginname = "Archive Meeting Notes"
plugindescription = "Generate meeting notes archive based on the options selected."
plugintable = "projects" # the table or view that the plugin applies to.  This will enable the right click
childtablesfilter = "projects/project_notes/meeting_attendees/item_tracker/project_locations" # a list of child tables that can be sent to the plugin.  This will be used to exclude items like notes or action items when they aren't used

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

    # make global to pass between sections

    emaillist = ""

    # processing main def
    def event_data_rightclick(xmlstr):
        print("called event: " + __file__)

        #print(xmlstr) # debug output
        
        
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""

        if not pnc.verify_global_settings():
            return ""

        # setup global variables
        ProjectsFolder = pnc.get_plugin_setting("ProjectsFolder")

        executedate = QDate.currentDate()
        internalreport = False
        keepexcel = False
        emailashtml = False
        emailaspdf = False
        emailasexcel = False
        noemail = True

        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()   

        ui = uic.loadUi("plugins/includes/dialogNotesArchiveOptions.ui")
        ui.m_datePickerRptDateNotes.setDate(executedate)
        ui.m_datePickerRptDateNotes.setCalendarPopup(True)
        ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint |
            QtCore.Qt.WindowType.WindowStaysOnTopHint
            )

        if ui.exec():
            internalreport = ui.m_checkBoxInternalRptNotes.isChecked()
            keepexcel = ui.m_checkBoxExcelRptNotes.isChecked()
            emailashtml = ui.m_radioBoxEmailAsHTML.isChecked()
            emailaspdf = ui.m_radioBoxEmailAsPDF.isChecked()
            emailasexcel = ui.m_radioBoxEmailAsExcel.isChecked()
            noemail = ui.m_radioBoxDoNotEmail.isChecked()
            QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
            QtWidgets.QApplication.processEvents()
        else:
            QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
            QtWidgets.QApplication.processEvents()
            return ""

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        pm = xmlroot.toElement().attribute("managing_manager_name")

        #print("locating project folder...")
        QtWidgets.QApplication.processEvents()
        projectfolder = pnc.get_projectfolder(xmlroot)
        #print("finding projects table ..")
        print("found project folder: " + projectfolder)
        QtWidgets.QApplication.processEvents()
        
        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = pnc.get_column_value(projtab.firstChild(), "project_name")

        email = False
        nm = None
        stat = None
        receivers = ""

        if (projectfolder is None or projectfolder ==""):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""
        else:
            projectfolder = projectfolder + "/Meeting Minutes/"

        projectfolder = projectfolder + "/"

        progbar = QProgressDialog()
        progbar.setWindowTitle("Archiving...")
        progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint 
            )
        progbar.setMinimumWidth(350)
        progbar.setCancelButton(None)
        progbar.show()

        progval = 0
        progtot = 6
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Generating Report...")

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Copying files...")

        excelreportname = ""
        pdfreportname = ""

        if internalreport:
            excelreportname = projectfolder + projnum + " Meeting Minutes Internal.xlsx"
            pdfreportname = projectfolder + projnum + " Meeting Minutes Internal.pdf"
        else:
            excelreportname = projectfolder + projnum + " Meeting Minutes.xlsx"
            pdfreportname = projectfolder + projnum + " Meeting Minutes.pdf"


        templatefile = "plugins/templates/Meeting Template.xlsx"
        QFile.remove(excelreportname)

        if not QFile.copy(templatefile, excelreportname):
            QMessageBox.critical(None, "Unable to copy template", "Could not copy " + templatefile + " to " + excelreportname, QMessageBox.StandardButton.Cancel)
            return ""

        handle = pne.open_excel_document(excelreportname)
        sheet = handle['workbook'].Sheets("Meeting Notes")

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Gathering notes...")


        # count expand out excell rows for status report items
        notes = pnc.find_node(xmlroot, "table", "name", "project_notes")
        itemcount = 0

        if notes:
            notesrow = notes.firstChild()

            while not notesrow.isNull():
                QtWidgets.QApplication.processEvents()
                isinternal = pnc.get_column_value(notesrow, "internal_item")

                if isinternal == "1" and internalreport:
                    itemcount = itemcount + 1
                elif isinternal != "1":
                    itemcount = itemcount + 1

                notesrow = notesrow.nextSibling()

            pne.expand_section(sheet, "<MEETINGTOP>", "<MEETINGBOTTOM>", itemcount)
            progtot = progtot + itemcount

            progval = progval + 1
            progbar.setValue(int(min(progval / progtot * 100, 100)))
            progbar.setLabelText("Gathering notes...")

        itemcount = 0
        notesrow = notes.firstChild()

        while not notesrow.isNull():
            QtWidgets.QApplication.processEvents()
            isinternal = pnc.get_column_value(notesrow, "internal_item")
            includeitem = False

            if isinternal == "1" and internalreport:
                includeitem = True
            elif isinternal != "1":
                includeitem = True

            if includeitem:
                itemcount = itemcount + 1

            progval = progval + 1
            progbar.setValue(int(min(progval / progtot * 100, 100)))
            progbar.setLabelText("Gathering notes...")

            pne.replace_cell_tag(sheet, "<MEETING_TITLE" + str(itemcount) + ">", "'" + pnc.get_column_value(notesrow, "note_title"))
            pne.replace_cell_tag(sheet, "<MEETING_DATE" + str(itemcount) + ">", pnc.get_column_value(notesrow, "note_date"))
            # replace feature doesn't like long text values
            # expand the row so notes show

            # EXCEL WON'T AUTOFIT THIS FOR SOME REASON I BELIEVE IT IS BECAUSE THE CELLS ARE MERGED
            #print("replaced title and meeting date....")

            note = pnc.get_column_value(notesrow, "note")
            doc = QTextDocument()
            doc.setHtml(note)
            pne.set_cell_by_tag(sheet, "<MEETING_NOTES" + str(itemcount) + ">", "'" + doc.toPlainText() )
            """
            if cell :
              cell.EntireRow:AutoFit()

            """
            #print("going to look for attendees...")

            attendees = pnc.find_node(notesrow, "table", "name", "meeting_attendees")
            attendeelist = ""
            if attendees:
                attendeerow = attendees.firstChild()

                while not attendeerow.isNull():
                    QtWidgets.QApplication.processEvents()
                    if attendeelist != "" :
                        attendeelist = attendeelist + ", "

                    attendeelist = attendeelist + pnc.get_column_value(attendeerow, "name")
                    #print("processing attendees...")
                    attendeerow = attendeerow.nextSibling()

            # expand the row so attees show
            cell = pne.find_cell_tag(sheet, "<ATTENDEE_NAME" + str(itemcount) + ">")
            pne.replace_cell_tag(sheet, "<ATTENDEE_NAME" + str(itemcount) + ">", attendeelist)
            if cell:
                cell.EntireRow:AutoFit()

            #print("Going threw items....")
            # count expand out excel rows for status report items
            trackeritems = pnc.find_node(notesrow, "table", "name", "item_tracker")
            #print("found a tracker table...")
            #print("gathering notes... for note_id " + pnc.get_column_value(notesrow, "note_id"))
            trackercount = 0
            if trackeritems:
                trackerrow = trackeritems.firstChild()

                while not trackerrow.isNull():
                    QtWidgets.QApplication.processEvents()
                    trackercount = trackercount + 1
                    #print("counting " + pnc.get_column_value(trackerrow, "item_id"))
                    trackerrow = trackerrow.nextSibling()

            #print("replacing item and status tags...")
            pne.replace_cell_tag(sheet, "<ITEM" + str(itemcount) + ">", "<ITEM>")
            pne.replace_cell_tag(sheet, "<ASSIGNED_TO" + str(itemcount) + ">", "<ASSIGNED_TO>")
            pne.replace_cell_tag(sheet, "<STATUS" + str(itemcount) + ">", "<STATUS>")
            pne.replace_cell_tag(sheet, "<DUE_DATE" + str(itemcount) + ">", "<DUE_DATE>")

            pne.expand_row(sheet, "<ITEM>", trackercount)

            trackercount = 0
            if trackeritems:
                trackerrow = trackeritems.firstChild()

                while not trackerrow.isNull():
                    QtWidgets.QApplication.processEvents()
                    trackercount = trackercount + 1
                    #print("processing tracker rows")
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
        progbar.setValue(int(min(progval / progtot * 100, 100)))
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
            QDesktopServices.openUrl(QUrl("file:///" + pdfreportname))

        progbar.setValue(100)
        progbar.setLabelText("Finalizing Excel files...")

        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed

        return ""

# setup test data
"""
import sys
print("Buld up QDomDocument")

app = QApplication(sys.argv)
xmldoc = QDomDocument("TestDocument")
f = QFile("C:/Users/pamcki/Desktop/project.xml")

if f.open(QIODevice.OpenModeFlag.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

event_data_rightclick(xmldoc.toString())
"""
