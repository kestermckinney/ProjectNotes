# Copyright (C) 2026 Paul McKinney
import os
import re
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
pluginname = "Export Status Report"
plugindescription = "Export a project status report to HTML and PDF."

pluginmenus = [
    {"menutitle" : "Status Report", "function" : "menu_export_status_report", "tablefilter" : "projects/status_report_items/item_tracker/project_locations/project_people", "submenu" : "Export", "dataexport" : "projects"},
    {"menutitle" : "Export Status Report", "function" : "menu_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""}
]

# Maps the status_report_items.task_category values to the report sections they populate.
CATEGORY_SECTION = {
    "In Progress": "since_last",
    "Starting": "next_period",
    "Completed": "completed",
}

# Only open issues are relevant to a status report; Resolved/Defered/Cancelled are left off.
ISSUE_STATUS_INCLUDE = {"New", "Assigned"}

PRIORITY_SORT_ORDER = {"High": 0, "Medium": 1, "Low": 2}

# projects.status_report_period -> number of days back from the report date the review period covers.
REVIEW_PERIOD_DAYS = {
    "Weekly": 7,
    "Bi-Weekly": 14,
}


class ExportStatusReportSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "Export Status Report"

        self.ui = uic.loadUi("plugins/forms/dialogExportLocation.ui", self)
        self.ui.setWindowTitle("Status Report Export Sub Folder")
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


def compute_review_period_dates(status_report_period, end_qdate):
    """Derive the review period start/end dates from the project's reporting frequency.

    Monthly -> one month back, Weekly -> one week back, Bi-Weekly -> two weeks back.
    Any other/unset frequency (e.g. "None") leaves the review period blank.
    """
    if status_report_period == "Monthly":
        start_qdate = end_qdate.addMonths(-1)
    elif status_report_period in REVIEW_PERIOD_DAYS:
        start_qdate = end_qdate.addDays(-REVIEW_PERIOD_DAYS[status_report_period])
    else:
        return "", ""

    return start_qdate.toString("MM/dd/yyyy"), end_qdate.toString("MM/dd/yyyy")


def _parse_number(val):
    if not val:
        return None
    cleaned = re.sub(r"[^0-9.\-]", "", val)
    if cleaned in ("", "-", "."):
        return None
    try:
        return float(cleaned)
    except ValueError:
        return None


def compute_earned_value_metrics(budget, actual, bcwp, bcws, bac):
    """Mirrors the earned-value formulas used by the Projects list (see projectsmodel.cpp)."""
    a = _parse_number(actual)
    ev = _parse_number(bcwp)
    pv = _parse_number(bcws)
    bac_v = _parse_number(bac)

    eac = None
    if a is not None and a > 0 and pv is not None and pv > 0 and ev not in (None, 0) and bac_v is not None:
        eac = round(a + (bac_v - ev) / ((ev / a) * (ev / pv)), 2)

    cv = round((a - ev) / ev * 100.0, 2) if ev is not None and ev > 0 and a is not None else None
    sv = round((ev - pv) / pv * 100.0, 2) if pv is not None and pv > 0 and ev is not None else None
    pct_complete = round(ev / bac_v * 100.0, 2) if bac_v is not None and bac_v > 0 and ev is not None else None
    cpi = round(ev / a, 2) if a is not None and a > 0 and ev is not None else None

    return {
        "bcwp": bcwp or "No Schedule",
        "bcws": bcws or "No Schedule",
        "bac": bac or "No Schedule",
        "actual": actual or "No Schedule",
        "eac": f"{eac:.2f}" if eac is not None else "No Schedule",
        "cv": f"{cv:.2f}%" if cv is not None else "No Schedule",
        "sv": f"{sv:.2f}%" if sv is not None else "No Schedule",
        "pct_complete": f"{pct_complete:.2f}%" if pct_complete is not None else "No Schedule",
        "cpi": f"{cpi:.2f}" if cpi is not None else "No Schedule",
    }


# Renders "Actual Cost + (Budget At Completion - Earned Value) / ((Earned Value/Actual Cost) *
# (Earned Value/Planned Value))" as stacked fractions, spelled out in full words like the
# example report's formula graphic rather than abbreviations.
EAC_FORMULA_HTML = """<div class="formula-block">
Actual&nbsp;Cost&nbsp;+&nbsp;
<span class="fraction">
  <span class="num">(Budget&nbsp;At&nbsp;Completion&nbsp;&minus;&nbsp;Earned&nbsp;Value)</span>
  <span class="den">
    <span class="fraction"><span class="num">Earned&nbsp;Value</span><span class="den">Actual&nbsp;Cost</span></span>
    &nbsp;&times;&nbsp;
    <span class="fraction"><span class="num">Earned&nbsp;Value</span><span class="den">Planned&nbsp;Value</span></span>
  </span>
</span>
</div>"""

APPENDIX_HTML = f"""<div class="section-title">Appendix</div>
<table class="appendix-table">
<tr><td colspan="2" class="appendix-title-cell">Earned Value Project Management Terms</td></tr>
<tr><td class="cell-label">BCWP</td><td class="cell-value">The approved budget for the work actually completed by the specified date; also referred to as the budgeted cost of work performed (BCWP) or Earned Value. Travel costs are not included in calculations.</td></tr>
<tr><td class="cell-label">BCWS</td><td class="cell-value">The approved budget for the work scheduled to be completed by a specified date; also referred to as the budgeted cost of work scheduled (BCWS) and Planned Value (PV). Travel costs are not included in calculations.</td></tr>
<tr><td class="cell-label">Budget At Completion</td><td class="cell-value">The total Planned Value (PV or BCWS) at the end of the project. If a project has a Management Reserve (MR), it is typically not included in the Budget At Completion (BAC).</td></tr>
<tr><td class="cell-label">Estimate At Completion</td><td class="cell-value">The most likely estimate of what the project will cost, taking into account both current schedule performance and budget performance. Travel costs are not included in calculations.{EAC_FORMULA_HTML}</td></tr>
<tr><td class="cell-label">Cost Variance</td><td class="cell-value">The percentage difference between the Earned Value and Actual Cost. A negative number indicates the project is running under budget as of the status date.</td></tr>
<tr><td class="cell-label">Schedule Variance</td><td class="cell-value">The percentage difference between the Earned Value and the Planned Value. A positive number indicates the project is running ahead of schedule as of the status date.</td></tr>
<tr><td class="cell-label">Percent Work Complete</td><td class="cell-value">The percent of Earned Value compared to Budget At Completion. If greater than the Percentage of Budget Consumed, it is a good indication the project is running under budget.</td></tr>
</table>
"""


def build_activity_block(title, items):
    """Build one of the Activities in Progress / Next Period / Completed boxes."""
    if items:
        body = "<br>".join(_html_escape(item) for item in items)
    else:
        body = ""

    return f"""<div class="activity-title">{_html_escape(title)}</div>
<div class="activity-box">{body}</div>
"""


def build_issue_row(item_name, assigned_to, priority, due_date, status):
    if priority == "High":
        priority_class = "priority-high"
    elif priority == "Medium":
        priority_class = "priority-medium"
    else:
        priority_class = "priority-low"

    return (
        f'<tr>'
        f'<td class="col-item">{_html_escape(item_name)}</td>'
        f'<td class="col-assigned">{_html_escape(assigned_to)}</td>'
        f'<td class="col-priority"><span class="{priority_class}">{_html_escape(priority)}</span></td>'
        f'<td class="col-duedate">{_html_escape(due_date)}</td>'
        f'<td class="col-status">{_html_escape(status)}</td>'
        f'</tr>\n'
    )


def build_earned_value_html(reportingperiod, startdate, enddate, metrics):
    """Two side-by-side tables, matching the example report's layout: reporting period on
    the left, cost metrics on the right."""
    return f"""<div class="section-title">Earned Value Project Metrics</div>
<div class="ev-columns">
<table class="ev-table">
<tr><td class="cell-label">Reporting Period</td><td class="cell-value">{_html_escape(reportingperiod)}</td></tr>
<tr><td class="cell-label">Starting Date</td><td class="cell-value">{_html_escape(startdate)}</td></tr>
<tr><td class="cell-label">Ending Date</td><td class="cell-value">{_html_escape(enddate)}</td></tr>
</table>
<table class="ev-table">
<tr><td class="cell-label">Budgeted Cost of Work Performed</td><td class="cell-value">{_html_escape(metrics["bcwp"])}</td></tr>
<tr><td class="cell-label">Budgeted Cost of Work Scheduled</td><td class="cell-value">{_html_escape(metrics["bcws"])}</td></tr>
<tr><td class="cell-label">Budget At Completion</td><td class="cell-value">{_html_escape(metrics["bac"])}</td></tr>
<tr><td class="cell-label">Actual Costs</td><td class="cell-value">{_html_escape(metrics["actual"])}</td></tr>
<tr><td class="cell-label">Estimate At Completion</td><td class="cell-value">{_html_escape(metrics["eac"])}</td></tr>
<tr><td class="cell-label">Cost Variance</td><td class="cell-value">{_html_escape(metrics["cv"])}</td></tr>
<tr><td class="cell-label">Schedule Variance</td><td class="cell-value">{_html_escape(metrics["sv"])}</td></tr>
<tr><td class="cell-label">Percent Work Complete</td><td class="cell-value">{_html_escape(metrics["pct_complete"])}</td></tr>
<tr><td class="cell-label">CPI</td><td class="cell-value">{_html_escape(metrics["cpi"])}</td></tr>
</table>
</div>
"""


def generate_status_report_html(projnum, projdes, pmname, stakeholders, reportdate, reviewperiod,
                                 since_last_html, next_period_html, completed_html, issues_html,
                                 earned_value_html):
    """Build the complete HTML document for the status report."""
    return f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=780">
<style>
body {{
    font-family: Calibri, Arial, sans-serif;
    font-size: 10pt;
    margin: 0;
    padding: 0.2in;
}}
h1 {{
    font-size: 15pt;
    color: #FFFFFF;
    background-color: #1F497D;
    font-weight: bold;
    margin: 0 0 8px 0;
    padding: 6px 8px;
}}
table.header-table {{
    border-collapse: collapse;
    width: 100%;
    margin-bottom: 10px;
}}
table.header-table td {{
    border: 1px solid #808080;
    padding: 3px 6px;
    font-size: 10pt;
    vertical-align: top;
}}
.cell-label {{ background-color: #EEECE1; font-weight: bold; width: 110px; }}
.cell-value {{ background-color: #FFFFFF; }}
.section-title {{
    font-size: 10pt;
    font-weight: bold;
    color: #1F497D;
    text-transform: uppercase;
    margin-top: 10px;
    padding-bottom: 3px;
    border-bottom: 3px solid #1F497D;
}}
.activity-title {{
    font-size: 10pt;
    font-weight: bold;
    color: #1F497D;
    text-transform: uppercase;
    margin-top: 10px;
}}
.activity-box {{
    background-color: #DCE6F1;
    border: 1px solid #808080;
    min-height: 40px;
    padding: 6px;
    font-size: 10pt;
    white-space: pre-wrap;
}}
table.issues-table {{
    border-collapse: collapse;
    width: 100%;
    table-layout: fixed;
    margin-top: 4px;
}}
table.issues-table th {{
    background-color: #EEECE1;
    border: 1px solid #808080;
    padding: 3px 6px;
    text-align: center;
    font-size: 9pt;
    font-weight: bold;
}}
table.issues-table td {{
    background-color: #FFFFFF;
    border: 1px solid #808080;
    padding: 3px 6px;
    vertical-align: top;
    font-size: 9pt;
    word-wrap: break-word;
    white-space: pre-wrap;
}}
.col-item     {{ width: 40%; }}
.col-assigned {{ width: 20%; text-align: center; }}
.col-priority {{ width: 12%; text-align: center; }}
.col-duedate  {{ width: 14%; text-align: center; }}
.col-status   {{ width: 14%; text-align: center; }}

.priority-high   {{ color: #C00000; font-weight: bold; }}
.priority-medium {{ color: #9C6500; font-weight: bold; }}
.priority-low    {{ color: #375623; font-weight: bold; }}

.ev-columns {{
    display: flex;
    gap: 12px;
    margin-top: 4px;
    align-items: flex-start;
}}
table.ev-table {{
    border-collapse: collapse;
    flex: 1;
    width: 50%;
}}
table.ev-table td {{
    border: 1px solid #808080;
    padding: 3px 6px;
    font-size: 9pt;
    vertical-align: top;
}}
table.ev-table .cell-label {{ width: 60%; }}

table.appendix-table {{
    border-collapse: collapse;
    width: 100%;
    margin-top: 4px;
    margin-bottom: 10px;
}}
table.appendix-table td {{
    border: 1px solid #808080;
    padding: 3px 6px;
    font-size: 8.5pt;
    vertical-align: top;
}}
table.appendix-table .cell-label {{ width: 15%; }}
.appendix-title-cell {{
    background-color: #EEECE1;
    font-weight: bold;
    font-size: 9.5pt;
}}

.formula-block {{
    text-align: center;
    margin: 6px 0 4px 0;
    font-family: "Cambria Math", Cambria, Georgia, serif;
    font-style: italic;
    font-size: 10.5pt;
}}
.fraction {{
    display: inline-flex;
    flex-direction: column;
    vertical-align: middle;
    text-align: center;
    margin: 0 3px;
}}
.fraction .num {{
    padding: 0 4px 2px 4px;
    border-bottom: 1px solid #000000;
}}
.fraction .den {{
    padding: 2px 4px 0 4px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
}}

.footer {{
    margin-top: 8px;
    font-size: 8pt;
    color: #666;
}}
</style>
</head>
<body>
<h1>Project Status Report</h1>
<table class="header-table">
<tr><td class="cell-label">Project Manager:</td><td class="cell-value">{_html_escape(pmname)}</td><td class="cell-label">Date:</td><td class="cell-value">{_html_escape(reportdate)}</td></tr>
<tr><td class="cell-label">Stakeholders:</td><td class="cell-value">{_html_escape(stakeholders)}</td><td class="cell-label">Review Period:</td><td class="cell-value">{_html_escape(reviewperiod)}</td></tr>
<tr><td class="cell-label">Description:</td><td class="cell-value">{_html_escape(projdes)}</td><td class="cell-label">Project #:</td><td class="cell-value">{_html_escape(projnum)}</td></tr>
</table>
{build_activity_block("Activities in Progress", since_last_html)}
{build_activity_block("Activities For Next Period", next_period_html)}
{build_activity_block("Activities Completed", completed_html)}
<div class="section-title">Issues</div>
<table class="issues-table">
<thead>
<tr>
  <th class="col-item">Issue Items</th>
  <th class="col-assigned">Assigned To</th>
  <th class="col-priority">Priority</th>
  <th class="col-duedate">Due Date</th>
  <th class="col-status">Status</th>
</tr>
</thead>
<tbody>
{issues_html}
</tbody>
</table>
{earned_value_html}
{APPENDIX_HTML}
<p class="footer">Report Date: {_html_escape(reportdate)}</p>
</body>
</html>"""


class StatusReportExporter(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "Export Status Report"

        self.ui = uic.loadUi("plugins/forms/dialogExportStatusReportOptions.ui", self)
        self.ui.m_datePickerRptDateStatusReport.setCalendarPopup(True)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint |
            QtCore.Qt.WindowType.WindowStaysOnTopHint
        )
        self.ui.setModal(True)

        self.set_execute_date(QDate.currentDate())

        self.ui.pushButtonOK.clicked.connect(self.export_status_report)
        self.ui.pushButtonCancel.clicked.connect(self.close_dialog)

        self.xmlstr = None
        self.html_content = None
        self.progbar = None

        # Letter portrait page layout to match the standard status report template
        self.page_layout = QPageLayout()
        self.page_layout.setPageSize(QPageSize(QPageSize.PageSizeId.Letter))
        self.page_layout.setOrientation(QPageLayout.Orientation.Portrait)
        self.page_layout.setMargins(QMarginsF(20, 20, 20, 20))

        # Web view embedded in the dialog for PDF generation. It must stay in the widget
        # hierarchy and remain visible (Qt.isVisible() == True) — a widget hidden via
        # setVisible(False)/hide() never paints, and loadFinished/printToPdf will not
        # complete. Collapsing it to a zero-size footprint keeps it "visible" to Qt while
        # taking up no space in the dialog, so it does not appear on screen.
        self.web_view = QWebEngineView(self.ui)
        self.web_view.setFixedSize(0, 0)
        self.ui.verticalLayout_2.addWidget(self.web_view)
        self.web_view.loadFinished.connect(self.on_load_finished)
        self.web_view.page().pdfPrintingFinished.connect(self.pdf_printed)

    def set_execute_date(self, edate):
        self.executedate = edate
        self.ui.m_datePickerRptDateStatusReport.setDate(self.executedate)

    def close_dialog(self):
        self.hide()

    def set_xml_doc(self, xmlval):
        self.xmlstr = xmlval

    def on_load_finished(self, success):
        if not success:
            print("Failed to load HTML for PDF generation.")
            return

        print("HTML load finished, generating PDF...")
        self.web_view.page().printToPdf(self.pdfreportname, self.page_layout)
        print("Finished generate_pdf call")

    def export_status_report(self):
        xmldoc = QDomDocument()
        if not xmldoc.setContent(self.xmlstr):
            QMessageBox.critical(self, "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return

        self.executedate = self.ui.m_datePickerRptDateStatusReport.date()
        self.internalreport = self.ui.m_checkBoxInternalRptStatusReport.isChecked()
        self.keephtml = self.ui.m_checkBoxHTMLRptStatusReport.isChecked()
        self.emailasinlinehtml = self.ui.m_radioBoxEmailAsInlineHTML.isChecked()
        self.emailaspdf = self.ui.m_radioBoxEmailAsPDF.isChecked()
        self.emailashtml = self.ui.m_radioBoxEmailAsHTML.isChecked()
        self.noemail = self.ui.m_radioBoxDoNotEmail.isChecked()

        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()

        xmlroot = xmldoc.elementsByTagName("projectnotes").at(0)

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        if not projtab or projtab.isNull() or projtab.firstChild().isNull():
            QtWidgets.QApplication.restoreOverrideCursor()
            QMessageBox.warning(self, "No Records", "No project is available to report on.", QMessageBox.StandardButton.Ok)
            self.hide()
            return

        projectrow = projtab.firstChild()
        projnum = pnc.get_column_value(projectrow, "project_number")
        projdes = pnc.get_column_value(projectrow, "project_name")
        statusreportperiod = pnc.get_column_value(projectrow, "status_report_period")

        pmname = xmlroot.toElement().attribute("managing_manager_name")
        reportdate = self.executedate.toString("MM/dd/yyyy")
        reviewperiod_start, reviewperiod_end = compute_review_period_dates(statusreportperiod, self.executedate)
        reviewperiod = f"{reviewperiod_start} to {reviewperiod_end}" if reviewperiod_start else ""

        earned_value_metrics = compute_earned_value_metrics(
            pnc.get_column_value(projectrow, "budget"),
            pnc.get_column_value(projectrow, "actual"),
            pnc.get_column_value(projectrow, "bcwp"),
            pnc.get_column_value(projectrow, "bcws"),
            pnc.get_column_value(projectrow, "bac"),
        )

        self.export_subfolder = pnc.get_plugin_setting("ExportSubFolder", self.settings_pluginname)
        projectfolder = pnc.get_projectfolder(xmlroot)

        if projectfolder:
            projectfolder = f"{projectfolder}/{self.export_subfolder}/"
            if not pnc.folder_exists(projectfolder):
                print(f"Project folder '{projectfolder}' does not exist; output will not be copied there.")
                projectfolder = ""

        self.progbar = QProgressDialog(self)
        self.progbar.setWindowTitle("Exporting Status Report...")
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
        self.htmlreportname = f"{temporaryfolder}{projnum} Status Report{suffix}.html"
        self.pdfreportname = f"{temporaryfolder}{projnum} Status Report{suffix}.pdf"
        self.project_pdfreportname = f"{projectfolder}{projnum} Status Report{suffix}.pdf" if projectfolder else ""
        self.project_htmlreportname = f"{projectfolder}{projnum} Status Report{suffix}.html" if projectfolder else ""

        self.subject = f"{projnum} {projdes} - Status Report {reportdate}"

        self.progbar.setValue(20)
        self.progbar.setLabelText("Gathering recipients...")
        QtWidgets.QApplication.processEvents()

        # Stakeholders line: team members flagged to receive the status report
        stakeholders = ""
        teammembers = pnc.find_node(xmlroot, "table", "name", "project_people")
        if not teammembers.isNull():
            memberrow = teammembers.firstChild()
            names = []
            while not memberrow.isNull():
                if pnc.get_column_value(memberrow, "receive_status_report") == "1":
                    nm = pnc.get_column_value(memberrow, "name")
                    if nm:
                        names.append(nm)
                memberrow = memberrow.nextSibling()
            stakeholders = ", ".join(names)

        self.progbar.setValue(40)
        self.progbar.setLabelText("Gathering activities...")
        QtWidgets.QApplication.processEvents()

        # Group status report items by task_category into the three activity sections
        activities = {"since_last": [], "next_period": [], "completed": []}
        statusitems = pnc.find_node(xmlroot, "table", "name", "status_report_items")
        if not statusitems.isNull():
            itemrow = statusitems.firstChild()
            while not itemrow.isNull():
                category = pnc.get_column_value(itemrow, "task_category")
                description = pnc.get_column_value(itemrow, "task_description")
                section = CATEGORY_SECTION.get(category)
                if section and description:
                    activities[section].append(description)
                itemrow = itemrow.nextSibling()

        self.progbar.setValue(60)
        self.progbar.setLabelText("Gathering issues...")
        QtWidgets.QApplication.processEvents()

        # Issues table: open Tracker items only (not Action items, not Resolved/Defered/Cancelled)
        issue_items = []
        trackeritems = pnc.find_node(xmlroot, "table", "name", "item_tracker")
        if not trackeritems.isNull():
            trackerrow = trackeritems.firstChild()
            while not trackerrow.isNull():
                itemtype = pnc.get_column_value(trackerrow, "item_type")
                isinternal = pnc.get_column_value(trackerrow, "internal_item")
                status = pnc.get_column_value(trackerrow, "status")

                includeinternal = (isinternal == "1" and self.internalreport) or (isinternal != "1")

                if itemtype == "Tracker" and status in ISSUE_STATUS_INCLUDE and includeinternal:
                    issue_items.append({
                        "item_name": pnc.get_column_value(trackerrow, "item_name"),
                        "assigned_to": pnc.get_column_value(trackerrow, "assigned_to"),
                        "priority": pnc.get_column_value(trackerrow, "priority"),
                        "date_due": pnc.get_column_value(trackerrow, "date_due"),
                        "status": pnc.get_column_value(trackerrow, "status"),
                    })

                trackerrow = trackerrow.nextSibling()

        def issue_sort_key(indexed_item):
            idx, item = indexed_item
            priority_rank = PRIORITY_SORT_ORDER.get(item["priority"], len(PRIORITY_SORT_ORDER))
            parsed_due = QDate.fromString(item["date_due"], "MM/dd/yyyy")
            if parsed_due.isValid():
                due_rank = (0, -parsed_due.toJulianDay())
            else:
                due_rank = (1, 0)
            return (priority_rank, due_rank[0], due_rank[1], idx)

        sorted_issues = [item for _, item in sorted(enumerate(issue_items), key=issue_sort_key)]
        issues_html = "".join(
            build_issue_row(item["item_name"], item["assigned_to"], item["priority"], item["date_due"], item["status"])
            for item in sorted_issues
        )

        self.progbar.setValue(80)
        self.progbar.setLabelText("Writing HTML file...")
        QtWidgets.QApplication.processEvents()

        earned_value_html = build_earned_value_html(statusreportperiod, reviewperiod_start, reviewperiod_end, earned_value_metrics)

        self.html_content = generate_status_report_html(
            projnum, projdes, pmname, stakeholders, reportdate, reviewperiod,
            activities["since_last"], activities["next_period"], activities["completed"], issues_html,
            earned_value_html
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

        self.progbar.setValue(90)
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
            ct.send_an_email(self.xmlstr, self.subject, self.html_content, None, "Receives Status", True)
        elif self.emailashtml:
            ct.send_an_email(self.xmlstr, self.subject, "", self.htmlreportname, "Receives Status", True)
        elif self.emailaspdf:
            ct.send_an_email(self.xmlstr, self.subject, "", self.pdfreportname, "Receives Status", True)

        # Copy PDF to project folder if defined
        if self.project_pdfreportname:
            QFile.remove(self.project_pdfreportname)
            if not QFile(self.pdfreportname).copy(self.project_pdfreportname):
                QMessageBox.critical(self, "Unable to copy generated export", "Could not copy " + self.pdfreportname + " to " + self.project_pdfreportname)

        if self.ui.m_checkBoxDisplayStatusReport.isChecked():
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


def setup_default_export_status_report_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "Export Status Report"
    if pnc_tmp.get_plugin_setting("ExportSubFolder", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("ExportSubFolder", settings_pluginname, "Project Management/Status Reports")

# Module-level instances (kept alive to prevent Qt from closing the app)
pnc = None
sre = None
sres = None

if __name__ == '__main__':
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)
    pnc = ProjectNotesCommon()
    sre = StatusReportExporter()
    sres = ExportStatusReportSettings()

    xml_content = ""
    with open("test_project.xml", 'r', encoding='utf-8') as f:
        xml_content = f.read()
    sre.set_xml_doc(xml_content)
    sre.show()

    app.exec()
else:
    setup_default_export_status_report_settings()
    pnc = ProjectNotesCommon()
    sre = StatusReportExporter(pnc.get_main_window())
    sres = ExportStatusReportSettings()


def menu_export_status_report(xmlstr, parameter):
    sre.set_xml_doc(xmlstr)
    QtWidgets.QApplication.restoreOverrideCursor()
    QtWidgets.QApplication.processEvents()
    sre.set_execute_date(QDate.currentDate())
    sre.show()
    return ""


def menu_settings(parameter):
    sres.show()
    return ""
