import subprocess
import os

from includes.common import ProjectNotesCommon
from includes.collaboration_tools import CollaborationTools

from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDate, QDir, QTextStream, QStringConverter, QProcess, QMarginsF, QUrl, QTimer
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices, QPageLayout, QPageSize
from PyQt6.QtWebEngineWidgets import QWebEngineView

# try and get more logs
os.environ["QTWEBENGINE_CHROMIUM_FLAGS"] = "--enable-logging=stderr --log-level=0"


# Project Notes Plugin Parameters
pluginname = "Export Meeting Notes"
plugindescription = "Generate meeting notes to be exported.  Notes can be exported as HTML or a PDF."

pluginmenus = [{"menutitle" : "Meeting Notes", "function" : "menuExportMeetingNotes", "tablefilter" : "projects/project_notes/meeting_attendees/item_tracker/project_locations", "submenu" : "Export", "dataexport" : "projects"}]

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

def generateHeader(project_number, project_name):
    html = """
    <html xmlns:v="urn:schemas-microsoft-com:vml"
    xmlns:o="urn:schemas-microsoft-com:office:office"
    xmlns:w="urn:schemas-microsoft-com:office:word"
    xmlns:x="urn:schemas-microsoft-com:office:excel"
    xmlns:m="http://schemas.microsoft.com/office/2004/12/omml"
    xmlns="http://www.w3.org/TR/REC-html40">

    <head>
    <meta http-equiv=Content-Type content="text/html; charset=windows-1252">
    <meta name=ProgId content=Word.Document>
    <meta name=Generator content="Microsoft Word 15">
    <meta name=Originator content="Microsoft Word 15">
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
        mso-font-signature:-469750017 -1040178053 9 0 511 0;}
    @font-face
        {font-family:Aptos;
        mso-font-charset:0;
        mso-generic-font-family:swiss;
        mso-font-pitch:variable;
        mso-font-signature:536871559 3 0 0 415 0;}
     /* Style Definitions */
     p.MsoNormal, li.MsoNormal, div.MsoNormal
        {mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-parent:"";
        margin-top:0in;
        margin-right:0in;
        margin-bottom:8.0pt;
        margin-left:0in;
        line-height:115%;
        mso-pagination:widow-orphan;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:Aptos;
        mso-fareast-theme-font:minor-latin;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:minor-bidi;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;}
    h1
        {mso-style-priority:9;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-link:"Heading 1 Char";
        mso-style-next:Normal;
        margin-top:.25in;
        margin-right:0in;
        margin-bottom:4.0pt;
        margin-left:0in;
        line-height:115%;
        mso-pagination:widow-orphan lines-together;
        page-break-after:avoid;
        mso-outline-level:1;
        font-size:20.0pt;
        font-family:"Aptos Display",sans-serif;
        mso-ascii-font-family:"Aptos Display";
        mso-ascii-theme-font:major-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:"Aptos Display";
        mso-hansi-theme-font:major-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;
        font-weight:normal;}
    h2
        {mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-qformat:yes;
        mso-style-link:"Heading 2 Char";
        mso-style-next:Normal;
        margin-top:8.0pt;
        margin-right:0in;
        margin-bottom:4.0pt;
        margin-left:0in;
        line-height:115%;
        mso-pagination:widow-orphan lines-together;
        page-break-after:avoid;
        mso-outline-level:2;
        font-size:16.0pt;
        font-family:"Aptos Display",sans-serif;
        mso-ascii-font-family:"Aptos Display";
        mso-ascii-theme-font:major-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:"Aptos Display";
        mso-hansi-theme-font:major-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;
        font-weight:normal;}
    h3
        {mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-qformat:yes;
        mso-style-link:"Heading 3 Char";
        mso-style-next:Normal;
        margin-top:8.0pt;
        margin-right:0in;
        margin-bottom:4.0pt;
        margin-left:0in;
        line-height:115%;
        mso-pagination:widow-orphan lines-together;
        page-break-after:avoid;
        mso-outline-level:3;
        font-size:14.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;
        font-weight:normal;}
    h4
        {mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-qformat:yes;
        mso-style-link:"Heading 4 Char";
        mso-style-next:Normal;
        margin-top:4.0pt;
        margin-right:0in;
        margin-bottom:2.0pt;
        margin-left:0in;
        line-height:115%;
        mso-pagination:widow-orphan lines-together;
        page-break-after:avoid;
        mso-outline-level:4;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;
        font-weight:normal;
        font-style:italic;}
    h5
        {mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-qformat:yes;
        mso-style-link:"Heading 5 Char";
        mso-style-next:Normal;
        margin-top:4.0pt;
        margin-right:0in;
        margin-bottom:2.0pt;
        margin-left:0in;
        line-height:115%;
        mso-pagination:widow-orphan lines-together;
        page-break-after:avoid;
        mso-outline-level:5;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;
        font-weight:normal;}
    h6
        {mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-qformat:yes;
        mso-style-link:"Heading 6 Char";
        mso-style-next:Normal;
        margin-top:2.0pt;
        margin-right:0in;
        margin-bottom:0in;
        margin-left:0in;
        line-height:115%;
        mso-pagination:widow-orphan lines-together;
        page-break-after:avoid;
        mso-outline-level:6;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#595959;
        mso-themecolor:text1;
        mso-themetint:166;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;
        font-weight:normal;
        font-style:italic;}
    p.MsoHeading7, li.MsoHeading7, div.MsoHeading7
        {mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-qformat:yes;
        mso-style-link:"Heading 7 Char";
        mso-style-next:Normal;
        margin-top:2.0pt;
        margin-right:0in;
        margin-bottom:0in;
        margin-left:0in;
        line-height:115%;
        mso-pagination:widow-orphan lines-together;
        page-break-after:avoid;
        mso-outline-level:7;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#595959;
        mso-themecolor:text1;
        mso-themetint:166;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoHeading8, li.MsoHeading8, div.MsoHeading8
        {mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-qformat:yes;
        mso-style-link:"Heading 8 Char";
        mso-style-next:Normal;
        margin:0in;
        line-height:115%;
        mso-pagination:widow-orphan lines-together;
        page-break-after:avoid;
        mso-outline-level:8;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#272727;
        mso-themecolor:text1;
        mso-themetint:216;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;
        font-style:italic;}
    p.MsoHeading9, li.MsoHeading9, div.MsoHeading9
        {mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-qformat:yes;
        mso-style-link:"Heading 9 Char";
        mso-style-next:Normal;
        margin:0in;
        line-height:115%;
        mso-pagination:widow-orphan lines-together;
        page-break-after:avoid;
        mso-outline-level:9;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#272727;
        mso-themecolor:text1;
        mso-themetint:216;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoTitle, li.MsoTitle, div.MsoTitle
        {mso-style-priority:10;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-link:"Title Char";
        mso-style-next:Normal;
        margin-top:0in;
        margin-right:0in;
        margin-bottom:4.0pt;
        margin-left:0in;
        mso-add-space:auto;
        mso-pagination:widow-orphan;
        font-size:28.0pt;
        font-family:"Aptos Display",sans-serif;
        mso-ascii-font-family:"Aptos Display";
        mso-ascii-theme-font:major-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:"Aptos Display";
        mso-hansi-theme-font:major-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        letter-spacing:-.5pt;
        mso-font-kerning:14.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoTitleCxSpFirst, li.MsoTitleCxSpFirst, div.MsoTitleCxSpFirst
        {mso-style-priority:10;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-link:"Title Char";
        mso-style-next:Normal;
        mso-style-type:export-only;
        margin:0in;
        mso-add-space:auto;
        mso-pagination:widow-orphan;
        font-size:28.0pt;
        font-family:"Aptos Display",sans-serif;
        mso-ascii-font-family:"Aptos Display";
        mso-ascii-theme-font:major-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:"Aptos Display";
        mso-hansi-theme-font:major-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        letter-spacing:-.5pt;
        mso-font-kerning:14.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoTitleCxSpMiddle, li.MsoTitleCxSpMiddle, div.MsoTitleCxSpMiddle
        {mso-style-priority:10;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-link:"Title Char";
        mso-style-next:Normal;
        mso-style-type:export-only;
        margin:0in;
        mso-add-space:auto;
        mso-pagination:widow-orphan;
        font-size:28.0pt;
        font-family:"Aptos Display",sans-serif;
        mso-ascii-font-family:"Aptos Display";
        mso-ascii-theme-font:major-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:"Aptos Display";
        mso-hansi-theme-font:major-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        letter-spacing:-.5pt;
        mso-font-kerning:14.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoTitleCxSpLast, li.MsoTitleCxSpLast, div.MsoTitleCxSpLast
        {mso-style-priority:10;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-link:"Title Char";
        mso-style-next:Normal;
        mso-style-type:export-only;
        margin-top:0in;
        margin-right:0in;
        margin-bottom:4.0pt;
        margin-left:0in;
        mso-add-space:auto;
        mso-pagination:widow-orphan;
        font-size:28.0pt;
        font-family:"Aptos Display",sans-serif;
        mso-ascii-font-family:"Aptos Display";
        mso-ascii-theme-font:major-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:"Aptos Display";
        mso-hansi-theme-font:major-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        letter-spacing:-.5pt;
        mso-font-kerning:14.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoSubtitle, li.MsoSubtitle, div.MsoSubtitle
        {mso-style-priority:11;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-link:"Subtitle Char";
        mso-style-next:Normal;
        margin-top:0in;
        margin-right:0in;
        margin-bottom:8.0pt;
        margin-left:0in;
        line-height:115%;
        mso-pagination:widow-orphan;
        font-size:14.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#595959;
        mso-themecolor:text1;
        mso-themetint:166;
        letter-spacing:.75pt;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoListParagraph, li.MsoListParagraph, div.MsoListParagraph
        {mso-style-priority:34;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        margin-top:0in;
        margin-right:0in;
        margin-bottom:8.0pt;
        margin-left:.5in;
        mso-add-space:auto;
        line-height:115%;
        mso-pagination:widow-orphan;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:Aptos;
        mso-fareast-theme-font:minor-latin;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:minor-bidi;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoListParagraphCxSpFirst, li.MsoListParagraphCxSpFirst, div.MsoListParagraphCxSpFirst
        {mso-style-priority:34;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-type:export-only;
        margin-top:0in;
        margin-right:0in;
        margin-bottom:0in;
        margin-left:.5in;
        mso-add-space:auto;
        line-height:115%;
        mso-pagination:widow-orphan;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:Aptos;
        mso-fareast-theme-font:minor-latin;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:minor-bidi;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoListParagraphCxSpMiddle, li.MsoListParagraphCxSpMiddle, div.MsoListParagraphCxSpMiddle
        {mso-style-priority:34;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-type:export-only;
        margin-top:0in;
        margin-right:0in;
        margin-bottom:0in;
        margin-left:.5in;
        mso-add-space:auto;
        line-height:115%;
        mso-pagination:widow-orphan;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:Aptos;
        mso-fareast-theme-font:minor-latin;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:minor-bidi;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoListParagraphCxSpLast, li.MsoListParagraphCxSpLast, div.MsoListParagraphCxSpLast
        {mso-style-priority:34;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-type:export-only;
        margin-top:0in;
        margin-right:0in;
        margin-bottom:8.0pt;
        margin-left:.5in;
        mso-add-space:auto;
        line-height:115%;
        mso-pagination:widow-orphan;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:Aptos;
        mso-fareast-theme-font:minor-latin;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:minor-bidi;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;}
    p.MsoQuote, li.MsoQuote, div.MsoQuote
        {mso-style-priority:29;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-link:"Quote Char";
        mso-style-next:Normal;
        margin-top:8.0pt;
        margin-right:0in;
        margin-bottom:8.0pt;
        margin-left:0in;
        text-align:center;
        line-height:115%;
        mso-pagination:widow-orphan;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:Aptos;
        mso-fareast-theme-font:minor-latin;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:minor-bidi;
        color:#404040;
        mso-themecolor:text1;
        mso-themetint:191;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;
        font-style:italic;}
    p.MsoIntenseQuote, li.MsoIntenseQuote, div.MsoIntenseQuote
        {mso-style-priority:30;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        mso-style-link:"Intense Quote Char";
        mso-style-next:Normal;
        margin-top:.25in;
        margin-right:.6in;
        margin-bottom:.25in;
        margin-left:.6in;
        text-align:center;
        line-height:115%;
        mso-pagination:widow-orphan;
        border:none;
        mso-border-top-alt:solid #0F4761 .5pt;
        mso-border-top-themecolor:accent1;
        mso-border-top-themeshade:191;
        mso-border-bottom-alt:solid #0F4761 .5pt;
        mso-border-bottom-themecolor:accent1;
        mso-border-bottom-themeshade:191;
        padding:0in;
        mso-padding-alt:10.0pt 0in 10.0pt 0in;
        font-size:12.0pt;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:Aptos;
        mso-fareast-theme-font:minor-latin;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:minor-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;
        mso-font-kerning:1.0pt;
        mso-ligatures:standardcontextual;
        font-style:italic;}
    span.MsoIntenseEmphasis
        {mso-style-priority:21;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;
        font-style:italic;}
    span.MsoIntenseReference
        {mso-style-priority:32;
        mso-style-unhide:no;
        mso-style-qformat:yes;
        font-variant:small-caps;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;
        letter-spacing:.25pt;
        font-weight:bold;}
    span.Heading1Char
        {mso-style-name:"Heading 1 Char";
        mso-style-priority:9;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:"Heading 1";
        mso-ansi-font-size:20.0pt;
        mso-bidi-font-size:20.0pt;
        font-family:"Aptos Display",sans-serif;
        mso-ascii-font-family:"Aptos Display";
        mso-ascii-theme-font:major-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:"Aptos Display";
        mso-hansi-theme-font:major-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;}
    span.Heading2Char
        {mso-style-name:"Heading 2 Char";
        mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:"Heading 2";
        mso-ansi-font-size:16.0pt;
        mso-bidi-font-size:16.0pt;
        font-family:"Aptos Display",sans-serif;
        mso-ascii-font-family:"Aptos Display";
        mso-ascii-theme-font:major-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:"Aptos Display";
        mso-hansi-theme-font:major-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;}
    span.Heading3Char
        {mso-style-name:"Heading 3 Char";
        mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:"Heading 3";
        mso-ansi-font-size:14.0pt;
        mso-bidi-font-size:14.0pt;
        font-family:"Times New Roman",serif;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;}
    span.Heading4Char
        {mso-style-name:"Heading 4 Char";
        mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:"Heading 4";
        font-family:"Times New Roman",serif;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;
        font-style:italic;}
    span.Heading5Char
        {mso-style-name:"Heading 5 Char";
        mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:"Heading 5";
        font-family:"Times New Roman",serif;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;}
    span.Heading6Char
        {mso-style-name:"Heading 6 Char";
        mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:"Heading 6";
        font-family:"Times New Roman",serif;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#595959;
        mso-themecolor:text1;
        mso-themetint:166;
        font-style:italic;}
    span.Heading7Char
        {mso-style-name:"Heading 7 Char";
        mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:"Heading 7";
        font-family:"Times New Roman",serif;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#595959;
        mso-themecolor:text1;
        mso-themetint:166;}
    span.Heading8Char
        {mso-style-name:"Heading 8 Char";
        mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:"Heading 8";
        font-family:"Times New Roman",serif;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#272727;
        mso-themecolor:text1;
        mso-themetint:216;
        font-style:italic;}
    span.Heading9Char
        {mso-style-name:"Heading 9 Char";
        mso-style-noshow:yes;
        mso-style-priority:9;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:"Heading 9";
        font-family:"Times New Roman",serif;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#272727;
        mso-themecolor:text1;
        mso-themetint:216;}
    span.TitleChar
        {mso-style-name:"Title Char";
        mso-style-priority:10;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:Title;
        mso-ansi-font-size:28.0pt;
        mso-bidi-font-size:28.0pt;
        font-family:"Aptos Display",sans-serif;
        mso-ascii-font-family:"Aptos Display";
        mso-ascii-theme-font:major-latin;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-hansi-font-family:"Aptos Display";
        mso-hansi-theme-font:major-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        letter-spacing:-.5pt;
        mso-font-kerning:14.0pt;}
    span.SubtitleChar
        {mso-style-name:"Subtitle Char";
        mso-style-priority:11;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:Subtitle;
        mso-ansi-font-size:14.0pt;
        mso-bidi-font-size:14.0pt;
        font-family:"Times New Roman",serif;
        mso-fareast-font-family:"Times New Roman";
        mso-fareast-theme-font:major-fareast;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:major-bidi;
        color:#595959;
        mso-themecolor:text1;
        mso-themetint:166;
        letter-spacing:.75pt;}
    span.QuoteChar
        {mso-style-name:"Quote Char";
        mso-style-priority:29;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:Quote;
        color:#404040;
        mso-themecolor:text1;
        mso-themetint:191;
        font-style:italic;}
    span.IntenseQuoteChar
        {mso-style-name:"Intense Quote Char";
        mso-style-priority:30;
        mso-style-unhide:no;
        mso-style-locked:yes;
        mso-style-link:"Intense Quote";
        color:#0F4761;
        mso-themecolor:accent1;
        mso-themeshade:191;
        font-style:italic;}
    .MsoChpDefault
        {mso-style-type:export-only;
        mso-default-props:yes;
        font-family:"Aptos",sans-serif;
        mso-ascii-font-family:Aptos;
        mso-ascii-theme-font:minor-latin;
        mso-fareast-font-family:Aptos;
        mso-fareast-theme-font:minor-latin;
        mso-hansi-font-family:Aptos;
        mso-hansi-theme-font:minor-latin;
        mso-bidi-font-family:"Times New Roman";
        mso-bidi-theme-font:minor-bidi;}
    .MsoPapDefault
        {mso-style-type:export-only;
        margin-bottom:8.0pt;
        line-height:115%;}
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
    </head>

    <body lang=EN-US style='tab-interval:.5in;word-wrap:break-word'>

    <div class=WordSection1>

    <table class=MsoNormalTable border=0 cellspacing=0 cellpadding=0 width=746
     style='width:559.15pt;border-collapse:collapse;mso-yfti-tbllook:1184;
     mso-padding-alt:0in 5.4pt 0in 5.4pt'>
     <tr style='mso-yfti-irow:0;mso-yfti-firstrow:yes;height:16.5pt'>
      <td width=746 nowrap colspan=11 style='width:559.15pt;border:none;border-bottom:
      solid gray 1.0pt;padding:0in 5.4pt 0in 5.4pt;height:16.5pt'>
    """
      
    html += f"""<p class=MsoNormal style='margin-bottom:0in;line-height:normal'><b><span style='font-family:"Calibri",sans-serif;mso-fareast-font-family:"Times New Roman";color:#44546A;mso-font-kerning:0pt;mso-ligatures:none'>Meeting Notes: {project_number} {project_name}"""
      
    html += """
      <o:p></o:p></span></b></p>
      </td>
     </tr>
     <tr style='mso-yfti-irow:1;height:15.0pt'>
      <td width=105 nowrap valign=bottom style='width:79.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=60 nowrap valign=bottom style='width:45.25pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=57 nowrap valign=bottom style='width:42.45pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=143 nowrap colspan=2 valign=bottom style='width:107.05pt;
      border:none;mso-border-top-alt:solid gray 1.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'>
      <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
      style='font-size:10.0pt;mso-ascii-font-family:Aptos;mso-fareast-font-family:
      "Times New Roman";mso-hansi-font-family:Aptos;mso-bidi-font-family:"Times New Roman";
      color:black;mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
      </td>
      <td width=50 nowrap valign=bottom style='width:37.8pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=74 nowrap valign=bottom style='width:55.6pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
     </tr>
    """

    return html

def generateMeetingHeaderRow(meeting_title, meeting_date, attendee_names, meeting_notes):
    html = f"""
    <tr style='mso-yfti-irow:2;height:15.75pt'>
    <td width=746 nowrap colspan=11 style='width:559.15pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.75pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><b><span
    style='font-family:"Calibri",sans-serif;mso-fareast-font-family:"Times New Roman";
    color:#44546A;mso-font-kerning:0pt;mso-ligatures:none'>{meeting_title}<o:p></o:p></span></b></p>
    </td>
    </tr>
    <tr style='mso-yfti-irow:3;height:15.0pt'>
    <td width=105 style='width:79.0pt;border:solid gray 1.0pt;mso-border-alt:
    solid gray .5pt;background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'>
    <p class=MsoNormal align=right style='margin-bottom:0in;text-align:right;
    line-height:normal'><b><span style='font-size:11.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>Date:<o:p></o:p></span></b></p>
    </td>
    <td width=640 colspan=10 style='width:480.15pt;border:solid gray 1.0pt;
    border-left:none;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#D9E1F2;
    padding:0in 5.4pt 0in 5.4pt;height:15.0pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";color:black;mso-font-kerning:0pt;mso-ligatures:none'>{meeting_date}<o:p></o:p></span></p>
    </td>
    </tr>
    <tr style='mso-yfti-irow:4;height:15.0pt'>
    <td width=105 style='width:79.0pt;border:solid gray 1.0pt;border-top:none;
    mso-border-left-alt:solid gray .5pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'>
    <p class=MsoNormal align=right style='margin-bottom:0in;text-align:right;
    line-height:normal'><b><span style='font-size:11.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>Attendees:<o:p></o:p></span></b></p>
    </td>
    <td width=640 colspan=10 style='width:480.15pt;border-top:none;border-left:
    none;border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-top-alt:
    solid gray .5pt;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#D9E1F2;
    padding:0in 5.4pt 0in 5.4pt;height:15.0pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";color:black;mso-font-kerning:0pt;mso-ligatures:none'>{attendee_names}<o:p></o:p></span></p>
    </td>
    </tr>
    <tr style='mso-yfti-irow:5;height:15.0pt'>
    <td width=746 colspan=11 style='width:559.15pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:11.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>Meeting Notes<o:p></o:p></span></b></p>
    </td>
    </tr>
    <tr style='mso-yfti-irow:6;height:15.0pt'>
    <td width=746 nowrap colspan=11 style='width:559.15pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'>{pnc.strip_html_body(meeting_notes)}
    </td>
    </tr>
    <tr style='mso-yfti-irow:7;height:15.0pt'>
    <td width=105 style='width:79.0pt;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
    <td width=64 style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
    <td width=64 style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
    <td width=64 style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
    <td width=64 style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
    <td width=60 style='width:45.25pt;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
    <td width=57 style='width:42.45pt;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
    <td width=143 colspan=2 style='width:107.05pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=50 style='width:37.8pt;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
    <td width=74 style='width:55.6pt;padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
    </tr>
    <tr style='mso-yfti-irow:8;height:15.0pt'>
    <td width=746 colspan=11 style='width:559.15pt;border:solid gray 1.0pt;
    mso-border-alt:solid gray .5pt;background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>Action Items<o:p></o:p></span></b></p>
    </td>
    </tr>
    <tr style='mso-yfti-irow:9;height:25.5pt'>
    <td width=478 colspan=7 style='width:358.7pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;height:25.5pt'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>Item<o:p></o:p></span></b></p>
    </td>
    <td width=92 style='width:69.25pt;border-top:none;border-left:none;
    border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#E7E6E6;
    padding:0in 5.4pt 0in 5.4pt;height:25.5pt'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>Assigned To<o:p></o:p></span></b></p>
    </td>
    <td width=101 colspan=2 style='width:1.05in;border-top:none;border-left:none;
    border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-top-alt:
    solid gray .5pt;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#E7E6E6;
    padding:0in 5.4pt 0in 5.4pt;height:25.5pt'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>Status<o:p></o:p></span></b></p>
    </td>
    <td width=74 style='width:55.6pt;border-top:none;border-left:none;border-bottom:
    solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;
    height:25.5pt'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>Due Date<o:p></o:p></span></b></p>
    </td>
    </tr>
    """

    return html

def generateActionItemRow(item, assignedto, status, duedate):
    html = f"""
    <tr style='mso-yfti-irow:10;height:25.5pt'>
    <td width=478 colspan=7 style='width:358.7pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;height:25.5pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";color:black;mso-font-kerning:0pt;mso-ligatures:none'>{item}<o:p></o:p></span></p>
    </td>
    <td width=92 style='width:69.25pt;border-top:none;border-left:none;
    border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#D9E1F2;
    padding:0in 5.4pt 0in 5.4pt;height:25.5pt'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>{assignedto}<o:p></o:p></span></p>
    </td>
    <td width=101 colspan=2 style='width:1.05in;border-top:none;border-left:none;
    border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-top-alt:
    solid gray .5pt;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#D9E1F2;
    padding:0in 5.4pt 0in 5.4pt;height:25.5pt'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>{status}<o:p></o:p></span></p>
    </td>
    <td width=74 style='width:55.6pt;border-top:none;border-left:none;border-bottom:
    solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;
    height:25.5pt'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-font-kerning:0pt;
    mso-ligatures:none'>{duedate}<o:p></o:p></span></p>
    </td>
    </tr>
    """

    return html

def generateMeetingFooterRow():
    html = """
    <tr style='mso-yfti-irow:11;height:15.0pt'>
      <td width=105 nowrap valign=bottom style='width:79.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=60 nowrap valign=bottom style='width:45.25pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=57 nowrap valign=bottom style='width:42.45pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=143 nowrap colspan=2 valign=bottom style='width:107.05pt;
      padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
      <td width=50 nowrap valign=bottom style='width:37.8pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=74 nowrap valign=bottom style='width:55.6pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
     </tr>
    """

    return html

def generateFooter(reportdate):
    html = f"""
     <tr style='mso-yfti-irow:12;mso-yfti-lastrow:yes;height:15.0pt'>
      <td width=169 nowrap colspan=2 style='width:127.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'>
      <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
      style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
      "Times New Roman";color:black;mso-font-kerning:0pt;mso-ligatures:none'>Report Date: {reportdate}<o:p></o:p></span></p>
      </td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=60 nowrap valign=bottom style='width:45.25pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=57 nowrap valign=bottom style='width:42.45pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=143 nowrap colspan=2 valign=bottom style='width:107.05pt;
      padding:0in 5.4pt 0in 5.4pt;height:15.0pt'></td>
      <td width=50 nowrap valign=bottom style='width:37.8pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
      <td width=74 nowrap valign=bottom style='width:55.6pt;padding:0in 5.4pt 0in 5.4pt;
      height:15.0pt'></td>
     </tr>
    </table>
    </div>
    </body>
    </html>
    """

    return html

class MeetingsExporter(QDialog):
    def __init__(self, parent: QMainWindow = None):
        super().__init__(parent)


        self.ui = uic.loadUi("plugins/forms/dialogExportNotesOptions.ui", self)
        self.ui.m_datePickerRptDateNotes.setCalendarPopup(True)
        self.ui.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint |
            QtCore.Qt.WindowType.WindowStaysOnTopHint
            )
        self.ui.setModal(True)

        self.set_execute_date(QDate.currentDate())

        #self.ui.buttonBox.accepted.connect(self.export_notes)
        self.ui.pushButtonOK.clicked.connect(self.export_notes)
        self.ui.pushButtonCancel.clicked.connect(self.close_dialog)

        self.xmlstr = None
        self.html_content = None


        # Define page layout (A4, portrait, 20pt margins)
        self.page_layout = QPageLayout()
        self.page_layout.setPageSize(QPageSize(QPageSize.PageSizeId.A4))  # A4 page size
        self.page_layout.setOrientation(QPageLayout.Orientation.Portrait)
        self.page_layout.setMargins(QMarginsF(20, 20, 20, 20))  # Optional margins

        # Compute printable dimensions (in pixels, assuming 96 DPI)
        self.page_width_px = self.page_layout.paintRectPixels(96).width()
        self.page_height_px = self.page_layout.paintRectPixels(96).height()

        self.web_view = QWebEngineView(self.ui)
        self.web_view.loadFinished.connect(self.on_load_finished)
        self.ui.verticalLayout_2.layout().addWidget(self.web_view)

    def set_execute_date(self, edate):
        self.executedate = edate
        self.ui.m_datePickerRptDateNotes.setDate(self.executedate)

    def close_dialog(self):
        self.hide()

    def on_load_finished(self, success):
        if not success:
            print(f"Failed to process html.")
            return

        print("Html load finished")

        # JavaScript to get content dimensions and apply scale
        js_code = f"""
        (function() {{
            var body = document.body;
            var contentWidth = body.scrollWidth;
            var contentHeight = body.scrollHeight;
            
            // Compute scale to fit page (min of width/height ratios, min 0.5)
            var scaleX = {self.page_width_px} / contentWidth;
            var scaleY = {self.page_height_px} / contentHeight;
            var scale = Math.max(0.5, Math.min(scaleX, scaleY));
            
            // Apply scale transform to body (centers it)
            body.style.transform = 'scale(' + scale + ')';
            body.style.transformOrigin = 'top left';
            body.style.width = Math.ceil(contentWidth * scale) + 'px';
            body.style.height = Math.ceil(contentHeight * scale) + 'px';
            
            // Optional: Adjust viewport meta for better rendering
            var meta = document.querySelector('meta[name="viewport"]');
            if (!meta) {{
                meta = document.createElement('meta');
                meta.name = 'viewport';
                meta.content = 'width=device-width, initial-scale=1.0';
                document.head.appendChild(meta);
            }}
            
            console.log('Scaled to: ' + scale);
            return scale;
        }})();
        """
        
        # Connect pdfPrintingFinished signal to handle completion
        self.web_view.page().pdfPrintingFinished.connect(self.pdf_printed)

        # Run JS and then generate PDF
        self.web_view.page().runJavaScript(js_code, self.generate_pdf_after_scale)

        print("Finished generate_pdf call")

    def generate_pdf_after_scale(self, scale):
        # Generate PDF
        self.web_view.page().printToPdf(self.pdfreportname, self.page_layout)
        print("Finished generate_pdf_after_scale")

    def set_xml_doc(self, xmlval):
        self.xmlstr = xmlval

    def export_notes(self):
        xmldoc = QDomDocument()
        if (xmldoc.setContent(self.xmlstr) == False):
            QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.")
            return ""

        self.project_htmlreportname = ""
        self.project_pdfreportname = ""
        self.htmlreportname = ""
        self.pdfreportname = ""

        self.internalreport = self.ui.m_checkBoxInternalRptNotes.isChecked()
        self.keephtml = self.ui.m_checkBoxHTMLRptNotes.isChecked()
        self.emailasinlinehtml = self.ui.m_radioBoxEmailAsInlineHTML.isChecked()
        self.emailaspdf = self.ui.m_radioBoxEmailAsPDF.isChecked()
        self.emailashtml = self.ui.m_radioBoxEmailAsHTML.isChecked()
        self.noemail = self.ui.m_radioBoxDoNotEmail.isChecked()
        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()

        xmlroot = xmldoc.elementsByTagName("projectnotes").at(0) # get root node
        pm = xmlroot.toElement().attribute("managing_manager_name")

        #print("locating project folder...")
        QtWidgets.QApplication.processEvents()
        projectfolder = pnc.get_projectfolder(xmlroot)
        #print("finding projects table ..")
        print("found project folder: " + projectfolder)
        QtWidgets.QApplication.processEvents()
        
        projtab = pnc.find_node(xmlroot, "table", "name", "projects")
        projnum = pnc.get_column_value(projtab.firstChild(), "project_number")
        projdes = pnc.get_column_value(projtab.firstChild(), "project_name")

        email = False
        nm = None
        stat = None
        receivers = ""

        if (projectfolder is None or projectfolder ==""):
            projectfolder = QFileDialog.getExistingDirectory(None, "Select an output folder", QDir.home().path())

            if projectfolder == "" or projectfolder is None:
                return ""
        else:
            # this specific folder path may need to be configurable
            projectfolder = projectfolder + "/Project Management/Meeting Minutes"

        projectfolder += "/"

        if not pnc.folder_exists(projectfolder):
            msg = f"Folder {projectfolder} does not exist.  Cannot generate the report."
            print(msg)
            QMessageBox.critical(None, "Folder Does Not Exist")
            return

        self.progbar = QProgressDialog(self)
        self.progbar.setWindowTitle("Exporting...")
        self.progbar.setWindowFlags(
            QtCore.Qt.WindowType.Window |
            QtCore.Qt.WindowType.WindowCloseButtonHint 
            )
        self.progbar.setMinimumWidth(350)
        self.progbar.setCancelButton(None)
        self.progbar.show()

        progval = 0
        progtot = 6

        progval = progval + 1
        self.progbar.setValue(int(min(progval / progtot * 100, 100)))
        self.progbar.setLabelText("Copying files...")

        temporaryfolder = pnc.get_temporary_folder() + "/"

        if self.internalreport:
            self.htmlreportname = temporaryfolder + projnum + " Meeting Minutes Internal.html"
            self.pdfreportname = temporaryfolder + projnum + " Meeting Minutes Internal.pdf"
        else:
            self.htmlreportname = temporaryfolder + projnum + " Meeting Minutes.html"
            self.pdfreportname = temporaryfolder + projnum + " Meeting Minutes.pdf"

        self.html_content = generateHeader(projnum, projdes)

        progval = progval + 1
        self.progbar.setValue(int(min(progval / progtot * 100, 100)))
        self.progbar.setLabelText("Gathering notes...")

        # count expand out excell rows for status report items
        notes = pnc.find_node(xmlroot, "table", "name", "project_notes")
        itemcount = 0

        if not notes.isNull():
            notesrow = notes.firstChild()

            while not notesrow.isNull():
                QtWidgets.QApplication.processEvents()
                isinternal = pnc.get_column_value(notesrow, "internal_item")

                if isinternal == "1" and self.internalreport:
                    itemcount = itemcount + 1
                elif isinternal != "1":
                    itemcount = itemcount + 1

                notesrow = notesrow.nextSibling()

            progtot = progtot + itemcount

            progval = progval + 1
            self.progbar.setValue(int(min(progval / progtot * 100, 100)))
            self.progbar.setLabelText("Gathering notes...")

            itemcount = 0
            notesrow = notes.firstChild()

            while not notesrow.isNull():
                QtWidgets.QApplication.processEvents()
                isinternal = pnc.get_column_value(notesrow, "internal_item")
                includeitem = False

                if isinternal == "1" and self.internalreport:
                    includeitem = True
                elif isinternal != "1":
                    includeitem = True

                if includeitem:
                    itemcount = itemcount + 1

                progval = progval + 1
                self.progbar.setValue(int(min(progval / progtot * 100, 100)))
                self.progbar.setLabelText("Gathering notes...")

                note = pnc.get_column_value(notesrow, "note")
                attendees = pnc.find_node(notesrow, "table", "name", "meeting_attendees")
                attendeelist = ""
                if not attendees.isNull():
                    attendeerow = attendees.firstChild()

                    while not attendeerow.isNull():
                        QtWidgets.QApplication.processEvents()
                        if attendeelist != "" :
                            attendeelist = attendeelist + ", "

                        attendeelist = attendeelist + pnc.get_column_value(attendeerow, "name")
                        #print("processing attendees...")
                        attendeerow = attendeerow.nextSibling()

                self.html_content += generateMeetingHeaderRow(pnc.get_column_value(notesrow, "note_title"), pnc.get_column_value(notesrow, "note_date"), attendeelist, note) 

                trackeritems = pnc.find_node(notesrow, "table", "name", "item_tracker")

                trackercount = 0
                if trackeritems:
                    trackerrow = trackeritems.firstChild()

                    while not trackerrow.isNull():
                        QtWidgets.QApplication.processEvents()
                        trackercount = trackercount + 1

                        self.html_content += generateActionItemRow(pnc.get_column_value(trackerrow, "item_name"), pnc.get_column_value(trackerrow, "assigned_to"), pnc.get_column_value(trackerrow, "status"), pnc.get_column_value(trackerrow, "date_due")) 

                        trackerrow = trackerrow.nextSibling()

                
                self.html_content += generateMeetingFooterRow()

                notesrow = notesrow.nextSibling()

            self.html_content += generateFooter(self.executedate.toString("MM/dd/yyyy"))

            progval = progval + 1
            self.progbar.setValue(int(min(progval / progtot * 100, 100)))
            self.progbar.setLabelText("Finalizing files...")

            # should we email?
            if self.noemail == False :
                self.subject = projnum + " " + projdes + " - " + self.executedate.toString("MM/dd/yyyy")

            file = QFile(self.htmlreportname)
            if file.open(QFile.OpenModeFlag.WriteOnly):
                stream = QTextStream(file)
                stream.setEncoding(QStringConverter.Encoding.Utf8)
                stream << self.html_content
                file.close()
            else:
                print(f"Error attempting to write {self.htmlreportname}")

            # establish final file names and locations
            if self.internalreport:
                self.project_htmlreportname = projectfolder + projnum + " Meeting Minutes Internal.html"
                self.project_pdfreportname = projectfolder + projnum + " Meeting Minutes Internal.pdf"
            else:
                self.project_htmlreportname = projectfolder + projnum + " Meeting Minutes.html"
                self.project_pdfreportname = projectfolder + projnum + " Meeting Minutes.pdf"

        print(f"ready to generate pdf from {self.htmlreportname}")
        # call pdf generator
        self.progbar.setLabelText("Finalizing files...")
        self.web_view.load(QUrl.fromLocalFile(self.htmlreportname)) # trigger the load

    def pdf_printed(self, success: bool):
        if success:
            print(f"PDF saved: {self.pdfreportname}")
        else:
            print("Failed to generate PDF")

        ct = CollaborationTools()

        if self.emailasinlinehtml:
                ct.send_an_email(self.xmlstr, self.subject, self.html_content, None, "", True)
        elif self.emailashtml:
                ct.send_an_email(self.xmlstr, self.subject, self.html_content, self.htmlreportname, "", True)
        elif self.emailaspdf:
                ct.send_an_email(self.xmlstr, self.subject, self.html_content, self.pdfreportname, "", True)

        QFile.remove(self.project_pdfreportname)
        if not QFile(self.pdfreportname).copy(self.project_pdfreportname):
            QMessageBox.critical(None, "Unable to copy generated export", "Could not copy " + self.pdfreportname + " to " + self.project_pdfreportname)

        if self.keephtml == False:
            QFile.remove(self.htmlreportname)
        else:
            QFile.remove(self.project_htmlreportname)
            if not QFile(self.htmlreportname).copy(self.project_htmlreportname):
                QMessageBox.critical(None, "Unable to copy generated export", "Could not copy " + self.htmlreportname + " to " + self.project_htmlreportname)

        if self.ui.m_checkBoxDisplayNotes.isChecked():
            try:
                QDesktopServices.openUrl(QUrl("file:///" + self.project_pdfreportname))
            except:
                print(f"An error occured trying to open {self.project_pdfreportname}")
                pass

        QFile.remove(self.pdfreportname)

        self.progbar.setValue(100)

        self.progbar.hide()
        self.progbar.close()
        self.progbar = None # must be destroyed

        QtWidgets.QApplication.restoreOverrideCursor()

        self.hide()


# processing main def
def menuExportMeetingNotes(xmlstr, parameter):
    mex.set_xml_doc(xmlstr)

    QtWidgets.QApplication.restoreOverrideCursor()
    QtWidgets.QApplication.processEvents()   

    mex.set_execute_date(QDate.currentDate())

    print("Calling web view show.")

    mex.show()

    print("Finished calling web view show.")

    return ""

# keep the instance of windows open to avoid sending the main app a close message
pnc = None
mex = None

#setup test data
if __name__ == '__main__':
    import os
    import sys
    os.chdir("..")

    app = QApplication(sys.argv)
    pnc = ProjectNotesCommon()
    mex = MeetingsExporter()

    xml_content = ""
    with open("C:\\Users\\pamcki\\OneDrive - Cornerstone Controls\\Documents\\Work In Progress\\XML\\project.xml", 'r', encoding='utf-8') as file:
        xml_content = file.read()
    mex.set_xml_doc(xml_content)
    mex.show()

    # html = generateHeader("P400", "Project Name")
    # html += generateMeetingHeaderRow("Title", "1/1/2001", "Bob, Sam, Jill", "This is my really long note.") 
    # html += generateActionItemRow("Item Description", "Bob Smith", "Assigned", "4/3/2005") 
    # html += generateActionItemRow("Item Description", "Bob Smith", "Assigned", "4/3/2005") 
    # html += generateMeetingFooterRow()
    # html += generateFooter("1/1/2010")

    # menuExportMeetingNotes("","")

    app.exec()
else:
    pnc = ProjectNotesCommon()
    mex = MeetingsExporter(pnc.get_main_window())
