from PySide6 import QtSql, QtGui, QtCore
from PySide6.QtCore import QDirIterator, QDir, QSettings
from PySide6.QtXml import QDomDocument, QDomNode
from PySide6.QtWidgets import QMessageBox, QMainWindow

from win32com.client import GetObject
import win32com
import re
import subprocess
import sys

class ProjectNotesCommon:

    def to_xml(self, val):
        if val is None:
            return ("")
        else:
            val = val.replace(">", "&gt;")
            val = val.replace("<", "&lt;")
            val = val.replace("&", "&#38;")
            val = val.replace("\"", "&quot;")
            val = val.replace("'", "&#39;")
            return(val)

    def to_html(self, val):
        if val is None:
            return ("")
        else:
            val = val.replace(">", "&gt;")
            val = val.replace("<", "&lt;")
            val = val.replace("&", "&amp;")
            val = val.replace("\"", "&quot;")
            val = val.replace("'", "&#39;")
            val = val.replace(" ", "&#160;")
            val = val.replace("\n", "<br>")
            val = val.replace("\t", "&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;")
            return(val)

    def valid_filename(self, val):
        if val is None:
            return(None)
        else:
            val = re.sub(r"[`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[]", "_", val)
            return(val)

    def find_node(self, node, type, attribute, name):
        if node is None:
            return(None)
        #print("looking at node: ", type(node))
        children = node.firstChild()

        while not children.isNull():
            if ( children.nodeName() == type and children.attributes().namedItem(attribute).nodeValue() == name ):
                return(children)

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

    def find_projectfiles(self, projectnumber, subfolder, filedescription, filetypedescription):
        xmldoc = ""

        it = QDirIterator(subfolder)

        while it.hasNext():
            it.next()
            filename = it.fileName()

            if filename.lower().find("template") == -1 and not it.fileInfo().IsDir(): # don't show templates
                xmldoc = xmldoc + "<row>\n"
                xmldoc = xmldoc + "<column name=\"project_id\" number=\"1\" lookupvalue=\"" + self.to_xml(projectnumber) + "\"></column>\n"
                xmldoc = xmldoc + "<column name=\"location_type\" number=\"2\">" + self.to_xml(filetypedescription) + "</column>\n"
                xmldoc = xmldoc + "<column name=\"location_description\" number=\"3\">" + self.to_xml(filedescription) + " : " + it.fileName() + "</column>\n"
                xmldoc = xmldoc + "<column name=\"full_path\" number=\"4\">" + self.to_xml(filename) + "</column>\n"
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

                    # link all ms project files
                    if QDir(foldername + "\\Project Management\\Schedule").exists():
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Schedule\\*.mpp", "Project Schedule", "Microsoft Project" )

                    # link all purchase orders
                    if QDir(foldername + "\\Project Management\\Purchase Orders").exists():
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Purchase Orders\\*.pdf", "Customer PO", "PDF File" )

                    # link all quotes
                    if QDir(foldername + "\\Project Management\\Quotes").exists():
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Quotes\\*.pdf", "Quote", "PDF File" )
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Quotes\\*.xlsx", "Estimate", "Excel Document" )
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Quotes\\*.xls", "Estimate", "Excel Document" )

                    # link all quotes
                    if QDir(foldername + "\\Project Management\\Risk Management").exists():
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\Risk Management\\*.xlsx", "Risk Register", "Excel Document" )

                    # link all PCRs
                    if QDir(foldername + "\\Project Management\\PCR\'s").exists():
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\PCR\'s\\*.pdf", "Change Request", "PDF File" )
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\PCR\'s\\*.docx", "Change Request", "Word Document" )
                        xmldoc = xmldoc + self.find_projectfiles(projectnumber, foldername + "\\Project Management\\PCR\'s\\*.doc", "Change Request", "Word Document" )

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
        table = self.find_node(node.toElement(), "table", "name", "ix_project_locations")
        projectfolder = ""

        if table is None:
            return(projectfolder)

        row = table.firstChild()

        while not row.isNull():
            desc = self.get_column_value(row, "location_description")
            if desc == "Project Folder":
                projectfolder = self.get_column_value(row, "full_path")
                return(projectfolder)

            row = row.nextSibling()

        return(projectfolder)

    def ping_server(self, servername):
        colProcess = None
        strList = None
        p = None

        objWMIService = win32com.client.GetObject("winmgmts:{impersonationLevel=impersonate}!\\\\")
        colProcess = objWMIService.ExecQuery("select * from Win32_PingStatus where address = '" + servername + "'")

        #print(objWMIService)
        for p in colProcess:
            #print("found row " + p.Address)
            #if p.StatusCode:
            #    print("srv: " + servername + " stat: " + p.StatusCode + "\n")
            if p.StatusCode == 0 and not p.StatusCode is None:
                return(True)

        objWMIService = None

        return(False)

    def check_drive(self, network, letter):
        drives = network.EnumNetworkDrives()

        i = 0
        while i < drives.count():
            k = drives.Item(i)
            # print ("enm: " .. k .. " arg: " .. letter .. "\n")
            if k == letter:
                return(True)

            i = i + 1

        return(False)

    def connect_drives(self):
        network = win32com.client.Dispatch("WScript.Network")

        if self.ping_server("INDFP03.cornerstonecontrols.local"):
            if not self.check_drive(network, "K:"):
                network.MapNetworkDrive("K:", "\\\\INDFP03.cornerstonecontrols.local\\DATA", True)
            if not self.check_drive(network, "P:"):
                network.MapNetworkDrive("P:", "\\\\INDFP03.cornerstonecontrols.local\\DATA\\ENGINEERING\\PROJECTS", True)

        if self.ping_server("CINVFP01.cornerstonecontrols.local"):
            if not self.check_drive(network, "N:"):
                network.MapNetworkDrive("N:", "\\\\CINVFP01.cornerstonecontrols.local\\DATA", True)
            if not self.check_drive(network, "O:"):
                network.MapNetworkDrive("O:", "\\\\CINVFP01.cornerstonecontrols.local\\PROJECTS", True)

        if self.ping_server("CORNERSTONECONTROLS.LOCAL"):
            if not self.check_drive(network, "R:"):
                network.MapNetworkDrive("R:", "\\\\CORNERSTONECONTROLS.LOCAL\\COMMON\\Public", True)

        network = None
        return(None)

    def exec_program(self, fullpath):
        result = subprocess.run( [fullpath], capture_output=True, text=True)

        print("stdout:", result.stdout)
        print("stderr:", result.stderr)

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

        return(True)

    def connect(self):
        op = self.get_global_setting("OraclePassword")
        ou = self.get_global_setting("OracleUsername")
        ds = self.get_global_setting("OracleDataSource")

        strconnect = "Provider=OraOLEDB.Oracle.1;User ID=" + ou + ";Password=" + op + ";Data Source=" + ds + ";"

        adodb = win32com.client.Dispatch("ADODB.Connection")

        adodb.ConnectionString = strconnect;
        adodb.Open()

        return(adodb)

    def close(self, adodb):
      adodb.Close()
      adodb = None