from includes.common import ProjectNotesCommon
from includes.excel_tools import ProjectNotesExcelTools

from PySide6 import QtSql, QtGui, QtCore, QtUiTools
from PySide6.QtSql import QSqlDatabase
from PySide6.QtXml import QDomDocument, QDomNode
from PySide6.QtCore import QFile, QIODevice, QDateTime, QUrl, QDir, QFileInfo
from PySide6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog, QInputDialog
from PySide6.QtGui import QDesktopServices
import sys
import win32com

# Project Notes Plugin Parameters
pluginname = "Send Meeting Notes"
plugindescription = "Using Outlook sends meeting notes out for the selected meeting."

# events must have a data structure and data view specified
#
# Structures:
#      disabled        The event will not be enabled
#      wxxmldocument   The event will pass a wxLua wx.wxXmlDocument containing the spedified data view and expect the plugin to return a wx.wxXmlDocument
#      string          The event will pass a wxLua string containing XML and will expect the plugin to return an XML string
#      nodata          The event will pass a wxLua None and will expect the plugin to return an XML string
#
# all tables in the database have corresponding import/export data views the views are prefixed by ix_
#
# Data Views:
#      ix_clients
#      ix_people
#      ix_projects
#      ix_project_people
#      ix_status_report_items
#      ix_project_locations
#      ix_project_notes
#      ix_meeting_attendees
#      ix_item_tracker_updates
#      ix_item_tracker

# Active Events
Startup="disabled"
Shutdown="disabled"
EveryMinute="disabled"
Every5Minutes="disabled"
Every10Minutes="disabled"
Every30Minutes="disabled"
PluginMenuClick="disabled"
RightClickProject="disabled"
RightClickPeople="disabled"
RightClickClient="disabled"
RightClickStatusReportItem="disabled"
RightClickLocationItem="disabled"
RightClickTeamMember="disabled"
RightClickMeeting="wxxmldocument:ix_project_notes"
RightClickAttendee="disabled"
RightCickTrackerItem="disabled"

# Parameters specified here will show in the Project Notes plugin settings window
# the global variable name must be specified as a string value to be read by project notes
# Project Notes will set these values before calling any defs

# Project Notes Parameters
parameters = {
}

pnc = ProjectNotesCommon()
pne = ProjectNotesExcelTools()

# processing main def
def main_process( xmlval ):
    outlook = win32com.client.Dispatch("Outlook.Application")
    message = outlook.CreateItem(0)
    message.To = ""

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
    htmlbody = ""

    childnode = xmlroot.firstChild()
    pm = xmlroot.attributes().namedItem("managing_manager_name").nodeValue()
    email = None
    nm = None

    while not childnode.isNull():

        if childnode.attributes().namedItem("name").nodeValue() == "ix_project_notes":
            rownode = childnode.firstChild()

            while not rownode.isNull():
                htmlbody = htmlbody + get_html_header(pnc.get_column_value(rownode, "project_id"),
                pnc.get_column_value(rownode, "project_id_name"),
                pnc.get_column_value(rownode, "note_date"),
                pnc.get_column_value(rownode, "note_title"))

                attendeetable = pnc.find_node(rownode, "table", "name", "ix_meeting_attendees")
                if attendeetable:
                    attendeerow = attendeetable.firstChild()

                    attendeelist = ""
                    while not attendeerow.isNull():
                        if attendeelist != "":
                            attendeelist = attendeelist + ", "

                        attendeelist = attendeelist + pnc.get_column_value(attendeerow, "name")

                        nm = pnc.get_column_value(attendeerow, "name")
                        email = pnc.get_column_value(attendeerow, "email")

                        if nm != pm:
                            if (email != None and email != ""):
                                message.Recipients.Add(email)

                        attendeerow = attendeerow.nextSibling()


                htmlbody = htmlbody + get_html_attendee(attendeelist)
                htmlbody = htmlbody + get_html_notes(pnc.get_column_value(rownode, "note"))
                htmlbody = htmlbody + get_html_trackerheader()

                trackertable = pnc.find_node(rownode, "table", "name", "ix_item_tracker")
                if trackertable:
                    trackerrow = trackertable.firstChild()

                    while not trackerrow.isNull():
                        htmlbody = htmlbody + get_html_trackerrow(
                        pnc.get_column_value(trackerrow, "item_name"),
                        pnc.get_column_value(trackerrow, "assigned_to"),
                        pnc.get_column_value(trackerrow, "status"),
                        pnc.get_column_value(trackerrow, "date_due") )

                        trackerrow = trackerrow.nextSibling()

                htmlbody = htmlbody + get_html_footer()

                message.Display()
                outlook.ActiveExplorer().Activate()

                DefaultSignature = message.HTMLBody

                message.Subject = pnc.get_column_value(rownode, "project_id") + " " + pnc.get_column_value(rownode, "project_id_name") + " - " + pnc.get_column_value(rownode, "note_date") + " " + pnc.get_column_value(rownode, "note_title") + " Notes"
                message.BodyFormat = 2 # olFormatHTML
                message.HTMLBody = htmlbody + DefaultSignature

                rownode = rownode.nextSibling()

        childnode = childnode.nextSibling()

    outlook = None
    message = None

    return xmldoc

# Project Notes Plugin Events
def event_startup(xmlstr):
    return main_process(xmlstr)

def event_shutdown(xmlstr):
    return main_process(xmlstr)

def event_everyminute(xmlstr):
    return main_process(xmlstr)

def event_every5minutes(xmlstr):
    return main_process(xmlstr)

def event_every10minutes(xmlstr):
    return main_process(xmlstr)

def event_every30Mmnutes(xmlstr):
    return main_process(xmlstr)

def event_menuclick(xmlstr):
    return main_process(xmlstr)

def event_projectrightclick(xmlstr):
    return main_process(xmlstr)

def event_peoplerightclick(xmlstr):
    return main_process(xmlstr)

def event_clientrightclick(xmlstr):
    return main_process(xmlstr)

def event_statusreportitemrightclick(xmlstr):
    return main_process(xmlstr)

def event_teammemberrightclick(xmlstr):
    return main_process(xmlstr)

def event_locationitemrightclick(xmlstr):
    return main_process(xmlstr)

def event_meetingrightclick(xmlstr):
    return main_process(xmlstr)

def event_attendeerightclick(xmlstr):
    return main_process(xmlstr)

def event_trackeritemrightclick(xmlstr):
    return main_process(xmlstr)

def get_html_header(projectnumber, projectname, day, title):
    htmldoc ="""
    <html>
    <head>
    <style>
    <!--
    /* Font Definitions */
    @font-face
    {font-family:"Cambria Math";
    panose-1:2 4 5 3 5 4 6 3 2 4;
    mso-font-charset:0;
    mso-generic-font-family:roman;
    mso-font-pitch:variable;
    mso-font-signature:-536869121 1107305727 33554432 0 415 0;}
    @font-face
    {font-family:Calibri;
    panose-1:2 15 5 2 2 2 4 3 2 4;
    mso-font-charset:0;
    mso-generic-font-family:swiss;
    mso-font-pitch:variable;
    mso-font-signature:-469750017 -1073732485 9 0 511 0;}
    /* Style Definitions */
    p.MsoNormal, li.MsoNormal, div.MsoNormal
    {mso-style-unhide:no;
    mso-style-qformat:yes;
    mso-style-parent:"";
    margin:0in;
    mso-pagination:widow-orphan;
    font-size:11.0pt;
    font-family:"Calibri",sans-serif;
    mso-ascii-font-family:Calibri;
    mso-ascii-theme-font:minor-latin;
    mso-fareast-font-family:Calibri;
    mso-fareast-theme-font:minor-latin;
    mso-hansi-font-family:Calibri;
    mso-hansi-theme-font:minor-latin;
    mso-bidi-font-family:"Times New Roman";
    mso-bidi-theme-font:minor-bidi;}
    a:link, span.MsoHyperlink
    {mso-style-noshow:yes;
    mso-style-priority:99;
    color:#0563C1;
    mso-themecolor:hyperlink;
    text-decoration:underline;
    text-underline:single;}
    a:visited, span.MsoHyperlinkFollowed
    {mso-style-noshow:yes;
    mso-style-priority:99;
    color:#954F72;
    mso-themecolor:followedhyperlink;
    text-decoration:underline;
    text-underline:single;}
    span.EmailStyle17
    {mso-style-type:personal-compose;
    mso-style-noshow:yes;
    mso-style-unhide:no;
    mso-ansi-font-size:11.0pt;
    mso-bidi-font-size:11.0pt;
    font-family:"Calibri",sans-serif;
    mso-ascii-font-family:Calibri;
    mso-ascii-theme-font:minor-latin;
    mso-fareast-font-family:Calibri;
    mso-fareast-theme-font:minor-latin;
    mso-hansi-font-family:Calibri;
    mso-hansi-theme-font:minor-latin;
    mso-bidi-font-family:"Times New Roman";
    mso-bidi-theme-font:minor-bidi;
    color:windowtext;}
    .MsoChpDefault
    {mso-style-type:export-only;
    mso-default-props:yes;
    font-family:"Calibri",sans-serif;
    mso-ascii-font-family:Calibri;
    mso-ascii-theme-font:minor-latin;
    mso-fareast-font-family:Calibri;
    mso-fareast-theme-font:minor-latin;
    mso-hansi-font-family:Calibri;
    mso-hansi-theme-font:minor-latin;
    mso-bidi-font-family:"Times New Roman";
    mso-bidi-theme-font:minor-bidi;}
    @page WordSection1
    {size:8.5in 11.0in;
    margin:1.0in 1.0in 1.0in 1.0in;
    mso-header-margin:.5in;
    mso-footer-margin:.5in;
    mso-paper-source:0;}
    div.WordSection1
    {page:WordSection1;}
    -->
    </style>
    <!--[if gte mso 10]>
    <style>
    /* Style Definitions */
    table.MsoNormalTable
    {mso-style-name:"Table Normal";
    mso-tstyle-rowband-size:0;
    mso-tstyle-colband-size:0;
    mso-style-noshow:yes;
    mso-style-priority:99;
    mso-style-parent:"";
    mso-padding-alt:0in 5.4pt 0in 5.4pt;
    mso-para-margin:0in;
    mso-pagination:widow-orphan;
    font-size:11.0pt;
    font-family:"Calibri",sans-serif;
    mso-ascii-font-family:Calibri;
    mso-ascii-theme-font:minor-latin;
    mso-hansi-font-family:Calibri;
    mso-hansi-theme-font:minor-latin;
    mso-bidi-font-family:"Times New Roman";
    mso-bidi-theme-font:minor-bidi;}
    </style>
    <![endif]--><!--[if gte mso 9]><xml>
    <o:shapedefaults v:ext="edit" spidmax="1026"/>
    </xml><![endif]--><!--[if gte mso 9]><xml>
    <o:shapelayout v:ext="edit">
    <o:idmap v:ext="edit" data="1"/>
    </o:shapelayout></xml><![endif]-->
    </head>

    <body lang=EN-US link="#0563C1" vlink="#954F72" style='tab-interval:.5in'>

    <div class=WordSection1>

    <table class=MsoNormalTable border=0 cellspacing=0 cellpadding=0 width=844
    style='width:633.05pt;border-collapse:collapse;mso-yfti-tbllook:1184;
    mso-padding-alt:0in 5.4pt 0in 5.4pt'>
    <tr style='mso-yfti-irow:0;mso-yfti-firstrow:yes;height:20.1pt'>
    <td width=844 nowrap colspan=11 style='width:633.05pt;border:none;border-bottom:
    solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;
    height:20.1pt'>
    <p class=MsoNormal><b><span style='font-size:15.0pt;mso-ascii-font-family:
    Calibri;mso-fareast-font-family:"Times New Roman";mso-hansi-font-family:Calibri;
    mso-bidi-font-family:Calibri;color:#44546A'>"""

    htmldoc = htmldoc + pnc.to_html(projectnumber) + " " + pnc.to_html(projectname) + " - " + pnc.to_html(title) + """<o:p></o:p></span></b></p>
    </td>
    </tr>
    <tr style='mso-yfti-irow:1;height:.25in'>
    <td width=204 valign=top style='width:153.0pt;border:solid gray 1.0pt;
    border-top:none;mso-border-left-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#E7E6E6;
    padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal align=right style='text-align:right'><b><span
    style='mso-ascii-font-family:Calibri;mso-fareast-font-family:"Times New Roman";
    mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;color:black;
    mso-color-alt:windowtext'>Date:</span></b><b><span style='mso-ascii-font-family:
    Calibri;mso-fareast-font-family:"Times New Roman";mso-hansi-font-family:Calibri;
    mso-bidi-font-family:Calibri'><o:p></o:p></span></b></p>
    </td>
    <td width=640 colspan=10 style='width:480.05pt;border-top:none;border-left:
    none;border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-top-alt:
    solid gray .5pt;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#D9E1F2;
    padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>
    """
    htmldoc = htmldoc + pnc.to_html(day) + """
    </span><span
    style='mso-ascii-font-family:Calibri;mso-fareast-font-family:"Times New Roman";
    mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></p>
    </td>
    </tr>
    """
    return htmldoc

def get_html_attendee(name):
    htmldoc = """<tr style='mso-yfti-irow:2;height:.25in'>
    <td width=204 valign=top style='width:153.0pt;border:solid gray 1.0pt;
    border-top:none;mso-border-left-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#E7E6E6;
    padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal align=right style='text-align:right'><b><span
    style='mso-ascii-font-family:Calibri;mso-fareast-font-family:"Times New Roman";
    mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;color:black;
    mso-color-alt:windowtext'>Attendees:</span></b><b><span style='mso-ascii-font-family:
    Calibri;mso-fareast-font-family:"Times New Roman";mso-hansi-font-family:Calibri;
    mso-bidi-font-family:Calibri'><o:p></o:p></span></b></p>
    </td>
    <td width=640 colspan=10 style='width:480.05pt;border-top:none;border-left:
    none;border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-top-alt:
    solid gray .5pt;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#D9E1F2;
    padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>
    """ + pnc.to_html(name) + """</span><span
    style='mso-ascii-font-family:Calibri;mso-fareast-font-family:"Times New Roman";
    mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></p>
    </td>
    </tr>
    """
    return htmldoc

def get_html_notes(notes):
    htmldoc = """<tr style='mso-yfti-irow:3;height:.25in'>
    <td width=844 colspan=11 style='width:633.05pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal align=center style='text-align:center'><b><span
    style='mso-ascii-font-family:Calibri;mso-fareast-font-family:"Times New Roman";
    mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;color:black;
    mso-color-alt:windowtext'>Meeting Notes</span></b><b><span style='mso-ascii-font-family:
    Calibri;mso-fareast-font-family:"Times New Roman";mso-hansi-font-family:Calibri;
    mso-bidi-font-family:Calibri'><o:p></o:p></span></b></p>
    </td>
    </tr>
    <tr style='mso-yfti-irow:4;height:17.25pt'>
    <td width=844 colspan=11 valign=top style='width:633.05pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;height:17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>""" + pnc.to_html(notes) + """</span><span
    style='mso-ascii-font-family:Calibri;mso-fareast-font-family:"Times New Roman";
    mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></p>
    </td>
    </tr>
    """
    return htmldoc

def get_html_trackerheader():
    htmldoc = """<tr style='mso-yfti-irow:5;height:17.25pt'>
    <td width=204 valign=top style='width:153.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=63 valign=top style='width:47.35pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=61 valign=top style='width:45.4pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=58 valign=top style='width:43.7pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=74 valign=top style='width:55.6pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal><span style='mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'>&nbsp;<o:p></o:p></span></p>
    </td>
    </tr>
    <tr style='mso-yfti-irow:6;height:.25in'>
    <td width=844 colspan=11 style='width:633.05pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal align=center style='text-align:center'><b><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>Action Items</span></b><b><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></b></p>
    </td>
    </tr>
    <tr style='mso-yfti-irow:7;height:.25in'>
    <td width=587 colspan=7 style='width:440.35pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal align=center style='text-align:center'><b><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>Item</span></b><b><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></b></p>
    </td>
    <td width=119 colspan=2 style='width:89.1pt;border-top:none;border-left:none;
    border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-top-alt:
    solid gray .5pt;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#E7E6E6;
    padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal align=center style='text-align:center'><b><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>Assigned To</span></b><b><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></b></p>
    </td>
    <td width=64 style='width:48.0pt;border-top:none;border-left:none;border-bottom:
    solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;
    height:.25in'>
    <p class=MsoNormal align=center style='text-align:center'><b><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>Status</span></b><b><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></b></p>
    </td>
    <td width=74 style='width:55.6pt;border-top:none;border-left:none;border-bottom:
    solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;
    height:.25in'>
    <p class=MsoNormal align=center style='text-align:center'><b><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>Due Date</span></b><b><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></b></p>
    </td>
    </tr>
    """

    return htmldoc

def get_html_trackerrow(item, assignedto, status, duedate ):
    """
    bg_color = "#D9E1F2"
    fg_color = "black"

    if priority == "High":
        fg_color = "#9C0006"
        bg_color = "#FFC7CE"
    elif priority == "Low":
        fg_color = "#006100"
        bg_color = "#C6EFCE"
    elif priority == "Medium":
        fg_color = "#9C5700"
        bg_color = "#FFEB9C"
    """

    htmldoc ="""<tr style='mso-yfti-irow:8;mso-yfti-lastrow:yes;height:26.25pt'>
    <td width=587 colspan=7 style='width:440.35pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;height:26.25pt'>
    <p class=MsoNormal><span style='font-size:10.0pt;mso-ascii-font-family:Calibri;
    mso-fareast-font-family:"Times New Roman";mso-hansi-font-family:Calibri;
    mso-bidi-font-family:Calibri;color:black;mso-color-alt:windowtext'>""" + pnc.to_html(item) + """</span><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></p>
    </td>
    <td width=119 colspan=2 style='width:89.1pt;border-top:none;border-left:none;
    border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-top-alt:
    solid gray .5pt;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#D9E1F2;
    padding:0in 5.4pt 0in 5.4pt;height:26.25pt'>
    <p class=MsoNormal align=center style='text-align:center'><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>""" + pnc.to_html(assignedto) + """</span><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></p>
    </td>
    <td width=64 style='width:48.0pt;border-top:none;border-left:none;border-bottom:
    solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;
    height:26.25pt'>
    <p class=MsoNormal align=center style='text-align:center'><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>""" + pnc.to_html(status) + """</span><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></p>
    </td>
    <td width=74 style='width:55.6pt;border-top:none;border-left:none;border-bottom:
    solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;
    height:26.25pt'>
    <p class=MsoNormal align=center style='text-align:center'><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri;
    color:black;mso-color-alt:windowtext'>""" + pnc.to_html(duedate) + """</span><span
    style='font-size:10.0pt;mso-ascii-font-family:Calibri;mso-fareast-font-family:
    "Times New Roman";mso-hansi-font-family:Calibri;mso-bidi-font-family:Calibri'><o:p></o:p></span></p>
    </td>
    </tr>
    """

    return htmldoc


def get_html_footer():
    htmldoc = """</table>
    </body>
    </html>
    """
    return htmldoc

# setup test data
"""
print("Buld up QDomDocument")
app = QApplication(sys.argv)

xmldoc = QDomDocument("TestDocument")
f = QFile("examplemeeting.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

main_process(xmldoc)
"""
