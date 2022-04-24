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
    clientslistview.cpp \
    clientsmodel.cpp \
    clientspage.cpp \
    columnview.cpp \
    comboboxdelegate.cpp \
    filterdatadialog.cpp \
    itemdetailteamlistmodel.cpp \
    main.cpp \
    mainwindow.cpp \
    meetingattendeesmodel.cpp \
    noteactionitemsmodel.cpp \
    notesactionitemsmodel.cpp \
    peoplemodel.cpp \
    peoplepage.cpp \
    pnbasepage.cpp \
    pncolumnmodel.cpp \
    pncomboboxdelegate.cpp \
    pndatabaseobjects.cpp \
    pndateeditdelegate.cpp \
    pnsettings.cpp \
    pnsortfilterproxymodel.cpp \
    pnsqlquerymodel.cpp \
    pntableview.cpp \
    projectactionitemsmodel.cpp \
    projectlistview.cpp \
    projectlocationsmodel.cpp \
    projectnotesmodel.cpp \
    projectslistmodel.cpp \
    projectslistpage.cpp \
    projectsmodel.cpp \
    projectteammembersmodel.cpp \
    searchresultsmodel.cpp \
    statusreportitemsmodel.cpp \
    teamsmodel.cpp \
    trackeritemcommentsmodel.cpp \
    valueselectmodel.cpp \
    valuesview.cpp

HEADERS += \
    FilterSaveStructure.h \
    actionitemprojectnotesmodel.h \
    actionitemsdetailsmeetingsmodel.h \
    clientslistview.h \
    clientsmodel.h \
    clientspage.h \
    columnview.h \
    comboboxdelegate.h \
    filterdatadialog.h \
    itemdetailteamlistmodel.h \
    mainwindow.h \
    meetingattendeesmodel.h \
    noteactionitemsmodel.h \
    notesactionitemsmodel.h \
    peoplemodel.h \
    peoplepage.h \
    pnbasepage.h \
    pncolumnmodel.h \
    pncomboboxdelegate.h \
    pndatabaseobjects.h \
    pndateeditdelegate.h \
    pnsettings.h \
    pnsortfilterproxymodel.h \
    pnsqlquerymodel.h \
    pntableview.h \
    projectactionitemsmodel.h \
    projectlistview.h \
    projectlocationsmodel.h \
    projectnotesmodel.h \
    projectslistmodel.h \
    projectslistpage.h \
    projectsmodel.h \
    projectteammembersmodel.h \
    searchresultsmodel.h \
    statusreportitemsmodel.h \
    teamsmodel.h \
    trackeritemcommentsmodel.h \
    valueselectmodel.h \
    valuesview.h

FORMS += \
    filterdatadialog.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resources.qrc

DISTFILES += \
    database/ProjectNotes.db
