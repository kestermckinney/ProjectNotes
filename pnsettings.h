#ifndef PNSETTINGS_H
#define PNSETTINGS_H

#include <QSettings>
#include <QMainWindow>
#include <QTableView>

class PNSettings
{
public:
    PNSettings();
    ~PNSettings();

    QVariant getPluginSetting(const QString& PluginName, const QString& ParameterName);
    bool getPluginEnabled(const QString& PluginName);
    void setPluginEnabled(const QString& PluginName, bool Enabled);

    QVariant getLastDatabase();
    void setLastDatabase(const QString& LastDatabase);

    QVariant getWindowStateData(const QString& StateDataName);
    void setWindowStateData(const QString& StateDataName, const QVariant& Data);

    void setWindowState(const QString& WindowName, const QMainWindow& Window);
    bool getWindowState(const QString& WindowName, QMainWindow& Window);

    QString getDefaultDictionary();
    void setDefaultDictionary(const QString& Dictionary);

    QString getPersonalDictionary();
    void setPersonalDictionary(const QString& Dictionary);

    void setTableViewState(const QString& ViewName, const QTableView& View);
    bool getTableViewState(const QString& ViewName, QTableView& View);

    void setTableSortColumn(const QString& ViewName, const int Column, const QString Direction);
    bool getTableSortColumn(const QString& ViewName, int& Column, QString& Direction);

private:
    int getWindowX(const QString& WindowName);
    int getWindowY(const QString& WindowName);
    void setWindowX(const QString& WindowName, int X);
    void setWindowY(const QString& WindowName, int Y);

    int getWindowWidth(const QString& WindowName);
    int getWindowHeight(const QString& WindowName);
    bool getWindowMaximized(const QString& WindowName);

    void setWindowWidth(const QString& WindowName, int Width);
    void setWindowHeight(const QString& WindowName, int Height);
    void setWindowMaximized(const QString& WindowName, bool Maximized);

    void setWindowStatusBar(const QString& WindowName, bool StatusBar);
    bool getWindowStatusBar(const QString& WindowName);


private:
    QSettings* m_AppConfig;
    QSettings* m_PluginConfig;
};

static PNSettings global_Settings;

#endif // PNSETTINGS_H
