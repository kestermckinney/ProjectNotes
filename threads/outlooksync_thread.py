import os
import sys
import inspect
import threading

# make sure includes folder can be found
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../plugins')))

from includes.graphapitools import GraphAPITools, TokenAPI
from PyQt6.QtCore import QDateTime, QElapsedTimer, QCoreApplication


# Project Notes Plugin Parameters
pluginname = "Outlook Integration"
plugindescription = "Imports Outlook contacts, Exports Contacts that don't already exist in Outlook, Exports project related emails, and Export the project managers assigned active tracker and action items.  All activites are done using the Microsoft Graph API."
plugintimerevent = 1 # how many minutes between the timer event

pluginmenus = [
    {"menutitle" : "Sync Tracker Items", "function" : "menuSyncTrackerItems", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : "", "parameter" : "all"},
    {"menutitle" : "Download All Emails", "function" : "event_data_rightclick", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : "", "parameter" : "all"},
    {"menutitle" : "Import Contacts", "function" : "event_data_rightclick", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : "", "parameter" : "all"},
]

# all events return an xml string that can be processed by ProjectNotes
#
# Supported Events

# def event_startup(parameter):
#     return
#
# def event_shutdown(parameter):
#     return
#
# def event_timer(parameter):
#     return
#

# keep authentication information in memory
# allow authentication calls to use main GUI event loop
tapi = TokenAPI()

class OutlookSync:
    def __init__(self):
        super().__init__()
    
    def syncEverything(self, token):
        timer = QElapsedTimer()
        timer.start()

        gapi = GraphAPITools()
        gapi.setToken(token)

        gapi.sync_tracker_to_tasks()
        #gapi.download_batch_of_emails()
        #gapi.import_batch_of_contacts()
        #gapi.export_batch_of_contacts()

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")

        return

    def syncTrackerItems(self, token):
        timer = QElapsedTimer()
        timer.start()

        gapi = GraphAPITools()
        gapi.setToken(token)

        gapi.sync_tracker_to_tasks()

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds")

        return        

def event_data_rightclick(xmlstr, parameter):
    token = tapi.authenticate()

    if token is not None:
        o365 = OutlookSync()
        o365.syncEverything(token)
    else:
        print("No token was returned.  Office 365 sync failed.")

    return ""

def event_timer(parameter):
    return event_data_rightclick("", parameter)

def menuSyncTrackerItems(parameter):
    token = tapi.authenticate()

    if token is not None:
        o365 = OutlookSync()
        o365.syncTrackerItems(token)
    else:
        print("No token was returned.  Office 365 sync failed.")

    return ""


# call when testing outside of Project Notes

#print("Test Outlook Sync")
#event_menuclick("")
#app.exec()
