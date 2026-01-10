
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QFileInfo, QDir
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "PCR Registry"
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

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pnc = ProjectNotesCommon()

    # processing main function
    def menuPCRRegistry(xmlstr, parameter):

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

        if (projectfolder is None or projectfolder =="" or not QDir(projectfolder).exists()):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""
        else:
            projectfolder = projectfolder + "\\Project Management\\PCR's\\"


        templatefile =  "plugins\\templates\\PCR Registry Template.xlsx"

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

    pluginmenus.append({"menutitle" : "PCR Registry", "function" : "menuPCRRegistry", "tablefilter" : "", "submenu" : "Templates", "dataexport" : "projects"})

#setup test data
if __name__ == '__main__':
    import os
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)

    xml_content = ""
    with open("C:\\Users\\pamcki\\OneDrive - Cornerstone Controls\\Documents\\Work In Progress\\XML\\project.xml", 'r', encoding='utf-8') as file:
        xml_content = file.read()

    menuPCRRegistry(xml_content, "")    

    app.exec()
