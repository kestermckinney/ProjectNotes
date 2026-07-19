# Copyright (C) 2026 Paul McKinney
# SPDX-License-Identifier: GPL-3.0-only

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "../plugins")))

import projectnotes
from PyQt6.QtCore import QDateTime, QSettings, QThread

from includes.common import ProjectNotesCommon, _settings_organization
from includes.icloud_tools import (APPLE_SERVICE, ICloudAuthenticationError,
                                   ICloudContactSync, ICloudError)

pluginname = "iCloud Contacts"
plugindescription = "Imports iCloud contacts and optionally exports new Project Notes contacts using CardDAV."
plugintimerevent = 5
pluginmenus = [
    {"menutitle": "Sync iCloud Contacts Now", "function": "menu_import_now",
     "tablefilter": "", "submenu": "Utilities", "dataexport": "", "parameter": ""}
]

pnc = ProjectNotesCommon()
running = False
auth_failed = False
shutting_down = False


def _setting(name, default=""):
    value = pnc.get_plugin_setting(name, pluginname)
    return default if value is None else value


def _set_status(message, success=False):
    pnc.set_plugin_setting("LastStatus", pluginname, message)
    if success:
        pnc.set_plugin_setting("LastSuccess", pluginname,
                               QDateTime.currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))


def _enabled():
    return str(_setting("Enabled", "false")).lower() == "true"


def _credentials():
    account = str(_setting("Account", "")).strip()
    if not account:
        raise ICloudAuthenticationError("Enter an Apple Account in iCloud Contacts settings.")
    password = projectnotes.get_secret(APPLE_SERVICE, account)
    if not password:
        raise ICloudAuthenticationError("Store an app-specific password in iCloud Contacts settings.")
    return account, password


def sync_contacts(manual=False):
    global running, auth_failed
    if shutting_down or QThread.currentThread().isInterruptionRequested():
        return ""
    if not manual and (not _enabled() or auth_failed):
        return ""
    if running:
        if manual:
            print("iCloud Contacts: an import is already running.", flush=True)
        return ""
    running = True
    try:
        if manual:
            _set_status("Sync in progress...")
            print("iCloud Contacts: sync started.", flush=True)
        account, password = _credentials()
        export_new = str(_setting("ExportNewContacts", "false")).lower() == "true"
        stats = ICloudContactSync(projectnotes, pnc.get_temporary_folder(), account, password).run(export_new)
        message = stats.summary()
        _set_status(message, success=True)
        auth_failed = False
        print(f"iCloud Contacts: {message}", flush=True)
    except ICloudAuthenticationError as exc:
        auth_failed = True
        _set_status(str(exc))
        print(f"iCloud Contacts authentication failed: {exc}", flush=True)
    except ICloudError as exc:
        _set_status(str(exc))
        print(f"iCloud Contacts import failed: {exc}", flush=True)
    except Exception as exc:
        _set_status("Unexpected iCloud Contacts error. See the error log.")
        print(f"iCloud Contacts unexpected error: {exc}", flush=True)
    finally:
        running = False
    return ""


def event_startup(parameter):
    return sync_contacts(False)


def event_timer(parameter):
    return sync_contacts(False)


def event_shutdown(parameter):
    global shutting_down
    shutting_down = True
    return ""


def menu_import_now(parameter):
    global auth_failed
    auth_failed = False
    return sync_contacts(True)
