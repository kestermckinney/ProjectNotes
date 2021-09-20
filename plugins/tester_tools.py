from PySide6 import QtSql, QtGui, QtCore
from PySide6.QtSql import QSqlDatabase
from PySide6.QtXml import QDomDocument, QDomNode
from PySide6.QtCore import QFile, QIODevice
from includes.common import ProjectNotesCommon
from PySide6.QtWidgets import QMessageBox, QMainWindow, QApplication
import sys
import re
app = QApplication(sys.argv)
#QMessageBox.warning(None, "Invalid Global Setting", "OraclePassword must be set in the Global Settigns plugin.", QMessageBox.Ok)

db = QSqlDatabase.addDatabase('QSQLITE')
db.setDatabaseName('plugintest.db')

if not db.open():
    QMessageBox.critical(None, "Cannot open database",
     "Unable to establish a database connection.\n"
        "This example needs SQLite support. Please read "
        "the Qt SQL driver documentation for information "
        "how to build it.\n\n" "Click Cancel to exit.",
     QMessageBox.Cancel)

pnc = ProjectNotesCommon()

print(pnc.to_xml("for < five"))
print(pnc.to_html("for < five"))

#print(pnc.find_projectfiles("P2334", "P:\\Active\\P2477_Cincinnati Childrens Hospital Inventory Management Solution\\Project Management\\Quote\\Equipment Tracking", "file desc", "type desc"))
print(pnc.find_projectlocations("P2477", "P:\\Active"))

doc = QDomDocument("TestDocument")
f = QFile("exampleproject.xml")

if f.open(QIODevice.ReadOnly):
    print("example project opened")
doc.setContent(f)
f.close()

projnotesxml = doc.elementsByTagName("projectnotes")
node = pnc.find_node(projnotesxml.at(0), "column", "number", "1")

if not node is None:
    print("Found Node: " + node.toElement().text())

print("Project Folder in XML:" + pnc.get_projectfolder(projnotesxml.at(0)))


node = pnc.find_node_by2(projnotesxml.at(0), "table", "filter_field", "project_id", "filter_value", "1598011616000493")

if not node is None:
    print("Found Node: " + node.nodeName())

node = pnc.find_node(projnotesxml.at(0), "row", "id", "1598011616000493")
if not node is None:
    print("Found Project Name: " + pnc.get_column_value(node, "project_name"))
    print("Found Client Name: " + pnc.get_column_value(node, "client_id"))

# xml creation functions
doc = pnc.xml_doc_root()
table = pnc.xml_table(doc, "test_table")
row = pnc.xml_row(doc)
col1 = pnc.xml_col(doc, "col1", "col 1 value", None)
col2 = pnc.xml_col(doc, "col2", "col 2 value", "look up")
row.appendChild(col1)
row.appendChild(col2)
table.appendChild(row)
doc.appendChild(table)

print(doc.toString())

print("Testing Com Items")
# WMI drive stuff
if pnc.ping_server("INDFP03.cornerstonecontrols.local"):
    print("Found Server")
else:
    print("Didn't Find Server")

#pnc.connect_drives()

#print("Test execute external program.")
#pnc.exec_program("notepad.exe")

print("Testing Configuration Settings")
print("Get Global Settings ProjectsFolder: " + pnc.get_global_setting("ProjectsFolder"))

pnc.verify_global_settings()

val = "2021-04-14 12:35:06+00:00[`!@#$%%%^&%*%(%)+\\|%[%]{}\"\';:/<>,%.~%?]"
print(re.sub(r"[`!@#$%^&*()+\\|{}/';:<>,.~?\"\]\[]", "", val))
print(re.sub(r"[-:+ ]", "", val))
