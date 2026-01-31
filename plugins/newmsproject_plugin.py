import sys
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo, QRect
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "New MS Project"
plugindescription = "Copy the selected MS Project Template"

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
class NewMSProjectSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "New MS Project"

        self.ui = uic.loadUi("plugins/forms/dialogExportLocation.ui", self)
        self.ui.setWindowTitle("New MS Project Sub Folder")
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
    #
    pnc = ProjectNotesCommon()

    def menuMSProject(xmlstr, parameter):

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

        #print(f"Searching for output folder {projectfolder}")
        export_subfolder = pnc.get_plugin_setting("ExportSubFolder", "New MS Project")

        if (projectfolder is None or projectfolder =="" or not QDir(projectfolder).exists()):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""
        else:
            projectfolder = projectfolder + f"/{export_subfolder}/"

        templatefile = QFileDialog.getOpenFileName(None, "Select the MS Project Template", QDir.currentPath() + "\\plugins\\templates\\","Project files (*.mpp)|*.mpp")

        if templatefile is None or templatefile[0] == "":
            return ""

        basename = projnam[:30]
        projectfile = projectfolder + basename + ".mpp"

        projectfile = projectfile.replace("/", "\\")

        if not pnc.folder_exists(projectfile):
            msg = f'Folder for "{projectfile}" does not exist.  Cannot copy the template.'
            print(msg)
            QMessageBox.critical(None, "Folder Does Not Exist", msg)
            return ""

        # copy the file
        if not QFile(projectfile).exists():
            if not QFile(templatefile[0]).copy(projectfile):
                QMessageBox.critical(None, "Unable to copy template", "Could not copy " + templatefile[0] + " to " + projectfile)
                return ""


        project = win32com.client.Dispatch("MSProject.Application")

        project.FileOpen(projectfile)
        project.Visible = 1

        # this causes the project to error since all project dates are in the past // project.ActiveProject.ProjectStart = QDateTime.currentDateTime().addDays(-7).toString("MM/dd/yyyy")

        project.ReplaceEx("Name", "contains", "PROJECTNAME", basename, True, True, False, 188743694, 7, True)

        project.FileSave()

        # add the location to the project
        docxml = QDomDocument()
        docroot = docxml.createElement("projectnotes");
        docxml.appendChild(docroot);

        table = pnc.xml_table(docxml, "project_locations")
        docroot.appendChild(table)

        row = pnc.xml_row(docxml)
        table.appendChild(row)

        row.appendChild(pnc.xml_col(docxml, "project_id",None, projnum))
        row.appendChild(pnc.xml_col(docxml, "location_type", "Microsoft Project", None))
        row.appendChild(pnc.xml_col(docxml, "location_description", "Project Schedule : " + basename + ".mpp", None))
        row.appendChild(pnc.xml_col(docxml, "full_path", projectfile, None))

        return docxml.toString()

    def menuSettings(parameter):
        nms.show()
        return ""

    nms = NewMSProjectSettings()

    pluginmenus.append({"menutitle" : "MS Project", "function" : "menuMSProject", "tablefilter" : "", "submenu" : "Templates", "dataexport" : "projects"})
    pluginmenus.append({"menutitle" : "New MS Project", "function" : "menuSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""})

#setup test data
if __name__ == '__main__':
    import os
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)

    xml_content = ""
    with open("C:\\Users\\pamcki\\OneDrive - Cornerstone Controls\\Documents\\Work In Progress\\XML\\project.xml", 'r', encoding='utf-8') as file:
        xml_content = file.read()

    menuMSProject(xml_content, "")

    app.exec()