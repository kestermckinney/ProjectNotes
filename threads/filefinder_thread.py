import sys
import platform
import projectnotes
import time
import re

from PyQt6.QtSql import QSqlDatabase
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QThread
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "File Finder Thread" # name used in the menu
plugindescription = "This is test thread. Supported platforms: Windows, Linux, MacOS"
plugintimerevent = 1 # how many minutes between the timer event

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
        def filter_match_files(project_xml, parent_folder):
            xmldoc = ""

            project_id = pnc.scrape_project_number(project_xml)

            if QDir(parent_folder).exists():
                it = QDirIterator(parent_folder, {"*"})

                while it.hasNext():
                    it.next()

                    if it.fileInfo().isDir():
                        xmldoc += filter_match_files(project_xml, it.filePath())
                    
                    # loop through all of the patterns to identify a match
                    if (self.classifications is None or self.classifications == ""):
                        return xmldoc

                    data = json.loads(self.classifications)

                    if (len(data) > 0):
                        for row, row_data in enumerate(data): 
                            pattern = row_data["Pattern Match"]
                            pattern = self.pnc.replace_variables(pattern, project_xml)  # where do I get the project info?

                            # test if we have a pattern match for  the file
                            if re.search(pattern, it.filePath()):
                                locationtype = "Generic File (System Identified)"

                                if it.fileInfo().isDir():
                                    locationtype = "File Folder"
                                else:
                                    if it.filePath().lower().endwith(".xlsx"):
                                        locationtype = "Excel Document"
                                    else if it.filePath().lower().endwith(".xls"):
                                        locationtype = "Excel Document"                                        
                                    else if it.filePath().lower().endwith(".docx"):
                                        locationtype = "Word Document"          
                                    else if it.filePath().lower().endwith(".doc"):
                                        locationtype = "Word Document"          
                                    else if it.filePath().lower().endwith(".pdf"):
                                        locationtype = "PDF"          
                                    else if it.filePath().lower().endwith(".mpp"):
                                        locationtype = "Microsoft Project"          
                                    else if it.filePath().lower().endwith(".pptx"):
                                        locationtype = "PowerPoint Document"
                                    else if it.filePath().lower().startswith("http"):
                                        locationtype = "Web Link"

                                xmldoc += "<row>\n"
                                xmldoc += "<column name=\"project_id\" number=\"1\" lookupvalue=\"" + pnc.to_xml(project_id) + "\"></column>\n"
                                xmldoc += "<column name=\"location_type\" number=\"2\">"  + locationtype + "/column>\n"
                                xmldoc += "<column name=\"location_description\" number=\"3\">" + row_data["Classification"] + "</column>\n"
                                xmldoc += "<column name=\"full_path\" number=\"4\">" + pnc.to_xml(it.filePath()) + "\\Project Management</column>\n"
                                xmldoc += "</row>\n"

            return xmldoc

        # Project Notes Plugin Events
        def find_project_folders(project_xml);
            xml = ""

            # look through base folders and identify folders that are associated with specific project numbers

            # store progress, don't go through all projects at once
            if (self.search_locations is None or self.search_locations == ""):
                return

            data = json.loads(self.search_locations)

            if (len(data) > 0):
                for row, row_data in enumerate(data): 
                    folder = row_data["Location"]

                    xmldoc += filter_match_files(project_xml, folder)

            return xmldoc

        def parse_by_project():

                contact = """<?xml version="1.0" encoding="UTF-8"?>
    <projectnotes>
    <table name="people" filter_field_1="name" filter_value_1="C%" top="2" skip="1" />
    <table name="clients" filter_field_1="name" filter_value_1="C%" top="5" />
    </projectnotes>
    """

    print(projectnotes.get_data(contact))


def event_timer(parameter):
    for count in range(1, 300):
        time.sleep(0.05)
        print(f"Thread 1 - event minute counter {count}")

    print("Test Thread 1: Timer event called...")

    return ""

#todo: setup the  plugin to do defaullllltt searching