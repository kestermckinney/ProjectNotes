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

    # processing main function
    def menuPowerPoint(xmlstr, parameter):

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
            QMessageBox.critical(None, "Folder Does Not Exist", msg)
            return ""

        # copy the file
        if not QFile(projectfile).exists():
            if not QFile(templatefile[0]).copy(projectfile):
                QMessageBox.critical(None, "Unable to copy template", "Could not copy " + templatefile[0] + " to " + projectfile)
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

        row.appendChild(pnc.xml_col(docxml, "project_id",None, projnum))
        row.appendChild(pnc.xml_col(docxml, "location_type", "PowerPoint Document", None))
        row.appendChild(pnc.xml_col(docxml, "location_description", basename, None))
        row.appendChild(pnc.xml_col(docxml, "full_path", projectfile, None))

        return docxml.toString()

    def menuSettings(parameter):
        nps.show()
        return ""

    nps = NewPowerPointSettings()

    pluginmenus.append({"menutitle" : "PowerPoint", "function" : "menuPowerPoint", "tablefilter" : "", "submenu" : "Templates", "dataexport" : "projects"})
    pluginmenus.append({"menutitle" : "New PowerPoint", "function" : "menuSettings", "tablefilter" : "", "submenu" : "Settings", "dataexport" : ""})


#setup test data
if __name__ == '__main__':
    import os
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)

    xml_content = ""
    with open("C:\\Users\\pamcki\\OneDrive - Cornerstone Controls\\Documents\\Work In Progress\\XML\\project.xml", 'r', encoding='utf-8') as file:
        xml_content = file.read()

    menuPowerPoint(xml_content, "")    

    app.exec()