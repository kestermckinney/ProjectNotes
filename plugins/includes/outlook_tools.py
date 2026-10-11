# Copyright (C) 2025, 2026 Paul McKinney
import platform

if (platform.system() == 'Windows'):
    from win32com.client import GetObject
    import win32com
    import win32gui
    
import sys
import time
import re
import gc

from PyQt6 import QtSql, QtGui, QtCore, QtWidgets
from PyQt6.QtCore import Qt, QDirIterator, QDir, QSettings, QFile, QDateTime
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QFileDialog
from includes.common import ProjectNotesCommon

top_windows = []

def handler_window_enumerator(hwnd, topwindows):
    topwindows.append((hwnd, win32gui.GetWindowText(hwnd)))

class ProjectNotesOutlookTools:
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "Outlook Integration"
        self.backup_inbox_folder = self.pnc.get_plugin_setting("BackupInBoxFolder", self.settings_pluginname)
        self.backup_sent_folder = self.pnc.get_plugin_setting("BackupSentFolder", self.settings_pluginname)

    def bring_window_to_front(self, title):
        try:
            win32gui.EnumWindows(handler_window_enumerator, top_windows)

            for i in top_windows:
                if title.lower() in i[1].lower():
                    win32gui.ShowWindow(i[0],5)
                    win32gui.SetForegroundWindow(i[0])
                    return

        except Exception as e:  # catches any Python exception
            # If it’s a COM error, the details are in e.args
            msg = f'Error: {e} Attempting to bring window {title} to the foreground.'
            print(msg)
            if hasattr(e, 'args') and e.args:
                print(f'COM error code: {e.args[0]}')   # usually a HRESULT like -2147352567

        return


    def make_filename(self, datetime, subject):
        qdt = QDateTime.fromString(datetime, Qt.DateFormat.ISODate)
        iso_string = qdt.toString(Qt.DateFormat.ISODate)  # UTC we want it to be the same format as the GraphAPI

        id = re.sub(r"[-`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[ ]", "", iso_string)
        cleanname = id + "-" + self.pnc.valid_filename(subject)
        # there is no guarantee this length will work.  It depends on system settigns and the base path length
        return cleanname[:70]

    def download_emails(self, xmlstr):
        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()   

        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            QMessageBox.critical(QApplication.activeWindow(),"Cannot Parse XML", "Unable to parse XML sent to plugin.")
            QtWidgets.QApplication.restoreOverrideCursor()
            QtWidgets.QApplication.processEvents()  
            return ""

        answer = QMessageBox.question(QApplication.activeWindow(),
            "WARNING: Long Process",
            "WARNING: This process can take some time.  Are you sure you want to continue?",
            QMessageBox.StandardButton.Yes,
            QMessageBox.StandardButton.No)

        if (answer != QMessageBox.StandardButton.Yes):
            QtWidgets.QApplication.restoreOverrideCursor()
            QtWidgets.QApplication.processEvents()  
            return ""

        xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
        projectfolder = self.pnc.get_projectfolder(xmlroot)
        print("project folder " + projectfolder)

        projtab = self.pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = self.pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = self.pnc.get_column_value(projtab.firstChild(), "project_name")

        if (projectfolder is None or projectfolder ==""):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""

        progbar = QProgressDialog()
        progbar.setValue(0)
        progbar.setLabelText("Archiving project emails...")
        progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint 
            )
        progbar.setMinimumWidth(350)
        progbar.setCancelButton(None)
        progbar.show()

        progval = 0

        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()

        sentfolder = projectfolder + "/" + self.backup_sent_folder
        receivedfolder = projectfolder + "/" + self.backup_inbox_folder

        if not QDir(sentfolder).exists():
            QDir().mkpath(sentfolder)

        if not QDir(receivedfolder).exists():
            QDir().mkpath(receivedfolder)


        try:
            outlook = win32com.client.Dispatch("Outlook.Application")
            mapi = outlook.GetNamespace("MAPI")
            #mailfold = mapi.Folders.GetFirst()
            inbox = mapi.GetDefaultFolder(6)
            sent = mapi.GetDefaultFolder(5)

            progtot = inbox.Items.Count + sent.Items.Count
            for message in inbox.Items:
                progval = progval + 1
                progbar.setValue(int(min(progval / progtot * 100, 100)))
                progbar.setLabelText("Parsing Inbox items...")
                QtWidgets.QApplication.processEvents()

                if message.Subject.find(projnum) >= 0:
                    if hasattr(message, "SentOn"):
                        filename = receivedfolder + "/" + self.make_filename(str(message.SentOn), message.Subject) + ".msg"
                        eml_filename = receivedfolder + "/" + self.make_filename(str(message.SentOn), message.Subject) + ".eml"
                        filename = filename.replace("/", "\\")
                        eml_filename = eml_filename.replace("/", "\\")

                        #print (filename + "\n")

                        if not QFile.exists(filename) and not QFile.exists(eml_filename):
                            message.SaveAs(filename, 3)
                    else:
                        print("Message has no sent date: " + message.Subject)

            for message in sent.Items:
                progval = progval + 1
                progbar.setValue(int(min(progval / progtot * 100, 100)))
                progbar.setLabelText("Parsing Sent items...")
                QtWidgets.QApplication.processEvents()

                if message.Subject.find(projnum) >= 0:
                    filename = sentfolder + "/" + self.make_filename(str(message.SentOn), message.Subject) + ".msg"
                    eml_filename = sentfolder + "/" + self.make_filename(str(message.SentOn), message.Subject) + ".eml"
                    filename = filename.replace("/", "\\")
                    eml_filename = eml_filename.replace("/", "\\")

                    if not QFile.exists(filename) and not QFile.exists(eml_filename):
                        message.SaveAs(filename, 3)

        except Exception as e:  # catches any Python exception
            # If it's a COM error, the details are in e.args
            msg = f'Error: {e} Attempting to export email from Outlook.'
            print(msg)
            QMessageBox.critical(QApplication.activeWindow(),"Python Exception", msg)
            if hasattr(e, 'args') and e.args:
                print(f'COM error code: {e.args[0]}')   # usually a HRESULT like -2147352567

        mail_enum = None
        message = None
        inbox = None
        sent = None
        outlook = None
        mapi = None
        gc.collect()

        progbar.setValue(100)
        progbar.setLabelText("Complete ...")
        progbar.hide()
        progbar.close()
        progbar = None # must be destroyed

        QtWidgets.QApplication.restoreOverrideCursor()
        QtWidgets.QApplication.processEvents()  

        return ""

    def send_email(self, addresses, subject, content, attachments):
        try:
            outlook = win32com.client.Dispatch("Outlook.Application")

            message = outlook.CreateItem(0)
            message.To = ""

            for address in addresses:
                message.Recipients.Add(address["emailAddress"]["address"])

            message.Recipients.ResolveAll()

            message.Display()

            # Activate the Inspector window for THIS message
            inspector = message.GetInspector
            inspector.Activate()


            DefaultSignature = message.HTMLBody

            message.Subject = subject
            message.HTMLBody = content + DefaultSignature

            for attachment in attachments:
                message.Attachments.Add(attachment, 1)

        except Exception as e:  # catches any Python exception
            # If it's a COM error, the details are in e.args
            msg = f'Error: {e} Attempting to send email {subject} in Outlook.'
            print(msg)
            QMessageBox.critical(QApplication.activeWindow(),"Python Exception", msg)
            if hasattr(e, 'args') and e.args:
                print(f'COM error code: {e.args[0]}')   # usually a HRESULT like -2147352567

        outlook = None
        message = None
        gc.collect()

        self.bring_window_to_front(subject)

        return None
