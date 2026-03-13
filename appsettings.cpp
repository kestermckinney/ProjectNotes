// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "appsettings.h"
#include "mainwindow.h"

#include <QStatusBar>

AppSettings::AppSettings()
{
    m_appConfig = new QSettings("ProjectNotes", "AppSettings");
    m_appConfig->setFallbacksEnabled(false);

    m_pluginConfig = new QSettings("ProjectNotes", "PluginSettings");
    m_pluginConfig->setFallbacksEnabled(false);
}

AppSettings global_Settings;

AppSettings::~AppSettings()
{
    // shutdown() should have been called while QApplication was alive.
    // These deletes are safe no-ops if shutdown() already ran.
    delete m_appConfig;
    m_appConfig = nullptr;
    delete m_pluginConfig;
    m_pluginConfig = nullptr;

    if (m_spellchecker)
        delete m_spellchecker;
}

void AppSettings::shutdown()
{
    // Called from main() while QApplication is still alive so QSettings::sync() works.
    if (m_appConfig) {
        m_appConfig->sync();
        delete m_appConfig;
        m_appConfig = nullptr;
    }
    if (m_pluginConfig) {
        m_pluginConfig->sync();
        delete m_pluginConfig;
        m_pluginConfig = nullptr;
    }
}

SpellChecker* AppSettings::spellchecker()
{
    if (m_spellchecker == nullptr)
        m_spellchecker = new SpellChecker();

    return m_spellchecker;
}

QVariant AppSettings::getPluginSetting(const QString& pluginName, const QString& parameterName)
{
    QString path = pluginName + "/" + parameterName;

    return m_pluginConfig->value(path);
}

void AppSettings::setPluginSetting(const QString& pluginName, const QString& parameterName, const QString& parameterValue)
{
    QString path = pluginName + "/" + parameterName;

    m_pluginConfig->setValue(path, parameterValue);
}

bool AppSettings::getPluginEnabled(const QString& pluginName)
{
    QString path = pluginName + "/PluginEnabled";

    QVariant val = m_pluginConfig->value(path);

    // default to all plugins being enabled when first loaded
    if (val.isNull()) val = "1";

    return val.toBool();
}

void AppSettings::setPluginEnabled(const QString& pluginName, bool enabled)
{
    QString path = pluginName + "/PluginEnabled";
    QVariant value =enabled;

    m_pluginConfig->setValue(path, value);
}

QVariant AppSettings::getLastDatabase()
{
    return m_appConfig->value("LastDatabaseUsed");
}

void AppSettings::setLastDatabase(const QString& lastDatabase)
{
    m_appConfig->setValue("LastDatabaseUsed", lastDatabase);
}

QVariant AppSettings::getWindowStateData(const QString& stateDataName)
{
    return m_appConfig->value(stateDataName);
}

void AppSettings::setWindowStateData(const QString& stateDataName, const QVariant& data)
{
    m_appConfig->setValue(stateDataName, data);
}

void AppSettings::setWindowState(const QString& windowName, QWidget* window)
{
    setWindowX(windowName, window->geometry().left());
    setWindowY(windowName, window->geometry().top());
    if (window->geometry().height() > 0) setWindowHeight(windowName, window->geometry().height());
    if (window->geometry().width() > 0) setWindowWidth(windowName, window->geometry().width());
    setWindowMaximized(windowName, window->isMaximized());
    if (window->objectName() == "MainWindow")
        setWindowStatusBar(windowName, ((MainWindow*)window)->statusBar()->isVisibleTo(window));
}

bool AppSettings::getWindowState(const QString& windowName, QWidget* window)
{
    int x = getWindowX(windowName);
    int y = getWindowY(windowName);
    int w = getWindowWidth(windowName);
    int h = getWindowHeight(windowName);

    if ( x == -1 || y == -1 || w == -1 || h == -1)
         return false;

    window->setGeometry(
                  getWindowX(windowName),
                  getWindowY(windowName),
                  getWindowWidth(windowName),
                  getWindowHeight(windowName)
                );

    if (window->objectName() == "MainWindow")
        ((MainWindow*)window)->statusBar()->setVisible(getWindowStatusBar(windowName));

    return true;
}

void AppSettings::setTableViewState(const QString& viewName, const QTableView& view)
{
    // don't try and save when the model has been removed
    if (!view.model())
        return;

    int c = view.model()->columnCount();
    int w;
    QString savestring;

    for (int i = 0; i < c; i++)
    {
        w = view.columnWidth(i);

        savestring += QString("%1,").arg(w);
    }

    m_appConfig->setValue(viewName + "Columns", savestring);
}

void AppSettings::setTableSortColumn(const QString& viewName, const int column, const QString direction)
{
    QString savestring;

    savestring = QString("%1").arg(column);

    m_appConfig->setValue(viewName + "SortColumn", savestring);
    m_appConfig->setValue(viewName + "SortDirection", direction);
}

bool AppSettings::getTableSortColumn(const QString& viewName, int& column, QString& direction)
{
    QVariant loadstring;

    loadstring = m_appConfig->value(viewName + "SortColumn");
    direction = m_appConfig->value(viewName + "SortDirection").toString();

    if (!loadstring.isValid() || loadstring == "")
        column = -1;
    else
        column = loadstring.toInt();

    return true;
}

bool AppSettings::getTableViewState(const QString& viewName, QTableView& view)
{
    QVariant loadstring;

    loadstring = m_appConfig->value(viewName + "Columns");

    QStringList lst = loadstring.toString().split(",");

    int col = 0;
    int c = view.model()->columnCount();

    for ( auto& i : lst  )
    {
        if (col < c)
            view.setColumnWidth(col, i.toInt());

        col++;
    }

    return true;
}

int AppSettings::getStoredInt(const QString& valueName)
{
    QString path = valueName;

    return m_appConfig->value(path, -1).toInt();
}


int AppSettings::getWindowX(const QString& windowName)
{
    QString path = windowName + "/X";

    return m_appConfig->value(path, -1).toInt();
}

int AppSettings::getWindowY(const QString& windowName)
{
    QString path = windowName + "/Y";

    return m_appConfig->value(path, -1).toInt();
}

void AppSettings::setStoredInt(const QString& valueName, int intVal)
{
    QString path = valueName;
    QVariant value = intVal;

    m_appConfig->setValue(path, value);
}

void AppSettings::setWindowX(const QString& windowName, int X)
{
    QString path = windowName + "/X";
    QVariant value = X;

    m_appConfig->setValue(path, value);
}

void AppSettings::setWindowY(const QString& windowName, int Y)
{
    QString path = windowName + "/Y";
    QVariant value = Y;

    m_appConfig->setValue(path, value);
}

int AppSettings::getWindowWidth(const QString& windowName)
{
    QString path = windowName + "/Width";

    return m_appConfig->value(path, -1).toInt();
}

int AppSettings::getWindowHeight(const QString& windowName)
{
    QString path = windowName + "/Height";

    return m_appConfig->value(path, -1).toInt();
}

bool AppSettings::getWindowMaximized(const QString& windowName)
{
    QString path = windowName + "/Maximized";

    if (m_appConfig->value(path, QString("1")) == QString("1"))
        return true;
    else
        return false;
}

bool AppSettings::getWindowStatusBar(const QString& windowName)
{
    QString path = windowName + "/StatusBar";

    if (m_appConfig->value(path, QString("1")) == QString("1"))
        return true;
    else
        return false;
}

void AppSettings::setWindowWidth(const QString& windowName, int width)
{
    QString path = windowName + "/Width";
    QVariant value = width;

    m_appConfig->setValue(path, value);
}

void AppSettings::setWindowHeight(const QString& windowName, int height)
{
    QString path = windowName + "/Height";
    QVariant value = height;

    m_appConfig->setValue(path, value);
}

void AppSettings::setWindowMaximized(const QString& windowName, bool maximized)
{
    QString path = windowName + "/Maximized";
    QVariant value = maximized ? QString("1") : QString("0");

    m_appConfig->setValue(path, value);
}

void AppSettings::setWindowStatusBar(const QString& windowName, bool statusBar)
{
    QString path = windowName + "/StatusBar";
    QVariant value = statusBar ? QString("1") : QString("0");

    m_appConfig->setValue(path, value);
}

QString AppSettings::getDefaultDictionary()
{
    QString path = "DefaultDictionary";

    return m_appConfig->value(path).toString();
}

void AppSettings::setDefaultDictionary(const QString& dictionary)
{
    QString path = "DefaultDictionary";
    QVariant value = dictionary;

    m_appConfig->setValue(path, value);
}

QString AppSettings::getPersonalDictionary()
{
    QString path = "PersonalDictionary";

    return m_appConfig->value(path).toString();
}

void AppSettings::setPersonalDictionary(const QString& dictionary)
{
    QString path = "PersonalDictionary";
    QVariant value = dictionary;

    m_appConfig->setValue(path, value);
}


