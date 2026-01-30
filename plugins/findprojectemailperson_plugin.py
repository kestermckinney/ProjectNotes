import sys
import platform

from includes.common import ProjectNotesCommon

if (platform.system() == 'Windows'):
    import win32com
    from win32 import win32api    
    from includes.outlook_tools import ProjectNotesOutlookTools

from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtCore import Qt, QRect, QDateTime, QTime, QFile, QIODevice
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QDialog, QFileDialog, QWidget, QTableWidgetItem, QStyledItemDelegate, QComboBox
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtGui import QDesktopServices, QClipboard

# Project Notes Plugin Parameters
pluginname = "Find Project Email"
plugindescription = "Using Outlook find email related to the project sent to or from the selected person."

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

    def menuFindEmail(xmlstr, parameter):

        window_title = ""
        
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return ""
            
        outlook = win32com.client.Dispatch("Outlook.Application")

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        
        pm = xmlroot.toElement().attribute("managing_manager_name")
        co = xmlroot.toElement().attribute("managing_company_name")

        projectnumber = ""

        if xmlroot:
            projectnumber = pnc.scrape_project_number(xmlroot)

            email = None

            teammember = pnc.find_node(xmlroot, "table", "name", "project_people")
            if not teammember.isNull():
                memberrow = teammember.firstChild()

            while not memberrow.isNull():
                email = pnc.get_column_value(memberrow, "email")
                memberrow = memberrow.nextSibling()

            if (email is not None and email != ""):
                searchfilter = 'from:"' + email + '" subject:"' + projectnumber + '"'
                outlook.ActiveExplorer().Search(searchfilter, 2)

                window_title = "All Outlook Items"

                pnc.bring_window_to_front(window_title)
            
        outlook = None

        return ""


    pluginmenus.append({"menutitle" : "Find Project Email", "function" : "menuFindEmail", "tablefilter" : "project_people", "submenu" : "Utilities", "dataexport" : "project_people"})
    pluginmenus.append({"menutitle" : "Find Project Email", "function" : "menuFindEmail", "tablefilter" : "meeting_attendees", "submenu" : "Utilities", "dataexport" : "meeting_attendees"})


# setup test data
"""
print("Buld up QDomDocument")

xmldoc = QDomDocument("TestDocument")

f = QFile("C:\\Users\\pamcki\\Desktop\\projectpeople.xml")

if f.open(QIODevice.OpenModeFlag.ReadOnly):
    print("example project opened")
    xmldoc.setContent(f)
    f.close()

menuFindEmail(xmldoc.toString(), None)

"""