import platform

if (platform.system() == 'Windows'):
    from win32com.client import GetObject
    import win32com
    import win32api
    import win32gui

top_windows = []

def windowEnumerationHandler(hwnd, tpwindows):
    if (platform.system() == 'Windows'):
        tpwindows.append((hwnd, win32gui.GetWindowText(hwnd)))


from PyQt6 import QtSql, QtGui, QtCore, QtWidgets
from PyQt6.QtCore import QDirIterator, QDir, QSettings, QFile
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication

from PyQt6.QtCore import QT_VERSION_STR, PYQT_VERSION_STR
print("Qt: v", QT_VERSION_STR, "\tPyQt: v", PYQT_VERSION_STR)

import re
import subprocess

class ProjectNotesCommon:

    def to_xml(self, val):
        if val is None:
            return ("")
        else:
            newval = ""

            val = val.replace("&", "&amp;")
            val = val.replace(">", "&gt;")
            val = val.replace("<", "&lt;")
            val = val.replace("\"", "&quot;")
            val = val.replace("'", "&apos;")

            for c in val:
                if (ord(c) > 255):
                    print("invalid character " + f'&#x{ord(c):04X};' + " removed")
                elif (ord(c) < 32 or ord(c) > 122):
                    newval = newval + f'&#x{ord(c):04X};'
                else:
                    newval = newval + c

            return(newval)

    def to_html(self, val):
        if val is None:
            return ("")
        else:
            newval = ""

            val = val.replace("&", "&amp;")
            val = val.replace(">", "&gt;")
            val = val.replace("<", "&lt;")
            val = val.replace("\"", "&quot;")
            val = val.replace("'", "&apos;")
            val = val.replace(" ", "&#160;")
            val = val.replace("\n", "<br>")
            val = val.replace("\t", "&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;")

            for c in val:
                if (ord(c) > 255):
                    print("invalid character " + f'&#x{ord(c):04X};' + " removed")
                elif (ord(c) < 32 or ord(c) > 122):
                    newval = newval + f'&#x{ord(c):04X};'
                else:
                    newval = newval + c

            return(newval)

    def valid_filename(self, val):
        if val is None:
            return(None)
        else:
            val = re.sub(r"[`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[]", "_", val)
            return(val)

    def find_node(self, node, type, attribute, name):
        if node is None:
            return(None)

        #print("looking at node: ", name)
        
        QtWidgets.QApplication.processEvents()
        children = node.firstChild()

        while not children.isNull():
            if ( children.nodeName() == type and children.toElement().attribute(attribute) == name ):
                return(children)

            #print("node check : " + children.nodeName())
            QtWidgets.QApplication.processEvents()

            subsearch = self.find_node(children, type, attribute, name)
            if subsearch is not None:
                return(subsearch)

            children = children.nextSibling()

        return(None)

    def find_node_by2(self, node, type, attribute1, name1, attribute2, name2):
        if node is None:
            return(None)

        children = node.firstChild()

        while not children.isNull():
            if ( children.nodeName() == type and children.attributes().namedItem(attribute1).nodeValue() == name1 and children.attributes().namedItem(attribute2).nodeValue() == name2  ):
                return(children)

            subsearch = self.find_node_by2(children, type, attribute1, name1, attribute2, name2)
            if subsearch is not None:
                return(subsearch)

            children = children.nextSibling()

        return(None)

    def get_column_value(self, node, name):
        if node is None:
            return(None)

        colnode = node.firstChild()

        while not colnode.isNull():
            #print ("name: " + colnode.nodeName() + " text: " + colnode.toElement().text() + " attribute_name: " + colnode.attributes().namedItem("name").nodeValue())
            if colnode.nodeName() == "column":
                if colnode.attributes().namedItem("name").nodeValue() == name:
                    lookupvalue = colnode.attributes().namedItem("lookupvalue").nodeValue()

                    if (not lookupvalue is None and not lookupvalue == ""):
                        return(lookupvalue)
                    else:
                        return(colnode.toElement().text())

            colnode = colnode.nextSibling()
        return("")

    def find_projectfiles(self, projectnumber, subfolder, filedescription, filetypedescription, namefilter):
        xmldoc = ""

        #print("== initiating find project files in " + subfolder)
        it = QDirIterator(subfolder, namefilter)

        while it.hasNext():
            it.next()
            filename = it.fileName()
            #print("-> found file name : " + filename)

            if filename.lower().find("template") == -1 and not it.fileInfo().isDir(): # don't show templates
                xmldoc = xmldoc + "<row>\n"
                xmldoc = xmldoc + "<column name=\"project_id\" lookupvalue=\"" + self.to_xml(projectnumber) + "\"></column>\n"
                xmldoc = xmldoc + "<column name=\"location_type\">" + self.to_xml(filetypedescription) + "</column>\n"
                xmldoc = xmldoc + "<column name=\"location_description\">" + self.to_xml(filedescription) + " : " + self.to_xml(it.fileName()) + "</column>\n"
                xmldoc = xmldoc + "<column name=\"full_path\">" + self.to_xml(subfolder + "\\" + filename) + "</column>\n"
                xmldoc = xmldoc + "</row>\n"

        return(xmldoc)

    def find_projectlocations(self, projectnumber, projects_folder):
        xmldoc = ""


        if QDir(projects_folder).exists():

            it = QDirIterator(projects_folder, {"*" + projectnumber + "*"})
            while it.hasNext():
                it.next()
                foldername = projects_folder + "\\" + it.fileName();

                if it.fileInfo().isDir():
                    xmldoc = xmldoc + "<row>\n"
                    xmldoc = xmldoc + "<column name=\"project_id\" number=\"1\" lookupvalue=\"" + self.to_xml(projectnumber) + "\"></column>\n"
                    xmldoc = xmldoc + "<column name=\"location_type\" number=\"2\">File Folder</column>\n"
                    xmldoc = xmldoc + "<column name=\"location_description\" number=\"3\">Project Folder</column>\n"
                    xmldoc = xmldoc + "<column name=\"full_path\" number=\"4\">" + self.to_xml(foldername) + "\\Project Management</column>\n"
                    xmldoc = xmldoc + "</row>\n"

                    #print("found folder name : " + foldername)

                    # link all ms project files
                    if QDir(foldername + "\\Project Management\\Schedule").exists():
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Schedule", "Project Schedule", "Microsoft Project", ["*.mpp"] )

                    # link all purchase orders
                    if QDir(foldername + "\\Project Management\\Purchase Orders").exists():
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Purchase Orders", "Customer PO", "PDF File", ["*.pdf"] )

                    # link all quotes
                    if QDir(foldername + "\\Project Management\\Quotes").exists():
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Quotes", "Quote", "PDF File", ["*.pdf"] )
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Quotes", "Estimate", "Excel Document", ["*.xlsx", "*.xls"] )

                    # link all quotes
                    if QDir(foldername + "\\Project Management\\Risk Management").exists():
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Risk Management", "Risk Register", "Excel Document", ["*.xlsx"] )

                    # link all PCRs
                    if QDir(foldername + "\\Project Management\\PCR\'s").exists():
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\PCR\'s", "Change Request", "PDF File", ["*.pdf"] )
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\PCR\'s", "Change Request", "Word Document", ["*.docx"] )
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\PCR\'s", "Change Request", "Word Document", ["*.doc"] )
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\PCR\'s", "Change Request Calcs", "Excel Document", ["*.xlsx", "*.xls"] )

            return(xmldoc)

    #   return xmldoc

    def xml_col(self, xmldoc, name, content, lookupvalue):
        colnode = xmldoc.createElement("column")
        colnode.setAttribute("name", name)

        if (not lookupvalue == "" and not lookupvalue is None):
            colnode.setAttribute("lookupvalue", lookupvalue)

        if (not content is None):
            txtnode = xmldoc.createTextNode(content)
            colnode.appendChild(txtnode)
        return(colnode)

    def xml_table(self, xmldoc, name):
        tablenode = xmldoc.createElement("table")
        tablenode.setAttribute("name", name)
        return(tablenode)

    def xml_row(self, xmldoc):
        rownode = xmldoc.createElement("row")
        return(rownode)

    def get_projectfolder(self, node):
        table = self.find_node(node.toElement(), "table", "name", "project_locations")
        projectfolder = ""

        if table is None:
            return(projectfolder)

        row = table.firstChild()

        while not row.isNull():
            desc = self.get_column_value(row, "location_description")
            #print("Finding location : " + desc)
            if desc == "Project Folder":
                projectfolder = self.get_column_value(row, "full_path")
                return(projectfolder)

            row = row.nextSibling()

        return(projectfolder)

    def ping_server(self, servername):
        if (platform.system() != 'Windows'):
            print("ping_server requires win32com not supported on this platform")
            return(False)

        colProcess = None
        strList = None
        p = None

        objWMIService = win32com.client.GetObject("winmgmts:{impersonationLevel=impersonate}!\\\\")
        colProcess = objWMIService.ExecQuery("select * from Win32_PingStatus where address = '" + servername + "'")

        print(objWMIService)
        for p in colProcess:
            print("found row " + p.Address)
            if p.StatusCode:
                print("srv: " + servername + " stat: " + str(p.StatusCode))
            if p.StatusCode == 0 and not p.StatusCode is None:
                return(True)

        objWMIService = None
        colProcess = None

        return(False)

    def check_drive(self, network, letter):
        if (platform.system() != 'Windows'):
            print("check_drive requires win32com not supported on this platform")
            return(False)

        drives = network.EnumNetworkDrives()

        i = 0
        while i < drives.count():
            k = drives.Item(i)
            # print ("enm: " .. k .. " arg: " .. letter .. "\n")
            if k == letter:
                return(True)

            i = i + 1

        return(False)

    def exec_program(self, fullpath):

        if (platform.system() == 'Windows'):
            win32api.WinExec( fullpath )
        else:
            #result = subprocess.Popen( [fullpath] ) #, capture_output=False)
            os.system( fullpath ) # i think this will work on Linux

        # print("stdout:", result.stdout)
        # print("stderr:", result.stderr)

    def get_plugin_setting(self, settingname, pluginname = None):
        cfg = QSettings("ProjectNotes","PluginSettings")
        cfg.setFallbacksEnabled(False)
        val = ""

        if pluginname is None:
            val = cfg.value("Global Settings/" + settingname, "")
        else:
            val = cfg.value(pluginname + "/" + settingname, "")

        #print("reading global setting: " + settingname + " value: " + val)

        return val

    def verify_global_settings(self):

        pf = self.get_plugin_setting("ProjectsFolder")
        if pf == None or pf == "" or not QDir(pf).exists():
            QMessageBox.warning(None, "Invalid Global Setting", "ProjectsFolder must be set in the Global Settigns plugin.", QMessageBox.Ok)
            return(False)

        return(True)

    def bring_window_to_front(self, title):
        if (platform.system() != 'Windows'):
            print("bring window to front requires win32gui not supported on this platform")
            return

        #QtWidgets.QApplication.processEvents()
        print("looking for window title " + title)
        win32gui.EnumWindows(windowEnumerationHandler, top_windows)

        for i in top_windows:
            if title.lower() in i[1].lower():
                print("found " +  i[1])
                win32gui.ShowWindow(i[0],5)
                win32gui.SetForegroundWindow(i[0])
                break
        return

    def scrape_project_name(self, xmldoc):
        projectname = ""
        contents = None
        lookupvalue = None

        col = self.find_node(xmldoc, "column", "name", "project_name")
        if col:
            lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
            contents = col.toElement().text()

            if lookupvalue != "" and lookupvalue is not None:
                projectname = lookupvalue
            else:
                projectname = contents

        if projectname == "" or projectname is None:
            col = self.find_node(xmldoc, "column", "name", "project_id_name")
            if col:
                lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
                contents = col.toElement().text()

            if lookupvalue is not None and lookupvalue != "":
                projectname = lookupvalue
            else:
                projectname = contents

        return(projectname)

    def scrape_project_number(self, xmldoc):
        projectnumber = ""
        contents = None
        lookupvalue = None

        col = self.find_node(xmldoc, "column", "name", "project_number")
        if col:
            lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
            contents = col.toElement().text()

            if lookupvalue != "" and lookupvalue is not None:
                projectnumber = lookupvalue
            else:
                projectnumber = contents

        if projectnumber == "" or projectnumber is None:
            col = self.find_node(xmldoc, "column", "name", "project_id")
            if col:
                lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
                contents = col.toElement().text()

            if lookupvalue != "" and lookupvalue != None:
                projectnumber = lookupvalue
            else:
                projectnumber = contents

        return(projectnumber)

    def email_word_file_as_html(self, subject, recipients, attachment, wordfile):
        if (platform.system() != 'Windows'):
            print("email_word_file_as_html only supported on Windows")
            return

        if wordfile is not None:
            word = win32com.client.Dispatch("Word.Application")
            word.Visible = 0
            doc = word.Documents.Open(wordfile)
            doc.SaveAs(wordfile + ".html", 8)
            word.Quit()
            word = None

            file = open(wordfile + ".html", "r")
            html = file.read()
            file.close()
            QFile.remove(wordfile + ".html")
            dir = QDir(wordfile + "_files")
            dir.removeRecursively()

        outlook = win32com.client.Dispatch("Outlook.Application")
        message = outlook.CreateItem(0)
        message.To = ""

        outlook.ActiveExplorer().Activate()
        message.Display()

        message.To = recipients
        DefaultSignature = message.HTMLBody

        message.Subject = subject

        if wordfile is not None:
            message.HTMLBody = html + DefaultSignature

        if attachment is not None:
            message.Attachments.Add(attachment, 1)

        self.bring_window_to_front(subject)

        outlook = None
        message = None