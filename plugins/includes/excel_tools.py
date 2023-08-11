from win32com.client import GetObject
import win32com

class ProjectNotesExcelTools:
    def open_excel_document(self, fullpath):
        excel_obj = win32com.client.Dispatch("Excel.Application")
        workbook_obj = excel_obj.WorkBooks.Open(fullpath)

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
        #handle['excel'].CutCopyMode = 0  -- don't prompt about the clipboard
        #handle['excel'].DisplayAlerts = 0

        sheet.ExportAsFixedFormat(0, filename) #, 0, 1, 0, 0)

    def email_excel_html(self, sheet, subject, recipients, attachment):
        outlook = win32com.client.Dispatch("Outlook.Application")
        message = outlook.CreateItem(0)
        message.To = ""

        #wx.wxMessageBox("What:  " .. recipients)
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

        outlook = None
        message = None

    def close_excel_document(self, handle):
        handle['excel'].WorkBooks.Close()
        handle['excel'].Quit()

        handle['workbook'] = None
        handle['excel'] = None


    def killexcelautomation(self):
        colProcess = None
        strList = None
        p = None

        objWMIService = win32com.client.GetObject("winmgmts:{impersonationLevel=impersonate}!\\\\.\\root\\cimv2")
        colProcess = objWMIService.ExecQuery("Select * from Win32_Process Where Name like 'EXCEL.EXE' and CommandLine like '%automation%'")
        for p in colProcess:
            try:
                p.Terminate()
            except:
                print("Could not close an EXCEL.EXE process.")

        objWMIService = None
        colProcess = None

# begin testing procedures
#pne = ProjectNotesExcelTools()
#pne.killexcelautomation()
#pne.open_excel_document("C:/Users/pamcki/OneDrive - Cornerstone Controls/Documents/ProjectNotes_plugins/TestExcel.xlsx")
