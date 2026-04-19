# Copyright (C) 2021, 2022, 2023, 2024, 2025, 2026 Paul McKinney
import sys
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QFileInfo, QDir, QRect
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "New PowerPoint"
plugindescription = "Copy the selected PowerPoint template, adding project information to the file."

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
class NewPowerPointSettings(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)
        self.settings_pluginname = "New PowerPoint"

        self.ui = uic.loadUi("plugins/forms/dialogExportLocation.ui", self)
        self.ui.setWindowTitle("New PowerPoint Folder")
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

def setup_default_new_powerpoint_settings():
    pnc_tmp = ProjectNotesCommon()
    settings_pluginname = "New PowerPoint"
    if pnc_tmp.get_plugin_setting("ExportSubFolder", settings_pluginname) is None:
        pnc_tmp.set_plugin_setting("ExportSubFolder", settings_pluginname, "Project Management/Meeting Minutes")

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    setup_default_new_powerpoint_settings()
    #
    pnc = ProjectNotesCommon()

    # processing main function
    def menu_power_point(xmlstr, parameter):

        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(QApplication.activeWindow(),"Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return ""

        # prompt for the template to use
        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = pnc.get_projectfolder(xmlroot)
        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()
        cm = xmlroot.attributes().namedItem("managing_company_name").nodeValue()

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projnam = pnc.get_column_value(projtab.firstChild(), "project_name")

        export_subfolder = pnc.get_plugin_setting("ExportSubFolder", "New PowerPoint")

        if (projectfolder is None or projectfolder =="" or not QDir(projectfolder).exists()):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""
        else:
            projectfolder = projectfolder + f"/{export_subfolder}/"

        print(f"setting is {projectfolder}")

        templatefile = QFileDialog.getOpenFileName(None, "Select the PowerPoint Template", QDir.currentPath() + "\\plugins\\templates\\","PowerPoint files (*.ppt;*.pptx;*.pptm)|*.ppt;*.pptx;*.pptm")

        if templatefile is None or templatefile[0] == "":
            return ""

        tempfileinfo = QFileInfo(templatefile[0])
        basename = projnum + " " + tempfileinfo.baseName() + "." + tempfileinfo.suffix()
        basename = basename.replace(" Template", "")

        projectfile = projectfolder + basename

        projectfile = projectfile.replace("/", "\\") # office products have to have backslash

        if not pnc.folder_exists(projectfile):
            msg = f'Folder for "{projectfile}" does not exist.  Cannot copy the template.'
            print(msg)
            QMessageBox.critical(QApplication.activeWindow(),"Folder Does Not Exist", msg)
            return ""

        # copy the file
        if not QFile(projectfile).exists():
            if not QFile(templatefile[0]).copy(projectfile):
                QMessageBox.critical(QApplication.activeWindow(),"Unable to copy template", "Could not copy " + templatefile[0] + " to " + projectfile)
                return ""


        # change the values for the project specifics in the file
        #-exec_program("cscript.exe open_project.vbs \"" + projectfile + "\"  \"" + basename + "\" \"" + pm + "\"")
        powerpoint = win32com.client.Dispatch("PowerPoint.Application")
        pptfile = powerpoint.Presentations.Open(projectfile)
        count = pptfile.Slides.Count
        cur = 1

        while (cur <= count):
            for shap in pptfile.Slides(cur).Shapes:
                if shap.HasTextFrame != 0:
                    if shap.TextFrame.HasText != 0:
                        shap.TextFrame.TextRange.Replace("<PROJECTNUMBER>", projnum)
                        shap.TextFrame.TextRange.Replace("<PROJECTNAME>", projnam)

            cur = cur + 1

        pptfile.Save()
        pptfile = None
        powerpoint = None

        # add the location to the project
        docxml = QDomDocument()
        docroot = docxml.createElement("projectnotes");
        docxml.appendChild(docroot);

        table = pnc.xml_table(docxml, "project_locations")
        docroot.appendChild(table)

        row = pnc.xml_row(docxml)
        table.appendChild(row)

        row.appendChild(pnc.xml_col(docxml, "id",None, projnum))
        row.appendChild(pnc.xml_col(docxml, "location_type", "PowerPoint Document", None))
        row.appendChild(pnc.xml_col(docxml, "location_description", basename, None))
        row.appendChild(pnc.xml_col(docxml, "full_path", projectfile, None))

        return docxml.toString()

    def menu_settings(parameter):
        nps.show()
        return ""

    nps = NewPowerPointSettings()

    pluginmenus.append({"menutitle" : "PowerPoint", "function" : "menu_power_point", "tablefilter" : "", "submenu" : "Templates", "dataexport" : "projects"})
    pluginmenus.append({"menutitle" : "New PowerPoint", "function" : "menu_settings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""})


#setup test data
if __name__ == '__main__':
    import os
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)

    xml_content = ""
    with open("C:\\Users\\pamcki\\OneDrive - Cornerstone Controls\\Documents\\Work In Progress\\XML\\project.xml", 'r', encoding='utf-8') as file:
        xml_content = file.read()

    menu_power_point(xml_content, "")    

    app.exec()