import sys
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
    #
    pnc = ProjectNotesCommon()

    def event_menuclick(xmlstr):
        print("called event: " + __file__)

        #
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
            return ""

        outlook = win32com.client.Dispatch("Outlook.Application")
        mapi = outlook.GetNamespace("MAPI")
        contactsfold = mapi.GetDefaultFolder(10) # olFolderContacts

        xmlclients = ""
        xmldoc = ""

        #print("count of contacts : " + str(contactsfold.Items.Count))

        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()

        tot_contacts = contactsfold.Items.Count
        cur_contacts = 0

        progbar = QProgressDialog()
        progbar.setWindowTitle("Importing...")
        progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint 
            )
        progbar.setMinimumWidth(350)
        progbar.setCancelButton(None)
        progbar.show()


        cont_enum = contactsfold.Items
        for contact in cont_enum:
            cur_contacts = cur_contacts + 1

            progbar.setValue(int(cur_contacts / tot_contacts * 100))
            progbar.setLabelText("Importing Contacts...")
            QtWidgets.QApplication.processEvents()

            # olContactItem
            if contact is not None:
                if hasattr(contact, "FullName"):
                    #print("importing ... " + contact.FullName)
                    xmldoc = xmldoc + "<row>\n"

                    xmldoc = xmldoc + "<column name=\"name\">" + pnc.to_xml(contact.FullName.strip()) + "</column>\n"

                    if contact.Email1Address is not None:
                        # make sure the email address looks valid
                        if "@" in contact.Email1Address: 
                            xmldoc = xmldoc + "<column name=\"email\">" + pnc.to_xml(contact.Email1Address.strip()) + "</column>\n"
                        else:
                            print("email address is corrupt for " + contact.FullName)

                    if contact.BusinessTelephoneNumber is not None:
                        xmldoc = xmldoc + "<column name=\"office_phone\">" + pnc.to_xml(contact.BusinessTelephoneNumber.strip()) + "</column>\n"

                    if contact.MobileTelephoneNumber is not None:
                        xmldoc = xmldoc + "<column name=\"cell_phone\">" + pnc.to_xml(contact.MobileTelephoneNumber.strip()) + "</column>\n"

                    if contact.JobTitle is not None:
                        xmldoc = xmldoc + "<column name=\"role\">" + pnc.to_xml(contact.JobTitle.strip()) + "</column>\n"

                    # add the company name as a sub tablenode
                    if (contact.CompanyName is not None and contact.CompanyName != ''):
                        xmldoc = xmldoc + "<column name=\"client_id\" number=\"5\" lookupvalue=\"" + pnc.to_xml(contact.CompanyName.strip()) + "\"></column>\n"
                        xmlclients = xmlclients + "<row><column name=\"client_name\">" + pnc.to_xml(contact.CompanyName.strip()) + "</column></row>\n"

                    xmldoc = xmldoc + "</row>\n"
            else:
                print("Corrupt Contact Record Found")

        xmldoc = """
        <projectnotes>
        <table name="clients">
        """ + xmlclients + """
        </table>
        <table name="people">
        """ + xmldoc + """
        </table>
        </projectnotes>
        """

        outlook = None
        mapi = None
        contactsfold = None
        cont_enum = None

        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed

        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()   

        return xmldoc

#print("Run Test")
# call when testing outside of Project Notes
##
#print(event_menuclick(None))

# TESTED: Phase 1