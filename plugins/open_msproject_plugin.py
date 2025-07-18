import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Open MS Project"
plugindescription = "Open the Microsoft Project file found in Artifacts."

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

    def menuOpenMSProject(xmlstr, parameter):

        ui = uic.loadUi("plugins/forms/dialogDuplicateFilesFound.ui")
        
        openfile = None
        filelist = []
        xmlval = QDomDocument()

        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        

        if xmlroot:
            projlocation = pnc.find_node(xmlroot, "table", "name", "project_locations")

            if not projlocation.isNull():
                locationrow = projlocation.firstChild()

            while not locationrow.isNull():
                checkfile = pnc.get_column_value(locationrow, "full_path")

                if checkfile[-4:] == ".mpp":
                    filelist.append(checkfile)
                    openfile = checkfile

                locationrow = locationrow.nextSibling()

        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()

        if len(filelist) > 1:
            ui.m_listWidgetFiles.addItems(filelist)
            if ui.exec():
                # Get the selected items
                selected_items = ui.m_listWidgetFiles.selectedItems()

                # Check if there are any selected items
                if selected_items:
                    # Get the text of the first selected item
                    openfile = selected_items[0].text()
            else:
                openfile = None

        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.CursorShape.WaitCursor)
        QtWidgets.QApplication.processEvents()

        if not openfile is None:
            QDesktopServices.openUrl(QUrl("file:///" + openfile))

        return ""

    pluginmenus.append({"menutitle" : "Open MS Project", "function" : "menuOpenMSProject", "tablefilter" : "projects/project_locations", "submenu" : "", "dataexport" : "projects"})

# setup test data
"""
print("Buld up QDomDocument")

xmldoc = QDomDocument("TestDocument")

f = QFile("exampleproject.xml")

if f.open(QIODevice.OpenModeFlag.ReadOnly):
    print("example project opened")
    xmldoc.setContent(f)
    f.close()

event_data_rightclick(xmldoc.toString())
"""
