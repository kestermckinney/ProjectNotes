QT       += core gui sql xml

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
    itemdetailteamlistmodel.cpp \
    main.cpp \
    mainwindow.cpp \
    meetingattendeesmodel.cpp \
    noteactionitemsmodel.cpp \
    notesactionitemsmodel.cpp \
    peoplemodel.cpp \
    pncomboboxdelegate.cpp \
    pndatabaseobjects.cpp \
    pndateeditdelegate.cpp \
    pnsettings.cpp \
    pnsortfilterproxymodel.cpp \
    pnsqlquerymodel.cpp \
    projectactionitemsmodel.cpp \
    projectlocationsmodel.cpp \
    projectnotesmodel.cpp \
    projectslistmodel.cpp \
    projectsmodel.cpp \
    projectteammembersmodel.cpp \
    searchresultsmodel.cpp \
    statusreportitemsmodel.cpp \
    teamsmodel.cpp \
    trackeritemcommentsmodel.cpp

HEADERS += \
    actionitemprojectnotesmodel.h \
    actionitemsdetailsmeetingsmodel.h \
    clientsmodel.h \
    comboboxdelegate.h \
    itemdetailteamlistmodel.h \
    mainwindow.h \
    meetingattendeesmodel.h \
    noteactionitemsmodel.h \
    notesactionitemsmodel.h \
    peoplemodel.h \
    pncomboboxdelegate.h \
    pndatabaseobjects.h \
    pndateeditdelegate.h \
    pnsettings.h \
    pnsortfilterproxymodel.h \
    pnsqlquerymodel.h \
    projectactionitemsmodel.h \
    projectlocationsmodel.h \
    projectnotesmodel.h \
    projectslistmodel.h \
    projectsmodel.h \
    projectteammembersmodel.h \
    searchresultsmodel.h \
    statusreportitemsmodel.h \
    teamsmodel.h \
    trackeritemcommentsmodel.h

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
