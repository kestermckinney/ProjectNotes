# Copyright (C) 2026 Paul McKinney
import platform
import requests

if (platform.system() == 'Windows'):
    import win32com

import projectnotes
from includes.common import ProjectNotesCommon
from includes.word_tools import ProjectNotesWordTools
from includes.collaboration_tools import CollaborationTools
from includes.graphapi_tools import TokenAPI
from PyQt6 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt6.QtSql import QSqlDatabase
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDate, QRect
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "SSRS Report Capture"
plugindescription = "Generate an SSRS Report as a PDF."
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

tapi = TokenAPI()

# Custom Setting
class SSRSSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "SSRS Generate"

        self.ui = uic.loadUi("plugins/forms/dialogExportLocation.ui", self)
        self.ui.setWindowTitle("SQL Report Export Sub Folder")
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

class GenerateSSRSReport(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "SSRS Generate"

        self.ui = uic.loadUi("plugins/forms/dialogSSRSOptions.ui", self)
        self.ui.m_datePickerRptDate.setCalendarPopup(True)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint 
            )

        self.ui.m_checkBoxDisplay.setChecked(False)
        self.ui.m_radioBoxEmailAsHTML.setChecked(True)
        self.ui.setModal(True)

        self.ui.pushButtonOK.clicked.connect(self.generate_report)
        self.ui.pushButtonCancel.clicked.connect(self.close_dialog)

        self.xmlstr = None
        self.report_url = None

        self.statusdate = QDateTime.currentDateTime()
        self.export_subfolder = None

    def close_dialog(self):
        self.hide()

    def set_xml_doc(self, xmlval, url):
        self.xmlstr = xmlval
        self.report_url = url

        self.statusdate = QDateTime.currentDateTime()

        self.ui.m_datePickerRptDate.setDateTime(self.statusdate)

    def download_report(self, reporturl, savelocation):
        result = requests.get(reporturl,  auth=(self.domain_user, self.domain_password))

        if (result.status_code != 200):
            msg = f"File Download Failed {result.reason}: {result.text}"
            print(msg)
            QMessageBox.critical(self, "Download Failed", msg)
            return False

        QFile.remove(savelocation) 
        with open(savelocation, mode="wb") as file:
            file.write(result.content)

        return True

    def url_is_available(self, url):
        try:
            response = requests.head(url, timeout=5)
            return response.status_code > 0
        except (requests.ConnectionError, requests.Timeout):
            return False

    def generate_report(self):
        xmlval = QDomDocument()
        if (xmlval.setContent(self.xmlstr) == False):
            QMessageBox.critical(self, "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return

        self.report_server = pnc.get_plugin_setting("ReportServer", "IFS Cloud") or ""
        self.domain_user = pnc.get_plugin_setting("DomainUser", "IFS Cloud") or ""
        self.domain_password = pnc.get_plugin_setting("DomainPassword", "IFS Cloud") or ""

        self.export_subfolder = pnc.get_plugin_setting("ExportSubFolder", self.settings_pluginname)

        if not self.report_server:
            QMessageBox.critical(self, "Report Server Not Configured",
                "No report server has been configured.\n\n"
                "Please configure the report server address in Settings > SQL Report.")
            self.hide()
            return

        if not self.url_is_available(f"http://{self.report_server}/reports/browse/"):
            msg = f'The report server "{self.report_server}" is not available.  Cannot download the report.'
            print(msg)
            QMessageBox.critical(self, "Server Not Available", msg)
            self.hide()
            return

        emaillist = ""

        keepword = self.ui.m_checkBoxWordRpt.isChecked()

        emailashtml = self.ui.m_radioBoxEmailAsHTML.isChecked()
        emailaspdf = self.ui.m_radioBoxEmailAsPDF.isChecked()
        emailasword = self.ui.m_radioBoxEmailAsWord.isChecked()
        noemail = self.ui.m_radioBoxDoNotEmail.isChecked()

        self.statusdate = self.ui.m_datePickerRptDate.dateTime()

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = pnc.get_projectfolder(xmlroot)
        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()

        if projectfolder:
            projectfolder = f"{projectfolder}/{self.export_subfolder}/"
            if not pnc.folder_exists(projectfolder):
                print(f"Project folder '{projectfolder}' does not exist; output will not be copied there.")
                projectfolder = ""

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
        progtot = 5

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = pnc.get_column_value(projtab.firstChild(), "project_name")
        period = pnc.get_column_value(projtab.firstChild(), "status_report_period")

        email = None
        nm = None
        stat = None
        receivers = ""
        jsact = None

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

            progval = progval + 1
            progbar.setValue(int(min(progval / progtot * 100, 100)))
            progbar.setLabelText("Copying files...")
            QtWidgets.QApplication.processEvents()   

        temporaryfolder = pnc.get_temporary_folder() + "/"
        basereportpdf = temporaryfolder + self.statusdate.toString("yyyyMMdd ") + projnum + " Status Report.pdf"
        basereportdocx = temporaryfolder + self.statusdate.toString("yyyyMMdd ") + projnum + " Status Report.docx"
        project_basereportpdf = (projectfolder + self.statusdate.toString("yyyyMMdd ") + projnum + " Status Report.pdf") if projectfolder else ""
        project_basereportdocx = (projectfolder + self.statusdate.toString("yyyyMMdd ") + projnum + " Status Report.docx") if projectfolder else ""

        QFile.remove(basereportpdf)
        QFile.remove(basereportdocx)

        base_url = pnc.replace_variables(self.report_url, xmlroot)

        # reporting periods must be numbers for the report
        base_url = base_url.replace("&ReportPeriod=Bi-Weekly", "&ReportPeriod=14")
        base_url = base_url.replace("&ReportPeriod=Weekly", "&ReportPeriod=7")
        base_url = base_url.replace("&ReportPeriod=Monthly", "&ReportPeriod=30")
        base_url = base_url.replace("&ReportPeriod=None", "&ReportPeriod=30")

        #default to monthly if nothing was specified
        base_url = base_url.replace("&ReportPeriod=&", "&ReportPeriod=30&")

        #if nothing was specified then just use monthly
        if base_url.endswith("&ReportPeriod="):
            base_url = base_url.replace("&ReportPeriod=", "&ReportPeriod=30")

        # replace custom report date variable
        base_url = base_url.replace("[$REPORTDATE]",  self.statusdate.toString("MM/dd/yyyy"))

        wordurl = f"http://{self.report_server}{base_url}&rs:Command=Render&rs:Format=WORDOPENXML"
        pdfurl = f"http://{self.report_server}{base_url}&rs:Command=Render&rs:Format=PDF"

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Downloading files...")        
        QtWidgets.QApplication.processEvents()   

        # project status report fixed
        if emailashtml or emailasword or keepword:
            if not self.download_report(wordurl, basereportdocx):
                progbar.hide()
                progbar.close()
                self.hide()
                return

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Downloading files...")
        QtWidgets.QApplication.processEvents()   

        if not self.download_report(pdfurl, basereportpdf):
            progbar.hide()
            progbar.close()
            self.hide()
            return

        dochtml = None

        # should we email?
        if noemail == False:
            try:
                ct = CollaborationTools(tapi)

                subject = projnum + " " + projdes + " Status Report " + self.statusdate.toString("MM/dd/yyyy")

                if emailashtml:
                    pnwt = ProjectNotesWordTools()
                    temphtmlfile = pnc.get_temporary_folder() + "/" + self.statusdate.toString("yyyyMMdd") + projnum + "statusreport.html"
                    dochtml = pnwt.get_html_version_of_word_doc(basereportdocx, temphtmlfile)

                    if dochtml:
                        ct.send_an_email(self.xmlstr, subject, dochtml, None, "Receives Status", True)                
                elif emailasword:
                    ct.send_an_email(self.xmlstr, subject, "", [basereportdocx], "Receives Status", False)
                elif emailaspdf:
                    ct.send_an_email(self.xmlstr, subject, "", [basereportpdf], "Receives Status", False)
            except Exception as e:
                print(f"Error sending email: {e}")

        # Copy files to project folder if defined
        if project_basereportpdf:
            QFile.remove(project_basereportpdf)
            QFile.copy(basereportpdf, project_basereportpdf)

        if keepword and project_basereportdocx:
            QFile.remove(project_basereportdocx)
            QFile.copy(basereportdocx, project_basereportdocx)

        display_path = project_basereportpdf if project_basereportpdf else basereportpdf
        if self.ui.m_checkBoxDisplay.isChecked():
            QDesktopServices.openUrl(QUrl("file:///" + display_path))

        progbar.setValue(100)
        progbar.setLabelText("Finalizing files...")
        QtWidgets.QApplication.processEvents()   
        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed

        self.hide()

        docxml = f"""<?xml version="1.0" encoding=\"UTF-8\"?>
            <projectnotes>
            <table name=\"projects\">
            <row>
            <column name="project_number">{projnum}</column>
            <column name="last_status_date">{self.statusdate.toString("MM/dd/yyyy")}</column>
            </row>
            </table>
            </projectnotes>
        """

        projectnotes.update_data(docxml)

        return

def setup_default_ssrs_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "SSRS Generate"
    if pnc_tmp.get_plugin_setting("ExportSubFolder", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("ExportSubFolder", settings_pluginname, "Project Management/Status Reports")

# keep the instance of windows open to avoid sending the main app a close message
pnc = None
gssrsr = None
ss = None

#setup test data
if __name__ == '__main__':
    import os
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)
    pnc = ProjectNotesCommon()
    gssrsr = GenerateSSRSReport()
    ss = SSRSSettings()

    xml_content = ""
    with open("C:\\Users\\pamcki\\OneDrive - Cornerstone Controls\\Documents\\Work In Progress\\XML\\project.xml", 'r', encoding='utf-8') as file:
        xml_content = file.read()
    gssrsr.set_xml_doc(xml_content, "/ReportServer/Pages/ReportViewer.aspx?%2fProd%2fLead+Engineers%2fProject+Status+Report&ProjectId=[$projects.project_number.1]&StatusDate=[$REPORTDATE]&ReportPeriod=[$projects.status_report_period.1]") 
    gssrsr.show()

    app.exec()
else:
    setup_default_ssrs_settings()
    pnc = ProjectNotesCommon()
    gssrsr = GenerateSSRSReport(pnc.get_main_window())
    ss = SSRSSettings()

def menu_settings(parameter):
    ss.show()
    return ""

def menu_generate_ssrs_report(xmlstr, parameter):        
    gssrsr.set_xml_doc(xmlstr, parameter)

    QtWidgets.QApplication.restoreOverrideCursor()
    QtWidgets.QApplication.processEvents()   

    gssrsr.show()

    return ""

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pluginmenus.append({"menutitle" : "Generate Status Report", "function" : "menu_generate_ssrs_report", "tablefilter" : "projects/item_tracker/item_tracker_updates/project_locations/project_people", "submenu" : "", "dataexport" : "projects", "parameter" : "/ReportServer/Pages/ReportViewer.aspx?%2fProd%2fLead+Engineers%2fProject+Status+Report&ProjectId=[$projects.project_number.1]&StatusDate=[$REPORTDATE]&ReportPeriod=[$projects.status_report_period.1]"})
    pluginmenus.append({"menutitle" : "SQL Report", "function" : "menu_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""})
