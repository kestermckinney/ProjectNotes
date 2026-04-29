# Copyright (C) 2025, 2026 Paul McKinney
import os
import sys
import platform
import threading
import time
import json
import projectnotes

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtCore import Qt, QRect, QSettings
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QDialog, QFileDialog, QWidget, QTableWidgetItem, QStyledItemDelegate, QComboBox

# Project Notes Plugin Parameters
pluginname = "IFS Cloud Plugins Settings" # name used in the menu
plugindescription = "This plugin provide settings input for the base install of the IFS Cloud plugin. Supported platforms: Windows, Linux, MacOS"

# define menus to be added to ProjectNotes Plugins menu and data export/import right click.
# the menutitle is the menu name
# the function value is the the python function to be called
# the table filter filters the XML sent to the plugin to make the export more efficient
# the menu can be placed under a submenu
# the function wil only show on the right click if it matches the table specified in dataexport
# if a dataexport value exist the menu will not appear on the plugin menu
pluginmenus = [
    {"menutitle" : "IFS Cloud Settings", "function" : "menu_ifs_cloud_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
]

# File Finder populates the list of files associated with a project
class IFSCloudSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "IFS Cloud"

        self.ui = uic.loadUi("plugins/forms/dialogIFSCloud.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
        self.ui.setModal(True)
 
        self.ui.buttonBox.accepted.connect(self.save_settings)
        self.ui.buttonBox.rejected.connect(self.reject_changes)

        self.url = self.pnc.get_plugin_setting("URL", self.settings_pluginname)
        self.ui.lineEditURL.setText(self.url or "")

        self.realm = self.pnc.get_plugin_setting("Realm", self.settings_pluginname)
        self.ui.lineEditRealm.setText(self.realm or "")

        self.domain_user = self.pnc.get_plugin_setting("DomainUser", self.settings_pluginname)
        self.ui.lineEditDomainUser.setText(self.domain_user or "")

        self.domain_password = self.pnc.get_plugin_setting("DomainPassword", self.settings_pluginname)
        self.ui.lineEditDomainPassword.setText(self.domain_password or "")

        self.report_server = self.pnc.get_plugin_setting("ReportServer", self.settings_pluginname)
        self.ui.lineEditReportServer.setText(self.report_server or "")

        self.sync_tracker_items = self.pnc.get_plugin_setting("SyncTrackerItems", self.settings_pluginname)
        self.ui.m_checkBoxSyncTrackerItems.setChecked((self.sync_tracker_items or "").lower() == "true")

        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        if w is not None and h is not None:
            # print(f"loading dimensions {int(w)},{int(h)}")
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
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # print(f"saving dimensions {self.size().width()},{self.size().height()}")

    def save_settings(self):

        self.url = self.ui.lineEditURL.text()
        self.pnc.set_plugin_setting("URL", self.settings_pluginname, self.url)

        self.realm = self.ui.lineEditRealm.text()
        self.pnc.set_plugin_setting("Realm", self.settings_pluginname, self.realm)

        self.domain_user = self.ui.lineEditDomainUser.text()
        self.pnc.set_plugin_setting("DomainUser", self.settings_pluginname, self.domain_user)

        self.domain_password = self.ui.lineEditDomainPassword.text()
        self.pnc.set_plugin_setting("DomainPassword", self.settings_pluginname, self.domain_password)

        self.report_server = self.ui.lineEditReportServer.text()
        self.pnc.set_plugin_setting("ReportServer", self.settings_pluginname, self.report_server)

        self.pnc.set_plugin_setting("SyncTrackerItems", self.settings_pluginname, "true" if self.ui.m_checkBoxSyncTrackerItems.isChecked() else "false")

        projectnotes.force_reload("ifssync_thread")

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

def setup_default_ifs_cloud_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "IFS Cloud"

    if pnc_tmp.get_plugin_setting("URL", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("URL", settings_pluginname, "https://ifs.cornerstonecontrols.com")
    if pnc_tmp.get_plugin_setting("ReportServer", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("ReportServer", settings_pluginname, "indvifsbi05")

def setup_default_ifs_shortcuts():
    pnc_tmp = ProjectNotesCommon()

    value = pnc_tmp.get_plugin_setting("MyShortcuts", "My Shortcuts")
    if value is None:
        default_shortcuts = json.dumps([
            {"Submenu": "IFS Cloud", "Menu": "Open Time Entry", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/TimeRegistrationManager/TimeHeaderPage;$filter=%28startswith%28CompanyPersonRef%2FInternalDisplayName,'[$people.name.1]'%29%29", "Data Type": "people"},
            {"Submenu": "IFS Cloud", "Menu": "Open Project", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/ProjectDefinition/Form;path=0.1731531774.1886945984.458817809;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29", "Data Type": "projects"},
            {"Submenu": "IFS Cloud", "Menu": "Open Gantt", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/ProjectGantt/Form;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29", "Data Type": "projects"},
            {"Submenu": "IFS Cloud", "Menu": "Invoicing Plan", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/InvoicingPlan/List;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29;usedefaultfilter=false", "Data Type": "projects"},
            {"Submenu": "IFS Cloud", "Menu": "Open PC Distribution", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/CPcDistribution/Form;$filter=%28%20DistributionId%20eq%20'[$projects.project_number.1]'%20%29", "Data Type": "projects"},
            {"Submenu": "IFS Cloud", "Menu": "Open PO Table", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/CCustomerPOs/List;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29", "Data Type": "projects"},
            {"Submenu": "IFS Cloud", "Menu": "Open Standard Price", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/StandardSalesPrice/Form;$filter=%28%20ProjectSalesPriceId%20eq%20'[$projects.project_number.1]'%20%29", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Customer Parts Tracking", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FLead+Engineers%2FCustomer+Parts+Tracking&ProjectID=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Invalid Report Codes", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FProjects%2FInvalid+Report+Code+Assignments&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Parts Ordered", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FProjects%2FProject+Parts+Ordered&ProjectID=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Project Report", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FProjects%2FProject+Report&ProjectId=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Project Status", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FLead+Engineers%2FProject+Status+Report&ProjectId=[$projects.project_number.1]&ReportPeriod=30&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Missing Time Entry", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2fProd%2fProjects%2fMissing+Time+Entry&rs:Command=Render&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},  # noqa: E501
            {"Submenu": "Reports", "Menu": "Item Tracker", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FLead+Engineers%2FIssues+List&ProjectId=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"}
        ])
        pnc_tmp.set_plugin_setting("MyShortcuts", "My Shortcuts", default_shortcuts)

def menu_ifs_cloud_settings(parameter):
    ifc.show()
    return ""

setup_default_ifs_cloud_settings()
setup_default_ifs_shortcuts()
pnc = ProjectNotesCommon()
ifc = IFSCloudSettings(pnc.get_main_window())

# Use code below for testing
if __name__ == '__main__':
    print("Entered __main__")
    app = QApplication(sys.argv)
    menu_ifs_cloud_settings("")
    sys.exit(app.exec())
