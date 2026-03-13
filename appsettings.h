// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include <QMainWindow>
#include <QTableView>

#include "spellchecker.h"

class AppSettings
{
public:
    AppSettings();
    ~AppSettings();
    void shutdown();

    QVariant getPluginSetting(const QString& pluginName, const QString& parameterName);
    void setPluginSetting(const QString& pluginName, const QString& parameterName, const QString& parameterValue);
    bool getPluginEnabled(const QString& pluginName);
    void setPluginEnabled(const QString& pluginName, bool enabled);

    QVariant getLastDatabase();
    void setLastDatabase(const QString& lastDatabase);

    QVariant getWindowStateData(const QString& stateDataName);
    void setWindowStateData(const QString& stateDataName, const QVariant& data);

    void setWindowState(const QString& windowName, QWidget* window);
    bool getWindowState(const QString& windowName, QWidget* window);

    QString getDefaultDictionary();
    void setDefaultDictionary(const QString& dictionary);

    QString getPersonalDictionary();
    void setPersonalDictionary(const QString& dictionary);

    void setTableViewState(const QString& viewName, const QTableView& view);
    bool getTableViewState(const QString& viewName, QTableView& view);

    void setTableSortColumn(const QString& viewName, const int column, const QString direction);
    bool getTableSortColumn(const QString& viewName, int& column, QString& direction);
    int getStoredInt(const QString& valueName);
    void setStoredInt(const QString& valueName, int intValue);

    SpellChecker* spellchecker();

private:
    int getWindowX(const QString& windowName);
    int getWindowY(const QString& windowName);
    void setWindowX(const QString& windowName, int x);
    void setWindowY(const QString& windowName, int y);

    int getWindowWidth(const QString& windowName);
    int getWindowHeight(const QString& windowName);
    bool getWindowMaximized(const QString& windowName);

    void setWindowWidth(const QString& windowName, int idth);
    void setWindowHeight(const QString& windowName, int height);
    void setWindowMaximized(const QString& windowName, bool maximized);

    void setWindowStatusBar(const QString& windowName, bool statusBar);
    bool getWindowStatusBar(const QString& windowName);

private:
    QSettings* m_appConfig = nullptr;
    QSettings* m_pluginConfig = nullptr;

    SpellChecker* m_spellchecker = nullptr;
};

extern AppSettings global_Settings;

#endif // APPSETTINGS_H
