
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt5 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt5.QtSql import QSqlDatabase
from PyQt5.QtXml import QDomDocument, QDomNode
from PyQt5.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt5.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt5.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Get PowerPoint Template"
plugindescription = "Copy the selected PowerPoint template, adding project information to the file."
plugintable = "projects" # the table or view that the plugin applies to.  This will enable the right click
childtablesfilter = "" # a list of child tables that can be sent to the plugin.  This will be used to exclude items like notes or action items when they aren't used

# events must have a data structure and data view specified
#
# Structures:
#      string          The event will pass a python string containing XML and will expect the plugin to return an XML string
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

# Active Events

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# Project Notes Parameters
parameters = [
]

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pnc = ProjectNotesCommon()

    # processing main function
    def event_data_rightclick(xmlstr):
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)
            return ""

        if not pnc.verify_global_settings():
            return ""

        # setup global variable
        ProjectsFolder = pnc.get_global_setting("ProjectsFolder")

        # prompt for the template to use
        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = pnc.get_projectfolder(xmlroot)
        pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()
        cm = xmlroot.attributes().namedItem("managing_company_name").nodeValue()

        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projnam = pnc.get_column_value(projtab.firstChild(), "project_name")

        if (projectfolder is None or projectfolder =="" or not QDir(projectfolder).exists()):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QtCore.QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""
        else:
            projectfolder = projectfolder + "\\Meeting Minutes\\"

        templatefile = QFileDialog.getOpenFileName(None, "Select the PowerPoint Template", QDir.currentPath() + "\\templates\\","PowerPoint files (*.ppt;*.pptx;*.pptm)|*.ppt;*.pptx;*.pptm")

        if templatefile is None or templatefile[0] == "":
            return ""

        tempfileinfo = QFileInfo(templatefile[0])
        basename = projnum + " " + tempfileinfo.baseName() + "." + tempfileinfo.suffix()
        basename = basename.replace(" Template", "")

        projectfile = projectfolder + basename

        # copy the file
        if not QDir(projectfile).exists():
            QFile(templatefile[0]).copy(projectfile)

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
        docxml = pnc.xml_doc_root()

        table = pnc.xml_table(docxml, "project_locations")
        docxml.appendChild(table)

        row = pnc.xml_row(docxml)
        table.appendChild(row)

        row.appendChild(pnc.xml_col(docxml, "project_id",None, projnum))
        row.appendChild(pnc.xml_col(docxml, "location_type", "Generic File (System Identified)", None))
        row.appendChild(pnc.xml_col(docxml, "location_description", basename, None))
        row.appendChild(pnc.xml_col(docxml, "full_path", projectfile, None))

        return docxml

# setup test data
"""
print("Buld up QDomDocument")
app = QApplication(sys.argv)


xmldoc = QDomDocument("TestDocument")
f = QFile("exampleproject.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

print("Run Test")
# call when testing outside of Project Notes
print(main_process(xmldoc).toString())
print("Finished")
"""
