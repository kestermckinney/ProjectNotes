
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
pluginname = "Import Outlook Contacts"
plugindescription = "Import Outlook contacts and assocated companies."
plugintable = "" # the table or view that the plugin applies to.  This will enable the right click
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

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any functions

# this plugin is only supported on windows
if (platform.system() == 'Windows'):
    pnc = ProjectNotesCommon()

    def event_menuclick(xmlstr):
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.Cancel)
            return ""

        outlook = win32com.client.Dispatch("Outlook.Application")
        mapi = outlook.GetNamespace("MAPI")
        contactsfold = mapi.GetDefaultFolder(10) # olFolderContacts

        xmlclients = ""
        xmldoc = ""

        cont_enum = contactsfold.Items
        for contact in cont_enum:
            # olContactItem
            #try:

            if (contact.FullName is not None and contact.FullName != ""):
                print(contact.FullName)
                xmldoc = xmldoc + "<row>\n"

                xmldoc = xmldoc + "<column name=\"name\" number=\"1\">" + pnc.to_xml(contact.FullName) + "</column>\n"

                if contact.Email1Address is not None:
                    xmldoc = xmldoc + "<column name=\"email\" number=\"2\">" + pnc.to_xml(contact.Email1Address) + "</column>\n"

                if contact.BusinessTelephoneNumber is not None:
                    xmldoc = xmldoc + "<column name=\"office_phone\" number=\"3\">" + pnc.to_xml(contact.BusinessTelephoneNumber) + "</column>\n"

                if contact.MobileTelephoneNumber is not None:
                    xmldoc = xmldoc + "<column name=\"cell_phone\" number=\"4\">" + pnc.to_xml(contact.MobileTelephoneNumber) + "</column>\n"

                if contact.JobTitle is not None:
                    xmldoc = xmldoc + "<column name=\"role\" number=\"4\">" + pnc.to_xml(contact.JobTitle) + "</column>\n"

                # add the company name as a sub tablenode
                if (contact.CompanyName is not None and contact.CompanyName != ''):
                    xmldoc = xmldoc + "<column name=\"client_id\" number=\"5\" lookupvalue=\"" + pnc.to_xml(contact.CompanyName) + "\"></column>\n"
                    xmlclients = xmlclients + "<row><column name=\"client_name\" number=\"1\">" + pnc.to_xml(contact.CompanyName) + "</column></row>\n"

                xmldoc = xmldoc + "</row>\n"
            #except:
            #    print("Group Name Found")

        xmldoc = """
        <?xml version="1.0" encoding="UTF-8"?>
        <projectnotes>
        <table name="ix_clients">
        """ + xmlclients + """
        </table>
        <table name="ix_people">
        """ + xmldoc + """
        </table>
        </projectnotes>
        """

        outlook = None
        mapi = None
        contactsfold = None
        cont_enum = None

        return xmldoc

#print("Run Test")
# call when testing outside of Project Notes
#print(main_process(None))
