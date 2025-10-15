import sys
import platform

from includes.common import ProjectNotesCommon
from includes.collaboration_tools import CollaborationTools

from PyQt6 import QtGui, QtCore, QtWidgets, uic
from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDate, QUrl, QDir
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication, QProgressDialog, QDialog, QFileDialog
from PyQt6.QtGui import QDesktopServices, QTextDocument
from PyQt6.QtGui import QPdfWriter, QPageSize, QPageLayout, QPainter
from PyQt6.QtWebEngineWidgets import QWebEngineView

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


pnc = ProjectNotesCommon()

project_htmlreportname = ""
project_pdfreportname = ""
htmlreportname = ""
pdfreportname = ""


def generateHeader(project_number, project_name):
    header = f"""<html xmlns:v="urn:schemas-microsoft-com:vml"
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
    {{font-family:"Cambria Math";
    panose-1:2 4 5 3 5 4 6 3 2 4;
    mso-font-charset:0;
    mso-generic-font-family:roman;
    mso-font-pitch:variable;
    mso-font-signature:-536869121 1107305727 33554432 0 415 0;}}
    @font-face
    {{font-family:Calibri;
    panose-1:2 15 5 2 2 2 4 3 2 4;
    mso-font-charset:0;
    mso-generic-font-family:swiss;
    mso-font-pitch:variable;
    mso-font-signature:-469750017 -1040178053 9 0 511 0;}}
    @font-face
    {{font-family:Aptos;
    mso-font-charset:0;
    mso-generic-font-family:swiss;
    mso-font-pitch:variable;
    mso-font-signature:536871559 3 0 0 415 0;}}
    /* Style Definitions */
    p.MsoNormal, li.MsoNormal, div.MsoNormal
    {{mso-style-unhide:no;
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
    mso-ligatures:standardcontextual;}}
    h1
    {{mso-style-priority:9;
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
    font-weight:normal;}}
    h2
    {{mso-style-noshow:yes;
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
    font-weight:normal;}}
    h3
    {{mso-style-noshow:yes;
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
    font-weight:normal;}}
    h4
    {{mso-style-noshow:yes;
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
    font-style:italic;}}
    h5
    {{mso-style-noshow:yes;
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
    font-weight:normal;}}
    h6
    {{mso-style-noshow:yes;
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
    font-style:italic;}}
    p.MsoHeading7, li.MsoHeading7, div.MsoHeading7
    {{mso-style-noshow:yes;
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
    mso-ligatures:standardcontextual;}}
    p.MsoHeading8, li.MsoHeading8, div.MsoHeading8
    {{mso-style-noshow:yes;
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
    font-style:italic;}}
    p.MsoHeading9, li.MsoHeading9, div.MsoHeading9
    {{mso-style-noshow:yes;
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
    mso-ligatures:standardcontextual;}}
    p.MsoTitle, li.MsoTitle, div.MsoTitle
    {{mso-style-priority:10;
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
    mso-ligatures:standardcontextual;}}
    p.MsoTitleCxSpFirst, li.MsoTitleCxSpFirst, div.MsoTitleCxSpFirst
    {{mso-style-priority:10;
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
    mso-ligatures:standardcontextual;}}
    p.MsoTitleCxSpMiddle, li.MsoTitleCxSpMiddle, div.MsoTitleCxSpMiddle
    {{mso-style-priority:10;
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
    mso-ligatures:standardcontextual;}}
    p.MsoTitleCxSpLast, li.MsoTitleCxSpLast, div.MsoTitleCxSpLast
    {{mso-style-priority:10;
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
    mso-ligatures:standardcontextual;}}
    p.MsoSubtitle, li.MsoSubtitle, div.MsoSubtitle
    {{mso-style-priority:11;
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
    mso-ligatures:standardcontextual;}}
    p.MsoListParagraph, li.MsoListParagraph, div.MsoListParagraph
    {{mso-style-priority:34;
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
    mso-ligatures:standardcontextual;}}
    p.MsoListParagraphCxSpFirst, li.MsoListParagraphCxSpFirst, div.MsoListParagraphCxSpFirst
    {{mso-style-priority:34;
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
    mso-ligatures:standardcontextual;}}
    p.MsoListParagraphCxSpMiddle, li.MsoListParagraphCxSpMiddle, div.MsoListParagraphCxSpMiddle
    {{mso-style-priority:34;
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
    mso-ligatures:standardcontextual;}}
    p.MsoListParagraphCxSpLast, li.MsoListParagraphCxSpLast, div.MsoListParagraphCxSpLast
    {{mso-style-priority:34;
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
    mso-ligatures:standardcontextual;}}
    p.MsoQuote, li.MsoQuote, div.MsoQuote
    {{mso-style-priority:29;
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
    font-style:italic;}}
    p.MsoIntenseQuote, li.MsoIntenseQuote, div.MsoIntenseQuote
    {{mso-style-priority:30;
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
    font-style:italic;}}
    span.MsoIntenseEmphasis
    {{mso-style-priority:21;
    mso-style-unhide:no;
    mso-style-qformat:yes;
    color:#0F4761;
    mso-themecolor:accent1;
    mso-themeshade:191;
    font-style:italic;}}
    span.MsoIntenseReference
    {{mso-style-priority:32;
    mso-style-unhide:no;
    mso-style-qformat:yes;
    font-variant:small-caps;
    color:#0F4761;
    mso-themecolor:accent1;
    mso-themeshade:191;
    letter-spacing:.25pt;
    font-weight:bold;}}
    span.Heading1Char
    {{mso-style-name:"Heading 1 Char";
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
    mso-themeshade:191;}}
    span.Heading2Char
    {{mso-style-name:"Heading 2 Char";
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
    mso-themeshade:191;}}
    span.Heading3Char
    {{mso-style-name:"Heading 3 Char";
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
    mso-themeshade:191;}}
    span.Heading4Char
    {{mso-style-name:"Heading 4 Char";
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
    font-style:italic;}}
    span.Heading5Char
    {{mso-style-name:"Heading 5 Char";
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
    mso-themeshade:191;}}
    span.Heading6Char
    {{mso-style-name:"Heading 6 Char";
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
    font-style:italic;}}
    span.Heading7Char
    {{mso-style-name:"Heading 7 Char";
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
    mso-themetint:166;}}
    span.Heading8Char
    {{mso-style-name:"Heading 8 Char";
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
    font-style:italic;}}
    span.Heading9Char
    {{mso-style-name:"Heading 9 Char";
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
    mso-themetint:216;}}
    span.TitleChar
    {{mso-style-name:"Title Char";
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
    mso-font-kerning:14.0pt;}}
    span.SubtitleChar
    {{mso-style-name:"Subtitle Char";
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
    letter-spacing:.75pt;}}
    span.QuoteChar
    {{mso-style-name:"Quote Char";
    mso-style-priority:29;
    mso-style-unhide:no;
    mso-style-locked:yes;
    mso-style-link:Quote;
    color:#404040;
    mso-themecolor:text1;
    mso-themetint:191;
    font-style:italic;}}
    span.IntenseQuoteChar
    {{mso-style-name:"Intense Quote Char";
    mso-style-priority:30;
    mso-style-unhide:no;
    mso-style-locked:yes;
    mso-style-link:"Intense Quote";
    color:#0F4761;
    mso-themecolor:accent1;
    mso-themeshade:191;
    font-style:italic;}}
    .MsoChpDefault
    {{mso-style-type:export-only;
    mso-default-props:yes;
    font-family:"Aptos",sans-serif;
    mso-ascii-font-family:Aptos;
    mso-ascii-theme-font:minor-latin;
    mso-fareast-font-family:Aptos;
    mso-fareast-theme-font:minor-latin;
    mso-hansi-font-family:Aptos;
    mso-hansi-theme-font:minor-latin;
    mso-bidi-font-family:"Times New Roman";
    mso-bidi-theme-font:minor-bidi;}}
    .MsoPapDefault
    {{mso-style-type:export-only;
    margin-bottom:8.0pt;
    line-height:115%;}}
    @page WordSection1
    {{size:8.5in 11.0in;
    margin:1.0in 1.0in 1.0in 1.0in;
    mso-header-margin:.5in;
    mso-footer-margin:.5in;
    mso-paper-source:0;}}
    div.WordSection1
    {{page:WordSection1;}}
    -->
    </style>
    </head>

    <body lang=EN-US style='tab-interval:.5in;word-wrap:break-word'>

    <div class=WordSection1>

    <table class=MsoNormalTable border=0 cellspacing=0 cellpadding=0 width=903
    style='width:677.05pt;border-collapse:collapse;mso-yfti-tbllook:1184;
    mso-padding-alt:0in 5.4pt 0in 5.4pt'>
    <tr style='mso-yfti-irow:0;mso-yfti-firstrow:yes;height:26.25pt'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:26.25pt'></td>
    <td width=872 nowrap colspan=12 style='width:654.05pt;border:none;border-bottom:
    solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;
    height:26.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><b><span
    style='font-size:15.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";color:#44546A;mso-font-kerning:0pt;mso-ligatures:none'>Meeting
    Notes: {project_number} {project_name}<o:p></o:p></span></b></p>
    </td>
    </tr>
    <tr style='mso-yfti-irow:1;height:15.0pt'>
    <td width=235 nowrap colspan=2 valign=bottom style='width:176.0pt;padding:
    0in 5.4pt 0in 5.4pt;height:15.0pt'>
    </td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:47.9pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=61 nowrap valign=bottom style='width:45.4pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=58 nowrap valign=bottom style='width:43.15pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=74 nowrap valign=bottom style='width:55.6pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    </tr>
    """

    return header

def generateMeetingHeaderRow(meeting_title, meeting_date, attendee_names, meeting_notes):
    html = f"""
    <tr style='mso-yfti-irow:2;'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    <td width=844 nowrap colspan=11 style='width:633.05pt;border:none;border-bottom:
    solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;
    '>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><b><span
    style='font-size:15.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";color:#44546A;mso-font-kerning:0pt;mso-ligatures:none'>{meeting_title}<o:p></o:p></span></b></p>
    </td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    </tr>
    <tr style='mso-yfti-irow:3;'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    <td width=204 valign=top style='width:153.0pt;border:solid gray 1.0pt;
    border-top:none;mso-border-left-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#E7E6E6;
    padding:0in 5.4pt 0in 5.4pt;'>
    <p class=MsoNormal align=right style='margin-bottom:0in;text-align:right;
    line-height:normal'><b><span style='font-size:11.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>Date:</span></b><b><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></b></p>
    </td>
    <td width=640 colspan=10 style='width:480.05pt;border:solid gray 1.0pt;
    border-left:none;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#D9E1F2;
    padding:0in 5.4pt 0in 5.4pt;'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";color:black;mso-color-alt:windowtext;mso-font-kerning:0pt;
    mso-ligatures:none'>{meeting_date}</span><span style='font-size:11.0pt;
    font-family:"Calibri",sans-serif;mso-fareast-font-family:"Times New Roman";
    mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></p>
    </td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    </tr>
    <tr style='mso-yfti-irow:4;'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    <td width=204 valign=top style='width:153.0pt;border:solid gray 1.0pt;
    border-top:none;mso-border-left-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#E7E6E6;
    padding:0in 5.4pt 0in 5.4pt;'>
    <p class=MsoNormal align=right style='margin-bottom:0in;text-align:right;
    line-height:normal'><b><span style='font-size:11.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>Attendees:</span></b><b><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></b></p>
    </td>
    <td width=640 colspan=10 style='width:480.05pt;border-top:none;border-left:
    none;border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-top-alt:
    solid gray .5pt;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#D9E1F2;
    padding:0in 5.4pt 0in 5.4pt;'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";color:black;mso-color-alt:windowtext;mso-font-kerning:0pt;
    mso-ligatures:none'>{attendee_names}</span><span style='font-size:11.0pt;
    font-family:"Calibri",sans-serif;mso-fareast-font-family:"Times New Roman";
    mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></p>
    </td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    </tr>
    <tr style='mso-yfti-irow:5;'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    <td width=844 colspan=11 style='width:633.05pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:11.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>Meeting Notes</span></b><b><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></b></p>
    </td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    </tr>
    <tr style='mso-yfti-irow:6;'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    <td width=844 nowrap colspan=11 valign=top style='width:633.05pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";color:black;mso-color-alt:windowtext;mso-font-kerning:0pt;
    mso-ligatures:none'>{meeting_notes}</span><span style='font-size:11.0pt;
    font-family:"Calibri",sans-serif;mso-fareast-font-family:"Times New Roman";
    mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></p>
    </td>
    </tr>

    <tr style='mso-yfti-irow:7;height:17.25pt'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:17.25pt'></td>
    <td width=204 valign=top style='width:153.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:47.9pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=61 valign=top style='width:45.4pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=58 valign=top style='width:43.15pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=64 valign=top style='width:48.0pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=74 valign=top style='width:55.6pt;border:none;border-bottom:solid gray 1.0pt;
    mso-border-bottom-alt:solid gray .5pt;padding:0in 5.4pt 0in 5.4pt;height:
    17.25pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'>&nbsp;<o:p></o:p></span></p>
    </td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:17.25pt'></td>
    </tr>
    <tr style='mso-yfti-irow:8;height:.25in'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:.25in'></td>
    <td width=844 colspan=11 style='width:633.05pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>Action Items</span></b><b><span
    style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></b></p>
    </td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:.25in'></td>
    </tr>
    <tr style='mso-yfti-irow:9;height:.25in'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:.25in'></td>
    <td width=588 colspan=7 style='width:440.9pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>Item</span></b><b><span
    style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></b></p>
    </td>
    <td width=118 colspan=2 style='width:88.55pt;border:solid gray 1.0pt;
    border-left:none;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#E7E6E6;
    padding:0in 5.4pt 0in 5.4pt;height:.25in'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>Assigned To</span></b><b><span
    style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></b></p>
    </td>
    <td width=64 style='width:48.0pt;border-top:none;border-left:none;border-bottom:
    solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;
    height:.25in'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>Status</span></b><b><span
    style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></b></p>
    </td>
    <td width=74 style='width:55.6pt;border-top:none;border-left:none;border-bottom:
    solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#E7E6E6;padding:0in 5.4pt 0in 5.4pt;
    height:.25in'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><b><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>Due Date</span></b><b><span
    style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></b></p>
    </td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:.25in'></td>
    </tr>

    """

    return html


def generateActionItemRow(item, assignedto, status, duedate):
    html = f"""
    <tr style='mso-yfti-irow:10;'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    <td width=588 colspan=7 style='width:440.9pt;border:solid gray 1.0pt;
    border-top:none;mso-border-top-alt:solid gray .5pt;mso-border-alt:solid gray .5pt;
    background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";color:black;mso-color-alt:windowtext;mso-font-kerning:0pt;
    mso-ligatures:none'>{item}</span><span style='font-size:10.0pt;
    font-family:"Calibri",sans-serif;mso-fareast-font-family:"Times New Roman";
    mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></p>
    </td>
    <td width=118 colspan=2 style='width:88.55pt;border-top:none;border-left:
    none;border-bottom:solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-top-alt:
    solid gray .5pt;mso-border-top-alt:solid gray .5pt;mso-border-bottom-alt:
    solid gray .5pt;mso-border-right-alt:solid gray .5pt;background:#D9E1F2;
    padding:0in 5.4pt 0in 5.4pt;'>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>{assignedto}</span><span
    style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></p>
    </td>
    <td width=64 style='width:48.0pt;border-top:none;border-left:none;border-bottom:
    solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;
    '>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>{status}</span><span
    style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></p>
    </td>
    <td width=74 style='width:55.6pt;border-top:none;border-left:none;border-bottom:
    solid gray 1.0pt;border-right:solid gray 1.0pt;mso-border-bottom-alt:solid gray .5pt;
    mso-border-right-alt:solid gray .5pt;background:#D9E1F2;padding:0in 5.4pt 0in 5.4pt;
    '>
    <p class=MsoNormal align=center style='margin-bottom:0in;text-align:center;
    line-height:normal'><span style='font-size:10.0pt;font-family:"Calibri",sans-serif;
    mso-fareast-font-family:"Times New Roman";color:black;mso-color-alt:windowtext;
    mso-font-kerning:0pt;mso-ligatures:none'>{duedate}</span><span
    style='font-size:10.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";mso-font-kerning:0pt;mso-ligatures:none'><o:p></o:p></span></p>
    </td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    '></td>
    </tr>
    """

    return html


def generateMeetingFooterRow():
    html = f"""
    <tr style='mso-yfti-irow:11;height:15.0pt'>
    <td width=235 nowrap colspan=2 valign=bottom style='width:176.0pt;padding:
    0in 5.4pt 0in 5.4pt;height:15.0pt'>
    </td>
    """

    return html

def generateFooter(reportdate):
    html = f"""
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:47.9pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=61 nowrap valign=bottom style='width:45.4pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=58 nowrap valign=bottom style='width:43.15pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=74 nowrap valign=bottom style='width:55.6pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    </tr>
    <tr style='mso-yfti-irow:12;mso-yfti-lastrow:yes;height:15.0pt'>
    <td width=31 nowrap valign=bottom style='width:23.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=204 nowrap valign=bottom style='width:153.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'>
    <p class=MsoNormal style='margin-bottom:0in;line-height:normal'><span
    style='font-size:11.0pt;font-family:"Calibri",sans-serif;mso-fareast-font-family:
    "Times New Roman";color:black;mso-font-kerning:0pt;mso-ligatures:none'>Report
    Date: {reportdate}<o:p></o:p></span></p>
    </td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:47.9pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=61 nowrap valign=bottom style='width:45.4pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=58 nowrap valign=bottom style='width:43.15pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=64 nowrap valign=bottom style='width:48.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=74 nowrap valign=bottom style='width:55.6pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    <td width=28 nowrap valign=bottom style='width:21.0pt;padding:0in 5.4pt 0in 5.4pt;
    height:15.0pt'></td>
    </tr>
    </table>

    <p class=MsoNormal><o:p>&nbsp;</o:p></p>

    </div>

    </body>

    </html>

    """

    return html


def export_to_pdf():
    page_layout = QPageLayout()
    page_layout.setPageSize(QPageSize(QPageSize.PageSizeId.A4))  # A4 page size
    page_layout.setOrientation(QPageLayout.Orientation.Portrait)
    page_layout.setMargins(QMarginsF(20, 20, 20, 20))  # Optional margins

    view.page().printToPdf(project_pdfreportname, page_layout)

# processing main def
def menuExportMeetingNotes(xmlstr, parameter):
    xmlval = QDomDocument()
    if (xmlval.setContent(xmlstr) == False):
        QMessageBox.critical(None, "Cannot Parse XML", "Unable to parse XML sent to plugin.",QMessageBox.StandardButton.Cancel)
        return ""

    executedate = QDate.currentDate()
    internalreport = False
    keephtml = False
    emailasinlinehtml = False
    emailaspdf = False
    emailashtml = False
    noemail = True

    QtWidgets.QApplication.restoreOverrideCursor()
    QtWidgets.QApplication.processEvents()   

    ui = uic.loadUi("plugins/forms/dialogExportNotesOptions.ui")
    ui.m_datePickerRptDateNotes.setDate(executedate)
    ui.m_datePickerRptDateNotes.setCalendarPopup(True)
    ui.setWindowFlags(
        QtCore.Qt.WindowType.Window |
        QtCore.Qt.WindowType.WindowCloseButtonHint |
        QtCore.Qt.WindowType.WindowStaysOnTopHint
        )

    if ui.exec():
        internalreport = ui.m_checkBoxInternalRptNotes.isChecked()
        keepexcel = ui.m_checkBoxExcelRptNotes.isChecked()
        emailasinlinehtml = ui.m_radioBoxEmailAsInlineHTML.isChecked()
        emailaspdf = ui.m_radioBoxEmailAsPDF.isChecked()
        emailashtml = ui.m_radioBoxEmailAsHTML.isChecked()
        noemail = ui.m_radioBoxDoNotEmail.isChecked()
        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()
    else:
        QtWidgets.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CursorShape.WaitCursor))
        QtWidgets.QApplication.processEvents()
        return ""

    xmlroot = xmlval.elementsByTagName("projectnotes").at(0) # get root node
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
        projectfolder = projectfolder + "/Meeting Minutes/"

    projectfolder = projectfolder + "/"

    progbar = QProgressDialog()
    progbar.setWindowTitle("Exporting...")
    progbar.setWindowFlags(
        QtCore.Qt.WindowType.Window |
        QtCore.Qt.WindowType.WindowCloseButtonHint 
        )
    progbar.setMinimumWidth(350)
    progbar.setCancelButton(None)
    progbar.show()

    progval = 0
    progtot = 6

    progval = progval + 1
    progbar.setValue(int(min(progval / progtot * 100, 100)))
    progbar.setLabelText("Copying files...")

    temporaryfolder = pnc.get_temporary_folder() + "/"

    if internalreport:
        htmlreportname = temporaryfolder + projnum + " Meeting Minutes Internal.html"
        pdfreportname = temporaryfolder + projnum + " Meeting Minutes Internal.pdf"
    else:
        htmlreportname = temporaryfolder + projnum + " Meeting Minutes.html"
        pdfreportname = temporaryfolder + projnum + " Meeting Minutes.pdf"

    html = generateHeader(projnum, projdes)

    progval = progval + 1
    progbar.setValue(int(min(progval / progtot * 100, 100)))
    progbar.setLabelText("Gathering notes...")

    # count expand out excell rows for status report items
    notes = pnc.find_node(xmlroot, "table", "name", "project_notes")
    itemcount = 0

    if not notes.isNull():
        notesrow = notes.firstChild()

        while not notesrow.isNull():
            QtWidgets.QApplication.processEvents()
            isinternal = pnc.get_column_value(notesrow, "internal_item")

            if isinternal == "1" and internalreport:
                itemcount = itemcount + 1
            elif isinternal != "1":
                itemcount = itemcount + 1

            notesrow = notesrow.nextSibling()

        progtot = progtot + itemcount

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Gathering notes...")

    itemcount = 0
    notesrow = notes.firstChild()

    while not notesrow.isNull():
        QtWidgets.QApplication.processEvents()
        isinternal = pnc.get_column_value(notesrow, "internal_item")
        includeitem = False

        if isinternal == "1" and internalreport:
            includeitem = True
        elif isinternal != "1":
            includeitem = True

        if includeitem:
            itemcount = itemcount + 1

        progval = progval + 1
        progbar.setValue(int(min(progval / progtot * 100, 100)))
        progbar.setLabelText("Gathering notes...")

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

        html += generateMeetingHeaderRow(pnc.get_column_value(notesrow, "note_title"), pnc.get_column_value(notesrow, "note_date"), attendeelist, note) 

        trackeritems = pnc.find_node(notesrow, "table", "name", "item_tracker")

        trackercount = 0
        if trackeritems:
            trackerrow = trackeritems.firstChild()

            while not trackerrow.isNull():
                QtWidgets.QApplication.processEvents()
                trackercount = trackercount + 1

                html += generateActionItemRow(pnc.get_column_value(trackerrow, "item_name"), pnc.get_column_value(trackerrow, "assigned_to"), pnc.get_column_value(trackerrow, "status"), pnc.get_column_value(trackerrow, "date_due")) 

                trackerrow = trackerrow.nextSibling()

        
        html += generateMeetingFooterRow()

        notesrow = notesrow.nextSibling()

    html += generateFooter("1/1/2010")

    progval = progval + 1
    progbar.setValue(int(min(progval / progtot * 100, 100)))
    progbar.setLabelText("Finalizing files...")

    # generate PDFs

    # should we email?
    if noemail == False :
        subject = projnum + " " + projdes + " - " + executedate.toString("MM/dd/yyyy")

    #if emailasinlinehtml:
    #    #        pne.email_excel_html(sheet, subject, "", None)
    #elif emailashtml:
    #    #        pne.email_excel_html(sheet, subject, "", excelreportname)
    #elif emailaspdf:
    #    #        pne.email_excel_html(sheet, subject, "", pdfreportname)

    # move from temporary location
    if internalreport:
        project_htmlreportname = projectfolder + projnum + " Meeting Minutes Internal.html"
        project_pdfreportname = projectfolder + projnum + " Meeting Minutes Internal.pdf"
    else:
        project_htmlreportname = projectfolder + projnum + " Meeting Minutes.html"
        project_pdfreportname = projectfolder + projnum + " Meeting Minutes.pdf"

    view = QWebEngineView()
    
    view.loadFinished.connect(export_to_pdf)
    view.setHtml(html)
    view.setWindowTitle("Test Report")
   
    if not QFile(project_pdfreportname).copy(pdfreportname):
        QMessageBox.critical(None, "Unable to copy generated export", "Could not copy " + pdfreportname + " to " + project_pdfreportname, QMessageBox.StandardButton.Cancel)   

    if keephtml == False:
        QFile.remove(htmlreportname)
    else:
        if not QFile(project_htmlreportname).copy(htmlreportname):
            QMessageBox.critical(None, "Unable to copy generated export", "Could not copy " + htmlreportname + " to " + project_htmlreportname, QMessageBox.StandardButton.Cancel)


    if ui.m_checkBoxDisplayNotes.isChecked():
        QDesktopServices.openUrl(QUrl("file:///" + project_pdfreportname))

    QFile.remove(pdfreportname)

    progbar.setValue(100)
    progbar.setLabelText("Finalizing Excel files...")

    progbar.hide()
    progbar.close()
    progbar = None # must be destroyed

    return ""



STOPPED HERE QtWebEngineWidgets must be installed with the installer

# setup test data
"""
import sys
print("Buld up QDomDocument")

app = QApplication(sys.argv)
xmldoc = QDomDocument("TestDocument")
f = QFile("C:/Users/pamcki/Desktop/project.xml")

if f.open(QIODevice.OpenModeFlag.ReadOnly):
    print("example project opened")
xmldoc.setContent(f)
f.close()

event_data_rightclick(xmldoc.toString())
"""
