#include "pnsettings.h"
#include "mainwindow.h"

#include <QStatusBar>

PNSettings::PNSettings()
{
    m_app_config = new QSettings("ProjectNotes", "AppSettings");
    m_app_config->setFallbacksEnabled(false);

    m_plugin_config = new QSettings("ProjectNotes", "PluginSettings");
    m_plugin_config->setFallbacksEnabled(false);
}

PNSettings::~PNSettings()
{
    delete m_app_config;
    delete m_plugin_config;
}

QVariant PNSettings::getPluginSetting(const QString& t_plugin_name, const QString& t_parameter_name)
{
    QString path = t_plugin_name + "/" + t_parameter_name;

    return m_plugin_config->value(path);
}

void PNSettings::setPluginSetting(const QString& t_plugin_name, const QString& t_parameter_name, const QString& t_parameter_value)
{
    QString path = t_plugin_name + "/" + t_parameter_name;

    m_plugin_config->setValue(path, t_parameter_value);
}

bool PNSettings::getPluginEnabled(const QString& t_plugin_name)
{
    QString path = t_plugin_name + "/PluginEnabled";

    QVariant val = m_plugin_config->value(path);

    // default to all plugins being enabled when first loaded
    if (val.isNull()) val = "1";

    return val.toBool();
}

void PNSettings::setPluginEnabled(const QString& t_plugin_name, bool t_enabled)
{
    QString path = t_plugin_name + "/PluginEnabled";
    QVariant t_value =t_enabled;

    m_plugin_config->setValue(path, t_value);
}

QVariant PNSettings::getLastDatabase()
{
    return m_app_config->value("LastDatabaseUsed");
}

void PNSettings::setLastDatabase(const QString& t_last_database)
{
    m_app_config->setValue("LastDatabaseUsed", t_last_database);
}

QVariant PNSettings::getWindowStateData(const QString& t_state_data_name)
{
    return m_app_config->value(t_state_data_name);
}

void PNSettings::setWindowStateData(const QString& t_state_data_name, const QVariant& t_data)
{
    m_app_config->setValue(t_state_data_name, t_data);
}

void PNSettings::setWindowState(const QString& t_window_name, const QWidget& t_window)
{
    setWindowX(t_window_name, t_window.geometry().left());
    setWindowY(t_window_name, t_window.geometry().top());
    if (t_window.geometry().height() > 0) setWindowHeight(t_window_name, t_window.geometry().height());
    if (t_window.geometry().width() > 0) setWindowWidth(t_window_name, t_window.geometry().width());
    setWindowMaximized(t_window_name, t_window.isMaximized());
    if (t_window.objectName() == "MainWindow")
        setWindowStatusBar(t_window_name, ((MainWindow&)t_window).statusBar()->isVisibleTo(&t_window));
}

bool PNSettings::getWindowState(const QString& t_window_name, QWidget& t_window)
{
    int x = getWindowX(t_window_name);
    int y = getWindowY(t_window_name);
    int w = getWindowWidth(t_window_name);
    int h = getWindowHeight(t_window_name);

    if ( x == -1 || y == -1 || w == -1 || h == -1)
         return false;

    t_window.setGeometry(
                  getWindowX(t_window_name),
                  getWindowY(t_window_name),
                  getWindowWidth(t_window_name),
                  getWindowHeight(t_window_name)
                );

    if (t_window.objectName() == "MainWindow")
        ((MainWindow&)t_window).statusBar()->setVisible(getWindowStatusBar(t_window_name));

    return true;
}

void PNSettings::setTableViewState(const QString& t_view_name, const QTableView& t_view)
{
    // don't try and save when the model has been removed
    if (!t_view.model())
        return;

    int c = t_view.model()->columnCount();
    int w;
    QString savestring;

    for (int i = 0; i < c; i++)
    {
        w = t_view.columnWidth(i);

        savestring += QString("%1,").arg(w);
    }

    m_app_config->setValue(t_view_name + "Columns", savestring);
}

void PNSettings::setTableSortColumn(const QString& t_view_name, const int t_column, const QString t_direction)
{
    QString savestring;

    savestring = QString("%1").arg(t_column);

    m_app_config->setValue(t_view_name + "SortColumn", savestring);
    m_app_config->setValue(t_view_name + "SortDirection", t_direction);
}

bool PNSettings::getTableSortColumn(const QString& t_view_name, int& t_column, QString& t_direction)
{
    QVariant loadstring;

    loadstring = m_app_config->value(t_view_name + "SortColumn");
    t_direction = m_app_config->value(t_view_name + "SortDirection").toString();

    if (!loadstring.isValid() || loadstring == "")
        t_column = -1;
    else
        t_column = loadstring.toInt();

    return true;
}

bool PNSettings::getTableViewState(const QString& t_view_name, QTableView& t_view)
{
    QVariant loadstring;

    loadstring = m_app_config->value(t_view_name + "Columns");

    QStringList lst = loadstring.toString().split(",");

    int col = 0;
    int c = t_view.model()->columnCount();

    for ( auto& i : lst  )
    {
        if (col < c)
            t_view.setColumnWidth(col, i.toInt());

        col++;
    }

    return true;
}


int PNSettings::getWindowX(const QString& t_window_name)
{
    QString path = t_window_name + "/X";

    return m_app_config->value(path, -1).toInt();
}

int PNSettings::getWindowY(const QString& t_window_name)
{
    QString path = t_window_name + "/Y";

    return m_app_config->value(path, -1).toInt();
}

void PNSettings::setWindowX(const QString& t_window_name, int X)
{
    QString path = t_window_name + "/X";
    QVariant t_value = X;

    m_app_config->setValue(path, t_value);
}

void PNSettings::setWindowY(const QString& t_window_name, int Y)
{
    QString path = t_window_name + "/Y";
    QVariant t_value = Y;

    m_app_config->setValue(path, t_value);
}

int PNSettings::getWindowWidth(const QString& t_window_name)
{
    QString path = t_window_name + "/Width";

    return m_app_config->value(path, -1).toInt();
}

int PNSettings::getWindowHeight(const QString& t_window_name)
{
    QString path = t_window_name + "/Height";

    return m_app_config->value(path, -1).toInt();
}

bool PNSettings::getWindowMaximized(const QString& t_window_name)
{
    QString path = t_window_name + "/Maximized";

    if (m_app_config->value(path, QString("1")) == QString("1"))
        return true;
    else
        return false;
}

bool PNSettings::getWindowStatusBar(const QString& t_window_name)
{
    QString path = t_window_name + "/StatusBar";

    if (m_app_config->value(path, QString("1")) == QString("1"))
        return true;
    else
        return false;
}

void PNSettings::setWindowWidth(const QString& t_window_name, int t_width)
{
    QString path = t_window_name + "/Width";
    QVariant t_value = t_width;

    m_app_config->setValue(path, t_value);
}

void PNSettings::setWindowHeight(const QString& t_window_name, int t_height)
{
    QString path = t_window_name + "/Height";
    QVariant t_value = t_height;

    m_app_config->setValue(path, t_value);
}

void PNSettings::setWindowMaximized(const QString& t_window_name, bool t_maximized)
{
    QString path = t_window_name + "/Maximized";
    QVariant t_value = t_maximized ? QString("1") : QString("0");

    m_app_config->setValue(path, t_value);
}

void PNSettings::setWindowStatusBar(const QString& t_window_name, bool t_status_bar)
{
    QString path = t_window_name + "/StatusBar";
    QVariant t_value = t_status_bar ? QString("1") : QString("0");

    m_app_config->setValue(path, t_value);
}

QString PNSettings::getDefaultDictionary()
{
    QString path = "DefaultDictionary";

    return m_app_config->value(path).toString();
}

void PNSettings::setDefaultDictionary(const QString& t_dictionary)
{
    QString path = "DefaultDictionary";
    QVariant t_value = t_dictionary;

    m_app_config->setValue(path, t_value);
}

QString PNSettings::getPersonalDictionary()
{
    QString path = "PersonalDictionary";

    return m_app_config->value(path).toString();
}

void PNSettings::setPersonalDictionary(const QString& t_dictionary)
{
    QString path = "PersonalDictionary";
    QVariant t_value = t_dictionary;

    m_app_config->setValue(path, t_value);
}


