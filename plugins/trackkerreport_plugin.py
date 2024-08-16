import sys
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt6.QtSql import QSqlDatabase
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDate, QUrl, QDir, QFileInfo
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Generate Tracker Report"
plugindescription = "Generate a tracker report based on the options selected."
plugintable = "projects" # the table or view that the plugin applies to.  This will enable the right click
childtablesfilter = "projects/item_tracker/item_tracker_updates/project_locations/project_people" # a list of child tables that can be sent to the plugin.  This will be used to exclude items like notes or action items when they aren't used

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

        
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""
            
        emaillist = ""

        # setup global variable
        ProjectsFolder = pnc.get_plugin_setting("ProjectsFolder")

        executedate = QDate.currentDate()
        internalreport = False
        keepexcel = False
        emailashtml = False
        emailaspdf = False
        emailasexcel = False
        noemail = True
        isinternal = False

        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()   

        ui = uic.loadUi("plugins/includes/dialogTrackerRptOptions.ui")
        ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )

        ui.m_checkBoxDisplayTracker.setChecked(True)
        ui.m_checkBoxItemsTracker.setChecked(True)
        ui.m_checkBoxNewTracker.setChecked(True)
        ui.m_checkBoxAssignedTracker.setChecked(True)

        if ui.exec():
            internalreport = ui.m_checkBoxInternalRptTracker.isChecked()
            keepexcel = ui.m_checkBoxExcelRptTracker.isChecked()

            emailashtml = ui.m_radioBoxEmailAsHTML.isChecked()
            emailaspdf = ui.m_radioBoxEmailAsPDF.isChecked()
            emailasexcel = ui.m_radioBoxEmailAsExcel.isChecked()
            noemail = ui.m_radioBoxDoNotEmail.isChecked()
        else:
            QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
            QtWidgets.QApplication.processEvents()
            return ""

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        
        pm = xmlroot.toElement().attribute("managing_manager_name")
        co = xmlroot.toElement().attribute("managing_company_name")

        check_tag = pnc.find_node_by2(xmlroot, "table", "name", "item_tracker", "filter_field_1", "project_id")
        check_row = None
        if check_tag:
            check_row = check_tag.firstChild()

        if not check_row or not check_tag:
            QMessageBox.warning(None, "No Records", "No tracker or action items are available.", QMessageBox.StandardButton.Ok)
            QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
            QtWidgets.QApplication.processEvents()
            return ""

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = pnc.get_column_value(projtab.firstChild(), "project_name")

        email = None
        nm = None
        company = None
        receivers = ""

        projectfolder = pnc.get_projectfolder(xmlroot)

        if (projectfolder is None or projectfolder ==""):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
                QtWidgets.QApplication.processEvents()
                return ""
        else:
            projectfolder = projectfolder + "\\Issues List\\"

        progbar = QProgressDialog()
        progbar.setWindowTitle("Generating Report...")
        progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )

        progbar.setMinimumWidth(350)
        progbar.setCancelButton(None)
        progbar.show()
        progval = 0
        progtot = 7


        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("looking up recipients...")

        teammember = pnc.find_node(xmlroot, "table", "name", "project_people")
        if teammember:
            memberrow = teammember.firstChild()

            while not memberrow.isNull():
                nm = pnc.get_column_value(memberrow, "name")
                email = pnc.get_column_value(memberrow, "email")
                company = pnc.get_column_value(memberrow, "client_name")

                if nm != pm:
                    if (email != None and email != "" and  ( (internalreport and company == co) or not internalreport )):
                        if receivers != "":
                            receivers = receivers + ", "
                            emaillist = emaillist + ";"

                        receivers = receivers + nm
                        emaillist = emaillist + email

                memberrow = memberrow.nextSibling()


        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Copying files...")

        excelreportname = ""
        pdfreportname = ""

        if ui.m_checkBoxInternalRptTracker.isChecked():
            excelreportname = projectfolder + projnum + " Tracker Report Internal.xlsx"
            pdfreportname = projectfolder + projnum + " Tracker Report Internal.pdf"
        else:
            excelreportname = projectfolder + projnum + " Tracker Report.xlsx"
            pdfreportname = projectfolder + projnum + " Tracker Report.pdf"

        templatefile ="plugins\\templates\\Tracker Items Template.xlsx"
        QFile.remove(excelreportname)
        if not QFile.copy(templatefile, excelreportname):
            QMessageBox.critical(None, "Unable to copy template", "Could not copy " + templatefile + " to " + excelreportname, QMessageBox.StandardButton.Cancel)
            QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
            QtWidgets.QApplication.processEvents()
            return ""


        handle = pne.open_excel_document(excelreportname)
        sheet = handle['workbook'].Sheets("Item Tracker")

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Gathering status items...")

        # count expand out excel rows for status report items
        repitem = pnc.find_node_by2(xmlroot, "table", "name", "item_tracker", "filter_field_1", "project_id")
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

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
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
                trackerupdates = pnc.find_node(repitemrow, "table", "name", "item_tracker_updates")

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

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))

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
        progbar.setValue(int(min(progval / progtot * 100, 100)))
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
            QDesktopServices.openUrl(QUrl("file:///" + pdfreportname))

        progbar.setValue(100)
        progbar.setLabelText("Finalizing Excel files...")
        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed

        if keepexcel == False:
            QFile.remove(excelreportname)

        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()

        return ""
        
# setup test data
"""
print("Buld up QDomDocument")
#

xmldoc = QDomDocument("TestDocument")
f = QFile("exampleproject.xml")

if f.open(QIODevice.OpenModeFlag.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()
main_process(xmldoc)
"""
