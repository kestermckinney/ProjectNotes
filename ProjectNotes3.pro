QT       += core gui sql xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

ICON = AppIcon.icns

CONFIG += c++11
CONFIG += console

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
    itemdetailsdelegate.cpp \
    itemdetailspage.cpp \
    itemdetailteamlistmodel.cpp \
    main.cpp \
    mainwindow.cpp \
    meetingattendeesmodel.cpp \
    noteactionitemsmodel.cpp \
    notesactionitemsmodel.cpp \
    peoplelistview.cpp \
    peoplemodel.cpp \
    peoplepage.cpp \
    pnbasepage.cpp \
    pncheckboxdelegate.cpp \
    pncolumnmodel.cpp \
    pncomboboxdelegate.cpp \
    pndatabaseobjects.cpp \
    pndateeditdelegate.cpp \
    pndateeditex.cpp \
    pnlineeditfilebutton.cpp \
    pnlineeditfilebuttondelegate.cpp \
    pnsettings.cpp \
    pnsortfilterproxymodel.cpp \
    pnsqlquerymodel.cpp \
    pntableview.cpp \
    preferencesdialog.cpp \
    projectactionitemsmodel.cpp \
    projectactionitemsview.cpp \
    projectdetailsdelegate.cpp \
    projectdetailspage.cpp \
    projectlocationsmodel.cpp \
    projectlocationsview.cpp \
    projectnotesmodel.cpp \
    projectnotesview.cpp \
    projectslistmodel.cpp \
    projectslistpage.cpp \
    projectslistview.cpp \
    projectsmodel.cpp \
    projectteammembersmodel.cpp \
    projectteammembersview.cpp \
    searchresultsmodel.cpp \
    statusreportitemsmodel.cpp \
    statusreportitemsview.cpp \
    teamsmodel.cpp \
    trackeritemcommentsmodel.cpp \
    trackeritemcommentsview.cpp \
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
    itemdetailsdelegate.h \
    itemdetailspage.h \
    itemdetailteamlistmodel.h \
    mainwindow.h \
    meetingattendeesmodel.h \
    noteactionitemsmodel.h \
    notesactionitemsmodel.h \
    peoplelistview.h \
    peoplemodel.h \
    peoplepage.h \
    pnbasepage.h \
    pncheckboxdelegate.h \
    pncolumnmodel.h \
    pncomboboxdelegate.h \
    pndatabaseobjects.h \
    pndateeditdelegate.h \
    pndateeditex.h \
    pnlineeditfilebutton.h \
    pnlineeditfilebuttondelegate.h \
    pnsettings.h \
    pnsortfilterproxymodel.h \
    pnsqlquerymodel.h \
    pntableview.h \
    preferencesdialog.h \
    projectactionitemsmodel.h \
    projectactionitemsview.h \
    projectdetailsdelegate.h \
    projectdetailspage.h \
    projectlocationsmodel.h \
    projectlocationsview.h \
    projectnotesmodel.h \
    projectnotesview.h \
    projectslistmodel.h \
    projectslistpage.h \
    projectslistview.h \
    projectsmodel.h \
    projectteammembersmodel.h \
    projectteammembersview.h \
    searchresultsmodel.h \
    statusreportitemsmodel.h \
    statusreportitemsview.h \
    teamsmodel.h \
    trackeritemcommentsmodel.h \
    trackeritemcommentsview.h \
    valueselectmodel.h \
    valuesview.h \
    widgets_export.h

FORMS += \
    filterdatadialog.ui \
    mainwindow.ui \
    preferencesdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resources.qrc

DISTFILES += \
    database/ProjectNotes.db
