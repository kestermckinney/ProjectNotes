from win32com.client import GetObject
import win32com
import sys

from PyQt6 import QtSql, QtGui, QtCore, QtWidgets
from PyQt6.QtCore import QDirIterator, QDir, QSettings, QFile
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication

class ProjectNotesWordTools:
    def open_word_document(self, fullpath):
        word = win32com.client.DispatchEx("Word.Application")

        doc = word.Documents.Open(fullpath)
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
        return(handle)

    def close_word_document(self, handle):
        handle['document'].Close()
        handle['word'].Quit()

        handle['document'] = None
        handle['word'] = None


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

# begin testing procedures
"""
app = QApplication(sys.argv)

pnw = ProjectNotesWordTools()
hn = pnw.open_word_document("C:\\Users\\pamcki\\Documents\\Example Quote Doc.docx")
#pnw.close_word_document(hn)
pnw.killwordautomation()
"""