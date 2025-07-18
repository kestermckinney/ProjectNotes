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
    {"menutitle" : "File Collector", "function" : "menuFileCollectorSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Editor", "function" : "menuEditorSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Outlook Integration", "function" : "menuOutlookIntegrationSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "My Shortcuts", "function" : "menuMyShortcutSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Meeting and Email Types", "function" : "menuMeetingEmailTypesSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
    {"menutitle" : "Settings Migrator", "function" : "menuSettingsMigrator", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""},
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

    def setItems(self, items):
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

# File Finder populates the list of files associated with a project
class FileFinderSettings(QDialog):
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "File Finder"

        self.ui = uic.loadUi("plugins/forms/dialogFileFinder.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
 
        self.ui_class = uic.loadUi("plugins/forms/dialogClassification.ui")
        self.ui_class.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )

        self.ui.pushButtonAddLocation.clicked.connect(self.addlocation)
        self.ui.pushButtonEditLocation.clicked.connect(self.editlocation)
        self.ui.pushButtonDeleteLocation.clicked.connect(self.deletelocation)
        self.ui.pushButtonAddClassification.clicked.connect(self.addclassification)
        self.ui.pushButtonEditClassification.clicked.connect(self.editclassification)
        self.ui.pushButtonDeleteClassification.clicked.connect(self.deleteclassification)
        self.ui.buttonBox.accepted.connect(self.save_settings)

        delegate = ComboBoxDelegate()
        delegate.setItems([
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

        if (x != '' and y != '' and w != '' and h != ''):
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

        self.show()

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

    def addlocation(self):
        folder_path = QFileDialog.getExistingDirectory(caption="Select a folder")
        if (folder_path is not None and folder_path != ''):
            row_count = self.ui.tableSearchLocations.rowCount()
            self.ui.tableSearchLocations.setRowCount(row_count + 1)
            self.ui.tableSearchLocations.setItem(row_count, 0, QTableWidgetItem(folder_path))

    def editlocation(self):
        row = self.ui.tableSearchLocations.currentRow()

        if (row > -1):
            value = self.ui.tableSearchLocations.item(row, 0).text()
            folder_path = QFileDialog.getExistingDirectory(caption="Select a folder", directory=value)
            if (folder_path is not None and folder_path != ''):
                self.ui.tableSearchLocations.setItem(row, 0, QTableWidgetItem(folder_path))

    def deletelocation(self):
        row = self.ui.tableSearchLocations.currentRow()
        if (row > -1):
            value = self.ui.tableSearchLocations.removeRow(row)

    def addclassification(self):
        self.ui_class.comboBoxClassification.setCurrentText('')
        self.ui_class.lineEditPatternMatch.setText('')

        if (self.ui_class.exec()):
            row_count = self.ui.tableClassifications.rowCount()
            self.ui.tableClassifications.setRowCount(row_count + 1)
            self.ui.tableClassifications.setItem(row_count, 0, QTableWidgetItem(self.ui_class.comboBoxClassification.currentText()))
            self.ui.tableClassifications.setItem(row_count, 1, QTableWidgetItem(self.ui_class.lineEditPatternMatch.text()))

    def editclassification(self):
        row = self.ui.tableClassifications.currentRow()

        if (row > -1):
            clss = self.ui.tableClassifications.item(row, 0).text()
            pat = self.ui.tableClassifications.item(row, 1).text()

            self.ui_class.comboBoxClassification.setCurrentText(clss)
            self.ui_class.lineEditPatternMatch.setText(pat)

            if (self.ui_class.exec()):
                self.ui.tableClassifications.setItem(row, 0, QTableWidgetItem(self.ui_class.comboBoxClassification.currentText()))
                self.ui.tableClassifications.setItem(row, 1, QTableWidgetItem(self.ui_class.lineEditPatternMatch.text()))

    def deleteclassification(self):
        row = self.ui.tableClassifications.currentRow()
        if (row > -1):
            value = self.ui.tableClassifications.removeRow(row)

    def save_settings(self):
        self.search_locations = self.copy_table_to_json(self.ui.tableSearchLocations)
        self.pnc.set_plugin_setting("SearchLocations", self.settings_pluginname, self.search_locations)

        self.classifications = self.copy_table_to_json(self.ui.tableClassifications)
        self.pnc.set_plugin_setting("Classifications", self.settings_pluginname, self.classifications)

        self.ui.close()

    def closeEvent(self, event):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        self.pnc.set_plugin_setting("lc1", self.settings_pluginname, f"{self.ui.tableSearchLocations.columnWidth(0)}")
        self.pnc.set_plugin_setting("mc1", self.settings_pluginname, f"{self.ui.tableClassifications.columnWidth(0)}")
        self.pnc.set_plugin_setting("mc2", self.settings_pluginname, f"{self.ui.tableClassifications.columnWidth(1)}")

        # Call the base class implementation
        super().closeEvent(event)

# Custom Editor Setting
class EditorSettings(QDialog):
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "Custom Editor"

        self.ui = uic.loadUi("plugins/forms/dialogEditor.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )

        self.ui.pushButtonFilePick.clicked.connect(self.editlocation)
        self.ui.buttonBox.accepted.connect(self.save_settings)

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        self.editor_path = self.pnc.get_plugin_setting("EditorPath", self.settings_pluginname)
        self.ui.lineEditFullPath.setText(self.editor_path)

        if (x != '' and y != '' and w != '' and h != ''):
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

        self.show()

    def editlocation(self):
        value = self.ui.lineEditFullPath.text()
        if sys.platform == "win32":
            pat = "Application (*.exe)"
        else:
            pat = "Application (*)"

        file_path, filt = QFileDialog.getOpenFileName(caption="Select an application", directory=value,filter=pat)

        if (file_path is not None and file_path != ''):
            self.ui.lineEditFullPath.setText(file_path)        

    def save_settings(self):
        self.editor_path = self.ui.lineEditFullPath.text()
        self.pnc.set_plugin_setting("EditorPath", self.settings_pluginname, self.editor_path)

        self.ui.close()

    def closeEvent(self, event):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # Call the base class implementation
        super().closeEvent(event)

# Outlook Integration Settings
class OutlookIntegrationSettings(QDialog):
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "Outlook Integration"

        self.ui = uic.loadUi("plugins/forms/dialogOutlookIntegrationOptions.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )

        self.ui.buttonBox.accepted.connect(self.save_settings)

        x = self.pnc.get_plugin_setting("X", self.settings_pluginname)
        y = self.pnc.get_plugin_setting("Y", self.settings_pluginname)
        w = self.pnc.get_plugin_setting("W", self.settings_pluginname)
        h = self.pnc.get_plugin_setting("H", self.settings_pluginname)

        self.ui.comboBoxIntegrationType.setCurrentText(self.pnc.get_plugin_setting("IntegrationType", self.settings_pluginname))
        self.ui.lineEditApplicationID.setText(self.pnc.get_plugin_setting("ApplicationID", self.settings_pluginname))
        self.ui.lineEditTenantID.setText(self.pnc.get_plugin_setting("TenantID", self.settings_pluginname))
        self.ui.checkBoxExportContacts.setChecked(self.pnc.get_plugin_setting("ExportContacts", self.settings_pluginname).lower() == "true")
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

        self.show()

    def save_settings(self):

        self.pnc.set_plugin_setting("IntegrationType", self.settings_pluginname, self.ui.comboBoxIntegrationType.currentText())
        self.pnc.set_plugin_setting("ApplicationID", self.settings_pluginname, self.ui.lineEditApplicationID.text())
        self.pnc.set_plugin_setting("TenantID", self.settings_pluginname, self.ui.lineEditTenantID.text())
        self.pnc.set_plugin_setting("ExportContacts", self.settings_pluginname, "true" if self.ui.checkBoxExportContacts.isChecked() else "false")
        self.pnc.set_plugin_setting("ExportContacts", self.settings_pluginname, "true" if self.ui.checkBoxExportContacts.isChecked() else "false")
        self.pnc.set_plugin_setting("SyncToDoWithDue", self.settings_pluginname, "true" if self.ui.checkBoxSyncToDoWithDue.isChecked() else "false")
        self.pnc.set_plugin_setting("SyncToDoDoWithoutDue", self.settings_pluginname, "true" if self.ui.checkBoxSyncToDoWithoutDue.isChecked() else "false")
        self.pnc.set_plugin_setting("BackupEmails", self.settings_pluginname, "true" if self.ui.checkBoxBackupEmails.isChecked() else "false")
        self.pnc.set_plugin_setting("BackupInBoxFolder", self.settings_pluginname, self.ui.lineEditBackupInboxFolder.text())
        self.pnc.set_plugin_setting("BackupSentFolder", self.settings_pluginname, self.ui.lineEditBackupSentFolder.text())
        self.pnc.set_plugin_setting("ScheduleO365", self.settings_pluginname, "true" if self.ui.checkBoxScheduleO365.isChecked() else "false")
        self.pnc.set_plugin_setting("SendO365", self.settings_pluginname, "true" if self.ui.checkBoxSendO365.isChecked() else "false")

        projectnotes.force_reload("outlooksync_thread")

        self.ui.close()

    def closeEvent(self, event):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # Call the base class implementation
        super().closeEvent(event)

class MyShortcutSettings(QDialog):
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "My Shortcuts"

        self.ui = uic.loadUi("plugins/forms/dialogMyShortcuts.ui", self)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
 
        self.ui.pushButtonAddShortcut.clicked.connect(self.addshortcut)
        self.ui.pushButtonEditShortcut.clicked.connect(self.editshortcut)
        self.ui.pushButtonDeleteShortcut.clicked.connect(self.deleteshortcut)
        self.ui.buttonBox.accepted.connect(self.save_settings)

        delegate = ComboBoxDelegate(self.ui, False)
        delegate.setItems([
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

        self.show()

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

    def addshortcut(self):

        row_count = self.ui.tableShortcuts.rowCount()
        self.ui.tableShortcuts.setRowCount(row_count + 1)
        self.ui.tableShortcuts.setItem(row_count, 0, QTableWidgetItem("[New Shortcut]"))

    def editshortcut(self):
        row = self.ui.tableShortcuts.currentRow()

        if (row > -1):
            self.ui.tableShortcuts.setCurrentCell(row, 0)
            self.ui.tableShortcuts.edit(self.ui.tableShortcuts.currentIndex())

    def deleteshortcut(self):
        row = self.ui.tableShortcuts.currentRow()
        if (row > -1):
            value = self.ui.tableShortcuts.removeRow(row)

    def save_settings(self):
        self.myshortcuts = self.copy_table_to_json(self.ui.tableShortcuts)
        self.pnc.set_plugin_setting("MyShortcuts", self.settings_pluginname, self.myshortcuts)

        projectnotes.force_reload("myshortcuts_plugin")

        self.ui.close()

    def closeEvent(self, event):
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        self.pnc.set_plugin_setting("c1", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(0)}")
        self.pnc.set_plugin_setting("c2", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(1)}")
        self.pnc.set_plugin_setting("c3", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(2)}")
        self.pnc.set_plugin_setting("c4", self.settings_pluginname, f"{self.ui.tableShortcuts.columnWidth(3)}")

        # Call the base class implementation
        super().closeEvent(event)

class MeetingEmailTypesSettings(QDialog):
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "Meeting and Email Types"

        self.ui = uic.loadUi("plugins/forms/dialogMeetingEmailTypes.ui", self)

        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
 
        self.ui_template = uic.loadUi("plugins/forms/dialogMeetingEmailTemplate.ui")
        self.ui_template.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )

        self.ui.pushButtonAddType.clicked.connect(self.addtype)
        self.ui.pushButtonEditType.clicked.connect(self.edittype)
        self.ui.pushButtonDeleteType.clicked.connect(self.deletetype)
        self.ui.buttonBox.accepted.connect(self.save_settings)

        delegate = ComboBoxDelegate()
        delegate.setItems([
           "Internal Project Team",
           "Exclude Client",
           "Only Client",
           "Full Project Team",
           "Individual",
           "Attachment Only"])

        self.ui.tableWidgetMeetingEmailTypes.setItemDelegateForColumn(1, delegate)

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

        geometry = self.pnc.get_plugin_setting("types_geometry", self.settings_pluginname)

        if (c1 != '' and c2 != '' and c3 != '' and c4 != '' and c5 != ''):
            print(f"loading column sizes {c1},{c2},{c3},{c4}")

            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(0, int(c1))
            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(1, int(c2))
            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(2, int(c3))
            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(3, int(c4))
            self.ui.tableWidgetMeetingEmailTypes.setColumnWidth(5, int(c5))

        if (x != '' and y != '' and w != '' and h != ''):
            print(f"loading dimensions {int(x)},{int(y)},{int(w)},{int(h)}")
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

        self.show()

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

    def addtype(self):
        self.ui_template.comboBoxType.setCurrentText("Meeting")
        self.ui_template.lineEditName.setText('')
        self.ui_template.comboBoxInvitees.setCurrentText('')
        self.ui_template.textEditTemplate.setHtml('')

        if (self.ui_template.exec()):
            row_count = self.ui.tableWidgetMeetingEmailTypes.rowCount()
            self.ui.tableWidgetMeetingEmailTypes.setRowCount(row_count + 1)
            self.ui.tableWidgetMeetingEmailTypes.setItem(row_count, 0, QTableWidgetItem(self.ui_template.comboBoxType.currentText()))
            self.ui.tableWidgetMeetingEmailTypes.setItem(row_count, 1, QTableWidgetItem(self.ui_template.lineEditName.text()))
            self.ui.tableWidgetMeetingEmailTypes.setItem(row_count, 2, QTableWidgetItem(self.ui_template.comboBoxInvitees.currentText()))
            self.ui.tableWidgetMeetingEmailTypes.setItem(row_count, 3, QTableWidgetItem(self.ui_template.lineEditSubject.text()))
            self.ui.tableWidgetMeetingEmailTypes.setItem(row_count, 4, QTableWidgetItem(self.ui_template.textEditTemplate.toHtml()))

    def edittype(self):
        row = self.ui.tableWidgetMeetingEmailTypes.currentRow()

        if (row > -1):
            mtype = self.ui.tableWidgetMeetingEmailTypes.item(row, 0).text()
            nam = self.ui.tableWidgetMeetingEmailTypes.item(row, 1).text()
            matt = self.ui.tableWidgetMeetingEmailTypes.item(row, 2).text()
            subj = self.ui.tableWidgetMeetingEmailTypes.item(row, 3).text() 
            mhtml = self.ui.tableWidgetMeetingEmailTypes.item(row, 4).text() 

            self.ui_template.comboBoxType.setCurrentText(mtype)
            self.ui_template.lineEditName.setText(nam)
            self.ui_template.comboBoxInvitees.setCurrentText(matt)
            self.ui_template.lineEditSubject.setText(subj)
            self.ui_template.textEditTemplate.setHtml(mhtml)

            if (self.ui_template.exec()):
                self.ui.tableWidgetMeetingEmailTypes.setItem(row, 0, QTableWidgetItem(self.ui_template.comboBoxType.currentText()))
                self.ui.tableWidgetMeetingEmailTypes.setItem(row, 1, QTableWidgetItem(self.ui_template.lineEditName.text()))
                self.ui.tableWidgetMeetingEmailTypes.setItem(row, 2, QTableWidgetItem(self.ui_template.comboBoxInvitees.currentText()))
                self.ui.tableWidgetMeetingEmailTypes.setItem(row, 3, QTableWidgetItem(self.ui_template.lineEditSubject.text()))
                self.ui.tableWidgetMeetingEmailTypes.setItem(row, 4, QTableWidgetItem(self.ui_template.textEditTemplate.toHtml()))

    def deletetype(self):
        row = self.ui.tableWidgetMeetingEmailTypes.currentRow()
        if (row > -1):
            value = self.ui.tableWidgetMeetingEmailTypes.removeRow(row)

    def save_settings(self):
        self.meeting_types = self.copy_table_to_json(self.ui.tableWidgetMeetingEmailTypes)
        self.pnc.set_plugin_setting("MeetingEmailTypes", self.settings_pluginname, self.meeting_types)

        self.ui.close()

        projectnotes.force_reload("base_plugin")


    def closeEvent(self, event):
        print("saving values")
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

        # Call the base class implementation
        super().closeEvent(event)


class SettingsMigrator(QDialog):
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "Settings Migrator"

        self.ui = uic.loadUi("plugins/forms/dialogSettingsMigrator.ui", self)

        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint
            )
 
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

        self.show()

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
            QMessageBox.critical(None, "Cannot Parse JSON", "Unable to import settings.",QMessageBox.StandardButton.Cancel)
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
            QMessageBox.critical(None, "Cannot Parse JSON", "Unable to import settings.",QMessageBox.StandardButton.Cancel)

        self.load_all_plugin_settings()

        return

    def delete_settings(self):

        # Show confirmation dialog
        msg_box = QMessageBox(self)
        msg_box.setWindowTitle("Confirm Deletion")
        msg_box.setText("Are you sure you want to delete the selected settings?")
        msg_box.setStandardButtons(QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        msg_box.setDefaultButton(QMessageBox.StandardButton.No)
        msg_box.setIcon(QMessageBox.Icon.Warning)
        
        if msg_box.exec() != QMessageBox.StandardButton.Yes:        
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
            QMessageBox.critical(None, "Cannot Delete Settings", "An error occured while removing settings settings.",QMessageBox.StandardButton.Cancel)

        self.load_all_plugin_settings()
        
        return

    def closeEvent(self, event):
        print("saving values")
        # Save window position and size
        self.pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        self.pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        self.pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        self.pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # Call the base class implementation
        super().closeEvent(event)


def menuFileCollectorSettings(parameter):
    settings_dialog = FileFinderSettings() 
    return ""

def menuEditorSettings(parameter):
    settings_dialog = EditorSettings()
    return ""

def menuOutlookIntegrationSettings(parameter):
    settings_dialog = OutlookIntegrationSettings()
    return ""

def menuMyShortcutSettings(parameter):
    settings_dialog = MyShortcutSettings()
    return ""

def menuMeetingEmailTypesSettings(parameter):
    settings_dialog = MeetingEmailTypesSettings()
    return ""

def menuSettingsMigrator(parameter):
    settings_dialog = SettingsMigrator()
    return ""


# Use code below for testing
if __name__ == '__main__':
    print("Entered __main__")
    app = QApplication(sys.argv)
    #menuOutlookIntegrationSettings("") 
    menuMeetingEmailTypesSettings("")
    sys.exit(app.exec())

#todo: add the ability to quickly add a team member that isn't in the       databse