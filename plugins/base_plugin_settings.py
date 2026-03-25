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
    {"menutitle" : "File Finder", "function" : "menu_file_collector_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Editor", "function" : "menu_editor_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Outlook Integration", "function" : "menu_outlook_integration_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "My Shortcuts", "function" : "menu_my_shortcut_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Meeting and Email Types", "function" : "menu_meeting_email_types_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
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

class ClassificationEdit(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.ui = uic.loadUi("plugins/forms/dialogClassification.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )

# File Finder populates the list of files associated with a project
class FileFinderSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "File Finder"

        self.ui = uic.loadUi("plugins/forms/dialogFileFinder.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
 
        self.ui_class = ClassificationEdit(self.ui)
        self.ui_class.buttonBox.accepted.connect(self.update_classification_row)

        self.ui.pushButtonAddLocation.clicked.connect(self.add_location)
        self.ui.pushButtonEditLocation.clicked.connect(self.edit_location)
        self.ui.pushButtonDeleteLocation.clicked.connect(self.delete_location)
        self.ui.pushButtonAddClassification.clicked.connect(self.add_classification)
        self.ui.pushButtonEditClassification.clicked.connect(self.edit_classification)
        self.ui.pushButtonDeleteClassification.clicked.connect(self.delete_classification)
        self.ui.buttonBox.accepted.connect(self.save_settings)
        self.ui.buttonBox.rejected.connect(self.reject_changes)
        self.ui.tableSearchLocations.verticalHeader().sectionDoubleClicked.connect(self.on_location_row_header_clicked)
        self.ui.tableClassifications.verticalHeader().sectionDoubleClicked.connect(self.on_classification_row_header_clicked)

        delegate = ComboBoxDelegate(self.ui, False)
        delegate.set_items([
           "Quote",
           "Functional Design",
           "Estimate",
           "Purchase Order",
           "Change Request",
           "Risk Register",
           "Project Schedule",
           "Issues List",
           "Activity Report",
           "Meeting Presentation"])

        self.ui.tableClassifications.setItemDelegateForColumn(0, delegate)

        self.search_locations = self.pnc.get_plugin_setting("SearchLocations", self.settings_pluginname)
        self.classifications = self.pnc.get_plugin_setting("Classifications", self.settings_pluginname)

        self.populate_table_from_json(self.search_locations, self.ui.tableSearchLocations)
        self.populate_table_from_json(self.classifications, self.ui.tableClassifications)

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        lc1 = self.pnc.get_plugin_setting("lc1", self.settings_pluginname)
        mc1 = self.pnc.get_plugin_setting("mc1", self.settings_pluginname)
        mc2 = self.pnc.get_plugin_setting("mc2", self.settings_pluginname)

        if (lc1 != '' and mc1 != '' and mc2 != ''):
            self.ui.tableSearchLocations.setColumnWidth(0, int(lc1))
            self.ui.tableClassifications.setColumnWidth(0, int(mc1))
            self.ui.tableClassifications.setColumnWidth(1, int(mc2))

            #print(f"loading file finder columns {lc1}, {mc1}, {mc2}")

        if (x != '' and y != '' and w != '' and h != ''):
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

    def copy_table_to_json(self, qtable):
        data = []
        for row in range(qtable.rowCount()):
            row_data = {}
            for column in range(qtable.columnCount()):
                header = qtable.horizontalHeaderItem(column).text()
                value = qtable.item(row, column).text()
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

    def update_classification_row(self):
        if (self.edit_classification_row >= self.ui.tableClassifications.rowCount()):
            self.ui.tableClassifications.setRowCount(self.edit_classification_row + 1)

        self.ui.tableClassifications.setItem(self.edit_classification_row, 0, QTableWidgetItem(self.ui_class.comboBoxClassification.currentText()))
        self.ui.tableClassifications.setItem(self.edit_classification_row, 1, QTableWidgetItem(self.ui_class.lineEditPatternMatch.text()))

    def on_location_row_header_clicked(self, row):
        self.ui.tableSearchLocations.selectRow(row)
        self.edit_location()

    def on_classification_row_header_clicked(self, row):
        self.ui.tableClassifications.selectRow(row)
        self.edit_classification()

    def add_location(self):
        folder_path = QFileDialog.getExistingDirectory(caption="Select a folder")
        if (folder_path is not None and folder_path != ''):
            row_count = self.ui.tableSearchLocations.rowCount()
            self.ui.tableSearchLocations.setRowCount(row_count + 1)
            self.ui.tableSearchLocations.setItem(row_count, 0, QTableWidgetItem(folder_path))

    def edit_location(self):
        row = self.ui.tableSearchLocations.currentRow()

        if (row > -1):
            value = self.ui.tableSearchLocations.item(row, 0).text()
            folder_path = QFileDialog.getExistingDirectory(caption="Select a folder", directory=value)
            if (folder_path is not None and folder_path != ''):
                self.ui.tableSearchLocations.setItem(row, 0, QTableWidgetItem(folder_path))

    def delete_location(self):
        row = self.ui.tableSearchLocations.currentRow()
        if (row > -1):
            value = self.ui.tableSearchLocations.removeRow(row)

    def add_classification(self):
        self.edit_classification_row = self.ui.tableClassifications.rowCount()
        self.ui_class.comboBoxClassification.setCurrentText('')
        self.ui_class.lineEditPatternMatch.setText('')
        self.ui_class.setModal(True)

        self.ui_class.show()

    def edit_classification(self):
        self.edit_classification_row = self.ui.tableClassifications.currentRow()

        if (self.edit_classification_row > -1):
            clss = self.ui.tableClassifications.item(self.edit_classification_row, 0).text()
            pat = self.ui.tableClassifications.item(self.edit_classification_row, 1).text()

            self.ui_class.comboBoxClassification.setCurrentText(clss)
            self.ui_class.lineEditPatternMatch.setText(pat)
            self.ui_class.setModal(True)

            self.ui_class.show()

    def delete_classification(self):
        row = self.ui.tableClassifications.currentRow()
        if (row > -1):
            value = self.ui.tableClassifications.removeRow(row)

    def save_window_state(self):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        self.pnc.set_plugin_setting("lc1", self.settings_pluginname, f"{self.ui.tableSearchLocations.columnWidth(0)}")
        self.pnc.set_plugin_setting("mc1", self.settings_pluginname, f"{self.ui.tableClassifications.columnWidth(0)}")
        self.pnc.set_plugin_setting("mc2", self.settings_pluginname, f"{self.ui.tableClassifications.columnWidth(1)}")

        # print(f"saving dimensions {self.pos().x()},{self.pos().y()},{self.size().width()},{self.size().height()}")

    def save_settings(self):
        self.search_locations = self.copy_table_to_json(self.ui.tableSearchLocations)
        self.pnc.set_plugin_setting("SearchLocations", self.settings_pluginname, self.search_locations)

        self.classifications = self.copy_table_to_json(self.ui.tableClassifications)
        self.pnc.set_plugin_setting("Classifications", self.settings_pluginname, self.classifications)

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

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        self.editor_path = self.pnc.get_plugin_setting("EditorPath", self.settings_pluginname)
        self.ui.lineEditFullPath.setText(self.editor_path)

        if (x != '' and y != '' and w != '' and h != ''):
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

    def edit_location(self):
        value = self.ui.lineEditFullPath.text()
        if sys.platform == "win32":
            pat = "Application (*.exe)"
        else:
            pat = "Application (*)"

        file_path, filt = QFileDialog.getOpenFileName(caption="Select an application", directory=value,filter=pat)

        if (file_path is not None and file_path != ''):
            self.ui.lineEditFullPath.setText(file_path)        

    def save_window_state(self):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # print(f"saving dimensions {self.pos().x()},{self.pos().y()},{self.size().width()},{self.size().height()}")

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

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        self.ui.comboBoxIntegrationType.setCurrentText(self.pnc.get_plugin_setting("IntegrationType", self.settings_pluginname))
        self.ui.lineEditApplicationID.setText(self.pnc.get_plugin_setting("ApplicationID", self.settings_pluginname))
        self.ui.lineEditTenantID.setText(self.pnc.get_plugin_setting("TenantID", self.settings_pluginname))
        self.ui.checkBoxImportContacts.setChecked(self.pnc.get_plugin_setting("ImportContacts", self.settings_pluginname).lower() == "true")
        self.ui.checkBoxExportContacts.setChecked(self.pnc.get_plugin_setting("ExportContacts", self.settings_pluginname).lower() == "true")
        self.ui.checkBoxSyncToDoWithDue.setChecked(self.pnc.get_plugin_setting("SyncToDoWithDue", self.settings_pluginname).lower() == "true")
        self.ui.checkBoxSyncToDoWithoutDue.setChecked(self.pnc.get_plugin_setting("SyncToDoDoWithoutDue", self.settings_pluginname).lower() == "true")
        self.ui.checkBoxBackupEmails.setChecked(self.pnc.get_plugin_setting("BackupEmails", self.settings_pluginname).lower() == "true")
        self.ui.lineEditBackupInboxFolder.setText(self.pnc.get_plugin_setting("BackupInBoxFolder", self.settings_pluginname))
        self.ui.lineEditBackupSentFolder.setText(self.pnc.get_plugin_setting("BackupSentFolder", self.settings_pluginname))
        self.ui.checkBoxSendO365.setChecked(self.pnc.get_plugin_setting("SendO365", self.settings_pluginname).lower() == "true")
        self.ui.checkBoxScheduleO365.setChecked(self.pnc.get_plugin_setting("ScheduleO365", self.settings_pluginname).lower() == "true")

        if (x != '' and y != '' and w != '' and h != ''):
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

    def save_window_state(self):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # print(f"saving dimensions {self.pos().x()},{self.pos().y()},{self.size().width()},{self.size().height()}")

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
        self.pnc.set_plugin_setting("ScheduleO365", self.settings_pluginname, "true" if self.ui.checkBoxScheduleO365.isChecked() else "false")
        self.pnc.set_plugin_setting("SendO365", self.settings_pluginname, "true" if self.ui.checkBoxSendO365.isChecked() else "false")

        projectnotes.force_reload("outlooksync_thread")

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

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        c1 = self.pnc.get_plugin_setting("c1", self.settings_pluginname)
        c2 = self.pnc.get_plugin_setting("c2", self.settings_pluginname)
        c3 = self.pnc.get_plugin_setting("c3", self.settings_pluginname)
        c4 = self.pnc.get_plugin_setting("c4", self.settings_pluginname)

        if (c1 != '' and c2 != '' and c3 != ''and c4 != ''):
            self.ui.tableShortcuts.setColumnWidth(0, int(c1))
            self.ui.tableShortcuts.setColumnWidth(1, int(c2))
            self.ui.tableShortcuts.setColumnWidth(2, int(c3))
            self.ui.tableShortcuts.setColumnWidth(3, int(c4))

        if (x != '' and y != '' and w != '' and h != ''):
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

    def copy_table_to_json(self, qtable):
        data = []
        for row in range(qtable.rowCount()):
            row_data = {}
            for column in range(qtable.columnCount()):
                header = qtable.horizontalHeaderItem(column).text()
                value = qtable.item(row, column).text()
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

    def save_window_state(self):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        self.pnc.set_plugin_setting("c1", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(0)}")
        self.pnc.set_plugin_setting("c2", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(1)}")
        self.pnc.set_plugin_setting("c3", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(2)}")
        self.pnc.set_plugin_setting("c4", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(3)}")

        # print(f"saving dimensions {self.pos().x()},{self.pos().y()},{self.size().width()},{self.size().height()}")

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

class EditMETemplate(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.ui = uic.loadUi("plugins/forms/dialogMeetingEmailTemplate.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
        self.ui.setModal(True)

class MeetingEmailTypesSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "Meeting and Email Types"

        self.ui = uic.loadUi("plugins/forms/dialogMeetingEmailTypes.ui", self)

        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
        self.ui.setModal(True)
 
        self.ui.pushButtonAddType.clicked.connect(self.add_type)
        self.ui.pushButtonEditType.clicked.connect(self.edit_type)
        self.ui.pushButtonDeleteType.clicked.connect(self.delete_type)
        self.ui.buttonBox.accepted.connect(self.save_settings)
        self.ui.buttonBox.rejected.connect(self.reject_changes)
        self.ui.tableWidgetMeetingEmailTypes.verticalHeader().sectionDoubleClicked.connect(self.on_row_header_clicked)

        type_delegate = ComboBoxDelegate(self.ui, False)
        type_delegate.set_items([
            "Email",
            "Meeting"])

        delegate = ComboBoxDelegate(self.ui, False)
        delegate.set_items([
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

        group_delegate = ComboBoxDelegate(self.ui, False)
        group_delegate.set_items([
           "Attachment Only",
           "Exclude Client",
           "Full Project Team",
           "Individual",
           "Internal Project Team",
           "Only Client",           
           "Receives Status"])

        self.ui.tableWidgetMeetingEmailTypes.setItemDelegateForColumn(0, type_delegate)
        self.ui.tableWidgetMeetingEmailTypes.setItemDelegateForColumn(2, group_delegate)
        self.ui.tableWidgetMeetingEmailTypes.setItemDelegateForColumn(5, delegate)

        self.meeting_types = self.pnc.get_plugin_setting("MeetingEmailTypes", self.settings_pluginname)
        self.populate_table_from_json(self.meeting_types, self.ui.tableWidgetMeetingEmailTypes)

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        c1 = self.pnc.get_plugin_setting("c1", self.settings_pluginname)
        c2 = self.pnc.get_plugin_setting("c2", self.settings_pluginname)
        c3 = self.pnc.get_plugin_setting("c3", self.settings_pluginname)
        c4 = self.pnc.get_plugin_setting("c4", self.settings_pluginname)
        c5 = self.pnc.get_plugin_setting("c5", self.settings_pluginname)
        c6 = self.pnc.get_plugin_setting("c6", self.settings_pluginname)

        geometry = self.pnc.get_plugin_setting("types_geometry", self.settings_pluginname)

        if (c1 != '' and c2 != '' and c3 != '' and c4 != '' and c5 != '' and c6 != ''):
            #print(f"loading column sizes {c1},{c2},{c3},{c4},{c5},{c6}")

            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(0, int(c1))
            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(1, int(c2))
            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(2, int(c3))
            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(3, int(c4))
            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(4, int(c5))
            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(5, int(c6))

        if (x != '' and y != '' and w != '' and h != ''):
            #print(f"loading dimensions {int(x)},{int(y)},{int(w)},{int(h)}")
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

        self.ui_template = EditMETemplate(self.ui)
        self.ui_template.buttonBox.accepted.connect(self.update_row)

    def copy_table_to_json(self, qtable):
        data = []
        for row in range(qtable.rowCount()):
            row_data = {}
            for column in range(qtable.columnCount()):
                header = qtable.horizontalHeaderItem(column).text()
                value = qtable.item(row, column).text()
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

    def update_row(self):
        if (self.edit_row >= self.ui.tableWidgetMeetingEmailTypes.rowCount()):
            self.ui.tableWidgetMeetingEmailTypes.setRowCount(self.edit_row + 1)

        self.ui.tableWidgetMeetingEmailTypes.setItem(self.edit_row, 0, QTableWidgetItem(self.ui_template.comboBoxType.currentText()))
        self.ui.tableWidgetMeetingEmailTypes.setItem(self.edit_row, 1, QTableWidgetItem(self.ui_template.lineEditName.text()))
        self.ui.tableWidgetMeetingEmailTypes.setItem(self.edit_row, 2, QTableWidgetItem(self.ui_template.comboBoxInvitees.currentText()))
        self.ui.tableWidgetMeetingEmailTypes.setItem(self.edit_row, 3, QTableWidgetItem(self.ui_template.lineEditSubject.text()))
        self.ui.tableWidgetMeetingEmailTypes.setItem(self.edit_row, 4, QTableWidgetItem(self.ui_template.textEditTemplate.toHtml()))
        self.ui.tableWidgetMeetingEmailTypes.setItem(self.edit_row, 5, QTableWidgetItem(self.ui_template.comboBoxData.currentText()))

    def on_row_header_clicked(self, row):
        self.ui.tableWidgetMeetingEmailTypes.selectRow(row)
        self.edit_type()

    def add_type(self):
        self.edit_row = self.ui.tableWidgetMeetingEmailTypes.rowCount()

        self.ui_template.comboBoxType.setCurrentText("Meeting")
        self.ui_template.lineEditName.setText('')
        self.ui_template.comboBoxInvitees.setCurrentText('')
        self.ui_template.lineEditSubject.setText('')
        self.ui_template.textEditTemplate.setHtml('')
        self.ui_template.comboBoxData.setCurrentText("people")
        self.ui_template.setModal(True)
        self.ui_template.show()

    def edit_type(self):
        self.edit_row = self.ui.tableWidgetMeetingEmailTypes.currentRow()

        print('Editing Meeting Email Type')

        # self.ui_template.textEditTemplate.setFontFamily("Arial")
        # self.ui_template.textEditTemplate.setFontSize("11")

        if (self.edit_row > -1):
            mtype = self.ui.tableWidgetMeetingEmailTypes.item(self.edit_row, 0).text()
            nam = self.ui.tableWidgetMeetingEmailTypes.item(self.edit_row, 1).text()
            matt = self.ui.tableWidgetMeetingEmailTypes.item(self.edit_row, 2).text()
            subj = self.ui.tableWidgetMeetingEmailTypes.item(self.edit_row, 3).text() 
            mhtml = self.ui.tableWidgetMeetingEmailTypes.item(self.edit_row, 4).text() 
            data = self.ui.tableWidgetMeetingEmailTypes.item(self.edit_row, 5).text() 


            self.ui_template.comboBoxType.setCurrentText(mtype)
            self.ui_template.lineEditName.setText(nam)
            self.ui_template.comboBoxInvitees.setCurrentText(matt)
            self.ui_template.comboBoxData.setCurrentText(data)
            self.ui_template.lineEditSubject.setText(subj)
            self.ui_template.textEditTemplate.setHtml(mhtml)
            self.ui_template.setModal(True)
            self.ui_template.show()

    def delete_type(self):
        row = self.ui.tableWidgetMeetingEmailTypes.currentRow()
        if (row > -1):
            value = self.ui.tableWidgetMeetingEmailTypes.removeRow(row)

    def save_window_state(self):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        self.pnc.set_plugin_setting("c1", self.settings_pluginname, f"{self.ui.tableWidgetMeetingEmailTypes.columnWidth(0)}")
        self.pnc.set_plugin_setting("c2", self.settings_pluginname, f"{self.ui.tableWidgetMeetingEmailTypes.columnWidth(1)}")
        self.pnc.set_plugin_setting("c3", self.settings_pluginname, f"{self.ui.tableWidgetMeetingEmailTypes.columnWidth(2)}")
        self.pnc.set_plugin_setting("c4", self.settings_pluginname, f"{self.ui.tableWidgetMeetingEmailTypes.columnWidth(3)}")
        self.pnc.set_plugin_setting("c5", self.settings_pluginname, f"{self.ui.tableWidgetMeetingEmailTypes.columnWidth(4)}")
        self.pnc.set_plugin_setting("c6", self.settings_pluginname, f"{self.ui.tableWidgetMeetingEmailTypes.columnWidth(5)}")

        # print(f"saving dimensions {self.pos().x()},{self.pos().y()},{self.size().width()},{self.size().height()}")

    def save_settings(self):
        self.meeting_types = self.copy_table_to_json(self.ui.tableWidgetMeetingEmailTypes)
        self.pnc.set_plugin_setting("MeetingEmailTypes", self.settings_pluginname, self.meeting_types)

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

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        if (x != '' and y != '' and w != '' and h != ''):
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

    def load_all_plugin_settings(self):
        self.listWidgetPlugins.clear()

        settings = QSettings("ProjectNotes","PluginSettings")
        
        # Get all keys in the group
        keys = settings.childGroups()
        
        # Add keys to the QListWidget
        for key in keys:
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
        settings = QSettings("ProjectNotes","PluginSettings")
        
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
        settings = QSettings("ProjectNotes","PluginSettings")
        
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
            QMessageBox.critical(None, "Cannot Parse JSON", "Unable to import settings.")
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
            QMessageBox.critical(None, "Cannot Parse JSON", "Unable to import settings.")
            return

        self.load_all_plugin_settings()

        projectnotes.force_reload("common") # this should cause most everything to reload

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
        settings = QSettings("ProjectNotes","PluginSettings")
        
        # Remove each selected group
        try:
            for group in selected_groups:
                settings.remove(group)
        except Exception as e:
            print(f"Error deleting settings group: {e}")
            QMessageBox.critical(None, "Cannot Delete Settings", "An error occured while removing settings settings.")

        self.load_all_plugin_settings()
        
        return

    def save_window_state(self):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # print(f"saving dimensions {self.pos().x()},{self.pos().y()},{self.size().width()},{self.size().height()}")

    def reject_changes(self):
        self.save_window_state()
        
        # Call the base class implementation
        self.reject()

    def closeEvent(self, event):
        self.save_window_state()
        # Call the base class implementation
        super().closeEvent(event)

def menu_file_collector_settings(parameter):
    ffs.show()
    return ""

def menu_editor_settings(parameter):
    es.show()
    return ""

def menu_outlook_integration_settings(parameter):
    ois.show()
    return ""

def menu_my_shortcut_settings(parameter):
    mss.show()
    return ""

def menu_meeting_email_types_settings(parameter):
    mets.show()
    return ""

def menu_settings_migrator(parameter):
    sm.show()
    return ""

def setup_default_file_finder_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "File Finder"

    if pnc_tmp.get_plugin_setting("SearchLocations", settings_pluginname) == "":
        default_locations = json.dumps([
            {"Location": os.path.join(os.path.expanduser("~"), "Documents", "Projects").replace("\\", "/")}
        ], indent=4)
        pnc_tmp.set_plugin_setting("SearchLocations", settings_pluginname, default_locations)

    if pnc_tmp.get_plugin_setting("Classifications", settings_pluginname) == "":
        default_classifications = json.dumps([
            {"Classification": "Project Schedule", "Pattern Match": ".*Project Management/Schedule.*\\.mpp$"},
            {"Classification": "Quote",            "Pattern Match": ".*Project Management/Quotes.*\\.pdf$"},
            {"Classification": "Issues List",      "Pattern Match": ".*Project Management/.*Tracker Report.*\\.pdf$"},
            {"Classification": "Issues List",      "Pattern Match": ".*Project Management/.*Issues List.*\\.xlsx$"},
            {"Classification": "Meeting Presentation", "Pattern Match": ".*Project Management/Meeting Minutes/.*\\.pptx$"},
            {"Classification": "Meeting Presentation", "Pattern Match": ".*Project Management/Meeting Minutes/.*\\.ppt$"},
            {"Classification": "Meeting Presentation", "Pattern Match": ".*Project Management/Meeting Minutes/.*\\.doc$"},
            {"Classification": "Meeting Presentation", "Pattern Match": ".*Project Management/Meeting Minutes/.*\\.docx$"},
            {"Classification": "Change Request",   "Pattern Match": ".*Project Management/PCR's/.*\\.pdf$"},
            {"Classification": "Change Request",   "Pattern Match": ".*Project Management/PCR's/.*\\.docx$"},
            {"Classification": "Change Request",   "Pattern Match": ".*Project Management/PCR's/.*\\.xlsx$"},
            {"Classification": "PM Plan",          "Pattern Match": ".*Project Management/PM Plan/.*\\.docx$"},
            {"Classification": "Purchase Order",   "Pattern Match": ".*Project Management/Purchase Orders/.*\\.pdf$"},
            {"Classification": "Estimate",         "Pattern Match": ".*Project Management/Quotes.*\\.xlsx$"},
            {"Classification": "Quote",            "Pattern Match": ".*Project Management/Quotes.*\\.docx$"},
            {"Classification": "Risk Register",    "Pattern Match": ".*Project Management/Risk Management.*\\.xlsx$"},
            {"Classification": "Risk Register",    "Pattern Match": ".*Project Management/Risk Management.*\\.docx$"},
        ], indent=4)
        pnc_tmp.set_plugin_setting("Classifications", settings_pluginname, default_classifications)

def setup_default_editor_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "Custom Editor"

    if pnc_tmp.get_plugin_setting("EditorPath", settings_pluginname) == "":
        if platform.system() == "Windows":
            default_editor = "C:/Windows/System32/notepad.exe"
        elif platform.system() == "Darwin":
            default_editor = "/Applications/TextEdit.app/Contents/MacOS/TextEdit"
        else:
            default_editor = "/usr/bin/vi"

        pnc_tmp.set_plugin_setting("EditorPath", settings_pluginname, default_editor)

_MEETING_EMAIL_TYPES_DEFAULTS = r'''{"Meeting and Email Types": {"MeetingEmailTypes": "[\n    {\n        \"Type\": \"Email\",\n        \"Name\": \"Individual\",\n        \"Invitees\": \"Individual\",\n        \"Subject\": \"\",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Aptos'; font-size:11pt; font-weight:400; font-style:normal;\\\"></body></html>\",\n        \"Data Type\": \"people\"\n    },\n    {\n        \"Type\": \"Email\",\n        \"Name\": \"Attendee\",\n        \"Invitees\": \"Individual\",\n        \"Subject\": \"[$meeting_attendee.project_number.1] [$meeting_attendee.project_name.1] - \",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Aptos'; font-size:11pt; font-weight:400; font-style:normal;\\\"></body></html>\",\n        \"Data Type\": \"meeting_attendees\"\n    },\n    {\n        \"Type\": \"Email\",\n        \"Name\": \"Non-Client\",\n        \"Invitees\": \"Exclude Client\",\n        \"Subject\": \"[$projects.project_number.1] [$projects.project_name.1] - \",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Aptos'; font-size:11pt; font-weight:400; font-style:normal;\\\"></body></html>\",\n        \"Data Type\": \"projects\"\n    },\n    {\n        \"Type\": \"Email\",\n        \"Name\": \"Full Team\",\n        \"Invitees\": \"Full Project Team\",\n        \"Subject\": \"[$projects.project_number.1] [$projects.project_name.1] - \",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Aptos'; font-size:11pt; font-weight:400; font-style:normal;\\\"></body></html>\",\n        \"Data Type\": \"projects\"\n    },\n    {\n        \"Type\": \"Email\",\n        \"Name\": \"Attachment\",\n        \"Invitees\": \"Attachment Only\",\n        \"Subject\": \"[$project_locations.project_number.1] [$project_locations.project_name.1] - [$project_locations.location_description.1]\",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Aptos'; font-size:11pt; font-weight:400; font-style:normal;\\\"></body></html>\",\n        \"Data Type\": \"project_locations\"\n    },\n    {\n        \"Type\": \"Email\",\n        \"Name\": \"Team Member\",\n        \"Invitees\": \"Individual\",\n        \"Subject\": \"[$project_people.project_number.1] [$project_people.project_name.1] - \",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Aptos'; font-size:11pt; font-weight:400; font-style:normal;\\\"></body></html>\",\n        \"Data Type\": \"project_people\"\n    },\n    {\n        \"Type\": \"Email\",\n        \"Name\": \"Internal Project Team\",\n        \"Invitees\": \"Internal Project Team\",\n        \"Subject\": \"[$projects.project_number.1] [$projects.project_name.1] - \",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Aptos'; font-size:11pt; font-weight:400; font-style:normal;\\\"></body></html>\",\n        \"Data Type\": \"projects\"\n    },\n    {\n        \"Type\": \"Meeting\",\n        \"Name\": \"Project Status (Internal)\",\n        \"Invitees\": \"Exclude Client\",\n        \"Subject\": \"[$projects.project_number.1] [$projects.project_name.1] - Project Status (Internal)\",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\\\">\\n<p style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Aptos Display','sans-serif'; font-weight:700;\\\">Status Meeting Agenda (Internal)</span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\"><br />The purpose of this meeting is to make sure all work is progressing, everyone understands their assignments, and to identify any roadblocks that are holding up work or could hold up work.\\u00a0 We also want to review how we are tracking against the schedule and determine if we are still on track. </span></p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Review Action Items From Prior Meeting</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Provide Updates On Assigned Tasks</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Identify Any Roadblocks</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Review Tasks To Start</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Review Status Report</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Review Schedule</span> </p></body></html>\",\n        \"Data Type\": \"projects\"\n    },\n    {\n        \"Type\": \"Meeting\",\n        \"Name\": \"Project Kickoff\",\n        \"Invitees\": \"Full Project Team\",\n        \"Subject\": \"[$projects.project_number.1] [$projects.project_name.1] - Project Kickoff\",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\\\">\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-weight:700;\\\">Project Kick Off Agenda </span></p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">The purpose of this meeting is to ensure that all team members have a comprehensive understanding of the project, we will conduct a thorough review of the current proposal. This review will cover the overall scope and the specific deliverables expected from the project. </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">During the proposal review, we will actively identify and highlight any potential risks associated with the project. Recognizing these risks early will help us prepare effective mitigation strategies and foster team alignment. </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-weight:700;\\\">Meeting Outline</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Review Team Roles / Introductions </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Review Project Business Drivers </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Review Scope of Work </p>\\n<ul style=\\\"margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; -qt-list-indent: 1;\\\">\\n<li style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Deliverables </li>\\n<li style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Assumptions &amp; Constraints </li>\\n<li style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Invoicing </li>\\n<li style=\\\" margin-top:0px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Identify Risks </li></ul>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Review Key Schedule Drivers and Schedule Risks </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Review Communication Plan </p>\\n<ul style=\\\"margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; -qt-list-indent: 1;\\\">\\n<li style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Discuss Approval Authority </li>\\n<li style=\\\" margin-top:0px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Discuss Primary Contact </li></ul>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Address Security and Safety Items </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Project Methodology </p>\\n<ul style=\\\"margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; -qt-list-indent: 1;\\\">\\n<li style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">DeltaV Waterfall Approach </li>\\n<li style=\\\" margin-top:0px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Syncade Workshops &amp; Workstreams </li></ul>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Review Company Standards </p>\\n<ul style=\\\"margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; -qt-list-indent: 1;\\\">\\n<li style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Drafting </li>\\n<li style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Programming </li>\\n<li style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Testing </li>\\n<li style=\\\" margin-top:0px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Other Corporate </li></ul>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Additional Items? </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Next Steps <br />\\u00a0\\u00a0\\u00a0 </p></body></html>\",\n        \"Data Type\": \"projects\"\n    },\n    {\n        \"Type\": \"Meeting\",\n        \"Name\": \"Project Kickoff (Internal)\",\n        \"Invitees\": \"Exclude Client\",\n        \"Subject\": \"[$projects.project_number.1] [$projects.project_name.1] - Project Kickoff (Internal)\",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\\\">\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-weight:700;\\\">Customer Project Kick Off Agenda (Internal) </span><br />The purpose of this meeting is to ensure that all team members have a comprehensive understanding of the project, we will conduct a thorough review of the current proposal. This review will cover the overall scope and the specific deliverables expected from the project. </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">During the proposal review, we will actively identify and highlight any potential risks associated with the project. Recognizing these risks early will help us prepare effective mitigation strategies and foster team alignment. </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Team members who have previous experience working with the client are encouraged to share their insights. These perspectives will provide valuable context on what it is like to collaborate with this client and may highlight important considerations or best practices for a successful partnership. </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">At the conclusion of the meeting, we will outline the actionable next steps for the team. This will ensure everyone is clear on their responsibilities and the path forward for the project. </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-weight:700;\\\">Meeting Outline</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Review Team Roles </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Sales Overview of Client &amp; Estimate </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Project Business Purpose </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Key Business Drivers </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Stakeholders Involved: People, Partners &amp; Vendors </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Future Sales Items/Sales Strategy </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Review Proposal </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Scope Of Work </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Deliverables </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Invoicing </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Assumptions &amp; Constraints </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Review Lessons Learned From Prior Projects </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Identify Risks </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Review Client Standards </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Drafting </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Programming </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Testing </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Other Corporate </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Additional Items? </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">Next Steps </p></body></html>\",\n        \"Data Type\": \"projects\"\n    },\n    {\n        \"Type\": \"Meeting\",\n        \"Name\": \"Project Status\",\n        \"Invitees\": \"Full Project Team\",\n        \"Subject\": \"[$projects.project_number.1] [$projects.project_name.1] - Project Status\",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\\\">\\n<p style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Aptos Display','sans-serif'; font-weight:700;\\\">Status Meeting Agenda</span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\"> <br />The purpose of this meeting is to make sure all work is progressing, everyone understands their assignments, and to identify any roadblocks that are holding up work or could hold up work.\\u00a0 We also want to review how we are tracking against the schedule and determine if we are still on track. </span></p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Review Action Items From Prior Meeting</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Provide Updates On Assigned Tasks</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Identify Any Roadblocks</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Review Tasks To Start</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Review Status Report</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Review Schedule</span> </p></body></html>\",\n        \"Data Type\": \"projects\"\n    },\n    {\n        \"Type\": \"Meeting\",\n        \"Name\": \"Lessons Learned (Internal)\",\n        \"Invitees\": \"Exclude Client\",\n        \"Subject\": \"[$projects.project_number.1] [$projects.project_name.1] - Lessons Learned (Internal)\",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\\\">\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-weight:700;\\\">Lessons Learned Meeting Agenda (Internal)</span></p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">The purpose of the meeting is to have a discussion and collectively identify lessons learned during the project, so future projects may benefit from our experience.\\u00a0 During our discussion we want to recognize and document what things went well, and why they were so successful.\\u00a0 We also want to identify areas that may need improvement, and how we might improve them on the next project. </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">During our time we will cover the following areas: </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Project Planning </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Project Execution </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Testing </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Project Communication </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Obstacles </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>On-Site Implementation </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Vendor Management </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Schedule And Budget </p></body></html>\",\n        \"Data Type\": \"projects\"\n    },\n    {\n        \"Type\": \"Meeting\",\n        \"Name\": \"Lessons Learned\",\n        \"Invitees\": \"Full Project Team\",\n        \"Subject\": \"[$projects.project_number.1] [$projects.project_name.1] - Lessons Learned\",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\\\">\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-weight:700;\\\">Lessons Learned Meeting Agenda </span></p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">The purpose of the meeting is to have a discussion and collectively identify lessons learned during the project, so future projects may benefit from our experience.\\u00a0 During our discussion we want to recognize and document what things went well, and why they were so successful.\\u00a0 We also want to identify areas that may need improvement, and how we might improve them on the next project. </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\">During our time we will cover the following areas: </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Project Planning </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Project Execution </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Testing </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Project Communication </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Obstacles </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>On-Site Implementation </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Vendor Management </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span>Schedule And Budget </p></body></html>\",\n        \"Data Type\": \"projects\"\n    },\n    {\n        \"Type\": \"Meeting\",\n        \"Name\": \"Proposal Working\",\n        \"Invitees\": \"Exclude Client\",\n        \"Subject\": \"[$projects.project_number.1] [$projects.project_name.1] - Proposal Working Meeting\",\n        \"Template\": \"<!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.0//EN\\\" \\\"http://www.w3.org/TR/REC-html40/strict.dtd\\\">\\n<html><head><meta name=\\\"qrichtext\\\" content=\\\"1\\\" /><meta charset=\\\"utf-8\\\" /><style type=\\\"text/css\\\">\\np, li { white-space: pre-wrap; }\\nhr { height: 1px; border-width: 0; }\\nli.unchecked::marker { content: \\\"\\\\2610\\\"; }\\nli.checked::marker { content: \\\"\\\\2612\\\"; }\\n</style></head><body style=\\\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\\\">\\n<p style=\\\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Aptos Display','sans-serif'; font-weight:700;\\\">Working Proposal Meeting</span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\"> <br />Review open items needed to complete the proposal. </span></p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Review Action Items From Prior Meeting</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Provide Updates On Assigned Tasks</span> </p>\\n<p style=\\\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\\\"><span style=\\\" font-family:'Symbol';\\\">\\u00b7</span><span style=\\\" font-family:'Times New Roman'; font-size:7pt;\\\">\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0\\u00a0 </span><span style=\\\" font-family:'Aptos Display','sans-serif';\\\">Identify Any Roadblocks</span> </p></body></html>\",\n        \"Data Type\": \"projects\"\n    }\n]"}}'''

def setup_default_meeting_email_types_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "Meeting and Email Types"

    if pnc_tmp.get_plugin_setting("MeetingEmailTypes", settings_pluginname) == "":
        defaults = json.loads(_MEETING_EMAIL_TYPES_DEFAULTS)
        pnc_tmp.set_plugin_setting("MeetingEmailTypes", settings_pluginname,
                                   defaults["Meeting and Email Types"]["MeetingEmailTypes"])

setup_default_file_finder_settings()
setup_default_editor_settings()
setup_default_meeting_email_types_settings()

pnc = ProjectNotesCommon()
mets = MeetingEmailTypesSettings(pnc.get_main_window())
sm = SettingsMigrator(pnc.get_main_window())
mss = MyShortcutSettings(pnc.get_main_window())
ois = OutlookIntegrationSettings(pnc.get_main_window())
es = EditorSettings(pnc.get_main_window())
ffs = FileFinderSettings(pnc.get_main_window())

# Use code below for testing
if __name__ == '__main__':
    print("Entered __main__")
    app = QApplication(sys.argv)
    os.chdir("..")
    #menu_outlook_integration_settings("") 
    menu_meeting_email_types_settings("")
    #menu_file_collector_settings("")
    sys.exit(app.exec())
