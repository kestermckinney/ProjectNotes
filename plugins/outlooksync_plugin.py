import sys
import inspect
import threading

from includes.graphapitools import GraphAPITools, TokenAPI
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QElapsedTimer, QThread, QEventLoop, QCoreApplication, pyqtSignal, pyqtSlot, QMetaObject, Qt, Q_ARG
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog, QWidget, QVBoxLayout, QPushButton, QLabel
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Outlook Web Sync"
plugindescription = "Imports Outlook contacts, Exports Contacts that don't already exist in Outlook, Exports project related emails, and Export the project managers assigned active tracker and action items.  All activites are done using the Microsoft Graph API."
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

# keep authentication information in memory
# allow authentication calls to use main GUI event loop
tapi = TokenAPI()

class OutlookSyncWorker(QThread):
    signalStartSync = pyqtSignal(str)


    def __init__(self):
         super().__init__()
         self.signalStartSync.connect(self.syncItems)
    
    @pyqtSlot(str)
    def syncItems(self, token):
        timer = QElapsedTimer()
        timer.start()

        self.setPriority(QThread.Priority.LowestPriority)

        gapi = GraphAPITools()
        gapi.setToken(token)

        gapi.sync_tracker_to_tasks()
        gapi.download_batch_of_emails()
        gapi.import_batch_of_contacts()
        #gapi.export_batch_of_contacts()

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")

        # Request the thread to quit
        self.quit()
        return

#app = QApplication(sys.argv) # must call this in debug mode
workerthread = OutlookSyncWorker()

def event_menuclick(xmlstr):
    if workerthread.isRunning():
        print("Outlook Sync Still Running.")
        return ""

    token = tapi.authenticate()

    if token is not None:
        workerthread.start()
        print("Invoking Outlook Sync.")
        QMetaObject.invokeMethod(workerthread, "signalStartSync", Qt.ConnectionType.QueuedConnection, Q_ARG("QString", token))
        print("Outlook Sync Invoked.")

    return ""

def event_everyminute(xmlstr):
    return event_menuclick(xmlstr)


# call when testing outside of Project Notes

#print("Test Outlook Sync")
#event_menuclick("")
#app.exec()
