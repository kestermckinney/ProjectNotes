import os
import sys
import inspect
import threading
import projectnotes

# make sure includes folder can be found
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../plugins')))

#from includes.common import ProjectNotesCommon
from includes.ifs_tools import IFSCommon
from PyQt6.QtCore import QDateTime, QElapsedTimer, QCoreApplication


# Project Notes Plugin Parameters
pluginname = "IFS Integration"
plugindescription = "Imporst IFS Projects, Exports Item Tracker as Tasks.  Uses HTTP Authentication."
plugintimerevent = 1 # how many minutes between the timer event

pluginmenus = []

stopevent = False  # after a key failure the event will stop processing

#pnc = ProjectNotesCommon()
ifs = IFSCommon()

if ifs.has_settings(): 
    pluginmenus = [
        {"menutitle" : "Import IFS Projects", "function" : "menuImportIFSProjects", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : "", "parameter" : "all"},
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

def event_timer(parameter):
    global stopevent
    
    if stopevent:
        return ""

    if ifs.has_settings():
        ifs.syncIFSProjects("")

    #    print("No token was returned.  Office 365 sync failed.  Make sure Outlook Integrations are configured correctly.")
    #    stopevent = True # don't flood the logs
    # STOPPED HERE need to make project import work in blocks of items like graphAPI

    return ""

def menuImportIFSProjects(parameter):
    global stopevent
    
    if stopevent:
        return ""

    if ifs.has_settings():
        ifs.syncIFSProjects("all")

    #    print("No token was returned.  Office 365 sync failed.  Make sure Outlook Integrations are configured correctly.")
    #    stopevent = True # don't flood the logs
    # STOPPED HERE need to make project import work in blocks of items like graphAPI

    return ""


# call when testing outside of Project Notes

#print("Test Outlook Sync")
#event_menuclick("")
#app.exec()
