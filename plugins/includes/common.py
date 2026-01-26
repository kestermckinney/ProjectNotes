import os
import sys
import json
import platform
import subprocess

from PyQt6 import QtCore
from PyQt6.QtCore import QFile, QLockFile, QFileInfo, QIODevice, QDateTime, QUrl, QElapsedTimer, QStandardPaths, QDir, QJsonDocument, QSettings
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtWidgets import QApplication, QMainWindow

import re

class ProjectNotesCommon:
    def __init__(self):
        self.temporary_folder = QStandardPaths.writableLocation(QStandardPaths.StandardLocation.TempLocation)
        self.saved_state_file = self.temporary_folder + '/projectnotes_saved_state.json'
        self.lock_file = self.saved_state_file + '.lock'
        self.lock = QLockFile(self.lock_file)

        self.app = QApplication.instance()
        
        if self.app is None:
            print("WARNING!! Creating a new QApplication instance!")
            self.app = QApplication(sys.argv)

        self.mainwindow = next((w for w in QApplication.topLevelWidgets() if isinstance(w, QMainWindow)), None)

    def get_main_window(self):
        return self.mainwindow

    def get_temporary_folder(self):
        return self.temporary_folder

    def get_save_state(self, state_name):
        
        #print(f"Attempting to read save state {state_name} to {self.saved_state_file}.")

        # get the last state
        skip = 0

        if self.lock.tryLock(10000):
            file = QFile(self.saved_state_file)
            if file.exists():
                if file.open(QIODevice.OpenModeFlag.ReadOnly):
                    saved_state = json.loads(file.readAll().data().decode("utf-8"))
                    file.close()

                skip = saved_state.get(state_name, {}).get("skip", 0)
            else:
                saved_state = json.loads("{}")
                print(f"Failed to load previous state {state_name}.")

                return None

            self.lock.unlock()
        else:
            print("Could not acquire lock within 10 seconds.  Could not get state.")

            error = self.lock.error()
            if error == QLockFile.LockError.LockFailedError:
                print("File is currently locked by another process/thread.")
            elif error == QLockFile.LockError.PermissionError:
                print("Permission denied.")

            return None

        return skip

    def state_range_attrib(self, top, skip):
        xmlskip = ""
        xmltop = ""

        if (skip >= 0):
            xmlskip = f' skip="{skip}" '

        if (top > 0):
            xmltop = f' top="{top}" '

        return f"{xmlskip} {xmltop}"

    def set_save_state(self, state_name, oldskip, top, nodecount):
        #print(f"Attempting to write save state {state_name} to {self.saved_state_file}.")

        skip = oldskip

        #start over the full count was not returned
        if nodecount < top: 
            skip = 0
        else:
            skip = skip + top

        if self.lock.tryLock(10000):
            file = QFile(self.saved_state_file)
            if file.open(QIODevice.OpenModeFlag.ReadWrite):
                saved_state = json.loads(file.readAll().data().decode("utf-8") or "{}")
            
                file.resize(0)

                # save the new state
                saved_state.setdefault(state_name, {})["skip"] = skip

                file.write(json.dumps(saved_state, indent=4).encode("utf-8"))
                file.close()
            else:
                print(f"Failed to save state {state_name} to {self.saved_state_file}.")

            self.lock.unlock()
        else:
            print("Could not acquire lock within 10 seconds. Could not save state.")

            error = lock.error()
            if error == QLockFile.LockFailedError:
                print("File is currently locked by another process/thread.")
            elif error == QLockFile.PermissionError:
                print("Permission denied.")

            return None

        return skip

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
                if (ord(c) < 32 or ord(c) > 122):
                    newval = newval + f'&#x{ord(c):04X};'
                elif (ord(c) <= 255):
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
                if (ord(c) < 32 or ord(c) > 122):
                    newval = newval + f'&#x{ord(c):04X};'
                elif (ord(c) <= 255):
                    newval = newval + c

            return(newval)

    def strip_html_body(self, html):
        # Remove everything between <header> and </header>, including the tags
        html = re.sub(r'<header>.*?</header>', '', html, flags=re.DOTALL)
        
        # Remove <html> and </html> tags, but keep their content
        html = re.sub(r'</?html>', '', html)
        
        return html.strip()

    def valid_filename(self, val):
        if val is None:
            return(None)
        else:
            val = re.sub(r"[`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[]", "_", val)
            return(val)

    def find_node(self, node, type, attribute, name):
        children = node.firstChild()

        while not children.isNull():
            if ( children.nodeName() == type and children.toElement().attribute(attribute) == name ):
                return(children)

            subsearch = self.find_node(children, type, attribute, name)
            if not subsearch.isNull():
                return(subsearch)

            children = children.nextSibling()  

        return(children)

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

        if table.isNull():
            print("did not find any project locations")
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

    def exec_program(self, fullpath):
        #os.startfile( fullpath ) # i think this will work on Linux
        
        if platform.system() == 'Windows':
            subprocess.Popen([fullpath], creationflags=subprocess.CREATE_NEW_CONSOLE)
        else:
            subprocess.Popen([fullpath])


    def get_plugin_setting(self, settingname, pluginname = None):
        cfg = QSettings("ProjectNotes4Beta","PluginSettings")
        cfg.setFallbacksEnabled(False)
        value = ""

        if pluginname is None:
            value = cfg.value("Global Settings/" + settingname, "")
        else:
            value = cfg.value(pluginname + "/" + settingname, "")

        return value

    def set_plugin_setting(self, settingname, pluginname = None, value = ""):
        cfg = QSettings("ProjectNotes4Beta","PluginSettings")
        cfg.setFallbacksEnabled(False)

        if pluginname is None:
            cfg.setValue("Global Settings/" + settingname, value)
        else:
            cfg.setValue(pluginname + "/" + settingname, value)

    def scrape_project_name(self, xmldoc):
        projectname = ""
        contents = None
        lookupvalue = None

        col = self.find_node(xmldoc, "column", "name", "project_name")
        if not col.isNull():
            lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
            contents = col.toElement().text()

            if lookupvalue != "" and lookupvalue is not None:
                projectname = lookupvalue
            else:
                projectname = contents

        if projectname == "" or projectname is None:
            col = self.find_node(xmldoc, "column", "name", "project_id_name")
            if not col.isNull():
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
        if not col.isNull():
            lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
            contents = col.toElement().text()

            if lookupvalue != "" and lookupvalue is not None:
                projectnumber = lookupvalue
            else:
                projectnumber = contents

        if projectnumber == "" or projectnumber is None:
            col = self.find_node(xmldoc, "column", "name", "project_id")
            if not col.isNull():
                lookupvalue = col.attributes().namedItem("lookupvalue").nodeValue()
                contents = col.toElement().text()

            if lookupvalue != "" and lookupvalue != None:
                projectnumber = lookupvalue
            else:
                projectnumber = contents

        return(projectnumber)

    def folder_exists(self, filename):
        qfi = QFileInfo(filename)
        ap = qfi.absolutePath()

        return QDir(ap).exists()

    def replace_variables(self, source_string, node, table_name = None, row_number = None):
        expanded_string = source_string
        table_string = None
        row = None

        if node.isElement():
            element = node.toElement()
            tag_name = element.tagName()

            #print(f"parsinng {tag_name}")

            #check for key attributes in the root tag
            if tag_name == "projectnotes":
                if element.hasAttribute("managing_company_name"):
                    name_value = element.attribute("managing_company_name")
                    expanded_string = expanded_string.replace("[$managing_company_name]", name_value)   

                if element.hasAttribute("managing_manager_name"):
                    name_value = element.attribute("managing_manager_name")
                    expanded_string = expanded_string.replace("[$managing_manager_name]", name_value)  

                if element.hasAttribute("project_manager_id"):
                    name_value = element.attribute("project_manager_id")
                    expanded_string = expanded_string.replace("[$project_manager_id]", name_value)    

            elif tag_name == "column": # and table_name is not None:
                if element.hasAttribute("name"):
                    name_value = element.attribute("name")
                    element_text = None

                    if element.hasAttribute("lookupvalue"):
                        element_text = element.attribute("lookupvalue")
                    else:
                        element_text = element.text().strip()

                    #print(f"attempting replace [${table_name}.{name_value}.{row_number}] ")
                    expanded_string = expanded_string.replace(f"[${table_name}.{name_value}.{row_number}]", element_text)

                return expanded_string

            elif tag_name == "table":
                # child elements are going to be columns or another table
                if element.hasAttribute("name"):
                    table_string = element.attribute("name")
                    row = 0  # we need to start couting rows

            elif tag_name == "row":
                    table_string = table_name # pass the current table down to the column
                    row = row_number  # pass the current row down to the column leve

        child = node.firstChild()

        while not child.isNull():
            if child.isElement():
                if child.toElement().tagName() == "row" and table_string is not None:
                    row += 1

            expanded_string = self.replace_variables(expanded_string, child, table_string, row)
            child = child.nextSibling()


        return expanded_string

# setup test data
"""
import sys
from PyQt6.QtCore import QFile, QIODevice, QDate, QUrl, QDir
print("Buld up QDomDocument")

app = QApplication(sys.argv)
xmldoc = QDomDocument("TestDocument")
f = QFile("/home/paulmckinney/Desktop/project.xml")

if f.open(QIODevice.OpenModeFlag.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
pnc = ProjectNotesCommon()
xmlroot = xmldoc.elementsByTagName("projectnotes").at(0)

projectfolder = pnc.get_projectfolder(xmlroot)
print("project folder: " + projectfolder)

original = "look for [$project_notes.project_id.1] to show"
print("replaced " + pnc.replace_variables(original, xmlroot))

original = "look for [$managing_company_name] to show"
print("replaced " + pnc.replace_variables(original, xmlroot))


f.close()

"""

# TODO: when closing project notes sometimes background tasks keep it open
