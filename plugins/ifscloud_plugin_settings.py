import os
import sys
import platform
import threading
import time
import json
#import projectnotes

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
    {"menutitle" : "IFS Cloud Settings", "function" : "menuIFSCloudSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
]

# File Finder populates the list of files associated with a project
class IFSCloudSettings(QDialog):
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "IFS Cloud"

        self.ui = uic.loadUi("plugins/forms/dialogIFSCloud.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
 
        self.ui.buttonBox.accepted.connect(self.save_settings)
        self.ui.buttonBox.rejected.connect(self.reject_changes)

        self.user_name = self.pnc.get_plugin_setting("UserName", self.settings_pluginname)
        self.ui.lineEditUserName.setText(self.user_name)

        self.password = self.pnc.get_plugin_setting("Password", self.settings_pluginname)
        self.ui.lineEditPassword.setText(self.password)

        self.url = self.pnc.get_plugin_setting("URL", self.settings_pluginname)
        self.ui.lineEditURL.setText(self.url)

        self.person_id = self.pnc.get_plugin_setting("PersonId", self.settings_pluginname)
        self.ui.lineEditPersonId.setText(self.person_id)

        self.domain_user = self.pnc.get_plugin_setting("DomainUser", self.settings_pluginname)
        self.ui.lineEditDomainUser.setText(self.domain_user)

        self.domain_password = self.pnc.get_plugin_setting("DomainPassword", self.settings_pluginname)
        self.ui.lineEditDomainPassword.setText(self.domain_password)

        self.report_server = self.pnc.get_plugin_setting("ReportServer", self.settings_pluginname)
        self.ui.lineEditReportServer.setText(self.report_server)

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        if (x != '' and y != '' and w != '' and h != ''):
            print(f"loading dimensions {int(x)},{int(y)},{int(w)},{int(h)}")
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

        self.show()

    def save_window_state(self):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        print(f"saving dimensions {self.pos().x()},{self.pos().y()},{self.size().width()},{self.size().height()}")

    def save_settings(self):

        self.user_name = self.ui.lineEditUserName.text()
        self.pnc.set_plugin_setting("UserName", self.settings_pluginname, self.user_name)

        self.password = self.ui.lineEditPassword.text()
        self.pnc.set_plugin_setting("Password", self.settings_pluginname, self.password)

        self.url = self.ui.lineEditURL.text()
        self.pnc.set_plugin_setting("URL", self.settings_pluginname, self.url)

        self.person_id = self.ui.lineEditPersonId.text()
        self.pnc.set_plugin_setting("PersonId", self.settings_pluginname, self.person_id)

        self.domain_user = self.ui.lineEditDomainUser.text()
        self.pnc.set_plugin_setting("DomainUser", self.settings_pluginname, self.domain_user)

        self.domain_password = self.ui.lineEditDomainPassword.text()
        self.pnc.set_plugin_setting("DomainPassword", self.settings_pluginname, self.domain_password)

        self.report_server = self.ui.lineEditReportServer.text()
        self.pnc.set_plugin_setting("ReportServer", self.settings_pluginname, self.report_server)

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

def menuIFSCloudSettings(parameter):
    settings_dialog = IFSCloudSettings()
    return ""


# Use code below for testing
if __name__ == '__main__':
    print("Entered __main__")
    app = QApplication(sys.argv)
    menuIFSCloudSettings("")
    sys.exit(app.exec())
