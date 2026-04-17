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

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        if (x is not None and y is not None and w is not None and h is not None):
            # print(f"loading dimensions {int(x)},{int(y)},{int(w)},{int(h)}")
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

    def save_window_state(self):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # print(f"saving dimensions {self.pos().x()},{self.pos().y()},{self.size().width()},{self.size().height()}")

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

def menu_ifs_cloud_settings(parameter):
    ifc.show()
    return ""

setup_default_ifs_cloud_settings()
pnc = ProjectNotesCommon()
ifc = IFSCloudSettings(pnc.get_main_window())

# Use code below for testing
if __name__ == '__main__':
    print("Entered __main__")
    app = QApplication(sys.argv)
    menu_ifs_cloud_settings("")
    sys.exit(app.exec())
