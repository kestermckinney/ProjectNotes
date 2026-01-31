
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo, QDir, QRect
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "New Risk Register"
plugindescription = "Copy the selected Excel Risk Register template, adding project information to the file."

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

# Custom Setting
class NewRiskRegisterSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "New Risk Register"

        self.ui = uic.loadUi("plugins/forms/dialogExportLocation.ui", self)
        self.ui.setWindowTitle("New Risk Register Sub Folder")
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
        self.ui.lineEditExportSubFolder.setText(self.export_subfolder)

        if (x != '' and y != '' and w != '' and h != ''):
            self.ui.setGeometry(QRect(int(x), int(y), int(w), int(h)))

    def save_window_state(self):
        # Save window position and size
        pnc.set_plugin_setting("X", self.settings_pluginname, f"{self.pos().x()}")
        pnc.set_plugin_setting("Y", self.settings_pluginname, f"{self.pos().y()}")
        pnc.set_plugin_setting("W", self.settings_pluginname, f"{self.size().width()}")
        pnc.set_plugin_setting("H", self.settings_pluginname, f"{self.size().height()}")

        # print(f"saving dimensions {self.pos().x()},{self.pos().y()},{self.size().width()},{self.size().height()}")

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

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pnc = ProjectNotesCommon()
    pne = ProjectNotesExcelTools()

    # processing main function
    def menuRiskRegistry(xmlstr, parameter):
        
        xmlval = QDomDocument()

        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return ""

        # prompt for the template to use
        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = pnc.get_projectfolder(xmlroot)
        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()
        cm = xmlroot.attributes().namedItem("managing_company_name").nodeValue()

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projnam = pnc.get_column_value(projtab.firstChild(), "project_name")

        export_subfolder = pnc.get_plugin_setting("ExportSubFolder", "New Risk Register")

        if (projectfolder is None or projectfolder =="" or not QDir(projectfolder).exists()):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""
        else:
            projectfolder = projectfolder + f"/{export_subfolder}/"

        templatefile =  "plugins\\templates\\Risk Register Template.xlsx"

        tempfileinfo = QFileInfo(templatefile)
        basename = projnum + " " + tempfileinfo.baseName() + "." + tempfileinfo.suffix()
        basename = basename.replace(" Template", "")

        projectfile = projectfolder + basename

        projectfile = projectfile.replace("/", "\\") # office products have to have backslash

        if not pnc.folder_exists(projectfile):
            msg = f'Folder for "{projectfile}" does not exist.  Cannot copy the template.'
            print(msg)
            QMessageBox.critical(None, "Folder Does Not Exist", msg)
            return ""

        # copy the file
        if not QFile(projectfile).exists():
            if not QFile(templatefile).copy(projectfile):
                QMessageBox.critical(None, "Unable to copy template", "Could not copy " + templatefile + " to " + projectfile)
                return ""


        handle = pne.open_excel_document(projectfile)
        sheet = handle['workbook'].Sheets("Risk Register")

        pne.replace_cell_tag(sheet, "<PROJECTNAME>", projnum + " " + projnam)

        handle['workbook'].Save()

        pne.close_excel_document(handle)

        QDesktopServices.openUrl(QUrl("file:///" + projectfile))

        # add the location to the project
        docxml = QDomDocument()
        docroot = docxml.createElement("projectnotes");
        docxml.appendChild(docroot);

        table = pnc.xml_table(docxml, "project_locations")
        docroot.appendChild(table)

        row = pnc.xml_row(docxml)
        table.appendChild(row)

        row.appendChild(pnc.xml_col(docxml, "project_id",None, projnum))
        row.appendChild(pnc.xml_col(docxml, "location_type", "Excel Document", None))
        row.appendChild(pnc.xml_col(docxml, "location_description", basename, None))
        row.appendChild(pnc.xml_col(docxml, "full_path", projectfile, None))

        return docxml.toString()

    def menuSettings(parameter):
        trs.show()
        return ""

    trs = NewRiskRegisterSettings()

    pluginmenus.append({"menutitle" : "Risk Register", "function" : "menuRiskRegistry", "tablefilter" : "", "submenu" : "Templates", "dataexport" : "projects"})
    pluginmenus.append({"menutitle" : "New Risk Resgister", "function" : "menuSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""})

#setup test data
if __name__ == '__main__':
    import os
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)

    xml_content = ""
    with open("C:\\Users\\pamcki\\OneDrive - Cornerstone Controls\\Documents\\Work In Progress\\XML\\project.xml", 'r', encoding='utf-8') as file:
        xml_content = file.read()

    menuRiskRegistry(xml_content, "")    

    app.exec()