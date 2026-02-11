import sys
import platform
import projectnotes
import time


from PyQt6.QtXml import QDomDocument, QDomNode
from PyQt6.QtCore import QFile, QIODevice, QDateTime, QUrl, QThread
from PyQt6.QtGui import QDesktopServices


# Project Notes Plugin Parameters
pluginname = "Testing Thread 1" # name used in the menu
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

# Project Notes Plugin Events

def event_startup(parameter):
    #print("Test Thread 1: Startup event called...")

    return ""

def event_shutdown(parameter):
    #print("Test Thread 1: Shutdown event called...")

    return ""

def event_timer(parameter):

    #print("Test Thread 1: Timer event called...")

    docxml = "<projectnotes>\n"
    docxml += "<table name=\"people\">\n"
    docxml += "  <row>" # id=\"99.9\">\n"
    #docxml += "    <column name=\"people_id\">99.9</column>\n"
    docxml += "    <column name=\"name\">Able To Test</column>\n"
    docxml += "    <column name=\"role\">Time Is " + QDateTime.currentDateTime().toString() + "</column>\n"
    docxml += "    <column name=\"client_id\" lookupvalue=\"Cornerstone Controls, Inc.\"></column>\n"
    docxml += "  </row>\n"
    docxml += "</table>\n"
    docxml += "</projectnotes>\n"

    #print(docxml)

    #projectnotes.update_data(docxml) # need to test and see if screen will refresh automatically

    #time.sleep(6)

    return ""

