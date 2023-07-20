import platform

if (platform.system() == 'Windows'):
    from win32com.client import GetObject
    import win32com

from PyQt5 import QtSql, QtGui, QtCore, QtWidgets
from PyQt5.QtCore import QDirIterator, QDir, QSettings
from PyQt5.QtXml import QDomDocument, QDomNode
from PyQt5.QtWidgets import QMessageBox, QMainWindow

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

            return(xmldoc)

    def xml_doc_root(self):
      xmldoc = QDomDocument()
      root = xmldoc.createElement("projectnotes");
      xmldoc.appendChild(root);

      return xmldoc

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

    # this doesn't seem to work over a VPN
    def connect_drives(self):
        if (platform.system() != 'Windows'):
            print("connect_drives requires win32com not supported on this platform")
            return(False)
            
        network = win32com.client.Dispatch("WScript.Network")

        if self.ping_server("INDFP03.cornerstonecontrols.local"):
            #print("ping indfp03 worked...")
            if not self.check_drive(network, "K:"):
                network.MapNetworkDrive("K:", "\\\\INDFP03.cornerstonecontrols.local\\DATA", True)
            if not self.check_drive(network, "P:"):
                network.MapNetworkDrive("P:", "\\\\INDFP03.cornerstonecontrols.local\\DATA\\ENGINEERING\\PROJECTS", True)

        if self.ping_server("CINVFP01.cornerstonecontrols.local"):
            #print("ping cinvfp01 worked...")
            if not self.check_drive(network, "N:"):
                network.MapNetworkDrive("N:", "\\\\CINVFP01.cornerstonecontrols.local\\DATA", True)
            if not self.check_drive(network, "O:"):
                network.MapNetworkDrive("O:", "\\\\CINVFP01.cornerstonecontrols.local\\PROJECTS", True)

        if self.ping_server("CORNERSTONECONTROLS.LOCAL"):
            #print("ping cornerstonecontrols worked...")
            if not self.check_drive(network, "R:"):
                network.MapNetworkDrive("R:", "\\\\CORNERSTONECONTROLS.LOCAL\\COMMON\\Public", True)

        network = None
        return(None)

    def exec_program(self, fullpath):
        result = subprocess.Popen( [fullpath] ) #, capture_output=False)

        # print("stdout:", result.stdout)
        # print("stderr:", result.stderr)

    def get_global_setting(self, settingname):
        cfg = QSettings("ProjectNotes","PluginSettings")
        cfg.setFallbacksEnabled(False)
        val = ""

        val = cfg.value("Global Settings/" + settingname, "")

        #print("reading global setting: " + settingname + " value: " + val)

        return val

    def verify_global_settings(self):
        pw = self.get_global_setting("OraclePassword")
        if pw == None or pw == "":
            print("OraclePassword not set.")
            QMessageBox.warning(None, "Invalid Global Setting", "OraclePassword must be set in the Global Settigns plugin.", QMessageBox.Ok)
            return(False)

        un = self.get_global_setting("OracleUsername")
        if un == None or un == "":
            QMessageBox.warning(None, "Invalid Global Setting", "OracleUsername must be set in the Global Settigns plugin.", QMessageBox.Ok)
            return(False)

        ds = self.get_global_setting("OracleDataSource")
        if ds == None or ds == "":
            QMessageBox.warning(None, "Invalid Global Setting", "OracleDataSource must be set in the Global Settigns plugin.", QMessageBox.Ok)
            return(False)

        pf = self.get_global_setting("ProjectsFolder")
        if pf == None or pf == "" or not QDir(pf).exists():
            QMessageBox.warning(None, "Invalid Global Setting", "ProjectsFolder must be set in the Global Settigns plugin.", QMessageBox.Ok)
            return(False)

        # code below will cause the script to fail if connection settings are wrong
        if (platform.system() == 'Windows'):
            adodb = self.connect()
            rs = win32com.client.Dispatch("ADODB.Recordset")
            rs.ActiveConnection = adodb

            rs.Open("select sysdate from dual", adodb)
            st = rs.State
            #print("State: " +  str(st) + "\n")
            rs = None

            self.close(adodb)
            adodb = None

            if st == 0:
                QMessageBox.warning(None, "Could not connect to Oracle verify parameters are correct in the Global Settigns plugin.", QMessageBox.Ok)
                return(False)
        else:
            print("connect to ADODB requires win32com not supported on this platform")

        return(True)

    def connect(self):
        if (platform.system() != 'Windows'):
            print("connect to ADODB requires win32com not supported on this platform")
            return(False)
            
        op = self.get_global_setting("OraclePassword")
        ou = self.get_global_setting("OracleUsername")
        ds = self.get_global_setting("OracleDataSource")

        strconnect = "Provider=OraOLEDB.Oracle.1;User ID=" + ou + ";Password=" + op + ";Data Source=" + ds + ";"

        adodb = win32com.client.Dispatch("ADODB.Connection")

        adodb.ConnectionString = strconnect;
        adodb.Open()

        return(adodb)

    def close(self, adodb):
        if (platform.system() !='Windows'):
            print("close ADODB requires win32com not supported on this platform")
            return(False)

        adodb.Close()
        adodb = None

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


#pnc = ProjectNotesCommon()
#print(pnc.to_xml("This is <a> rest & I wan t' it tow ork  COMPANY LLC â BLUFFTON ÂÃÂÃ crap "))
