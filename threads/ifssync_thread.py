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

ifs = IFSCommon()

if ifs.get_has_settings(): 
    pluginmenus = [
        {"menutitle" : "Import IFS Projects", "function" : "menuimport_ifs_projects", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : "", "parameter" : "all"},
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
    if ifs.get_has_settings():
        ifs.import_ifs_projects("")

        if ifs.get_sync_tracker_items():
            ifs.export_ifs_tracker_items("")

    return ""

def menuimport_ifs_projects(parameter):
    if ifs.get_has_settings():
        ifs.import_ifs_projects("all") 

        if ifs.get_sync_tracker_items():
            ifs.export_ifs_tracker_items("all")

    return ""

