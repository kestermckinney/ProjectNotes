import sys
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "MS Project"
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

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    #
    pnc = ProjectNotesCommon()

    def menuMSProject(xmlstr, parameter):

        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""
            
        if not pnc.verify_global_settings():
            return ""

        # setup global variable
        ProjectsFolder = pnc.get_plugin_setting("ProjectsFolder")

        # prompt for the template to use
        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = pnc.get_projectfolder(xmlroot)
        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()
        cm = xmlroot.attributes().namedItem("managing_company_name").nodeValue()

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projnam = pnc.get_column_value(projtab.firstChild(), "project_name")

        if (projectfolder is None or projectfolder =="" or not QDir(projectfolder).exists()):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""
        else:
            projectfolder = projectfolder + "\\Schedule\\"


        templatefile = QFileDialog.getOpenFileName(None, "Select the MS Project Template", QDir.currentPath() + "\\plugins\\templates\\","Project files (*.mpp)|*.mpp")

        if templatefile is None or templatefile[0] == "":
            return ""

        basename = projnam[:30]
        projectfile = projectfolder + basename + ".mpp"

        # copy the file
        if not QFile(projectfile).exists():
            if not QFile(templatefile[0]).copy(projectfile):
                QMessageBox.critical(None, "Unable to copy template", "Could not copy " + templatefile[0] + " to " + projectfile, QMessageBox.StandardButton.Cancel)
                return ""


        project = win32com.client.Dispatch("MSProject.Application")

        project.FileOpen(projectfile)
        project.Visible = 1

        project.ActiveProject.ProjectStart = QDateTime.currentDateTime().addDays(-7).toString("MM/dd/yyyy")

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

    pluginmenus.append({"menutitle" : "MS Project", "function" : "menuMSProject", "tablefilter" : "projects", "submenu" : "Templates", "dataexport" : "projects"})

# setup test data
"""
import sys
print("Buld up QDomDocument")
#

xmldoc = QDomDocument("TestDocument")
f = QFile("C:/Users/pamcki/Desktop/project.xml")

if f.open(QIODevice.OpenModeFlag.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

print("Run Test")
# call when testing outside of Project Notes
print(event_data_rightclick(xmldoc.toString()))
print("Finished")

"""
