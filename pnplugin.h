// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PNPLUGIN_H
#define PNPLUGIN_H

#include <QString>
#include <QStringList>
#include <QDomDocument>
#include <QFileInfo>
#include <QObject>
#include <QAction>

#pragma push_macro("slots")
#undef slots
#undef _DEBUG //Prevent linking debug build of python
#include "Python.h"
#pragma pop_macro("slots")


class PNPlugin
{

public:
    bool loadModule(const QFileInfo& t_filename);
    ~PNPlugin();
    bool hasStartupEvent() { return (m_Startup != nullptr);};
    bool hasShutdownEvent() { return (m_Shutdown != nullptr);};
    bool hasEveryMinuteEvent() { return (m_EveryMinute != nullptr);};
    bool hasEvery5MinutesEvent() { return (m_Every5Minutes != nullptr);};
    bool hasEvery10MinutesEvent() { return (m_Every10Minutes != nullptr);};
    bool hasEvery15MinutesEvent() { return (m_Every15Minutes != nullptr);};
    bool hasEvery30MinutesEvent() { return (m_Every30Minutes != nullptr);};
    bool hasPNPluginMenuEvent() { return (m_PNPluginMenu != nullptr);};
    bool hasDataRightClickEvent(const QString& t_tablename);
    QString callStartupEvent(const QString& t_xmlstring);
    QString callShutdownEvent(const QString& t_xmlstring);
    QString callEveryMinuteEvent(const QString& t_xmlstring);
    QString callEvery5MinutesEvent(const QString& t_xmlstring);
    QString callEvery10MinutesEvent(const QString& t_xmlstring);
    QString callEvery15MinutesEvent(const QString& t_xmlstring);
    QString callEvery30MinutesEvent(const QString& t_xmlstring);
    QString callPNPluginMenuEvent(const QString& t_xmlstring);
    QString callDataRightClickEvent(const QString& t_xmlstring);
    QString getPNPluginName() { return m_PNPluginName; }
    QString getPNPluginDescription() { return m_Description; }
    bool isEnabled() { return m_IsEnabled; }
    void setEnabled(bool t_enable);

    QStringList getEventNames();
    QStringList getParameterNames() { return m_Parameters;}
    QString getPluginLocation() { return m_PluginLocation;}
    QString getTableName() { return m_TableName; }
    QString getChildTablesFilter() { return m_ChildTablesFilter; }
    QString getSubmenu() { return m_Submenu; }


private:
    QString m_PluginLocation;
    QString m_PNPluginName;
    QString m_Submenu; // the submenu to display the plugin in
    QString m_Description;
    QString m_TableName;
    QString m_ChildTablesFilter; // "projects:meeting_notes:action_items" this allows for a smaller projects record export

    PyObject* m_PNPluginModule;
    QStringList m_Parameters;
    bool m_IsEnabled = true;

    // these events should happen from the main window
    // they don't take any parameters
    PyObject* m_Startup = nullptr;
    PyObject* m_Shutdown = nullptr;
    PyObject* m_EveryMinute = nullptr;
    PyObject* m_Every5Minutes = nullptr;
    PyObject* m_Every10Minutes = nullptr;
    PyObject* m_Every15Minutes = nullptr;
    PyObject* m_Every30Minutes = nullptr;
    PyObject* m_PNPluginMenu = nullptr;
    PyObject*  m_DataRightClickEvent = nullptr;

    QString callPythonMethod(PyObject* t_method);
    QString callPythonMethod(PyObject* t_method, const QString& t_xmlstring);
    int setPythonVariable(const QString& t_variablename, const QString& t_value);
    QString getPythonVariable(const QString& t_variablename);
    QStringList getPythonStringList(const QString& t_variablename);
};

#endif // PNPLUGIN_H
