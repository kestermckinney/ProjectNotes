# Copyright (C) 2026 Paul McKinney
import os
import subprocess
import sys

from includes.common import ProjectNotesCommon
from includes.collaboration_tools import CollaborationTools
from includes.graphapi_tools import TokenAPI

from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QDate, QDir, QTextStream, QStringConverter, QMarginsF, QUrl, QRect
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices, QPageLayout, QPageSize
from PyQt6.QtWebEngineWidgets import QWebEngineView

os.environ["QTWEBENGINE_CHROMIUM_FLAGS"] = "--enable-logging=stderr --log-level=0"

tapi = TokenAPI()

# Project Notes Plugin Parameters
pluginname = "Export Tracker Items"
plugindescription = "Export tracker items for a project to HTML and PDF."

pluginmenus = [
    {"menutitle" : "Tracker Items", "function" : "menu_export_tracker_items", "tablefilter" : "projects/item_tracker/item_tracker_updates/project_locations/project_people", "submenu" : "Export", "dataexport" : "projects"},
    {"menutitle" : "Export Tracker Items", "function" : "menu_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""}
]


class ExportTrackerItemsSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "Export Tracker Items"

        self.ui = uic.loadUi("plugins/forms/dialogExportLocation.ui", self)
        self.ui.setWindowTitle("Tracker Items Export Sub Folder")
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
        )
        self.ui.setModal(True)

        self.ui.buttonBox.accepted.connect(self.save_settings)
        self.ui.buttonBox.rejected.connect(self.reject_changes)

        x = pnc.get_plugin_setting("X", self.settings_pluginname)
        y = pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = pnc.get_plugin_setting("W", self.settings_pluginname)
        h = pnc.get_plugin_setting("H", self.settings_pluginname)

        self.export_subfolder = pnc.get_plugin_setting("ExportSubFolder", self.settings_pluginname)
        self.ui.lineEditExportSubFolder.setText(self.export_subfolder or "")

        if x is not None and y is not None and w is not None and h is not None:
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

    def save_window_state(self):
        pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

    def save_settings(self):
        self.export_subfolder = self.ui.lineEditExportSubFolder.text()
        pnc.set_plugin_setting("ExportSubFolder", self.settings_pluginname, self.export_subfolder)
        self.save_window_state()
        self.accept()

    def reject_changes(self):
        self.save_window_state()
        self.reject()

    def closeEvent(self, event):
        self.save_window_state()
        super().closeEvent(event)


def _html_escape(s):
    return (s or "").replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def generate_tracker_html(projnum, projdes, items_html, reportdate, show_internal):
    """Build the complete HTML document for the tracker items report."""
    return f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=1024">
<style>
body {{
    font-family: Calibri, Arial, sans-serif;
    font-size: 9pt;
    margin: 0;
    padding: 0.2in;
}}
h1 {{
    font-size: 13pt;
    color: #1F497D;
    font-weight: bold;
    margin-bottom: 6px;
}}
table {{
    border-collapse: collapse;
    width: 100%;
    table-layout: fixed;
}}
th {{
    background-color: #EEECE1;
    border: 1px solid #808080;
    padding: 3px 4px;
    text-align: center;
    font-size: 7.5pt;
    font-weight: bold;
    vertical-align: middle;
    word-wrap: break-word;
}}
td {{
    background-color: #DCE6F1;
    border: 1px solid #808080;
    padding: 3px 4px;
    vertical-align: top;
    font-size: 7.5pt;
    word-wrap: break-word;
    white-space: pre-wrap;
}}
.col-id        {{ width: 3%; text-align: center; }}
.col-item      {{ width: 11%; }}
.col-identby   {{ width: 7%; }}
.col-dateident {{ width: 6%; text-align: center; }}
.col-desc      {{ width: 13%; }}
.col-assigned  {{ width: 7%; }}
.col-priority  {{ width: 5%; text-align: center; }}
.col-status    {{ width: 6%; text-align: center; }}
.col-duedate   {{ width: 6%; text-align: center; }}
.col-lastupd   {{ width: 6%; text-align: center; }}
.col-resolved  {{ width: 6%; text-align: center; }}
.col-comments  {{ width: 21%; }}
.col-int       {{ width: 3%; text-align: center; }}

.row-tracker   {{ background-color: #DCE6F1; }}
.row-action    {{ background-color: #DCE6F1; }}

.priority-high   {{ color: #C00000; font-weight: bold; }}
.priority-medium {{ color: #9C6500; font-weight: bold; }}
.priority-low    {{ color: #375623; font-weight: bold; }}

.footer {{
    margin-top: 8px;
    font-size: 7.5pt;
    color: #666;
}}
</style>
</head>
<body>
<h1>Item Tracker: {_html_escape(projnum)} {_html_escape(projdes)}</h1>
<table>
<thead>
<tr>
  <th class="col-id">ID</th>
  <th class="col-item">Item</th>
  <th class="col-identby">Identified By</th>
  <th class="col-dateident">Date Identified</th>
  <th class="col-desc">Description</th>
  <th class="col-assigned">Assigned To</th>
  <th class="col-priority">Priority</th>
  <th class="col-status">Status</th>
  <th class="col-duedate">Due Date</th>
  <th class="col-lastupd">Last Update</th>
  <th class="col-resolved">Date Resolved</th>
  <th class="col-comments">Comments/Resolution</th>
  {'<th class="col-int">Int</th>' if show_internal else ''}
</tr>
</thead>
<tbody>
{items_html}
</tbody>
</table>
<p class="footer">Report Date: {_html_escape(reportdate)}</p>
</body>
</html>"""


def build_item_row(item_number, item_name, identified_by, date_identified,
                   description, assigned_to, priority, status,
                   date_due, last_update, date_resolved, comments,
                   is_internal, item_type, show_internal):
    """Build a single HTML table row for a tracker item."""
    row_class = "row-action" if item_type == "Action" else "row-tracker"

    if priority == "High":
        priority_class = "priority-high"
    elif priority == "Medium":
        priority_class = "priority-medium"
    else:
        priority_class = "priority-low"

    internal_display = "Y" if is_internal == "1" else "N"

    return (
        f'<tr class="{row_class}">'
        f'<td class="col-id">{_html_escape(item_number)}</td>'
        f'<td class="col-item">{_html_escape(item_name)}</td>'
        f'<td class="col-identby">{_html_escape(identified_by)}</td>'
        f'<td class="col-dateident">{_html_escape(date_identified)}</td>'
        f'<td class="col-desc">{_html_escape(description)}</td>'
        f'<td class="col-assigned">{_html_escape(assigned_to)}</td>'
        f'<td class="col-priority"><span class="{priority_class}">{_html_escape(priority)}</span></td>'
        f'<td class="col-status">{_html_escape(status)}</td>'
        f'<td class="col-duedate">{_html_escape(date_due)}</td>'
        f'<td class="col-lastupd">{_html_escape(last_update)}</td>'
        f'<td class="col-resolved">{_html_escape(date_resolved)}</td>'
        f'<td class="col-comments">{_html_escape(comments)}</td>'
        + (f'<td class="col-int">{internal_display}</td>' if show_internal else "")
        + '</tr>\n'
    )


class TrackerItemsExporter(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "Export Tracker Items"

        self.ui = uic.loadUi("plugins/forms/dialogExportTrackerOptions.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint |
            QtCore.Qt.WindowType.WindowStaysOnTopHint
        )

        # Default states
        self.ui.m_checkBoxDisplayTracker.setChecked(False)
        self.ui.m_checkBoxItemsTracker.setChecked(True)
        self.ui.m_checkBoxNewTracker.setChecked(True)
        self.ui.m_checkBoxAssignedTracker.setChecked(True)

        self.ui.setModal(True)

        self.ui.pushButtonOK.clicked.connect(self.export_tracker)
        self.ui.pushButtonCancel.clicked.connect(self.close_dialog)

        self.xmlstr = None
        self.html_content = None
        self.progbar = None

        # Landscape Letter page layout for the wide table
        self.page_layout = QPageLayout()
        self.page_layout.setPageSize(QPageSize(QPageSize.PageSizeId.Letter))
        self.page_layout.setOrientation(QPageLayout.Orientation.Landscape)
        self.page_layout.setMargins(QMarginsF(12, 12, 12, 12))

        # Web view embedded in the dialog for PDF generation.
        # Must be visible in the widget hierarchy — a hidden QWebEngineView
        # will not fire loadFinished or complete printToPdf.
        self.web_view = QWebEngineView(self.ui)
        self.ui.verticalLayout_5.addWidget(self.web_view)
        self.web_view.loadFinished.connect(self.on_load_finished)
        self.web_view.page().pdfPrintingFinished.connect(self.pdf_printed)

    def close_dialog(self):
        self.hide()

    def set_xml_doc(self, xmlval):
        self.xmlstr = xmlval

    def _is_item_included(self, repitemrow):
        isinternal = pnc.get_column_value(repitemrow, "internal_item")
        itemtype = pnc.get_column_value(repitemrow, "item_type")
        status = pnc.get_column_value(repitemrow, "status")

        includeiteminternal = (isinternal == "1" and self.ui.m_checkBoxInternalRptTracker.isChecked()) or \
                              (isinternal == "0" or isinternal == "")

        includeitemtype = (itemtype == "Tracker" and self.ui.m_checkBoxItemsTracker.isChecked()) or \
                          (itemtype == "Action" and self.ui.m_checkBoxActionTracker.isChecked())

        includeitemstatus = (status == "New" and self.ui.m_checkBoxNewTracker.isChecked()) or \
                            (status == "Assigned" and self.ui.m_checkBoxAssignedTracker.isChecked()) or \
                            (status == "Defered" and self.ui.m_checkBoxDeferedTracker.isChecked()) or \
                            (status == "Resolved" and self.ui.m_checkBoxResolvedTracker.isChecked()) or \
                            (status == "Cancelled" and self.ui.m_checkBoxCancelledTracker.isChecked())

        return includeiteminternal and includeitemtype and includeitemstatus, isinternal, itemtype, status

    def on_load_finished(self, success):
        if not success:
            print("Failed to load HTML for PDF generation.")
            return

        print("HTML load finished, generating PDF...")

        

        # Print directly — viewport width is set in the HTML meta tag so
        # the table lays out at the correct printable width. JS transform
        # scaling is not used here because printToPdf ignores CSS transforms
        # and renders based on the actual viewport/layout width.
        self.web_view.page().printToPdf(self.pdfreportname, self.page_layout)

        print("Finished generate_pdf call")

    def export_tracker(self):
        xmldoc = QDomDocument()
        if not xmldoc.setContent(self.xmlstr):
            QMessageBox.critical(self, "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return

        self.internalreport = self.ui.m_checkBoxInternalRptTracker.isChecked()
        self.keephtml = self.ui.m_checkBoxHTMLRptTracker.isChecked()
        self.emailasinlinehtml = self.ui.m_radioBoxEmailAsInlineHTML.isChecked()
        self.emailaspdf = self.ui.m_radioBoxEmailAsPDF.isChecked()
        self.emailashtml = self.ui.m_radioBoxEmailAsHTML.isChecked()
        self.noemail = self.ui.m_radioBoxDoNotEmail.isChecked()

        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()

        xmlroot = xmldoc.elementsByTagName("projectnotes").at(0)

        repitem = pnc.find_node(xmlroot, "table", "name", "item_tracker")
        if not repitem or repitem.isNull() or repitem.firstChild().isNull():
            QtWidgets.QApplication.restoreOverrideCursor()
            QMessageBox.warning(self, "No Records", "No tracker or action items are available.", QMessageBox.StandardButton.Ok)
            self.hide()
            return

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = pnc.get_column_value(projtab.firstChild(), "project_name")

        self.export_subfolder = pnc.get_plugin_setting("ExportSubFolder", self.settings_pluginname)
        projectfolder = pnc.get_projectfolder(xmlroot)

        if projectfolder:
            projectfolder = f"{projectfolder}/{self.export_subfolder}/"
            if not pnc.folder_exists(projectfolder):
                print(f"Project folder '{projectfolder}' does not exist; output will not be copied there.")
                projectfolder = ""

        self.progbar = QProgressDialog(self)
        self.progbar.setWindowTitle("Exporting Tracker Items...")
        self.progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
        )
        self.progbar.setWindowModality(QtCore.Qt.WindowModality.WindowModal)
        self.progbar.setMinimumWidth(350)
        self.progbar.setCancelButton(None)
        self.progbar.show()
        QtWidgets.QApplication.processEvents()

        temporaryfolder = pnc.get_temporary_folder() + "/"
        suffix = " Internal" if self.internalreport else ""
        self.htmlreportname = f"{temporaryfolder}{projnum} Tracker Items{suffix}.html"
        self.pdfreportname = f"{temporaryfolder}{projnum} Tracker Items{suffix}.pdf"
        self.project_pdfreportname = f"{projectfolder}{projnum} Tracker Items{suffix}.pdf" if projectfolder else ""
        self.project_htmlreportname = f"{projectfolder}{projnum} Tracker Items{suffix}.html" if projectfolder else ""

        executedate = QDate.currentDate()
        self.subject = f"{projnum} {projdes} - {executedate.toString('MM/dd/yyyy')}"

        self.progbar.setValue(20)
        self.progbar.setLabelText("Building item list...")
        QtWidgets.QApplication.processEvents()

        # Build HTML rows from tracker items
        rows_parts = []
        repitemrow = repitem.firstChild()

        while not repitemrow.isNull():
            include, isinternal, itemtype, status = self._is_item_included(repitemrow)

            if include:
                item_number = pnc.get_column_value(repitemrow, "item_number")
                item_name = pnc.get_column_value(repitemrow, "item_name")
                identified_by = pnc.get_column_value(repitemrow, "identified_by")
                date_identified = pnc.get_column_value(repitemrow, "date_identified")
                description = pnc.get_column_value(repitemrow, "description")
                assigned_to = pnc.get_column_value(repitemrow, "assigned_to")
                priority = pnc.get_column_value(repitemrow, "priority")
                date_due = pnc.get_column_value(repitemrow, "date_due")
                last_update = pnc.get_column_value(repitemrow, "last_update")
                date_resolved = pnc.get_column_value(repitemrow, "date_resolved")

                # Collect comments from item_tracker_updates
                comment_parts = []
                trackerupdates = pnc.find_node(repitemrow, "table", "name", "item_tracker_updates")
                if not trackerupdates.isNull():
                    updaterow = trackerupdates.firstChild()
                    while not updaterow.isNull():
                        updated_by = pnc.get_column_value(updaterow, "updated_by")
                        update_date = pnc.get_column_value(updaterow, "lastupdated_date")
                        update_note = pnc.get_column_value(updaterow, "update_note")
                        comment_parts.append(f"{updated_by} - {update_date}: {update_note}")
                        updaterow = updaterow.nextSibling()

                rows_parts.append(build_item_row(
                    item_number, item_name, identified_by, date_identified,
                    description, assigned_to, priority, status,
                    date_due, last_update, date_resolved,
                    "\n".join(comment_parts),
                    isinternal, itemtype, self.internalreport
                ))

            repitemrow = repitemrow.nextSibling()

        self.progbar.setValue(60)
        self.progbar.setLabelText("Writing HTML file...")
        QtWidgets.QApplication.processEvents()

        self.html_content = generate_tracker_html(
            projnum, projdes, "".join(rows_parts), executedate.toString("MM/dd/yyyy"), self.internalreport
        )

        QFile.remove(self.htmlreportname)
        QFile.remove(self.pdfreportname)

        file = QFile(self.htmlreportname)
        if file.open(QFile.OpenModeFlag.WriteOnly):
            stream = QTextStream(file)
            stream.setEncoding(QStringConverter.Encoding.Utf8)
            stream << self.html_content
            file.close()
        else:
            print(f"Error writing {self.htmlreportname}")

        self.progbar.setValue(80)
        self.progbar.setLabelText("Generating PDF...")
        QtWidgets.QApplication.processEvents()

        # Trigger PDF generation via the hidden web view
        self.web_view.load(QUrl.fromLocalFile(self.htmlreportname))

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

        # Copy PDF to project folder if defined
        if self.project_pdfreportname:
            QFile.remove(self.project_pdfreportname)
            if not QFile(self.pdfreportname).copy(self.project_pdfreportname):
                QMessageBox.critical(self, "Unable to copy generated export", "Could not copy " + self.pdfreportname + " to " + self.project_pdfreportname)

        if self.ui.m_checkBoxDisplayTracker.isChecked():
            display_path = self.project_pdfreportname if self.project_pdfreportname else self.pdfreportname
            try:
                print(f"Attempting to open {display_path}")
                self.open_pdf(display_path)
            except:
                print(f"An error occurred trying to open {display_path}")
                pass

        if not self.keephtml:
            pass
        elif self.project_htmlreportname:
            QFile.remove(self.project_htmlreportname)
            if not QFile(self.htmlreportname).copy(self.project_htmlreportname):
                QMessageBox.critical(self, "Unable to copy generated export", "Could not copy " + self.htmlreportname + " to " + self.project_htmlreportname)

        if not self.progbar is None:
            self.progbar.setValue(100)
            self.progbar.hide()
            self.progbar.close()
            self.progbar = None  # must be destroyed

        QtWidgets.QApplication.restoreOverrideCursor()
        self.hide()

    def open_pdf(self, path):
        if sys.platform.startswith("linux"):
            try:
                subprocess.run(["xdg-open", path], check=True)
                return True
            except (FileNotFoundError, subprocess.CalledProcessError):
                try:
                    subprocess.run(["gio", "open", path], check=True)
                    return True
                except:
                    pass

        url = QUrl.fromLocalFile(path)
        return QDesktopServices.openUrl(url)


def setup_default_export_tracker_items_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "Export Tracker Items"
    if pnc_tmp.get_plugin_setting("ExportSubFolder", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("ExportSubFolder", settings_pluginname, "Project Management/Issues List")

# Module-level instances (kept alive to prevent Qt from closing the app)
pnc = None
eti = None
etis = None

if __name__ == '__main__':
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)
    pnc = ProjectNotesCommon()
    eti = TrackerItemsExporter()
    etis = ExportTrackerItemsSettings()

    xml_content = ""
    with open("test_project.xml", 'r', encoding='utf-8') as f:
        xml_content = f.read()
    eti.set_xml_doc(xml_content)
    eti.show()

    app.exec()
else:
    setup_default_export_tracker_items_settings()
    pnc = ProjectNotesCommon()
    eti = TrackerItemsExporter(pnc.get_main_window())
    etis = ExportTrackerItemsSettings()


def menu_export_tracker_items(xmlstr, parameter):
    eti.set_xml_doc(xmlstr)
    QtWidgets.QApplication.restoreOverrideCursor()
    QtWidgets.QApplication.processEvents()
    eti.show()
    return ""


def menu_settings(parameter):
    etis.show()
    return ""
