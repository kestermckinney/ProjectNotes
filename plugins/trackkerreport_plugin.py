import sys
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDate, QUrl, QDir, QFileInfo
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Generate Tracker Report"
plugindescription = "Generate a tracker report based on the options selected."
pluginmenus = []

# events must have a data structure and data view specified
#
# Structures:
#      string          The event will pass a python string when dataexport is defined containing XML. 
#                      The plugin can return an XML string to be processed by ProjectNotes.
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


class GenerateTrackerReport(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.ui = uic.loadUi("plugins/forms/dialogTrackerRptOptions.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )

        self.ui.m_checkBoxDisplayTracker.setChecked(False)
        self.ui.m_checkBoxItemsTracker.setChecked(True)
        self.ui.m_checkBoxNewTracker.setChecked(True)
        self.ui.m_checkBoxAssignedTracker.setChecked(True)

        self.ui.setModal(True)

        self.ui.pushButtonOK.clicked.connect(self.generate_tracker)
        self.ui.pushButtonCancel.clicked.connect(self.close_dialog)

        self.xmlstr = None

    def close_dialog(self):
        self.hide()

    def set_xml_doc(self, xmlval):
        self.xmlstr = xmlval

    def generate_tracker(self):
        pne = ProjectNotesExcelTools()

        xmlval = QDomDocument()
        if (xmlval.setContent(self.xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return
            
        emaillist = ""

        executedate = QDate.currentDate()
        internalreport = False
        keepexcel = False
        emailashtml = False
        emailaspdf = False
        emailasexcel = False
        noemail = True
        isinternal = False

        internalreport = self.ui.m_checkBoxInternalRptTracker.isChecked()
        keepexcel = self.ui.m_checkBoxExcelRptTracker.isChecked()

        emailashtml = self.ui.m_radioBoxEmailAsHTML.isChecked()
        emailaspdf = self.ui.m_radioBoxEmailAsPDF.isChecked()
        emailasexcel = self.ui.m_radioBoxEmailAsExcel.isChecked()
        noemail = self.ui.m_radioBoxDoNotEmail.isChecked()

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        
        pm = xmlroot.toElement().attribute("managing_manager_name")
        co = xmlroot.toElement().attribute("managing_company_name")

        check_tag = pnc.find_node_by2(xmlroot, "table", "name", "item_tracker", "filter_field_1", "project_id")
        check_row = None
        if check_tag:
            check_row = check_tag.firstChild()

        if not check_row or not check_tag:
            QMessageBox.warning(None, "No Records", "No tracker or action items are available.", QMessageBox.StandardButton.Ok)
            self.ui.hide()
            return

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
                self.ui.hide()
                return
        else:
            projectfolder = projectfolder + "/Project Management/Issues List/"

        progbar = QProgressDialog(self.ui)
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
        if not teammember.isNull():
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

        if self.ui.m_checkBoxInternalRptTracker.isChecked():
            excelreportname = projectfolder + projnum + " Tracker Report Internal.xlsx"
            pdfreportname = projectfolder + projnum + " Tracker Report Internal.pdf"
        else:
            excelreportname = projectfolder + projnum + " Tracker Report.xlsx"
            pdfreportname = projectfolder + projnum + " Tracker Report.pdf"

        templatefile ="plugins/templates/Tracker Items Template.xlsx"
        QFile.remove(excelreportname)
        if not QFile.copy(templatefile, excelreportname):
            QMessageBox.critical(None, "Unable to copy template", "Could not copy " + templatefile + " to " + excelreportname, QMessageBox.StandardButton.Cancel)
            progbar.close()
            return

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
                if isinternal == "1" and self.ui.m_checkBoxInternalRptTracker.isChecked():
                    includeiteminternal = True
                elif isinternal == "0" or isinternal == "":
                    includeiteminternal = True

                # determine item type to include
                if itemtype == "Tracker" and self.ui.m_checkBoxItemsTracker.isChecked():
                    includeitemtype = True

                if itemtype == "Action" and self.ui.m_checkBoxActionTracker.isChecked():
                    includeitemtype = True

                # determine if it is an included status
                if status == "New" and self.ui.m_checkBoxNewTracker.isChecked():
                    includeitemstatus = True

                if status == "Assigned" and self.ui.m_checkBoxAssignedTracker.isChecked():
                    includeitemstatus = True

                if status == "Defered" and self.ui.m_checkBoxDeferedTracker.isChecked():
                    includeitemstatus = True

                if status == "Resolved" and self.ui.m_checkBoxResolvedTracker.isChecked():
                    includeitemstatus = True

                if status == "Cancelled" and ui.m_checkBoxCancelledTracker.isChecked():
                    includeitemstatus = True

                if includeiteminternal and includeitemtype and includeitemstatus:
                    itemcount = itemcount + 1

                repitemrow = repitemrow.nextSibling()

        #don't show the internal column on customer version
        if self.ui.m_checkBoxInternalRptTracker.isChecked() == False:
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
            if isinternal == "1" and self.ui.m_checkBoxInternalRptTracker.isChecked():
                includeiteminternal = True
            elif isinternal == "0" or isinternal == "":
                includeiteminternal = True

            # determine item type to include
            if itemtype == "Tracker" and self.ui.m_checkBoxItemsTracker.isChecked():
                includeitemtype = True

            if itemtype == "Action" and self.ui.m_checkBoxActionTracker.isChecked():
                includeitemtype = True

            # determine if it is an included status
            if status == "New" and self.ui.m_checkBoxNewTracker.isChecked():
                includeitemstatus = True

            if status == "Assigned" and self.ui.m_checkBoxAssignedTracker.isChecked():
                includeitemstatus = True

            if status == "Defered" and self.ui.m_checkBoxDeferedTracker.isChecked():
                includeitemstatus = True

            if status == "Resolved" and self.ui.m_checkBoxResolvedTracker.isChecked():
                includeitemstatus = True

            if status == "Cancelled" and self.ui.m_checkBoxCancelledTracker.isChecked():
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

                if not trackerupdates.isNull():
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
        progbar.setLabelText("Finalizing Excel files...")

        # generate PDFs
        pne.save_excel_as_pdf(handle, sheet, pdfreportname)

        ## testing why does window close
        progbar.close()
        self.ui.hide()
        return

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

        progbar.setValue(100)

        if self.ui.m_checkBoxDisplayTracker.isChecked():
            QDesktopServices.openUrl(QUrl("file:///" + pdfreportname))

        progbar = None # must be destroyed

        if keepexcel == False:
            QFile.remove(excelreportname)

        self.ui.hide()

# keep the instance of windows open to avoid sending the main app a close message
pnc = None
gtr = None

#setup test data
if __name__ == '__main__':
    import os
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)
    pnc = ProjectNotesCommon()
    gtr = GenerateTrackerReport()

    xml_content = ""
    with open("C:\\Users\\pamcki\\OneDrive - Cornerstone Controls\\Documents\\Work In Progress\\XML\\project.xml", 'r', encoding='utf-8') as file:
        xml_content = file.read()
    gtr.set_xml_doc(xml_content)
    gtr.show()

    app.exec()
else:
    pnc = ProjectNotesCommon()
    gtr = GenerateTrackerReport(pnc.get_main_window())


def menuGenerateTrackerReport(xmlstr, parameter):        
    gtr.set_xml_doc(xmlstr)

    QtWidgets.QApplication.restoreOverrideCursor()
    QtWidgets.QApplication.processEvents()   

    #TODO: maybe we don't want to have project notes set the cursor before going into the plugin

    gtr.show()

    return ""

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pluginmenus.append({"menutitle" : "Generate Tracker Report", "function" : "menuGenerateTrackerReport", "tablefilter" : "projects/item_tracker/item_tracker_updates/project_locations/project_people", "submenu" : "", "dataexport" : "projects"})
