import sys
import platform

if (platform.system() == 'Windows'):
    from includes.excel_tools import ProjectNotesExcelTools
    import win32com

from includes.common import ProjectNotesCommon
from PyQt6 import QtSql, QtGui, QtCore, QtWidgets, uic
from PyQt6.QtSql import QSqlDatabase
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Export Outlook Contacts"
plugindescription = "Export contacts and assocated companies to Outlook."
plugintable = "people" # the table or view that the plugin applies to.  This will enable the right click
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

# Supported Events

# def event_startup(xmlstr):
#     return ""
#
# def event_shutdown(xmlstr):
#     return ""
#
# def event_everyminute(xmlstr):
#     return ""
#
# def event_every5minutes(xmlstr):
#     return ""
#
# def event_every10minutes(xmlstr):
#     return ""
#
# def event_every30Mmnutes(xmlstr):
#     return ""
#
# def event_menuclick(xmlstr):
#     return ""

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# Project Notes Parameters
parameters = [
]

OracleUsername = ""
ProjectsFolder = ""

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    #
    pnc = ProjectNotesCommon()
    pne = ProjectNotesExcelTools()

    def find_contact( list, fullname ):
        for contact in list:
            if contact[1] == fullname.strip().upper():
                return contact[0]

    # processing main function
    def event_menuclick(xmlstr):
        print("called event: " + __file__)

        #
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""

        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()

        progbar = QProgressDialog()
        progbar.setWindowTitle("Exporting...")
        progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint 
            )
        progbar.setMinimumWidth(350)
        progbar.setCancelButton(None)
        progbar.show()


        outlook = win32com.client.Dispatch("Outlook.Application")
        mapi = outlook.GetNamespace("MAPI")
        contactsfold = mapi.GetDefaultFolder(10) # olFolderContacts

        # load all contacts into memory
        cont_enum = contactsfold.Items
        contactlist = []
        for contact in cont_enum:
            if hasattr(contact, "FullName"):
                cols = []
                cols.append(contact)
                cols.append(contact.FullName.strip().upper())
                contactlist.append(cols)

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node

        childnode = xmlroot.firstChild()
        tot_contacts = childnode.childNodes().count()

        cur_contacts = 0

        while not childnode.isNull():

            if childnode.attributes().namedItem("name").nodeValue() == "people":
                rownode = childnode.firstChild()

                while not rownode.isNull():
                    cur_contacts = cur_contacts + 1

                    progbar.setValue(int(cur_contacts / tot_contacts * 100))
                    progbar.setLabelText("Exporting Contacts...")
                    QtWidgets.QApplication.processEvents()

                    colnode = rownode.firstChild()

                    fullname = None
                    company = None
                    workphone = None
                    workemail = None
                    cellphone = None
                    jobtitle = None

                    while not colnode.isNull():
                        content = colnode.toElement().text()

                        if colnode.attributes().namedItem("name").nodeValue() == "name":
                            fullname = content

                        if colnode.attributes().namedItem("name").nodeValue() == "email":
                            workemail = content

                        if colnode.attributes().namedItem("name").nodeValue() == "office_phone":
                            workphone = content

                        if colnode.attributes().namedItem("name").nodeValue() == "cell_phone":
                            cellphone = content

                        if colnode.attributes().namedItem("name").nodeValue() == "client_id":
                            company = colnode.attributes().namedItem("lookupvalue").nodeValue()

                        if colnode.attributes().namedItem("name").nodeValue() == "role":
                            jobtitle = content

                        colnode = colnode.nextSibling()

                    #print("Exporting ..." + fullname)

                    #searchname = find_contact(outlook, fullname, mapi, contactsfold)
                    searchname = find_contact(contactlist, fullname)

                    if searchname == None:
                        searchname = contactsfold.Items.Add()

                    searchname.FullName = fullname
                    searchname.CompanyName = company
                    searchname.BusinessTelephoneNumber = workphone
                    searchname.MobileTelephoneNumber = cellphone
                    searchname.Email1Address = workemail
                    searchname.JobTitle = jobtitle
                    searchname.Save()

                    rownode = rownode.nextSibling()

            childnode = childnode.nextSibling()

        outlook = None
        mapi = None
        contactsfold = None

        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed

        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()  

        return ""


# setup test data
"""
import sys
print("Buld up QDomDocument")
#

xmldoc = QDomDocument("TestDocument")
f = QFile("C:/Users/pamcki/Desktop/zarse.xml")

if f.open(QIODevice.OpenModeFlag.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

event_menuclick(xmldoc.toString())
"""


# TESTED: Phase 1
