import json

from includes.common import ProjectNotesCommon
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "My Shortcuts"
plugindescription = "My Shortcuts provides configurable URLs that can be called from specified menu locations.  The URLs can contain variables that are populated based upon XML passed to the plugion."

# define menus to be added to ProjectNotes Plugins menu and data export/import right click.
# the menutitle is the menu name
# the function value is the the python function to be called
# the table filter filters the XML sent to the plugin to make the export more efficient
# the menu can be placed under a submenu
# the function wil only show on the right click if it matches the table specified in dataexport
# if a dataexport value exist the menu will not appear on the plugin menu
pluginmenus = [ ] # menu is dynamically created when module loads

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
# this plugin is only supported on windows

def populate_menu_from_json(json_string):
    menu_array = []

    # nothing was saved
    if (json_string is None or json_string == ""):
        return ""

    data = json.loads(json_string)

    if (len(data) > 0):
        # Populate the table with data
        # make sure to filter the xml to the top level.  We doon't want to get the full projeect xml
        for row, row_data in enumerate(data): 
            if "Data Type" in row_data and row_data["Data Type"] != "":
                menu_array.append({"menutitle" : row_data["Menu"], "function" : "event_data_rightclick",  "tablefilter" : row_data["Data Type"], "submenu" : row_data["Submenu"], "dataexport" : row_data["Data Type"], "parameter" : row_data["URL"] })
            else:
                menu_array.append({"menutitle" : row_data["Menu"], "function" : "menu_click",  "tablefilter" : "", "submenu" : row_data["Submenu"], "dataexport" : "", "parameter" : row_data["URL"] })

    return menu_array

def menu_click(parameter):
    return event_data_rightclick("", parameter)

def event_data_rightclick(xmlstr, parameter):

    xmlval = QDomDocument()

    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.")
        return ""

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node        

    url_link = pnc.replace_variables(parameter, xmlroot)

    QDesktopServices.openUrl(QUrl(url_link))

    return ""

def setup_default_shortcuts_settings():
    value = pnc.get_plugin_setting("MyShortcuts", "My Shortcuts")
    if value is None:
        default_shortcuts = json.dumps([
            {"Submenu": "IFS Cloud", "Menu": "Open Time Entry", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/TimeRegistrationManager/TimeHeaderPage;$filter=%28startswith%28CompanyPersonRef%2FInternalDisplayName,'[$people.name.1]'%29%29", "Data Type": "people"},
            {"Submenu": "IFS Cloud", "Menu": "Open Project", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/ProjectDefinition/Form;path=0.1731531774.1886945984.458817809;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29", "Data Type": "projects"},
            {"Submenu": "IFS Cloud", "Menu": "Open Gantt", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/ProjectGantt/Form;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29", "Data Type": "projects"},
            {"Submenu": "IFS Cloud", "Menu": "Invoicing Plan", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/InvoicingPlan/List;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29;usedefaultfilter=false", "Data Type": "projects"},
            {"Submenu": "IFS Cloud", "Menu": "Open PC Distribution", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/CPcDistribution/Form;$filter=%28%20DistributionId%20eq%20'[$projects.project_number.1]'%20%29", "Data Type": "projects"},
            {"Submenu": "IFS Cloud", "Menu": "Open PO Table", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/CCustomerPOs/List;$filter=%28%20ProjectId%20eq%20'[$projects.project_number.1]'%20%29", "Data Type": "projects"},
            {"Submenu": "IFS Cloud", "Menu": "Open Standard Price", "URL": "https://ifs.cornerstonecontrols.com/main/ifsapplications/web/page/StandardSalesPrice/Form;$filter=%28%20ProjectSalesPriceId%20eq%20'[$projects.project_number.1]'%20%29", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Customer Parts Tracking", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FLead+Engineers%2FCustomer+Parts+Tracking&ProjectID=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Invalid Report Codes", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FProjects%2FInvalid+Report+Code+Assignments&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Parts Ordered", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FProjects%2FProject+Parts+Ordered&ProjectID=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Project Report", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FProjects%2FProject+Report&ProjectId=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Project Status", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FLead+Engineers%2FProject+Status+Report&ProjectId=[$projects.project_number.1]&ReportPeriod=30&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Missing Time Entry", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2fProd%2fProjects%2fMissing+Time+Entry&rs:Command=Render&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"},
            {"Submenu": "Reports", "Menu": "Item Tracker", "URL": "http://indvifsbi05/ReportServer/Pages/ReportViewer.aspx?%2FProd%2FLead+Engineers%2FIssues+List&ProjectId=[$projects.project_number.1]&rs:ClearSession=true&rc:Zoom=Page%20Width", "Data Type": "projects"}
        ])
        pnc.set_plugin_setting("MyShortcuts", "My Shortcuts", default_shortcuts)

pnc = ProjectNotesCommon()

setup_default_shortcuts_settings()
menu_data = pnc.get_plugin_setting("MyShortcuts", "My Shortcuts")
pluginmenus = populate_menu_from_json(menu_data)

# setup test data
"""
print("Buld up QDomDocument")

xmldoc = QDomDocument("TestDocument")

f = QFile("exampleproject.xml")

if f.open(QIODevice.OpenModeFlag.ReadOnly):
    print("example project opened")
    xmldoc.setContent(f)
    f.close()

event_data_rightclick(xmldoc.toString())
"""



