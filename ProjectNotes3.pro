QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

ICON = AppIcon.icns

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    actionitemprojectnotesmodel.cpp \
    actionitemsdetailsmeetingsmodel.cpp \
    clientsmodel.cpp \
    comboboxdelegate.cpp \
    main.cpp \
    mainwindow.cpp \
    meetingattendeesmodel.cpp \
    noteactionitemsmodel.cpp \
    peoplemodel.cpp \
    pncomboboxdelegate.cpp \
    pndatabaseobjects.cpp \
    pndateeditdelegate.cpp \
    pnsqlquerymodel.cpp \
    projectlocationsmodel.cpp \
    projectnotesmodel.cpp \
    projectsmodel.cpp \
    projectteammembersmodel.cpp \
    statusreportitemsmodel.cpp \
    teamsmodel.cpp

HEADERS += \
    actionitemprojectnotesmodel.h \
    actionitemsdetailsmeetingsmodel.h \
    clientsmodel.h \
    comboboxdelegate.h \
    mainwindow.h \
    meetingattendeesmodel.h \
    noteactionitemsmodel.h \
    peoplemodel.h \
    pncomboboxdelegate.h \
    pndatabaseobjects.h \
    pndateeditdelegate.h \
    pnsqlquerymodel.h \
    projectlocationsmodel.h \
    projectnotesmodel.h \
    projectsmodel.h \
    projectteammembersmodel.h \
    statusreportitemsmodel.h \
    teamsmodel.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resources.qrc

DISTFILES += \
    database/ProjectNotes.db
