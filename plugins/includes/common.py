import platform
import os
import json

# todo: make compatible
# if (platform.system() == 'Windows'):
#     from win32com.client import GetObject
#     import win32com
#     import win32api
#     import win32gui

top_windows = []

#todo: make compatible
# def windowEnumerationHandler(hwnd, tpwindows):
#     if (platform.system() == 'Windows'):
#         tpwindows.append((hwnd, win32gui.GetWindowText(hwnd)))


from PyQt6 import QtGui, QtCore, QtWidgets
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QElapsedTimer, QStandardPaths, QDir, QJsonDocument, QSettings
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication

import re
import subprocess

class ProjectNotesCommon:
    def __init__(self):
        self.temporary_folder = QStandardPaths.writableLocation(QStandardPaths.StandardLocation.TempLocation)
        self.saved_state_file = self.temporary_folder + '/saved_state.json'

    def get_save_state(self, state_name):
        # get the last state
        skip = 0
        file = QFile(self.saved_state_file)
        if file.exists():
            if file.open(QIODevice.OpenModeFlag.ReadOnly):
                saved_state = json.loads(file.readAll().data().decode("utf-8"))
                file.close()

            skip = saved_state.get(state_name, {}).get("skip", 0)
        else:
            saved_state = json.loads("{}")
            print(f"Failed to load previous state {state_name}.")

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
        saved_state = json.loads("{}")
        skip = oldskip

        #start over the full count was not returned
        if nodecount < top: 
            skip = 0
        else:
            skip = skip + top

        # save the new state
        saved_state.setdefault(state_name, {})["skip"] = skip

        file = QFile(self.saved_state_file)
        if file.open(QIODevice.OpenModeFlag.WriteOnly):
            file.write(json.dumps(saved_state, indent=4).encode("utf-8"))
            file.close()    

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
        #todo: make compatible
        # if (platform.system() == 'Windows'):
        #     win32api.WinExec( fullpath )
        # else:
        os.system( fullpath ) # i think this will work on Linux


    def get_plugin_setting(self, settingname, pluginname = None):
        cfg = QSettings("ProjectNotes","PluginSettings")
        cfg.setFallbacksEnabled(False)
        value = ""

        if pluginname is None:
            value = cfg.value("Global Settings/" + settingname, "")
        else:
            value = cfg.value(pluginname + "/" + settingname, "")

        return value

    def set_plugin_setting(self, settingname, pluginname = None, value = ""):
        cfg = QSettings("ProjectNotes","PluginSettings")
        cfg.setFallbacksEnabled(False)

        if pluginname is None:
            cfg.setValue("Global Settings/" + settingname, value)
        else:
            cfg.setValue(pluginname + "/" + settingname, value)

    def verify_global_settings(self):

        pf = self.get_plugin_setting("ProjectsFolder")
        if pf == None or pf == "" or not QDir(pf).exists():
            QMessageBox.warning(None, "Invalid Global Setting", "ProjectsFolder must be set in the Global Settigns plugin.", QMessageBox.StandardButton.Ok)
            return(False)

        return(True)

    #todo: make compatible
    # def bring_window_to_front(self, title):
    #     if (platform.system() != 'Windows'):
    #         print("bring window to front requires win32gui not supported on this platform")
    #         return

    #     #QtWidgets.QApplication.processEvents()
    #     print("looking for window title " + title)
    #     win32gui.EnumWindows(windowEnumerationHandler, top_windows)

    #     for i in top_windows:
    #         if title.lower() in i[1].lower():
    #             print("found " +  i[1])
    #             win32gui.ShowWindow(i[0],5)
    #             win32gui.SetForegroundWindow(i[0])
    #             break
    #     return

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
  
    # make compatible
    # def email_word_file_as_html(self, subject, recipients, attachment, wordfile):
    #     if (platform.system() != 'Windows'):
    #         print("email_word_file_as_html only supported on Windows")
    #         return

    #     if wordfile is not None:
    #         word = win32com.client.Dispatch("Word.Application")
    #         word.Visible = 0
    #         doc = word.Documents.Open(wordfile)
    #         doc.SpellingChecked = False
    #         word.CheckLanguage = False
    #         doc.GrammarChecked = False
    #         word.AutoCorrect.CorrectCapsLock = False
    #         word.AutoCorrect.CorrectDays = False
    #         word.AutoCorrect.CorrectHangulAndAlphabet = False
    #         word.AutoCorrect.CorrectInitialCaps = False
    #         word.AutoCorrect.CorrectKeyboardSetting = False
    #         word.AutoCorrect.CorrectSentenceCaps = False

    #         doc.SaveAs(wordfile + ".html", 8)
    #         word.Quit()
    #         word = None

    #         file = open(wordfile + ".html", "r")
    #         html = file.read()
    #         file.close()
    #         QFile.remove(wordfile + ".html")
    #         dir = QDir(wordfile + "_files")
    #         dir.removeRecursively()

    #     outlook = win32com.client.Dispatch("Outlook.Application")
    #     message = outlook.CreateItem(0)
    #     message.To = ""

    #     outlook.ActiveExplorer().Activate()
    #     message.Display()

    #     message.To = recipients
    #     DefaultSignature = message.HTMLBody

    #     message.Subject = subject

    #     if wordfile is not None:
    #         message.HTMLBody = html + DefaultSignature

    #     if attachment is not None:
    #         message.Attachments.Add(attachment, 1)

    #     self.bring_window_to_front(subject)

    #     outlook = None
    #     message = None

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