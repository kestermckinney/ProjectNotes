from win32com.client import GetObject
import win32com
import sys

from PyQt6 import QtSql, QtGui, QtCore, QtWidgets
from PyQt6.QtCore import QDirIterator, QDir, QSettings, QFile
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication

class ProjectNotesWordTools:
    def open_word_document(self, fullpath):
        try:
            word = win32com.client.DispatchEx("Word.Application")

            cleanpath = fullpath.replace("/", "\\")

            doc = word.Documents.Open(cleanpath)
            word.Visible = False
            word.ScreenUpdating = False
            word.DisplayAlerts = 0
            word.CheckLanguage = False
            word.AutoCorrect.CorrectCapsLock = False
            word.AutoCorrect.CorrectDays = False
            word.AutoCorrect.CorrectHangulAndAlphabet = False
            word.AutoCorrect.CorrectInitialCaps = False
            word.AutoCorrect.CorrectKeyboardSetting = False
            word.AutoCorrect.CorrectSentenceCaps = False

            handle = {}
            handle['word'] = word
            handle['document'] = doc
            return handle

        except Exception as e:  # catches any Python exception
            # If it’s a COM error, the details are in e.args
            msg = f'Error: {e} Attempting to open {cleanpath} in Word.'
            print(msg)
            QMessageBox.critical(None, "Python Exception", msg)
            if hasattr(e, 'args') and e.args:
                print(f'COM error code: {e.args[0]}')   # usually a HRESULT like -2147352567

            return None

    def close_word_document(self, handle):
        try:
            handle['document'].Close()
            handle['word'].Quit()

            handle['document'] = None
            handle['word'] = None

        except Exception as e:  # catches any Python exception
            # If it’s a COM error, the details are in e.args
            msg = f'Error: {e} Attempting to close a document and quit Word. Be sure to kill any stranded instanced of Word.'
            print(msg)
            QMessageBox.critical(None, "Python Exception", msg)
            if hasattr(e, 'args') and e.args:
                print(f'COM error code: {e.args[0]}')   # usually a HRESULT like -2147352567

    def killwordautomation(self):
        wmi_service = win32com.client.GetObject("winmgmts:{impersonationLevel=impersonate}!\\\\.\\root\\cimv2")
        processes = wmi_service.ExecQuery("Select * from Win32_Process Where Name = 'WINWORD.EXE' and CommandLine like '%automation%'")

        closecount = 0

        for p in processes:
            try:
                p.Terminate
                closecount = closecount + 1
            except:
                print("WMI Terminate call failed.")

        if (closecount > 0):
            QMessageBox.information(None, "Terminated Stranded Word Programs", "Sucessfully terminated stranded Micorosoft Word programs running in the background.", QMessageBox.StandardButton.Ok)
        else:   
            QMessageBox.information(None, "No Stranded Word Program Found", "Was not able to terminate stranded Micorosoft Word programs running in the background.", QMessageBox.StandardButton.Ok)     

        wmi_service = None
        wmi = None

    def get_html_version_of_word_doc(self, wordfile, temphtmlfile):

        if wordfile is not None:
            try:
                cleandocpath = wordfile.replace("/", "\\")
                cleanhtmlpath = temphtmlfile.replace("/", "\\")

                word = win32com.client.Dispatch("Word.Application")
                word.Visible = 0
                doc = word.Documents.Open(cleandocpath)
                doc.SpellingChecked = False
                word.CheckLanguage = False
                doc.GrammarChecked = False
                word.AutoCorrect.CorrectCapsLock = False
                word.AutoCorrect.CorrectDays = False
                word.AutoCorrect.CorrectHangulAndAlphabet = False
                word.AutoCorrect.CorrectInitialCaps = False
                word.AutoCorrect.CorrectKeyboardSetting = False
                word.AutoCorrect.CorrectSentenceCaps = False

                # remove any old output
                QFile.remove(cleanhtmlpath)
                dir = QDir(cleanhtmlpath + "_files")
                dir.removeRecursively()

                doc.SaveAs(cleanhtmlpath, 8)

                word.Quit()
                word = None

            except Exception as e:  # catches any Python exception
                # If it’s a COM error, the details are in e.args
                msg = f'Error: {e} Attempting to get the html version of {cleandocpath}'
                print(msg)
                QMessageBox.critical(None, "Python Exception", msg)
                if hasattr(e, 'args') and e.args:
                    print(f'COM error code: {e.args[0]}')   # usually a HRESULT like -2147352567

                return None

            file = open(cleanhtmlpath, "r")
            html = file.read()
            file.close()

            # remove any old output
            QFile.remove(cleanhtmlpath)
            dir = QDir(cleanhtmlpath + "_files")
            dir.removeRecursively()

            return html

        return None
