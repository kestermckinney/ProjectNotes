// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include <QMainWindow>
#include <QTableView>
#include <QCoreApplication>
#include <QDir>

#include "spellchecker.h"

class AppSettings
{
public:
    AppSettings();
    ~AppSettings();
    void shutdown();

    void applyDeveloperProfile();

    static void setDeveloperProfile(const QString& profile);
    static QString developerProfile();
    static QString developerProfilePrefix();
    static QString settingsOrganization();
    static QString dataLocation();

    bool getSyncEnabled();
    void setSyncEnabled(bool enabled);
    QString getSyncEmail();
    void setSyncEmail(const QString& email);
    QString getSyncPassword();
    void setSyncPassword(const QString& password);
    QString getSyncEncryptionPhrase();
    void setSyncEncryptionPhrase(const QString& phrase);

    static void setTestSupabase(bool test);
    static bool isTestSupabase();

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

    qreal getZoomFactor();
    void setZoomFactor(qreal factor);

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

    SpellChecker* m_spellchecker = nullptr;

    static QString s_developerProfile;
    static bool s_testSupabase;
};

extern AppSettings global_Settings;

// Returns the directory containing app resources (dictionary, plugins, threads).
// Release builds on macOS run from inside .app/Contents/MacOS; resources live in .app/Contents/Resources.
// Debug builds run as a plain binary (no bundle); resources are symlinked next to the binary.
inline QString appResourcesPath()
{
#ifdef Q_OS_MAC
    QString bundleResources = QCoreApplication::applicationDirPath() + "/../Resources";
    if (QDir(bundleResources).exists())
        return bundleResources;
    return QCoreApplication::applicationDirPath();
#else
    return QCoreApplication::applicationDirPath();
#endif
}

#endif // APPSETTINGS_H
