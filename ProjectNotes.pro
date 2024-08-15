QT       += core gui sql xml help \
    widgets

greaterThan(QT_MAJOR_VERSION, 6): QT += widgets

ICON = AppIcon.icns

win32: RC_ICONS = icons/logo.ico

# CONFIG += console
CONFIG += c++11

# Disable qDebug() output in release build
CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdialog.cpp \
    actionitemprojectnotesmodel.cpp \
    actionitemsdetailsmeetingsmodel.cpp \
    clientslistview.cpp \
    clientsmodel.cpp \
    clientspage.cpp \
    columnview.cpp \
    comboboxdelegate.cpp \
    databasestructure.cpp \
    filterdatadialog.cpp \
    findreplacedialog.cpp \
    itemdetailsdelegate.cpp \
    itemdetailspage.cpp \
    itemdetailteamlistmodel.cpp \
    main.cpp \
    mainwindow.cpp \
    meetingattendeesmodel.cpp \
    meetingattendeesview.cpp \
    notesactionitemsmodel.cpp \
    notesactionitemsview.cpp \
    peoplelistview.cpp \
    peoplemodel.cpp \
    peoplepage.cpp \
    pluginsettingsdialog.cpp \
    pnbasepage.cpp \
    pncheckboxdelegate.cpp \
    pncolumnmodel.cpp \
    pncombobox.cpp \
    pncomboboxdelegate.cpp \
    pnconsoledialog.cpp \
    pndatabaseobjects.cpp \
    pndateeditdelegate.cpp \
    pndateeditex.cpp \
    pninlinespellchecker.cpp \
    pnlineeditfilebutton.cpp \
    pnlineeditfilebuttondelegate.cpp \
    pnplaintextedit.cpp \
    pnplaintexteditdelegate.cpp \
    pnplugin.cpp \
    pnpluginmanager.cpp \
    pnsettings.cpp \
    pnsortfilterproxymodel.cpp \
    pnspellchecker.cpp \
    pnsqlquerymodel.cpp \
    pntableview.cpp \
    pntextedit.cpp \
    pntexteditdelegate.cpp \
    preferencesdialog.cpp \
    runguard.cpp \
    searchpage.cpp \
    searchresultsview.cpp \
    spellcheckdialog.cpp \
    trackeritemsmodel.cpp \
    trackeritemsview.cpp \
    projectdetailsdelegate.cpp \
    projectdetailspage.cpp \
    projectlocationsmodel.cpp \
    projectlocationsview.cpp \
    projectnotesdelegate.cpp \
    projectnotesmodel.cpp \
    projectnotespage.cpp \
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
    aboutdialog.h \
    actionitemprojectnotesmodel.h \
    actionitemsdetailsmeetingsmodel.h \
    clientslistview.h \
    clientsmodel.h \
    clientspage.h \
    columnview.h \
    comboboxdelegate.h \
    databasestructure.h \
    filterdatadialog.h \
    findreplacedialog.h \
    itemdetailsdelegate.h \
    itemdetailspage.h \
    itemdetailteamlistmodel.h \
    mainwindow.h \
    meetingattendeesmodel.h \
    meetingattendeesview.h \
    notesactionitemsmodel.h \
    notesactionitemsview.h \
    peoplelistview.h \
    peoplemodel.h \
    peoplepage.h \
    pluginsettingsdialog.h \
    pnbasepage.h \
    pncheckboxdelegate.h \
    pncolumnmodel.h \
    pncombobox.h \
    pncomboboxdelegate.h \
    pnconsoledialog.h \
    pndatabaseobjects.h \
    pndateeditdelegate.h \
    pndateeditex.h \
    pninlinespellchecker.h \
    pnlineeditfilebutton.h \
    pnlineeditfilebuttondelegate.h \
    pnplaintextedit.h \
    pnplaintexteditdelegate.h \
    pnplugin.h \
    pnpluginmanager.h \
    pnsettings.h \
    pnsortfilterproxymodel.h \
    pnspellchecker.h \
    pnsqlquerymodel.h \
    pntableview.h \
    pntextedit.h \
    pntexteditdelegate.h \
    preferencesdialog.h \
    runguard.h \
    searchpage.h \
    searchresultsview.h \
    spellcheckdialog.h \
    trackeritemsmodel.h \
    trackeritemsview.h \
    projectdetailsdelegate.h \
    projectdetailspage.h \
    projectlocationsmodel.h \
    projectlocationsview.h \
    projectnotesdelegate.h \
    projectnotesmodel.h \
    projectnotespage.h \
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
    aboutdialog.ui \
    filterdatadialog.ui \
    findreplacedialog.ui \
    mainwindow.ui \
    pluginsettingsdialog.ui \
    pnconsoledialog.ui \
    preferencesdialog.ui \
    spellcheckdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resources.qrc

DISTFILES += \
    database/ProjectNotes.db \
    dictionary/en_GB.aff \
    dictionary/en_GB.dic \
    dictionary/en_US.aff \
    dictionary/en_US.dic \
    dictionary/es_ANY.aff \
    dictionary/es_ANY.dic \
    dictionary/index.ini


unix: {
   LIBS += -lhunspell
   INCLUDEPATH += /usr/include/python3.12
   #INCLUDEPATH += /usr/include/linux/
   INCLUDEPATH += /usr/lib/gcc/x86_64-linux-gnu/12/include/
   #$(shell python3-config --includes)
   LIBS += -lpython3.12 -lm -L/usr/lib/python3.12/config
   #$(shell python3-config --ldflags)
}

win32: {
    CONFIG(debug,debug|release): {
        #debug
        DEFINES +=   WIN32 \
                    _WINDOWS \
                    _USRDLL \
                    HUNSPELL_STATIC \
                    _CRT_SECURE_NO_WARNINGS

       INCLUDEPATH += "C:/Users/Paul McKinney/Documents/hunspell/src"
       INCLUDEPATH +="C:/Program Files/Python311/include"
       LIBS += "C:\Users\Paul McKinney\Documents\hunspell\msvc\x64\Debug/libhunspell.lib"
       LIBS += "C:/Program Files/Python311/libs/python311.lib"
    }

    CONFIG(release,debug|release): {
        #release
        DEFINES +=   WIN32 \
                   _WINDOWS \
                   _USRDLL \
                   HUNSPELL_STATIC \
                   _CRT_SECURE_NO_WARNINGS

        INCLUDEPATH += "C:/Users/Paul McKinney/Documents/hunspell/src"
        INCLUDEPATH +="C:/Program Files/Python311/include"
        LIBS += "C:\Users\Paul McKinney\Documents\hunspell\msvc\x64\Release/libhunspell.lib"
        LIBS += "C:/Program Files/Python311/libs/python311.lib"
    }
}
