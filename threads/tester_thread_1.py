import sys
import platform
#import projectnotes
import time

from PyQt6.QtSql import QSqlDatabase
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
    for count in range(1, 300):
        time.sleep(0.05)
        #print(f"Thread 1 - event minute counter {count}")

    #print("Test Thread 1: Timer event called...")

    return ""

