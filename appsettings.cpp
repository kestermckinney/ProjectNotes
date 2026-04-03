// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "appsettings.h"
#include "mainwindow.h"

#include <QStatusBar>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

QString AppSettings::s_developerProfile;

void AppSettings::setDeveloperProfile(const QString& profile)
{
    s_developerProfile = profile;
}

QString AppSettings::developerProfile()
{
    return s_developerProfile;
}

QString AppSettings::settingsOrganization()
{
    if (s_developerProfile.isEmpty())
        return QStringLiteral("ProjectNotes");
    return QStringLiteral("ProjectNotes") + s_developerProfile;
}

AppSettings::AppSettings()
{
    m_appConfig = new QSettings(settingsOrganization(), "AppSettings");
    m_appConfig->setFallbacksEnabled(false);
}

void AppSettings::applyDeveloperProfile()
{
    if (!s_developerProfile.isEmpty())
    {
        delete m_appConfig;
        m_appConfig = new QSettings(settingsOrganization(), "AppSettings");
        m_appConfig->setFallbacksEnabled(false);
    }
}

AppSettings global_Settings;

AppSettings::~AppSettings()
{
    // shutdown() should have been called while QApplication was alive.
    // These deletes are safe no-ops if shutdown() already ran.
    delete m_appConfig;
    m_appConfig = nullptr;
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
}

SpellChecker* AppSettings::spellchecker()
{
    if (m_spellchecker == nullptr)
        m_spellchecker = new SpellChecker();

    return m_spellchecker;
}

bool AppSettings::getSyncEnabled()
{
    return m_appConfig->value("Sync/Enabled", false).toBool();
}

void AppSettings::setSyncEnabled(bool enabled)
{
    m_appConfig->setValue("Sync/Enabled", enabled);
}

int AppSettings::getSyncHostType()
{
    return m_appConfig->value("Sync/HostType", 0).toInt();
}

void AppSettings::setSyncHostType(int type)
{
    m_appConfig->setValue("Sync/HostType", type);
}

QString AppSettings::getSyncPostgrestUrl()
{
    return m_appConfig->value("Sync/PostgrestUrl").toString();
}

void AppSettings::setSyncPostgrestUrl(const QString& url)
{
    m_appConfig->setValue("Sync/PostgrestUrl", url);
}

QString AppSettings::getSyncEmail()
{
    return m_appConfig->value("Sync/Email").toString();
}

void AppSettings::setSyncEmail(const QString& email)
{
    m_appConfig->setValue("Sync/Email", email);
}

QString AppSettings::getSyncPassword()
{
    return m_appConfig->value("Sync/Password").toString();
}

void AppSettings::setSyncPassword(const QString& password)
{
    m_appConfig->setValue("Sync/Password", password);
}

QString AppSettings::getSyncSupabaseKey()
{
    return m_appConfig->value("Sync/SupabaseKey").toString();
}

void AppSettings::setSyncSupabaseKey(const QString& key)
{
    m_appConfig->setValue("Sync/SupabaseKey", key);
}

QString AppSettings::getSyncEncryptionPhrase()
{
    return m_appConfig->value("Sync/EncryptionPhrase").toString();
}

void AppSettings::setSyncEncryptionPhrase(const QString& phrase)
{
    m_appConfig->setValue("Sync/EncryptionPhrase", phrase);
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
    int x = window->geometry().left();
    int y = window->geometry().top();
    int w = window->geometry().width();
    int h = window->geometry().height();
    bool maximized = window->isMaximized();

#ifdef QT_DEBUG
    qDebug() << QString("setWindowState: %1 x=%2 y=%3 w=%4 h=%5 maximized=%6 settingsFile=%7")
        .arg(windowName).arg(x).arg(y).arg(w).arg(h).arg(maximized)
        .arg(m_appConfig->fileName());
#endif

    setWindowX(windowName, x);
    setWindowY(windowName, y);
    if (h > 0) setWindowHeight(windowName, h);
    if (w > 0) setWindowWidth(windowName, w);
    setWindowMaximized(windowName, maximized);
    if (window->objectName() == "MainWindow")
        setWindowStatusBar(windowName, ((MainWindow*)window)->statusBar()->isVisibleTo(window));

    m_appConfig->sync();
#ifdef QT_DEBUG
    qDebug() << QString("setWindowState: sync complete, status=%1").arg(m_appConfig->status());
#endif
}

bool AppSettings::getWindowState(const QString& windowName, QWidget* window)
{
    int x = getWindowX(windowName);
    int y = getWindowY(windowName);
    int w = getWindowWidth(windowName);
    int h = getWindowHeight(windowName);

#ifdef QT_DEBUG
    qDebug() << QString("getWindowState: %1 x=%2 y=%3 w=%4 h=%5 settingsFile=%6")
        .arg(windowName).arg(x).arg(y).arg(w).arg(h)
        .arg(m_appConfig->fileName());
#endif

    if ( x == -1 || y == -1 || w == -1 || h == -1)
    {
#ifdef QT_DEBUG
        qDebug() << QString("getWindowState: %1 no saved state found, using defaults").arg(windowName);
#endif
        return false;
    }

    window->resize(w, h);
    window->move(x, y);

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

#ifdef QT_DEBUG
    qDebug() << QString("setTableViewState: %1 columns=%2 settingsFile=%3")
        .arg(viewName).arg(savestring).arg(m_appConfig->fileName());
#endif
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

#ifdef QT_DEBUG
    qDebug() << QString("getTableViewState: %1 raw=%2 settingsFile=%3")
        .arg(viewName).arg(loadstring.toString()).arg(m_appConfig->fileName());
#endif

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


