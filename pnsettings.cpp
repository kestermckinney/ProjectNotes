#include "pnsettings.h"
#include <QStatusBar>

PNSettings::PNSettings()
{
    m_AppConfig = new QSettings("ProjectNotes", "AppSettings");
    m_AppConfig->setFallbacksEnabled(false);

    m_PluginConfig = new QSettings("ProjectNotes", "PluginSettings");
    m_PluginConfig->setFallbacksEnabled(false);
}

PNSettings::~PNSettings()
{
    delete m_AppConfig;
    delete m_PluginConfig;
}

QVariant PNSettings::getPluginSetting(const QString& PluginName, const QString& ParameterName)
{
    QString path = PluginName + "/" + ParameterName;

    return m_PluginConfig->value(path);
}

bool PNSettings::getPluginEnabled(const QString& PluginName)
{
    QString path = PluginName + "/PluginEnabled";

    return m_PluginConfig->value(path).toBool();
}

void PNSettings::setPluginEnabled(const QString& PluginName, bool Enabled)
{
    QString path = PluginName + "/PluginEnabled";
    QVariant value = Enabled;

    m_PluginConfig->setValue(path, value);
}

QVariant PNSettings::getLastDatabase()
{
    return m_AppConfig->value("LastDatabaseUsed");
}

void PNSettings::setLastDatabase(const QString& LastDatabase)
{
    m_AppConfig->setValue("LastDatabaseUsed", LastDatabase);
}

QVariant PNSettings::getWindowStateData(const QString& StateDataName)
{
    return m_AppConfig->value(StateDataName);
}

void PNSettings::setWindowStateData(const QString& StateDataName, const QVariant& Data)
{
    m_AppConfig->setValue(StateDataName, Data);
}

void PNSettings::setWindowState(const QString& WindowName, const QMainWindow& Window)
{
    setWindowX(WindowName, Window.geometry().left());
    setWindowY(WindowName, Window.geometry().top());
    setWindowHeight(WindowName, Window.geometry().height());
    setWindowWidth(WindowName, Window.geometry().width());
    setWindowMaximized(WindowName, Window.isMaximized());
    setWindowStatusBar(WindowName, Window.statusBar()->isVisible());
}

bool PNSettings::getWindowState(const QString& WindowName, QMainWindow& Window)
{
    int x = getWindowX(WindowName);
    int y = getWindowY(WindowName);
    int w = getWindowWidth(WindowName);
    int h = getWindowHeight(WindowName);

    if ( x == -1 || y == -1 || w == -1 || h == -1)
         return false;

    Window.setGeometry(
                  getWindowX(WindowName),
                  getWindowY(WindowName),
                  getWindowWidth(WindowName),
                  getWindowHeight(WindowName)
                );

    return true;
}

void PNSettings::setTableViewState(const QString& ViewName, const QTableView& View)
{
    int c = View.model()->columnCount();
    int w;
    QString savestring;

    for (int i = 0; i < c; i++)
    {
        w = View.columnWidth(i);
        savestring += QString("%1,").arg(w);
    }

    m_AppConfig->setValue(ViewName, savestring);
}

bool PNSettings::getTableViewState(const QString& ViewName, QTableView& View)
{
    QVariant loadstring;

    loadstring = m_AppConfig->value(ViewName);

    QStringList lst = loadstring.toString().split(",");

    int col = 0;
    int c = View.model()->columnCount();

    for ( auto& i : lst  )
    {
        if (col < c)
            View.setColumnWidth(col, i.toInt());

        col++;
    }

    return true;
}


int PNSettings::getWindowX(const QString& WindowName)
{
    QString path = WindowName + "/X";

    return m_AppConfig->value(path, -1).toInt();
}

int PNSettings::getWindowY(const QString& WindowName)
{
    QString path = WindowName + "/Y";

    return m_AppConfig->value(path, -1).toInt();
}

void PNSettings::setWindowX(const QString& WindowName, int X)
{
    QString path = WindowName + "/X";
    QVariant value = X;

    m_AppConfig->setValue(path, value);
}

void PNSettings::setWindowY(const QString& WindowName, int Y)
{
    QString path = WindowName + "/Y";
    QVariant value = Y;

    m_AppConfig->setValue(path, value);
}

int PNSettings::getWindowWidth(const QString& WindowName)
{
    QString path = WindowName + "/Width";

    return m_AppConfig->value(path, -1).toInt();
}

int PNSettings::getWindowHeight(const QString& WindowName)
{
    QString path = WindowName + "/Height";

    return m_AppConfig->value(path, -1).toInt();
}

bool PNSettings::getWindowMaximized(const QString& WindowName)
{
    QString path = WindowName + "/Maximized";

    return m_AppConfig->value(path, 0).toBool();
}

bool PNSettings::getWindowStatusBar(const QString& WindowName)
{
    QString path = WindowName + "/StatusBar";

    return m_AppConfig->value(path, 1).toBool();
}

void PNSettings::setWindowWidth(const QString& WindowName, int Width)
{
    QString path = WindowName + "/Width";
    QVariant value = Width;

    m_AppConfig->setValue(path, value);
}

void PNSettings::setWindowHeight(const QString& WindowName, int Height)
{
    QString path = WindowName + "/Height";
    QVariant value = Height;

    m_AppConfig->setValue(path, value);
}

void PNSettings::setWindowMaximized(const QString& WindowName, bool Maximized)
{
    QString path = WindowName + "/Maximized";
    QVariant value = Maximized;

    m_AppConfig->setValue(path, value);
}

void PNSettings::setWindowStatusBar(const QString& WindowName, bool StatusBar)
{
    QString path = WindowName + "/StatusBar";
    QVariant value = StatusBar;

    m_AppConfig->setValue(path, value);
}

QString PNSettings::getDefaultDictionary()
{
    QString path = "DefaultDictionary";

    return m_AppConfig->value(path).toString();
}

void PNSettings::setDefaultDictionary(const QString& Dictionary)
{
    QString path = "DefaultDictionary";
    QVariant value = Dictionary;

    m_AppConfig->setValue(path, value);
}

QString PNSettings::getPersonalDictionary()
{
    QString path = "PersonalDictionary";

    return m_AppConfig->value(path).toString();
}

void PNSettings::setPersonalDictionary(const QString& Dictionary)
{
    QString path = "PersonalDictionary";
    QVariant value = Dictionary;

    m_AppConfig->setValue(path, value);
}
