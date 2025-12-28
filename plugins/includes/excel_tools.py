from win32com.client import GetObject
import win32com
import win32api
import win32gui

from includes.common import ProjectNotesCommon

from PyQt6 import QtSql, QtGui, QtCore, QtWidgets
from PyQt6.QtCore import QDirIterator, QDir, QSettings, QFile
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication

top_windows = []

def handler_window_enumerator(hwnd, topwindows):
    topwindows.append((hwnd, win32gui.GetWindowText(hwnd)))

class ProjectNotesExcelTools:
    def open_excel_document(self, fullpath):
        excel_obj = win32com.client.Dispatch("Excel.Application")
        excel_obj.Visible = False
        excel_obj.ScreenUpdating = False
        excel_obj.EnableEvents = False

        workbook_obj = excel_obj.WorkBooks.Open(fullpath)

        excel_obj.Calculation = -4109 # xlCalculationManual
        #excel_obj.CalculateBeforeSave = False

        excel_obj.CutCopyMode = False  # don't prompt about the
        excel_obj.DisplayAlerts = False

        handle = {}
        handle['excel'] = excel_obj
        handle['workbook'] = workbook_obj
        return(handle)
        #return({'excel':, 'workbook':workbook_obj})

    def find_cell_tag(self, sheet, tagname):
        searchrange = sheet.Range("A1", "Z10000") # hopefully this catches everything
        searchresult = searchrange.Find(tagname)

        return(searchresult)

    def bring_window_to_front(self, title):
        win32gui.EnumWindows(handler_window_enumerator, top_windows)

        for i in top_windows:
            if title.lower() in i[1].lower():
                win32gui.ShowWindow(i[0],5)
                win32gui.SetForegroundWindow(i[0])
                break
        return

    def replace_cell_tag(self, sheet, oldtagname, newtagname):
        if oldtagname is None:
            return False

        if newtagname is None:
            searchrange = sheet.Range("A1", "Z10000") # hopefully this catches everything
            replaceresult = searchrange.Replace(oldtagname, "", 2)
        else:
            searchrange = sheet.Range("A1", "Z10000") # hopefully this catches everything
            replaceresult = searchrange.Replace(oldtagname, newtagname[:255], 2)
            #print("replace cell tag: " + oldtagname + " with " + newtagname + " result: " + str(replaceresult))

        return(replaceresult)

    def set_cell_by_tag(self, sheet, oldtagname, cellvalue):
        searchrange = self.find_cell_tag(sheet, oldtagname)

        if searchrange:
            searchrange.Value2 = cellvalue
            return(True)

        return(False)

    def expand_row(self, sheet, tagname, rowcount):
        resrange = self.find_cell_tag(sheet, tagname)
        #print("expand row row count: " + str(rowcount))
        r = None
        i = 0
        if not resrange is None:
            #print("expand row found row")
            r = resrange.Row

        if not resrange is None:
            for i in range (2, rowcount + 1):
                #print("expand row performing copy insert...")
                resrange.EntireRow.Copy()
                resrange.EntireRow.Insert()

            for i in range(1, rowcount + 1):
                #print("expand row performing row count change...")
                sheet.Rows(r).EntireRow.Replace(">", str(i) + ">", 2) # 2 is replace part
                r = r + 1

    def expand_section(self, sheet, starttag, endtag, rowcount):
        resranges = self.find_cell_tag(sheet, starttag)
        resrangee = self.find_cell_tag(sheet, endtag)

        if not resrangee is None and not resranges is None:
            section = sheet.Range( str(resranges.Row) + ":" + str(resrangee.Row))
            size = resrangee.Row - resranges.Row
            r = resranges.Row

            if not section is None:
                for i in range(2, rowcount + 1):
                    section.EntireRow.Copy()
                    section.EntireRow.Insert()

                for i in range(1, rowcount + 1):
                    sheet.Rows(str(r) + ":" + str(r + size)).EntireRow.Replace(starttag, "", 2)
                    sheet.Rows(str(r) + ":" + str(r + size)).EntireRow.Replace(endtag, "", 2)

                    sheet.Rows(str(r) + ":" + str(r + size)).EntireRow.Replace(">", str(i) + ">", 2) # 2 is replace part
                    r = r + size + 1

    def cliprowsbytags(self, sheet, starttag, endtag):
        resranges = self.find_cell_tag(sheet, starttag)
        resrangee = self.find_cell_tag(sheet, endtag)

        if not resrangee is None and not resranges is None:
            cutrange = sheet.Range( str(resranges.Row) + ":" + str(resrangee.Row))
            cutrange.EntireRow.Delete()

    def save_excel_as_pdf(self, handle, sheet, filename):
        # handle['excel'].CutCopyMode = 0  # don't prompt about the clipboard
        # handle['excel'].DisplayAlerts = 0
        QFile.remove(filename)

        try:
            sheet.ExportAsFixedFormat(0, filename, 0, 1)
        except Exception as e:  # catches any Python exception
            # If it’s a COM error, the details are in e.args
            print(f'Error: {e} Attempting To Save: {filename}')
            if hasattr(e, 'args') and e.args:
                print(f'COM error code: {e.args[0]}')   # usually a HRESULT like -2147352567

    def email_excel_html(self, sheet, subject, recipients, attachment):
        outlook = win32com.client.Dispatch("Outlook.Application")
        message = outlook.CreateItem(0)
        message.To = ""

        outlook.ActiveExplorer().Activate()
        message.Display()

        message.To = recipients
        DefaultSignature = message.HTMLBody

        message.Subject = subject

        if attachment is None:
            copyrange = sheet.Range(sheet.PageSetup.PrintArea + "")
            copyrange.Copy()

            message.GetInspector.WordEditor.Content.Paste()

            message.HTMLBody = message.HTMLBody + DefaultSignature
        else:
            message.HTMLBody = "<html><p>Please see the attached report for project details.</p></html>" + DefaultSignature
            message.Attachments.Add(attachment, 1)


        self.bring_window_to_front(subject)

        outlook = None
        message = None

    def close_excel_document(self, handle):
        handle['excel'].WorkBooks.Close()
        handle['excel'].Quit()

        handle['workbook'] = None
        handle['excel'] = None


    def killexcelautomation(self):
        wmi_service = win32com.client.GetObject("winmgmts:{impersonationLevel=impersonate}!\\\\.\\root\\cimv2")
        processes = wmi_service.ExecQuery("Select * from Win32_Process Where Name = 'EXCEL.EXE' and CommandLine like '%automation%'")

        closecount = 0

        for p in processes:
            try:
                p.Terminate
                closecount = closecount + 1
            except:
                print("WMI Terminate call failed.")

        if (closecount > 0):
            QMessageBox.information(None, "Terminated Stranded Excel Programs", "Sucessfully terminated stranded Micorosoft Excel programs running in the background.", QMessageBox.StandardButton.Ok)
        else:   
            QMessageBox.information(None, "No Stranded Excel Program Found", "Was not able to terminate stranded Micorosoft Excel programs running in the background.", QMessageBox.StandardButton.Ok)     

        wmi_service = None
        wmi = None
