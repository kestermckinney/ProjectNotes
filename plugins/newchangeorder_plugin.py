import sys
import platform
import re

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QFileInfo, QDir
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog, QInputDialog, QLineEdit
from PyQt6.QtGui import QDesktopServices

# Project Notes Plugin Parameters
pluginname = "Change Order"
plugindescription = "Copy the Change Order template, adding project information to the file."

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

    def menuChangeOrder(xmlstr, parameter):
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""

        # prompt for the template to use
        statusdate = QDateTime.currentDateTime()
        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = pnc.get_projectfolder(xmlroot)
        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()


        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projnam = pnc.get_column_value(projtab.firstChild(), "project_name")
        clientnam = pnc.get_column_value(projtab.firstChild(), "client_id")

        ok = False

        changenum, ok = QInputDialog.getText(None, "Change Order Number", "Number 0#:", QLineEdit.EchoMode.Normal, "", QtCore.Qt.WindowType.Window | QtCore.Qt.WindowType.WindowCloseButtonHint | QtCore.Qt.WindowType.WindowStaysOnTopHint)

        if ok == False:
            return ""

        if (projectfolder is None or projectfolder =="" or not QDir(projectfolder).exists()):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""
        else:
            projectfolder = projectfolder + "\\Project Management\\PCR\'s\\"

        templatefile =  "plugins\\templates\\PCR Template.docx"
        tempfileinfo = QFileInfo(templatefile)
        basename = projnum + " " + tempfileinfo.baseName() + changenum + "." + tempfileinfo.suffix()
        basename = basename.replace(" Template", "")

        projectfile = projectfolder + basename

        projectfile = projectfile.replace("/", "\\") # office products have to have backslash

        # copy the file
        if not QFile(projectfile).exists():
            if not QFile(templatefile).copy(projectfile):
                QMessageBox.critical(None, "Unable to copy template", "Could not copy " + templatefile + " to " + projectfile, QMessageBox.StandardButton.Cancel)
                return ""

        # change the values for the project specifics in the file
        word = win32com.client.DispatchEx("Word.Application")

        doc = word.Documents.Open(projectfile)

        replace_text(doc, "<PROJECTNUMBER>", projnum)
        replace_text(doc, "<CLIENTNAME>", clientnam)
        replace_text(doc, "<PROJECTNAME>", projnam)
        replace_text(doc, "<CRDATE>", statusdate.toString("MM/dd/yyyy"))
        replace_text(doc, "<CRNUMBER>", re.sub(r"\D", "", projnum) + '-' + changenum)

        doc.Save()

        word.Visible = 1
        doc.Activate()
        word.Activate()

        #word.Quit()
        #word = None

        # add the location to the project
        docxml = QDomDocument()
        docroot = docxml.createElement("projectnotes");
        docxml.appendChild(docroot);

        table = pnc.xml_table(docxml, "project_locations")
        docroot.appendChild(table)

        row = pnc.xml_row(docxml)
        table.appendChild(row)

        row.appendChild(pnc.xml_col(docxml, "project_id",None, projnum))
        row.appendChild(pnc.xml_col(docxml, "location_type", "Word Document", None))
        row.appendChild(pnc.xml_col(docxml, "location_description", "Change Request : " + basename, None))
        row.appendChild(pnc.xml_col(docxml, "full_path", projectfile, None))

        return docxml.toString()


    def replace_text(doc, searchtext, replacetext):
        #return doc.Content.Find.Execute(searchtext, 0, 0, 0, 0, 0, 1, 1, 0, replacetext, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0)
        return doc.Content.Find.Execute(searchtext, False, False, False, False, False, True, 1, False, replacetext, 2)

    pluginmenus.append({"menutitle" : "Change Order", "function" : "menuChangeOrder", "tablefilter" : "", "submenu" : "Templates", "dataexport" : "projects"})


#setup test data
if __name__ == '__main__':
    import os
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)

    xml_content = ""
    with open("C:\\Users\\pamcki\\OneDrive - Cornerstone Controls\\Documents\\Work In Progress\\XML\\project.xml", 'r', encoding='utf-8') as file:
        xml_content = file.read()

    menuChangeOrder(xml_content, "")

    app.exec()
