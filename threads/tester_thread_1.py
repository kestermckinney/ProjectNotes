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

# All events return an xml string that can be processed by ProjectNotes.
# No thread events can receive an xml string.
# The entire module runs in its own thread.
#
# Supported Events

# def event_startup():
#     return
#
# def event_shutdown():
#     return
#
# def event_timer():
#     return
#

# Project Notes Plugin Events

def event_startup():

    print("Test Thread 1: Startup event called...")

    return ""

def event_shutdown():

    print("Test Thread 1: Shutdown event called...")

    return ""

def event_timer():

    for count in range(1, 300):
        time.sleep(0.05)
        print(f"Thread 1 - event minute counter {count}")

    print("Test Thread 1: Timer event called...")

    return ""
