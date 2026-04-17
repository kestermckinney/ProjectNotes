# Copyright (C) 2025, 2026 Paul McKinney
import os
import sys
import projectnotes

# make sure includes folder can be found
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../plugins')))

from includes.ifs_tools import IFSCommon
import includes.ifs_tools as ifs_tools
from PyQt6.QtCore import QThread


# Project Notes Plugin Parameters
pluginname = "IFS Integration"
plugindescription = "Imports IFS Projects, Exports Item Tracker as Tasks.  Uses HTTP Authentication."
plugintimerevent = 3 # how many minutes between the timer event

pluginmenus = []

ifs = IFSCommon()

if ifs.get_has_settings():
    pluginmenus = [
        {"menutitle" : "Import IFS Projects", "function" : "menuimport_ifs_projects", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : "", "parameter" : "all"},
    ]

# all events return an xml string that can be processed by ProjectNotes
#
# Supported Events

def event_timer(parameter):
    if not ifs.get_has_settings() or not ifs.url_is_available():
        return ""

    if QThread.currentThread().isInterruptionRequested():
        return ""

    ifs.import_ifs_projects("")

    return ""

def menuimport_ifs_projects(parameter):
    if not ifs.get_has_settings() or not ifs.url_is_available():
        print("IFS sync failed.  Make sure IFS Integrations are configured correctly.")
        return ""

    print("Importing IFS projects...")
    ifs.import_ifs_projects("all")

    return ""
