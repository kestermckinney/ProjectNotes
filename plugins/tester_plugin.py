import sys
import os
import platform
import projectnotes  
import threading
import time
import importlib

#from includes.common import ProjectNotesCommon
from PyQt6 import QtGui, QtCore, QtWidgets, uic

from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QThread
#from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices  
from PyQt6.QtWidgets import (QApplication, QDialog, QListWidget, QPushButton, 
                             QLineEdit, QVBoxLayout, QHBoxLayout, QFileDialog, QMessageBox)

import exportnotes_plugin

# Project Notes Plugin Parameters
pluginname = "Testing Plugin" # name used in the menu
plugindescription = "This is test plugin. Supported platforms: Windows, Linux, MacOS"

# define menus to be added to ProjectNotes Plugins menu and data export/import right click.
# the dictionary key is the menu name
# the function value is the the python function to be called
# the table filter filters the XML sent to the plugin to make the export more efficient
# the menu can be placed under a submenu
# the function wil only show on the right click if it matches the table specified in dataexport

# pluginmenus = [
#     {"menutitle" : "Menu 2", "function" : "event_menuclick", "tablefilter" : "", "submenu" : "Test Submenu B", "dataexport" : "", "parameter" : "look at me"},
#     {"menutitle" : "Menu 1", "function" : "event_menuclick", "tablefilter" : "", "submenu" : "Test Submenu B", "dataexport" : ""},
#     {"menutitle" : "Menu 2", "function" : "event_menuclick", "tablefilter" : "", "submenu" : "Test Submenu A", "dataexport" : ""},
#     {"menutitle" : "Force Reload Of tester_plugin", "function" : "force_reload", "tablefilter" : "", "submenu" : "Test Submenu A", "dataexport" : ""},
#     {"menutitle" : "Alpha Menu B" , "function" : "event_menuclick", "tablefilter" : "", "submenu" : "", "dataexport" : ""},
#     {"menutitle" : "Alpha Menu C", "function" : "event_menuclick", "tablefilter" : "", "submenu" : "", "dataexport" : ""},
#     {"menutitle" : "Export Project", "function" : "event_data_rightclick", "tablefilter" : "", "submenu" : "Test Submenu", "dataexport" : "projects", "parameter" : "look at me projects"},
#     {"menutitle" : "Beta Menu A", "function" : "event_menuclick", "tablefilter" : "", "submenu" : "", "dataexport" : ""},
#     {"menutitle" : "Gamma Menu A", "function" : "event_menuclick", "tablefilter" : "", "submenu" : "", "dataexport" : ""},
#     {"menutitle" : "Alpha Menu A", "function" : "event_menuclick", "tablefilter" : "", "submenu" : "", "dataexport" : ""},
#     {"menutitle" : "Client Update Test", "function" : "event_data_rightclick", "tablefilter" : "", "submenu" : "", "dataexport" : "clients", "parameter" : "look at me clients"},
# ]

#pluginmenus.append({"menutitle" :"Test Menu C", "function" : "menu_click",  "tablefilter" : "", "submenu" : "test submenu", "dataexport" : "", "parameter" : "parm" })  

# events must have a data structure and data view specified
#
# Structures:
#      string          The event will pass a python string when dataexport is defined containing XML. 
#                      The plugin can return an XML string to be processed by ProjectNotes.
#
# Data Views:
#      clients
#      people
#      projects
#      project_people
#      status_report_items
#      project_locations
#      project_notes
#      meeting_attendees
#      item_tracker_updates
#      item_tracker

# Supported Events

# def event_menuclick(xmlstr):
#     return

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# WARNING: The next line below can cause Project Notes to hang only use it for testing
#pnc = ProjectNotesCommon()

# Project Notes Plugin Events

def get_global_setting(settingname):
    cfg = QSettings("ProjectNotes4Beta","PluginSettings")
    cfg.setFallbacksEnabled(False)

    val = cfg.value("Global Settings/" + settingname, "")

    return val

def get_setting(settingname):
    cfg = QSettings("ProjectNotes4Beta","PluginSettings")
    cfg.setFallbacksEnabled(False)

    val = cfg.value(pluginname + "/" + settingname, "")

    return val

def force_reload(parameter):
    print("going to try to restart")

    projectnotes.force_reload("tester_plugin")

    return ""

def event_menuclick(parameter):

    print("Tester: Event Menu click called...")
    print("parameter passed: ", parameter)

    contact = """<?xml version="1.0" encoding="UTF-8"?>
    <projectnotes>
    <table name="people" filter_field_1="name" filter_value_1="C%" top="2" skip="1" />
    <table name="clients" filter_field_1="name" filter_value_1="C%" top="5" />
    </projectnotes>
    """

    print(projectnotes.get_data(contact))

    QMessageBox.information(None, "Test Plugin", "Menu click called.", QMessageBox.StandardButton.Ok)

    return ""

def event_data_rightclick(xmlstr, parameter): 
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return

    print("Tester: Right Click Data Event called...")
    print("parameter passed: ", parameter)
    print(xmlstr)

    QMessageBox.information(None, "Data Right-Click Event", "Returning XML", QMessageBox.StandardButton.Ok)


    retval = """<projectnotes>
         <table name="projects">
          <row id="16714573320006157">   
           <column name="project_id">16714573320006157</column>
            <column name="project_name">PaperWorks Boiler Combustion Configuration XXX</column>
          </row>
         </table>
        </projectnotes>"""

    projectnotes.update_data(retval)


    # simple test will always change the name back
    retval = """<projectnotes>
         <table name="clients">
          <row id="161357299500029810">   
           <column name="client_id">161357299500029810</column>
           <column name="client_name">E-Cubed """

    retval = retval + "X"

    retval = retval + """</column>
          </row>
         </table>
        </projectnotes>"""


    return retval  

# Dictionary mapping test names to function names
TEST_FUNCTIONS = {
    "Export Meeting Notes": { "module": "exportnotes_plugin", "function": "menuExportMeetingNotes", "parameter": "" },
}

# Sample test functions (replace with actual implementations)
def validate_process_parameters(xml_content):
    """Sample function to process XML content for process parameter validation using QDomDocument."""
    try:
        doc = QDomDocument()
        if not doc.setContent(xml_content):
            return "Error: Invalid XML content."
        root = doc.documentElement()
        return f"Processing XML for Validate Process Parameters: Root tag = {root.tagName()}"
        # Add actual processing logic here (e.g., validate parameters for pharma process)
    except Exception as e:
        return f"Error processing XML: {str(e)}"

def analyze_batch_data(xml_content):
    """Sample function to analyze batch data from XML using QDomDocument."""
    try:
        doc = QDomDocument()
        if not doc.setContent(xml_content):
            return "Error: Invalid XML content."
        root = doc.documentElement()
        return f"Analyzing batch data from XML: Root tag = {root.tagName()}"
        # Add actual analysis logic here (e.g., batch quality metrics)
    except Exception as e:
        return f"Error processing XML: {str(e)}"

def simulate_digital_twin(xml_content):
    """Sample function to simulate digital twin from XML using QDomDocument."""
    try:
        doc = QDomDocument()
        if not doc.setContent(xml_content):
            return "Error: Invalid XML content."
        root = doc.documentElement()
        return f"Simulating digital twin with XML: Root tag = {root.tagName()}"
        # Add actual digital twin simulation logic here (e.g., process simulation)
    except Exception as e:
        return f"Error processing XML: {str(e)}"

class TestDialog(QDialog):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Test Execution Dialog")
        self.setMinimumWidth(400)
        self.init_ui()

    def init_ui(self):
        # Main layout
        layout = QVBoxLayout()

        # List widget for test names
        self.test_list = QListWidget()
        self.test_list.addItems(TEST_FUNCTIONS.keys())
        layout.addWidget(self.test_list)

        # File selection button and text box
        file_layout = QHBoxLayout()
        self.file_input = QLineEdit()
        self.file_input.setReadOnly(True)
        self.file_input.setPlaceholderText("Select an XML file...")
        file_layout.addWidget(self.file_input)

        self.select_file_btn = QPushButton("Select XML File")
        self.select_file_btn.clicked.connect(self.select_file)
        file_layout.addWidget(self.select_file_btn)
        layout.addLayout(file_layout)

        # Execute test button
        self.execute_btn = QPushButton("Execute Test")
        self.execute_btn.clicked.connect(self.execute_test)
        layout.addWidget(self.execute_btn)

        self.setLayout(layout)

    def select_file(self):
        """Open file dialog to select an XML file."""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Select XML File", "", "XML Files (*.xml);;All Files (*)"
        )
        if file_path:
            self.file_input.setText(file_path)

    def execute_test(self):
        """Execute the selected test with the XML file contents."""
        selected_items = self.test_list.selectedItems()
        file_path = self.file_input.text()

        # Validate selections
        if not selected_items:
            QMessageBox.warning(self, "Error", "Please select a test from the list.")
            return
        if not file_path:
            QMessageBox.warning(self, "Error", "Please select an XML file.")
            return

        test_name = selected_items[0].text()
        test_dict = TEST_FUNCTIONS.get(test_name)
        function_name = test_dict["function"]
        module_name =  test_dict["module"]
        parameter_value = test_dict["parameter"]

        print(f"Calling function {module_name}.{function_name} with parameter: {parameter_value}")

        #try:
        # Read XML file contents
        with open(file_path, 'r', encoding='utf-8') as file:
            xml_content = file.read()

        # Call the corresponding function
        #func = globals()[function_name]
        module = importlib.import_module(module_name)
        func = getattr(module, function_name)
        result = func(xml_content, parameter_value)

        # Show result
        QMessageBox.information(self, "Test Result", result)
        # except FileNotFoundError:
        #     QMessageBox.critical(self, "Error", "Selected XML file could not be found.")
        # except KeyError:
        #     QMessageBox.critical(self, "Error", f"No function defined for test: {test_name}")
        # except Exception as e:
        #     QMessageBox.critical(self, "Error", f"Error executing test: {str(e)}")

if __name__ == '__main__':
    app = QApplication(sys.argv)

    os.chdir("..")

    dialog = TestDialog()
    dialog.show()
    sys.exit(app.exec())
