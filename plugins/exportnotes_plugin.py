# Copyright (C) 2025, 2026 Paul McKinney
import subprocess
import os
import sys

from includes.common import ProjectNotesCommon
from includes.collaboration_tools import CollaborationTools
from includes.graphapi_tools import TokenAPI

from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDate, QDir, QTextStream, QStringConverter, QProcess, QMarginsF, QUrl, QTimer, QRect
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices, QPageLayout, QPageSize
from PyQt6.QtWebEngineWidgets import QWebEngineView

# try and get more logs
os.environ["QTWEBENGINE_CHROMIUM_FLAGS"] = "--enable-logging=stderr --log-level=0"


# Project Notes Plugin Parameters
pluginname = "Export Meeting Notes"
plugindescription = "Generate meeting notes to be exported.  Notes can be exported as HTML or a PDF."

pluginmenus = [
    {"menutitle" : "Meeting Notes", "function" : "menu_export_meeting_notes", "tablefilter" : "projects/project_notes/meeting_attendees/item_tracker/project_locations", "submenu" : "Export", "dataexport" : "projects"},
    {"menutitle" : "Export Notes", "function" : "menu_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""}
]

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

# keep authentication information in memory
# allow authentication calls to use main GUI event loop
tapi = TokenAPI()

# Custom Setting
class ExportNotesSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "Export Meeting Notes"

        self.ui = uic.loadUi("plugins/forms/dialogExportLocation.ui", self)
        self.ui.setWindowTitle("Notes Export Sub Folder")
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
        self.ui.setModal(True)

        self.ui.buttonBox.accepted.connect(self.save_settings)
        self.ui.buttonBox.rejected.connect(self.reject_changes)

        w = pnc.get_plugin_setting("W", self.settings_pluginname)
        h = pnc.get_plugin_setting("H", self.settings_pluginname)

        self.export_subfolder = pnc.get_plugin_setting("ExportSubFolder", self.settings_pluginname)
        self.ui.lineEditExportSubFolder.setText(self.export_subfolder or "")

        if w is not None and h is not None:
            self.ui.resize(int(w), int(h))
        self.center_on_main_window()

    def center_on_main_window(self):
        main_window = QApplication.activeWindow()
        if main_window:
            main_geometry = main_window.geometry()
            x = main_geometry.x() + (main_geometry.width() - self.width()) // 2
            y = main_geometry.y() + (main_geometry.height() - self.height()) // 2
            self.move(max(0, x), max(0, y))

    def save_window_state(self):
        pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # print(f"saving dimensions {self.size().width()},{self.size().height()}")

    def save_settings(self):
        self.export_subfolder = self.ui.lineEditExportSubFolder.text()
        pnc.set_plugin_setting("ExportSubFolder", self.settings_pluginname, self.export_subfolder)

        self.save_window_state()
        self.accept()

    def reject_changes(self):
        self.save_window_state()
        
        # Call the base class implementation
        self.reject()

    def closeEvent(self, event):
        self.save_window_state()

        # Call the base class implementation
        super().closeEvent(event)

def _html_escape(s):
    if s is None:
        return ""
    return (str(s)
        .replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;"))


def generate_notes_html(projnum, projdes, meetings_html, reportdate):
    return f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=780">
<style>
body {{ font-family: Calibri, Arial, sans-serif; font-size: 10pt; margin: 0; padding: 0.2in; }}
h1 {{ font-size: 13pt; color: #1F497D; font-weight: bold; margin-bottom: 8px; }}
.meeting {{ margin-bottom: 16px; }}
.meeting-title {{ font-size: 11pt; font-weight: bold; color: #1F497D; margin-bottom: 4px; }}
.meeting-table {{ border-collapse: collapse; width: 100%; }}
.meeting-table td, .meeting-table th {{ border: 1px solid #808080; padding: 3px 6px; font-size: 10pt; vertical-align: top; }}
.cell-label {{ background-color: #EEECE1; font-weight: bold; text-align: right; white-space: nowrap; width: 80px; }}
.cell-value {{ background-color: #DCE6F1; }}
.cell-header {{ background-color: #EEECE1; font-weight: bold; text-align: center; }}
.cell-notes {{ background-color: #DCE6F1; }}
.col-item {{ width: 55%; }}
.footer {{ font-size: 9pt; color: #555; margin-top: 16px; }}
</style>
</head>
<body>
<h1>Meeting Notes: {_html_escape(projnum)} {_html_escape(projdes)}</h1>
{meetings_html}
<p class="footer">Report Date: {_html_escape(reportdate)}</p>
</body>
</html>"""


def build_meeting_block(meeting_title, meeting_date, attendee_names, meeting_notes, action_items_html):
    action_rows = ""
    if action_items_html:
        action_rows = f"""<tr><td colspan="4" class="cell-header">Action Items</td></tr>
<tr>
<th class="cell-header col-item">Item</th>
<th class="cell-header">Assigned To</th>
<th class="cell-header">Status</th>
<th class="cell-header">Due Date</th>
</tr>
{action_items_html}"""
    return f"""<div class="meeting">
<div class="meeting-title">{_html_escape(meeting_title)}</div>
<table class="meeting-table">
<tr><td class="cell-label">Date:</td><td class="cell-value" colspan="3">{_html_escape(meeting_date)}</td></tr>
<tr><td class="cell-label">Attendees:</td><td class="cell-value" colspan="3">{_html_escape(attendee_names)}</td></tr>
<tr><td colspan="4" class="cell-header">Meeting Notes</td></tr>
<tr><td colspan="4" class="cell-notes">{meeting_notes}</td></tr>
{action_rows}
</table>
</div>
"""


def build_action_item_row(item, assignedto, status, duedate):
    return (f'<tr>'
            f'<td class="cell-value">{_html_escape(item)}</td>'
            f'<td class="cell-value" style="text-align:center">{_html_escape(assignedto)}</td>'
            f'<td class="cell-value" style="text-align:center">{_html_escape(status)}</td>'
            f'<td class="cell-value" style="text-align:center">{_html_escape(duedate)}</td>'
            f'</tr>\n')


class MeetingsExporter(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "Export Meeting Notes"

        self.ui = uic.loadUi("plugins/forms/dialogExportNotesOptions.ui", self)
        self.ui.m_datePickerRptDateNotes.setCalendarPopup(True)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint |
            QtCore.Qt.WindowType.WindowStaysOnTopHint
            )
        self.ui.setModal(True)

        self.set_execute_date(QDate.currentDate())

        #self.ui.buttonBox.accepted.connect(self.export_notes)
        self.ui.pushButtonOK.clicked.connect(self.export_notes)
        self.ui.pushButtonCancel.clicked.connect(self.close_dialog)

        self.xmlstr = None
        self.html_content = None


        # Define page layout (A4, portrait, 20pt margins)
        self.page_layout = QPageLayout()
        self.page_layout.setPageSize(QPageSize(QPageSize.PageSizeId.A4))  # A4 page size
        self.page_layout.setOrientation(QPageLayout.Orientation.Portrait)
        self.page_layout.setMargins(QMarginsF(20, 20, 20, 20))  # Optional margins

        self.web_view = QWebEngineView(self.ui)
        self.web_view.loadFinished.connect(self.on_load_finished)
        self.web_view.page().pdfPrintingFinished.connect(self.pdf_printed)
        self.ui.verticalLayout_2.layout().addWidget(self.web_view)

    def set_execute_date(self, edate):
        self.executedate = edate
        self.ui.m_datePickerRptDateNotes.setDate(self.executedate)

    def close_dialog(self):
        self.hide()

    def on_load_finished(self, success):
        if not success:
            print(f"Failed to process html.")
            return

        print("Html load finished")

        
        self.web_view.page().printToPdf(self.pdfreportname, self.page_layout)

        print("Finished generate_pdf call")

    def set_xml_doc(self, xmlval):
        self.xmlstr = xmlval

    def export_notes(self):
        xmldoc = QDomDocument()
        if (xmldoc.setContent(self.xmlstr) == False):
            QMessageBox.critical(self, "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return ""

        self.project_htmlreportname = ""
        self.project_pdfreportname = ""
        self.htmlreportname = ""
        self.pdfreportname = ""

        self.internalreport = self.ui.m_checkBoxInternalRptNotes.isChecked()
        self.keephtml = self.ui.m_checkBoxHTMLRptNotes.isChecked()
        self.emailasinlinehtml = self.ui.m_radioBoxEmailAsInlineHTML.isChecked()
        self.emailaspdf = self.ui.m_radioBoxEmailAsPDF.isChecked()
        self.emailashtml = self.ui.m_radioBoxEmailAsHTML.isChecked()
        self.noemail = self.ui.m_radioBoxDoNotEmail.isChecked()
        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()

        xmlroot = xmldoc.elementsByTagName("projectnotes").at(0) # get root node
        pm = xmlroot.toElement().attribute("managing_manager_name")

        #print("locating project folder...")
        QtWidgets.QApplication.processEvents()
        projectfolder = pnc.get_projectfolder(xmlroot)
        #print("finding projects table ..")
        print("found project folder: " + projectfolder)
        QtWidgets.QApplication.processEvents()

        self.export_subfolder = pnc.get_plugin_setting("ExportSubFolder", self.settings_pluginname)
        
        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = pnc.get_column_value(projtab.firstChild(), "project_name")

        email = False
        nm = None
        stat = None
        receivers = ""

        if projectfolder:
            projectfolder = f"{projectfolder}/{self.export_subfolder}/"
            if not pnc.folder_exists(projectfolder):
                print(f"Project folder '{projectfolder}' does not exist; output will not be copied there.")
                projectfolder = ""

        self.progbar = QProgressDialog(self)
        self.progbar.setWindowTitle("Exporting...")
        self.progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint 
            )
        self.progbar.setMinimumWidth(350)
        self.progbar.setCancelButton(None)
        self.progbar.setWindowModality(QtCore.Qt.WindowModality.WindowModal)
        self.progbar.show()

        progval = 0
        progtot = 6

        progval = progval + 1
        self.progbar.setValue(int(min(progval / progtot * 100, 100)))
        self.progbar.setLabelText("Copying files...")

        temporaryfolder = pnc.get_temporary_folder() + "/"

        if self.internalreport:
            self.htmlreportname = temporaryfolder + projnum + " Meeting Minutes Internal.html"
            self.pdfreportname = temporaryfolder + projnum + " Meeting Minutes Internal.pdf"
        else:
            self.htmlreportname = temporaryfolder + projnum + " Meeting Minutes.html"
            self.pdfreportname = temporaryfolder + projnum + " Meeting Minutes.pdf"

        meetings_html = ""

        progval = progval + 1
        self.progbar.setValue(int(min(progval / progtot * 100, 100)))
        self.progbar.setLabelText("Gathering notes...")

        # count expand out excell rows for status report items
        notes = pnc.find_node(xmlroot, "table", "name", "project_notes")
        itemcount = 0

        if not notes.isNull():
            notesrow = notes.firstChild()

            while not notesrow.isNull():
                QtWidgets.QApplication.processEvents()
                isinternal = pnc.get_column_value(notesrow, "internal_item")

                if isinternal == "1" and self.internalreport:
                    itemcount = itemcount + 1
                elif isinternal != "1":
                    itemcount = itemcount + 1

                notesrow = notesrow.nextSibling()

            progtot = progtot + itemcount

            progval = progval + 1
            self.progbar.setValue(int(min(progval / progtot * 100, 100)))
            self.progbar.setLabelText("Gathering notes...")

            itemcount = 0
            notesrow = notes.firstChild()

            while not notesrow.isNull():
                QtWidgets.QApplication.processEvents()
                isinternal = pnc.get_column_value(notesrow, "internal_item")
                includeitem = False

                if isinternal == "1" and self.internalreport:
                    includeitem = True
                elif isinternal != "1":
                    includeitem = True

                if includeitem:
                    itemcount = itemcount + 1

                progval = progval + 1
                self.progbar.setValue(int(min(progval / progtot * 100, 100)))
                self.progbar.setLabelText("Gathering notes...")

                note = pnc.get_column_value(notesrow, "note")
                attendees = pnc.find_node(notesrow, "table", "name", "meeting_attendees")
                attendeelist = ""
                if not attendees.isNull():
                    attendeerow = attendees.firstChild()

                    while not attendeerow.isNull():
                        QtWidgets.QApplication.processEvents()
                        if attendeelist != "" :
                            attendeelist = attendeelist + ", "

                        attendeelist = attendeelist + pnc.get_column_value(attendeerow, "name")
                        #print("processing attendees...")
                        attendeerow = attendeerow.nextSibling()

                action_items_html = ""
                trackeritems = pnc.find_node(notesrow, "table", "name", "item_tracker")

                trackercount = 0
                if trackeritems:
                    trackerrow = trackeritems.firstChild()

                    while not trackerrow.isNull():
                        QtWidgets.QApplication.processEvents()
                        trackercount = trackercount + 1

                        action_items_html += build_action_item_row(pnc.get_column_value(trackerrow, "item_name"), pnc.get_column_value(trackerrow, "assigned_to"), pnc.get_column_value(trackerrow, "status"), pnc.get_column_value(trackerrow, "date_due"))

                        trackerrow = trackerrow.nextSibling()

                meetings_html += build_meeting_block(pnc.get_column_value(notesrow, "note_title"), pnc.get_column_value(notesrow, "note_date"), attendeelist, pnc.strip_html_body(note), action_items_html)

                notesrow = notesrow.nextSibling()

            self.html_content = generate_notes_html(projnum, projdes, meetings_html, self.executedate.toString("MM/dd/yyyy"))

            progval = progval + 1
            self.progbar.setValue(int(min(progval / progtot * 100, 100)))
            self.progbar.setLabelText("Finalizing files...")

            # should we email?
            if self.noemail == False :
                self.subject = projnum + " " + projdes + " - " + self.executedate.toString("MM/dd/yyyy")

            QFile.remove(self.htmlreportname)
            QFile.remove(self.pdfreportname)

            file = QFile(self.htmlreportname)
            if file.open(QFile.OpenModeFlag.WriteOnly):
                stream = QTextStream(file)
                stream.setEncoding(QStringConverter.Encoding.Utf8)
                stream << self.html_content
                file.close()
            else:
                print(f"Error attempting to write {self.htmlreportname}")

            # establish final file names and locations (only when project folder is defined)
            if projectfolder:
                if self.internalreport:
                    self.project_htmlreportname = projectfolder + projnum + " Meeting Minutes Internal.html"
                    self.project_pdfreportname = projectfolder + projnum + " Meeting Minutes Internal.pdf"
                else:
                    self.project_htmlreportname = projectfolder + projnum + " Meeting Minutes.html"
                    self.project_pdfreportname = projectfolder + projnum + " Meeting Minutes.pdf"

        print(f"ready to generate pdf from {self.htmlreportname}")
        # call pdf generator
        self.progbar.setLabelText("Finalizing files...")
        self.web_view.load(QUrl.fromLocalFile(self.htmlreportname)) # trigger the load

    def pdf_printed(self, success: bool):
        if success:
            print(f"PDF saved: {self.pdfreportname}")
        else:
            print("Failed to generate PDF")

        ct = CollaborationTools(tapi)

        if self.emailasinlinehtml:
                ct.send_an_email(self.xmlstr, self.subject, self.html_content, None, "", True)
        elif self.emailashtml:
                ct.send_an_email(self.xmlstr, self.subject, "", self.htmlreportname, "", True)
        elif self.emailaspdf:
                ct.send_an_email(self.xmlstr, self.subject, "", self.pdfreportname, "", True)

        if self.project_pdfreportname:
            QFile.remove(self.project_pdfreportname)
            if not QFile(self.pdfreportname).copy(self.project_pdfreportname):
                QMessageBox.critical(self, "Unable to copy generated export", "Could not copy " + self.pdfreportname + " to " + self.project_pdfreportname)

        if self.keephtml == False:
            pass
        elif self.project_htmlreportname:
            QFile.remove(self.project_htmlreportname)
            if not QFile(self.htmlreportname).copy(self.project_htmlreportname):
                QMessageBox.critical(self, "Unable to copy generated export", "Could not copy " + self.htmlreportname + " to " + self.project_htmlreportname)

        if self.ui.m_checkBoxDisplayNotes.isChecked():
            display_path = self.project_pdfreportname if self.project_pdfreportname else self.pdfreportname
            try:
                print(f"Attempting to open {display_path}")
                self.open_pdf(display_path)
            except:
                print(f"An error occurred trying to open {display_path}")
                pass

        if not self.progbar is None:
            self.progbar.setValue(100)

            self.progbar.hide()
            self.progbar.close()
            self.progbar = None # must be destroyed

        QtWidgets.QApplication.restoreOverrideCursor()

        self.hide()

    def open_pdf(self, path):
        if sys.platform.startswith("linux"):
            try:
                # Most reliable on modern Linux distros
                subprocess.run(["xdg-open", path], check=True)
                return True
            except (FileNotFoundError, subprocess.CalledProcessError):
                # Fallback if xdg-open is missing/broken
                try:
                    subprocess.run(["gio", "open", path], check=True)
                    return True
                except:
                    pass

        # Normal Qt way (good on Windows/macOS, sometimes works on Linux)
        url = QUrl.fromLocalFile(path)
        return QDesktopServices.openUrl(url)

# processing main def
def menu_export_meeting_notes(xmlstr, parameter):
    mex.set_xml_doc(xmlstr)

    QtWidgets.QApplication.restoreOverrideCursor()
    QtWidgets.QApplication.processEvents()   

    mex.set_execute_date(QDate.currentDate())

    print("Calling web view show.")

    mex.show()

    print("Finished calling web view show.")

    return ""

def menu_settings(parameter):
    ens.show()
    return ""

def setup_default_export_notes_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "Export Meeting Notes"
    if pnc_tmp.get_plugin_setting("ExportSubFolder", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("ExportSubFolder", settings_pluginname, "Project Management/Meeting Minutes")

# keep the instance of windows open to avoid sending the main app a close message
pnc = None
mex = None
ens = None

#setup test data
if __name__ == '__main__':
    import os
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)
    pnc = ProjectNotesCommon()
    mex = MeetingsExporter()
    ens = ExportNotesSettings()

    xml_content = ""
    with open("/home/paulmckinney/Desktop/project.xml", 'r', encoding='utf-8') as file:
        xml_content = file.read()
    mex.set_xml_doc(xml_content)
    mex.show()

    # html = generate_header("P400", "Project Name")
    # html += generate_meeting_header_row("Title", "1/1/2001", "Bob, Sam, Jill", "This is my really long note.") 
    # html += generate_action_item_row("Item Description", "Bob Smith", "Assigned", "4/3/2005") 
    # html += generate_action_item_row("Item Description", "Bob Smith", "Assigned", "4/3/2005") 
    # html += generate_meeting_footer_row()
    # html += generate_footer("1/1/2010")

    # menu_export_meeting_notes("","")

    app.exec()
else:
    setup_default_export_notes_settings()
    pnc = ProjectNotesCommon()
    mex = MeetingsExporter(pnc.get_main_window())
    ens = ExportNotesSettings()
