from includes.common import ProjectNotesCommon
from includes.excel_tools import ProjectNotesExcelTools

from PySide6 import QtSql, QtGui, QtCore, QtUiTools
from PySide6.QtSql import QSqlDatabase
from PySide6.QtXml import QDomDocument, QDomNode
from PySide6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo
from PySide6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog, QInputDialog
from PySide6.QtGui import QDesktopServices
import sys
import win32com

# events must have a data structure and data view specified
#
# Structures:
#      disabled        The event will not be enabled
#      wxxmldocument   The event will pass a wxLua wx.wxXmlDocument containing the spedified data view and expect the plugin to return a wx.wxXmlDocument
#      string          The event will pass a wxLua string containing XML and will expect the plugin to return an XML string
#      nodata          The event will pass a wxLua None and will expect the plugin to return an XML string
#
# all tables in the database have corresponding import/export data views the views are prefixed by ix_
#
# Data Views:
#      ix_clients
#      ix_people
#      ix_projects
#      ix_project_people
#      ix_status_report_items
#      ix_project_locations
#      ix_project_notes
#      ix_meeting_attendees
#      ix_item_tracker_updates
#      ix_item_tracker

# Project Notes Plugin Parameters
pluginname = "Get MS Project Template"
plugindescription = "Copy the selected MS Project Template"

# Active Events
Startup="disabled"
Shutdown="disabled"
EveryMinute="disabled"
Every5Minutes="disabled"
Every10Minutes="disabled"
Every30Minutes="disabled"
PluginMenuClick="disabled"
RightClickProject="wxxmldocument:ix_projects"
RightClickPeople="disabled"
RightClickClient="disabled"
RightClickStatusReportItem="disabled"
RightClickLocationItem="disabled"
RightClickTeamMember="disabled"
RightClickMeeting="disabled"
RightClickAttendee="disabled"
RightCickTrackerItem="disabled"

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any functions

# Project Notes Parameters
parameters = {
}

pnc = ProjectNotesCommon()

# processing main function
def main_process( xmlval ):
    if not pnc.verify_global_settings():
        return None

    # setup global variable
    ProjectsFolder = pnc.get_global_setting("ProjectsFolder")

    # prompt for the template to use
    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
    projectfolder = pnc.get_projectfolder(xmlroot)
    pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()
    cm = xmlroot.attributes().namedItem("managing_company_name").nodeValue()

    projtab = pnc.find_node(xmlroot, "table", "name", "ix_projects")
    projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
    projnam = pnc.get_column_value(projtab.firstChild(), "project_name")

    if (projectfolder is None or projectfolder =="" or not QDir(projectfolder).exists()):
        projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QtCore.QDir.home().path())

        if projectfolder == "" or projectfolder is None:
            return None
    else:
        projectfolder = projectfolder + "\\Schedule\\"


    templatefile = QFileDialog.getOpenFileName(None, "Select the MS Project Template", QDir.currentPath() + "\\templates\\","Project files (*.mpp)|*.mpp")

    if templatefile is None or templatefile[0] == "":
        return None

    basename = projnam[:30]
    projectfile = projectfolder + basename + ".mpp"

    print(templatefile[0])
    # copy the file
    if not QDir(projectfile).exists():
        QFile(templatefile[0]).copy(projectfile)

    print("Calling Create Object for MSProject\n")
    project = win32com.client.Dispatch("MSProject.Application")
    project.Visible = False

    print("Calling File Open\n")
    project.FileOpenOrCreate(projectfile)

    print("Replacing PROJECTNAME\n")
    project.ReplaceEx("Name", "contains", "PROJECTNAME", basename, True, True, False, 188743694, 7, True)

    print("Saving..\n")
    project.FileSave()

    project.Quit()

    project = None

    QDesktopServices.openUrl(QUrl("file:///" + projectfile, QUrl.TolerantMode))

    # add the location to the project
    docxml = pnc.xml_doc_root()

    table = pnc.xml_table(docxml, "ix_project_locations")
    docxml.appendChild(table)

    row = pnc.xml_row(docxml)
    table.appendChild(row)

    row.appendChild(pnc.xml_col(docxml, "project_id",None, projnum))
    row.appendChild(pnc.xml_col(docxml, "location_type", "Microsoft Project", None))
    row.appendChild(pnc.xml_col(docxml, "location_description", "Project Schedule : " + basename, None))
    row.appendChild(pnc.xml_col(docxml, "full_path", projectfile, None))

    return docxml

# Project Notes Plugin Events
def event_startup(xmlstr):
    return main_process(xmlstr)

def event_shutdown(xmlstr):
    return main_process(xmlstr)

def event_everyminute(xmlstr):
    return main_process(xmlstr)

def event_every5minutes(xmlstr):
    return main_process(xmlstr)

def event_every10minutes(xmlstr):
    return main_process(xmlstr)

def event_every30Mmnutes(xmlstr):
    return main_process(xmlstr)

def event_menuclick(xmlstr):
    return main_process(xmlstr)

def event_projectrightclick(xmlstr):
    return main_process(xmlstr)

def event_peoplerightclick(xmlstr):
    return main_process(xmlstr)

def event_clientrightclick(xmlstr):
    return main_process(xmlstr)

def event_statusreportitemrightclick(xmlstr):
    return main_process(xmlstr)

def event_teammemberrightclick(xmlstr):
    return main_process(xmlstr)

def event_locationitemrightclick(xmlstr):
    return main_process(xmlstr)

def event_meetingrightclick(xmlstr):
    return main_process(xmlstr)

def event_attendeerightclick(xmlstr):
    return main_process(xmlstr)

def event_trackeritemrightclick(xmlstr):
    return main_process(xmlstr)

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
#TODO:  Some large XML fields from ProjectNotes 2 break the parser.  For examle an email was pasted into description.  Maybe CDATA tags are needed there.
