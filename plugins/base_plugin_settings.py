# Copyright (C) 2025, 2026 Paul McKinney
import os
import sys
import platform
import threading
import time
import json
import projectnotes

from includes.common import ProjectNotesCommon, _settings_organization
from includes.icloud_tools import APPLE_SERVICE, CardDAVClient, ICloudError
from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtCore import Qt, QRect, QSettings
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QDialog, QFileDialog, QTableWidgetItem, QStyledItemDelegate, QComboBox

# Project Notes Plugin Parameters
pluginname = "Base Plugins Settings" # name used in the menu
plugindescription = "This plugin provide settigns input for the base install set of plugins. Supported platforms: Windows, Linux, MacOS"

# define menus to be added to ProjectNotes Plugins menu and data export/import right click.
# the menutitle is the menu name
# the function value is the the python function to be called
# the table filter filters the XML sent to the plugin to make the export more efficient
# the menu can be placed under a submenu
# the function wil only show on the right click if it matches the table specified in dataexport
# if a dataexport value exist the menu will not appear on the plugin menu
pluginmenus = [
    {"menutitle" : "Editor", "function" : "menu_editor_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Outlook Integration", "function" : "menu_outlook_integration_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "iCloud Contacts", "function" : "menu_icloud_contacts_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "My Shortcuts", "function" : "menu_my_shortcut_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Settings Migrator", "function" : "menu_settings_migrator", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
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

class ComboBoxDelegate(QStyledItemDelegate):
    def __init__(self, parent=None, editable=True):
        super().__init__(parent)
        self.editable = editable
        self.items = []

    def set_items(self, items):
        self.items = items

    def createEditor(self, parent, option, index):
        editor = QComboBox(parent)
        editor.addItems(self.items)
        editor.setEditable(self.editable)
        return editor

    def setEditorData(self, editor, index):
        value = index.model().data(index, Qt.ItemDataRole.DisplayRole)
        editor.setCurrentText(value)

    def setModelData(self, editor, model, index):
        value = editor.currentText()
        model.setData(index, value, Qt.ItemDataRole.DisplayRole)

    def updateEditorGeometry(self, editor, option, index):
        editor.setGeometry(option.rect)

# Custom Editor Setting
class EditorSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "Custom Editor"

        self.ui = uic.loadUi("plugins/forms/dialogEditor.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
        self.ui.setModal(True)

        self.ui.pushButtonFilePick.clicked.connect(self.edit_location)
        self.ui.buttonBox.accepted.connect(self.save_settings)
        self.ui.buttonBox.rejected.connect(self.reject_changes)

        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        self.editor_path = self.pnc.get_plugin_setting("EditorPath", self.settings_pluginname)
        self.ui.lineEditFullPath.setText(self.editor_path or "")

        if w is not None and h is not None:
            self.ui.resize(int(w), int(h))
        self.center_on_main_window()

    def edit_location(self):
        value = self.ui.lineEditFullPath.text()
        if sys.platform == "win32":
            pat = "Application (*.exe)"
        else:
            pat = "Application (*)"

        file_path, filt = QFileDialog.getOpenFileName(caption="Select an application", directory=value,filter=pat)

        if (file_path is not None and file_path != ''):
            self.ui.lineEditFullPath.setText(file_path)        

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
        self.editor_path = self.ui.lineEditFullPath.text()
        self.pnc.set_plugin_setting("EditorPath", self.settings_pluginname, self.editor_path)

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

# iCloud Contacts Settings
class ICloudContactsSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "iCloud Contacts"
        self.setWindowTitle("iCloud Contacts")
        self.setModal(True)
        self.setWindowModality(Qt.WindowModality.ApplicationModal)
        self.resize(560, 360)

        layout = QtWidgets.QVBoxLayout(self)
        intro = QtWidgets.QLabel(
            "Sync iCloud contacts when Project Notes starts and every five minutes. "
            "iCloud overwrites mapped contact fields; removing a contact from iCloud never "
            "removes it from Project Notes.")
        intro.setWordWrap(True)
        layout.addWidget(intro)

        form = QtWidgets.QFormLayout()
        self.enabled = QtWidgets.QCheckBox("Enable scheduled iCloud contact sync")
        self.export_new = QtWidgets.QCheckBox(
            "Export new Project Notes contacts not found in iCloud")
        self.account = QtWidgets.QLineEdit()
        self.account.setPlaceholderText("name@example.com")
        self.password = QtWidgets.QLineEdit()
        self.password.setEchoMode(QtWidgets.QLineEdit.EchoMode.Password)
        self.password.setPlaceholderText("Leave blank to keep the stored password")
        form.addRow("", self.enabled)
        form.addRow("", self.export_new)
        form.addRow("Apple Account:", self.account)
        form.addRow("App-specific password:", self.password)
        layout.addLayout(form)

        help_label = QtWidgets.QLabel(
            '<a href="https://support.apple.com/102654">How to create an app-specific password</a>. '
            "The password is saved only in your operating system credential store.")
        help_label.setOpenExternalLinks(True)
        help_label.setWordWrap(True)
        layout.addWidget(help_label)

        self.status = QtWidgets.QLabel()
        self.status.setWordWrap(True)
        layout.addWidget(self.status)

        action_row = QtWidgets.QHBoxLayout()
        self.test_button = QtWidgets.QPushButton("Test Connection")
        self.test_button.clicked.connect(self.test_connection)
        action_row.addWidget(self.test_button)
        action_row.addStretch()
        layout.addLayout(action_row)

        self.buttons = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.StandardButton.Save |
            QtWidgets.QDialogButtonBox.StandardButton.Cancel)
        self.buttons.accepted.connect(self.save_settings)
        self.buttons.rejected.connect(self.reject)
        layout.addWidget(self.buttons)

    def showEvent(self, event):
        self.enabled.setChecked(
            (self.pnc.get_plugin_setting("Enabled", self.settings_pluginname) or "false").lower() == "true")
        self.export_new.setChecked(
            (self.pnc.get_plugin_setting("ExportNewContacts", self.settings_pluginname) or "false").lower() == "true")
        self.account.setText(self.pnc.get_plugin_setting("Account", self.settings_pluginname) or "")
        self.password.clear()
        last_status = self.pnc.get_plugin_setting("LastStatus", self.settings_pluginname) or "Not imported yet."
        last_success = self.pnc.get_plugin_setting("LastSuccess", self.settings_pluginname) or "Never"
        self.status.setText(f"Last successful sync: {last_success}\nStatus: {last_status}")
        super().showEvent(event)

    def _password_for_form(self):
        entered = self.password.text().strip()
        if entered:
            return entered
        account = self.account.text().strip()
        if not account:
            return None
        return projectnotes.get_secret(APPLE_SERVICE, account)

    def test_connection(self):
        account = self.account.text().strip()
        if not account:
            QMessageBox.warning(self, "iCloud Contacts", "Enter your Apple Account first.")
            return
        QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
        self.test_button.setEnabled(False)
        try:
            password = self._password_for_form()
            if not password:
                raise ICloudError("Enter or store an app-specific password first.")
            count = CardDAVClient(account, password).test_connection()
            QMessageBox.information(self, "iCloud Contacts",
                                    f"Connection successful. Found {count} address book(s).")
        except (ICloudError, RuntimeError) as exc:
            QMessageBox.critical(self, "iCloud Contacts", str(exc))
        finally:
            self.test_button.setEnabled(True)
            QApplication.restoreOverrideCursor()

    def save_settings(self):
        old_account = self.pnc.get_plugin_setting("Account", self.settings_pluginname) or ""
        account = self.account.text().strip()
        if self.enabled.isChecked() and not account:
            QMessageBox.warning(self, "iCloud Contacts", "Enter your Apple Account before enabling imports.")
            return
        try:
            entered_password = self.password.text().strip()
            if entered_password:
                projectnotes.set_secret(APPLE_SERVICE, account, entered_password)
                if old_account and old_account != account:
                    projectnotes.delete_secret(APPLE_SERVICE, old_account)
            elif self.enabled.isChecked() and not projectnotes.get_secret(APPLE_SERVICE, account):
                QMessageBox.warning(self, "iCloud Contacts",
                                    "Enter an app-specific password before enabling imports.")
                return
        except RuntimeError as exc:
            QMessageBox.critical(self, "Secure Storage Unavailable", str(exc))
            return

        self.pnc.set_plugin_setting("Enabled", self.settings_pluginname,
                                    "true" if self.enabled.isChecked() else "false")
        self.pnc.set_plugin_setting("ExportNewContacts", self.settings_pluginname,
                                    "true" if self.export_new.isChecked() else "false")
        self.pnc.set_plugin_setting("Account", self.settings_pluginname, account)
        self.accept()

# Outlook Integration Settings
class OutlookIntegrationSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "Outlook Integration"

        self.ui = uic.loadUi("plugins/forms/dialogOutlookIntegrationOptions.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
        self.setModal(True)

        self.ui.buttonBox.accepted.connect(self.save_settings)
        self.ui.buttonBox.rejected.connect(self.reject_changes)

        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        self.ui.comboBoxIntegrationType.setCurrentText(self.pnc.get_plugin_setting("IntegrationType", self.settings_pluginname) or "")
        self.ui.lineEditApplicationID.setText(self.pnc.get_plugin_setting("ApplicationID", self.settings_pluginname) or "")
        self.ui.lineEditTenantID.setText(self.pnc.get_plugin_setting("TenantID", self.settings_pluginname) or "")
        self.ui.checkBoxImportContacts.setChecked((self.pnc.get_plugin_setting("ImportContacts", self.settings_pluginname) or "").lower() == "true")
        self.ui.checkBoxExportContacts.setChecked((self.pnc.get_plugin_setting("ExportContacts", self.settings_pluginname) or "").lower() == "true")
        self.ui.checkBoxSyncToDoWithDue.setChecked((self.pnc.get_plugin_setting("SyncToDoWithDue", self.settings_pluginname) or "").lower() == "true")
        self.ui.checkBoxSyncToDoWithoutDue.setChecked((self.pnc.get_plugin_setting("SyncToDoDoWithoutDue", self.settings_pluginname) or "").lower() == "true")
        self.ui.checkBoxBackupEmails.setChecked((self.pnc.get_plugin_setting("BackupEmails", self.settings_pluginname) or "").lower() == "true")
        self.ui.lineEditBackupInboxFolder.setText(self.pnc.get_plugin_setting("BackupInBoxFolder", self.settings_pluginname) or "")
        self.ui.lineEditBackupSentFolder.setText(self.pnc.get_plugin_setting("BackupSentFolder", self.settings_pluginname) or "")
        self.ui.checkBoxSendO365.setChecked((self.pnc.get_plugin_setting("SendO365", self.settings_pluginname) or "").lower() == "true")

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
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # print(f"saving dimensions {self.size().width()},{self.size().height()}")

    def save_settings(self):

        self.pnc.set_plugin_setting("IntegrationType", self.settings_pluginname, self.ui.comboBoxIntegrationType.currentText())
        self.pnc.set_plugin_setting("ApplicationID", self.settings_pluginname, self.ui.lineEditApplicationID.text())
        self.pnc.set_plugin_setting("TenantID", self.settings_pluginname, self.ui.lineEditTenantID.text())
        self.pnc.set_plugin_setting("ImportContacts", self.settings_pluginname, "true" if self.ui.checkBoxImportContacts.isChecked() else "false")
        self.pnc.set_plugin_setting("ExportContacts", self.settings_pluginname, "true" if self.ui.checkBoxExportContacts.isChecked() else "false")
        self.pnc.set_plugin_setting("SyncToDoWithDue", self.settings_pluginname, "true" if self.ui.checkBoxSyncToDoWithDue.isChecked() else "false")
        self.pnc.set_plugin_setting("SyncToDoDoWithoutDue", self.settings_pluginname, "true" if self.ui.checkBoxSyncToDoWithoutDue.isChecked() else "false")
        self.pnc.set_plugin_setting("BackupEmails", self.settings_pluginname, "true" if self.ui.checkBoxBackupEmails.isChecked() else "false")
        self.pnc.set_plugin_setting("BackupInBoxFolder", self.settings_pluginname, self.ui.lineEditBackupInboxFolder.text())
        self.pnc.set_plugin_setting("BackupSentFolder", self.settings_pluginname, self.ui.lineEditBackupSentFolder.text())
        self.pnc.set_plugin_setting("SendO365", self.settings_pluginname, "true" if self.ui.checkBoxSendO365.isChecked() else "false")

        projectnotes.force_reload("outlooksync_thread")
        projectnotes.force_reload("base_plugin")

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

class MyShortcutSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "My Shortcuts"

        self.ui = uic.loadUi("plugins/forms/dialogMyShortcuts.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
        self.ui.setModal(True)
 
        self.ui.pushButtonAddShortcut.clicked.connect(self.add_shortcut)
        self.ui.pushButtonEditShortcut.clicked.connect(self.edit_shortcut)
        self.ui.pushButtonDeleteShortcut.clicked.connect(self.delete_shortcut)
        self.ui.buttonBox.accepted.connect(self.save_settings)
        self.ui.buttonBox.rejected.connect(self.reject_changes)

        delegate = ComboBoxDelegate(self.ui, False)
        delegate.set_items([
            "",
            "clients",
            "people",
            "projects",
            "project_people",
            "status_report_items",
            "project_locations ",
            "project_notes",
            "meeting_attendees",
            "item_tracker_updates",
            "item_tracker"])

        self.ui.tableShortcuts.setItemDelegateForColumn(3, delegate)

        self.myshortcuts = self.pnc.get_plugin_setting("MyShortcuts", self.settings_pluginname)

        self.populate_table_from_json(self.myshortcuts, self.ui.tableShortcuts)

        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        c1 = self.pnc.get_plugin_setting("c1", self.settings_pluginname)
        c2 = self.pnc.get_plugin_setting("c2", self.settings_pluginname)
        c3 = self.pnc.get_plugin_setting("c3", self.settings_pluginname)
        c4 = self.pnc.get_plugin_setting("c4", self.settings_pluginname)

        if (c1 is not None and c2 is not None and c3 is not None and c4 is not None):
            self.ui.tableShortcuts.setColumnWidth(0, int(c1))
            self.ui.tableShortcuts.setColumnWidth(1, int(c2))
            self.ui.tableShortcuts.setColumnWidth(2, int(c3))
            self.ui.tableShortcuts.setColumnWidth(3, int(c4))

        if w is not None and h is not None:
            self.ui.resize(int(w), int(h))
        self.center_on_main_window()

    def copy_table_to_json(self, qtable):
        data = []
        for row in range(qtable.rowCount()):
            row_data = {}
            for column in range(qtable.columnCount()):
                header = qtable.horizontalHeaderItem(column).text()
                item = qtable.item(row, column)
                value = item.text() if item is not None else ''
                row_data[header] = value
            data.append(row_data)

        json_data = json.dumps(data, indent=4)
        return(json_data)

    def populate_table_from_json(self, json_string, qtable):

        # nothing was saved
        if (json_string is None or json_string == ""):
            return

        data = json.loads(json_string)

        if (len(data) > 0):
            # Get the column headers from the first row
            column_headers = list(data[0].keys())

            # Populate the table with data
            qtable.setRowCount(len(data))
            for row, row_data in enumerate(data):
                for column, header in enumerate(column_headers):
                    value = row_data.get(header, '')
                    qtable.setItem(row, column, QTableWidgetItem(value))

    def add_shortcut(self):
        row_count = self.ui.tableShortcuts.rowCount()
        self.ui.tableShortcuts.setRowCount(row_count + 1)
        self.ui.tableShortcuts.setItem(row_count, 0, QTableWidgetItem("[New Shortcut]"))

    def edit_shortcut(self):
        row = self.ui.tableShortcuts.currentRow()
        if (row > -1):
            self.ui.tableShortcuts.setCurrentCell(row, 0)
            self.ui.tableShortcuts.edit(self.ui.tableShortcuts.currentIndex())

    def delete_shortcut(self):
        row = self.ui.tableShortcuts.currentRow()
        if (row > -1):
            value = self.ui.tableShortcuts.removeRow(row)

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

        self.pnc.set_plugin_setting("c1", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(0)}")
        self.pnc.set_plugin_setting("c2", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(1)}")
        self.pnc.set_plugin_setting("c3", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(2)}")
        self.pnc.set_plugin_setting("c4", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(3)}")

        # print(f"saving dimensions {self.size().width()},{self.size().height()}")

    def save_settings(self):
        self.myshortcuts = self.copy_table_to_json(self.ui.tableShortcuts)
        self.pnc.set_plugin_setting("MyShortcuts", self.settings_pluginname, self.myshortcuts)

        projectnotes.force_reload("myshortcuts_plugin")

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

class SettingsMigrator(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "Settings Migrator"

        self.ui = uic.loadUi("plugins/forms/dialogSettingsMigrator.ui", self)

        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
        self.ui.setModal(True)
        
        self.ui.pushButtonExport.clicked.connect(self.export_settings)
        self.ui.pushButtonImport.clicked.connect(self.import_settings)
        self.ui.pushButtonDelete.clicked.connect(self.delete_settings)
        self.ui.pushButtonClose.clicked.connect(self.close)

        self.load_all_plugin_settings()

        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        if w is not None and h is not None:
            self.ui.resize(int(w), int(h))
        self.center_on_main_window()

    def load_all_plugin_settings(self):
        self.listWidgetPlugins.clear()

        settings = QSettings(_settings_organization(),"PluginSettings")

        # Add only groups that contain actual plugin settings keys,
        # filtering out macOS NSUserDefaults system-injected groups (e.g. "com")
        for key in settings.childGroups():
            settings.beginGroup(key)
            has_keys = len(settings.childKeys()) > 0
            settings.endGroup()
            if has_keys:
                self.listWidgetPlugins.addItem(key)
        

    def export_settings(self):
        # Open file dialog to get save location
        file_dialog = QFileDialog(self)
        file_dialog.setAcceptMode(QFileDialog.AcceptMode.AcceptSave)
        file_dialog.setNameFilter("JSON files (*.json)")
        file_dialog.setDefaultSuffix("json")
        file_dialog.setModal(True)

        if not file_dialog.exec():
            return # User cancelled the dialog

        output_file = file_dialog.selectedFiles()[0]

        if output_file is None:
            return

        selected_groups = [item.text() for item in self.listWidgetPlugins.selectedItems()]

        settings_dict = {}
        
        # Get all top-level groups
        settings = QSettings(_settings_organization(),"PluginSettings")
        
        # Iterate through each group
        for group in selected_groups:
            settings.beginGroup(group)
            settings_dict[group] = {}
            
            # Get all keys in the current group
            keys = settings.allKeys()
            for key in keys:
                # Store the key-value pair
                settings_dict[group][key] = settings.value(key)
            
            settings.endGroup()
        
        # Write to JSON file
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(settings_dict, f, indent=4, sort_keys=True)

        return

    def import_settings(self):
        # Initialize QSettings (replace with your organization and application name)
        settings = QSettings(_settings_organization(),"PluginSettings")
        
        # Open file dialog to select JSON file
        file_dialog = QFileDialog(self)
        file_dialog.setAcceptMode(QFileDialog.AcceptMode.AcceptOpen)
        file_dialog.setNameFilter("JSON files (*.json)")
        file_dialog.setModal(True)
        
        if not file_dialog.exec():
            return # User cancelled the dialog

        input_file = file_dialog.selectedFiles()[0]

        if input_file is None:
            return
        
        # Read JSON file
        try:
            with open(input_file, 'r', encoding='utf-8') as f:
                settings_dict = json.load(f)
        except Exception as e:
            print(f"Error reading JSON file: {e}")
            QMessageBox.critical(self, "Cannot Parse JSON", "Unable to import settings.")
            return

        # Import settings into QSettings
        try:
            for group, keys in settings_dict.items():
                settings.beginGroup(group)
                for key, value in keys.items():
                    settings.setValue(key, value)
                settings.endGroup()
        except Exception as e:
            print(f"Error importing settings: {e}")
            QMessageBox.critical(self, "Cannot Parse JSON", "Unable to import settings.")
            return

        self.load_all_plugin_settings()

        projectnotes.force_reload("myshortcuts_plugin")
        projectnotes.force_reload("base_plugin")

        return

    def delete_settings(self):

        # Show confirmation dialog
        msg_box = QMessageBox(self)
        msg_box.setWindowTitle("Confirm Deletion")
        msg_box.setText("Are you sure you want to delete the selected settings?")
        msg_box.setStandardButtons(QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        msg_box.setDefaultButton(QMessageBox.StandardButton.No)
        msg_box.setIcon(QMessageBox.Icon.Warning)
        msg_box.setModal(True)
        
        if msg_box.show() != QMessageBox.StandardButton.Yes:        
            return # User cancelled

        selected_groups = [item.text() for item in self.listWidgetPlugins.selectedItems()]
        if not selected_groups:
            return  # No groups selected
        
        # Initialize QSettings
        settings = QSettings(_settings_organization(),"PluginSettings")
        
        # Remove each selected group
        try:
            for group in selected_groups:
                settings.remove(group)
        except Exception as e:
            print(f"Error deleting settings group: {e}")
            QMessageBox.critical(self, "Cannot Delete Settings", "An error occured while removing settings settings.")

        self.load_all_plugin_settings()
        
        return

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

    def reject_changes(self):
        self.save_window_state()

        # Call the base class implementation
        self.reject()

    def closeEvent(self, event):
        self.save_window_state()
        # Call the base class implementation
        super().closeEvent(event)

def menu_editor_settings(parameter):
    es.show()
    return ""

def menu_outlook_integration_settings(parameter):
    ois.show()
    return ""

def menu_icloud_contacts_settings(parameter):
    ics.exec()
    return ""

def menu_my_shortcut_settings(parameter):
    mss.show()
    return ""

def menu_settings_migrator(parameter):
    sm.show()
    return ""

def setup_default_editor_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "Custom Editor"

    if pnc_tmp.get_plugin_setting("EditorPath", settings_pluginname) is None:
        if platform.system() == "Windows":
            default_editor = "C:/Windows/System32/notepad.exe"
        elif platform.system() == "Darwin":
            default_editor = "/Applications/TextEdit.app/Contents/MacOS/TextEdit"
        else:
            default_editor = "/usr/bin/vi"

        pnc_tmp.set_plugin_setting("EditorPath", settings_pluginname, default_editor)

def setup_default_outlook_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "Outlook Integration"

    if pnc_tmp.get_plugin_setting("IntegrationType", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("IntegrationType", settings_pluginname, "Outlook Automation")

    if pnc_tmp.get_plugin_setting("ImportContacts", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("ImportContacts", settings_pluginname, "true")
    if pnc_tmp.get_plugin_setting("ExportContacts", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("ExportContacts", settings_pluginname, "false")
    if pnc_tmp.get_plugin_setting("SyncToDoWithDue", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("SyncToDoWithDue", settings_pluginname, "false")
    if pnc_tmp.get_plugin_setting("SyncToDoDoWithoutDue", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("SyncToDoDoWithoutDue", settings_pluginname, "false")
    if pnc_tmp.get_plugin_setting("BackupEmails", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("BackupEmails", settings_pluginname, "true")
    if pnc_tmp.get_plugin_setting("BackupInBoxFolder", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("BackupInBoxFolder", settings_pluginname, "Project Management/Correspondence/Received Email")
    if pnc_tmp.get_plugin_setting("BackupSentFolder", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("BackupSentFolder", settings_pluginname, "Project Management/Correspondence/Sent Items")
    if pnc_tmp.get_plugin_setting("SendO365", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("SendO365", settings_pluginname, "false")

def setup_default_icloud_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "iCloud Contacts"
    if pnc_tmp.get_plugin_setting("Enabled", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("Enabled", settings_pluginname, "false")
    if pnc_tmp.get_plugin_setting("ExportNewContacts", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("ExportNewContacts", settings_pluginname, "false")

setup_default_editor_settings()
setup_default_outlook_settings()
setup_default_icloud_settings()

pnc = ProjectNotesCommon()
sm = SettingsMigrator(pnc.get_main_window())
mss = MyShortcutSettings(pnc.get_main_window())
ois = OutlookIntegrationSettings(pnc.get_main_window())
ics = ICloudContactsSettings(pnc.get_main_window())
es = EditorSettings(pnc.get_main_window())

# Use code below for testing
if __name__ == '__main__':
    print("Entered __main__")
    app = QApplication(sys.argv)
    os.chdir("..")
    menu_outlook_integration_settings("")
    sys.exit(app.exec())
