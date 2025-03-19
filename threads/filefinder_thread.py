import sys
import platform
import json
import projectnotes
import time
import re
import os 
import inspect

# make sure includes folder can be found
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../plugins')))

from includes.common import ProjectNotesCommon
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QDateTime, QElapsedTimer, QDir, QDirIterator, QFileInfo

# Project Notes Plugin Parameters
pluginname = "File Finder Thread" # name used in the menu
plugindescription = "This is test thread. Supported platforms: Windows, Linux, MacOS"
plugintimerevent = 1 # how many minutes between the timer event

pluginmenus = [
    {"menutitle" : "Find All Fiiles", "function" : "event_timer", "tablefilter" : "", "submenu" : "Utilities", "dataexport" : "", "parameter" : "all"},
    {"menutitle" : "Find Project Files", "function" : "event_data_rightclick", "tablefilter" : "projects", "submenu" : "Utilities", "dataexport" : "projects", "parameter" : ""}
]

# all events return an xml string that can be processed by ProjectNotes
#
# Supported Events

# def event_startup(parameter):
#     return
#
# def event_shutdown(parameter):
#     return
#
# def event_timer(parameter):
#     return
#

class  FileFinder:
    def __init__(self):
        super().__init__()

        self.pnc = ProjectNotesCommon()
        self.settings_pluginname = "File Finder"
        self.search_locations = self.pnc.get_plugin_setting("SearchLocations", self.settings_pluginname)
        self.classifications = self.pnc.get_plugin_setting("Classifications", self.settings_pluginname)

    # using the specified filters match file types and include them in teh Artifacts list
    def filter_match_files(self, project_xml, parent_folder, basefolder):
        xmldoc = ""

        projectnumber = self.pnc.get_column_value(project_xml, "project_number")
        re_pattern = re.compile(r"(?:[^a-zA-Z0-9]|^)" + re.escape(projectnumber) + r"(?:[^a-zA-Z0-9]|^)", re.IGNORECASE)

        if QDir(parent_folder).exists():

            it = QDirIterator(parent_folder, QDir.Filter.Files | QDir.Filter.Dirs | QDir.Filter.NoDotAndDotDot, QDirIterator.IteratorFlag.FollowSymlinks)

            #print(f"working on project {projectnumber} in {parent_folder}")

            while it.hasNext():
                it.next()

                #print(f"iterrator found {it.filePath()} basefolder is {basefolder} looking for project {projectnumber}")

                if basefolder:
                    # determine if this is the base project folddr
                    if it.fileInfo().isDir() and re_pattern.match(it.fileName()):
                        xmldoc += "<row>\n"
                        xmldoc += f"<column name=\"project_id\" lookupvalue=\"{self.pnc.to_xml(projectnumber)}\"></column>\n"
                        xmldoc += "<column name=\"location_type\">File Folder</column>\n"
                        xmldoc += "<column name=\"location_description\">Project Folder</column>\n"
                        xmldoc += f"<column name=\"full_path\">{self.pnc.to_xml(it.filePath())}</column>\n"
                        xmldoc += "</row>\n"

                        xmldoc += self.filter_match_files(project_xml, it.filePath(), False)

                        #print(f"identified project folder {it.filePath()}")
                else:
                    if it.fileInfo().isDir():
                        #print(f"traversing folder {it.filePath()}")
                        xmldoc += self.filter_match_files(project_xml, it.filePath(), False)

                    data = json.loads(self.classifications)

                    #print(f"found {Len(data)} classifications")

                    if (len(data) > 0):
                        for row, row_data in enumerate(data): 
                            pattern = row_data["Pattern Match"]

                            pattern = self.pnc.replace_variables(pattern, project_xml, "projects", 1) 

                            #print(f"variable replaced pattern {pattern}")

                            # test if we have a pattern match for  the file     
                            if re.search(pattern, it.filePath(), re.IGNORECASE) is not None:
                                locationtype = "Generic File (System Identified)"
                                if it.fileInfo().isDir():
                                    locationtype = "File Folder"
                                else:
                                    if it.filePath().lower().endswith(".xlsx"):
                                        locationtype = "Excel Document"
                                    elif it.filePath().lower().endswith(".xls"):
                                        locationtype = "Excel Document"                                        
                                    elif it.filePath().lower().endswith(".docx"):
                                        locationtype = "Word Document"          
                                    elif it.filePath().lower().endswith(".doc"):
                                        locationtype = "Word Document"          
                                    elif it.filePath().lower().endswith(".pdf"):
                                        locationtype = "PDF File"          
                                    elif it.filePath().lower().endswith(".mpp"):
                                        locationtype = "Microsoft Project"          
                                    elif it.filePath().lower().endswith(".pptx"):
                                        locationtype = "PowerPoint Document"
                                    elif it.filePath().lower().startswith("http"):
                                        locationtype = "Web Link"

                                xmldoc += "<row>\n"
                                xmldoc += f"<column name=\"project_id\" lookupvalue=\"{self.pnc.to_xml(projectnumber)}\"></column>\n"
                                xmldoc += f"<column name=\"location_type\">{locationtype}</column>\n"
                                xmldoc += f"<column name=\"location_description\">{row_data["Classification"]} : {self.pnc.to_xml(it.fileInfo().fileName())}</column>\n"
                                xmldoc += f"<column name=\"full_path\" number=\"4\">{self.pnc.to_xml(it.filePath())}</column>\n"
                                xmldoc += "</row>\n"

                                #print(f"found file {projectnumber} {it.filePath()}")

        return xmldoc

    # Project Notes Plugin Events
    def find_project_folders(self, project_xml):
        xmldoc = ""

        
        projectnumber = self.pnc.get_column_value(project_xml, "project_number")
        #print(f"focusing on project {projectnumber}")

        # look through base folders and identify folders that are associated with specific project numbers
        data = json.loads(self.search_locations)

        if (len(data) > 0):
            for row, row_data in enumerate(data): 
                folder = row_data["Location"]

                #print(f"starting search in folder {folder}")

                xmldoc += self.filter_match_files(project_xml, folder, True)

        return xmldoc

    def parse_by_project(self, all_projects = False, projectnumber = None):
        timer = QElapsedTimer()
        timer.start()
             
        # nothing to do if no settings 
        if (self.classifications is None or self.classifications == "" or self.search_locations is None or self.search_locations == ""):
            return ""

        statename = "file_finder"

        skip = 0
        top = 100

        skip = self.pnc.get_save_state(statename)

        if all_projects:
            skip = 0
            top = 10000

        xmldoc = ""
        data_request = None

        # store progress, don't go through all projects at once

        if projectnumber is not None:
            data_request = f'<?xml version="1.0" encoding="UTF-8"?><projectnotes><table name="projects" filter_field_1="projectnumber" filter_value_1="{projectnumber}"/></projectnotes>'
        else:
            data_request = f'<?xml version="1.0" encoding="UTF-8"?><projectnotes><table name="projects" {self.pnc.state_range_attrib(top, skip)} filter_field_1="project_status" filter_value_1="Active"/></projectnotes>'

        #print(f"making data request {data_request}")

        xmlresult = projectnotes.get_data(data_request)

        #print(f"data results {xmlresult}")

        # loop through each project        
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlresult) == False):
            print("Unable to parse XML send from Project Notes in File Finder.")
            return ""

        xmlroot = xmlval.documentElement()
        
        childnode = xmlroot.firstChild()
        
        nodecount = 0

        while not childnode.isNull():

            #print(f"found tag {childnode.toElement().tagName()}")

            if childnode.attributes().namedItem("name").nodeValue() == "projects":
                #print(f"found table {childnode.attributes().namedItem("name").nodeValue()}")
                rownode = childnode.firstChild()
                #print(f"found tag {rownode.toElement().tagName()}") 

                while not rownode.isNull():
                    nodecount += 1

                    #print(f"processing node {nodecount}")

                    xmldoc += self.find_project_folders(rownode)

                    rownode = rownode.nextSibling()

            childnode = childnode.nextSibling()

        self.pnc.set_save_state(statename, skip, top, nodecount)

        if xmldoc != "":
            xmldoc = f'<?xml version="1.0" encoding="UTF-8"?>\n<projectnotes>\n<table name="project_locations">{xmldoc}</table>\n</projectnotes>\n'

        #print(xmldoc)

        projectnotes.update_data(xmldoc)

        #print(xmldoc)

        execution_time = timer.elapsed() / 1000  # Convert milliseconds to seconds
        print(f"Function '{inspect.currentframe().f_code.co_name}' executed in {execution_time:.4f} seconds.")

    def get_projectnumber(self, xmlstr):
 
        xmlval = QDomDocument()
        if (xmlval.setContent(xmlstr) == False):
            print("Unable to parse XML send from Project Notes in File Finder.")
            return None

        return self.pnc.scrape_project_number(xmlval)


def event_timer(parameter):
    ff = FileFinder()
    ff.parse_by_project((parameter == "all"))

    return ""

def event_data_rightclick(xmlstr, parameter):
    ff = FileFinder()

    projectnumber = ff.get_projectnumber(xmlstr)

    if projectnumber is not None:
        ff.parse_by_project(False, projectnumber)

    return ""

#todo: setup the  plugin to do default searching
