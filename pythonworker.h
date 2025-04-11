#ifndef PYTHONWORKER_H
#define PYTHONWORKER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QTimer>

#include "QLogger.h"
#include "QLoggerWriter.h"

#ifdef _DEBUG
#pragma push_macro("slots")
#undef slots
#undef _DEBUG
#include "Python.h"
#define _DEBUG
#pragma pop_macro("slots")
#else
#pragma push_macro("slots")
#undef slots
#include "Python.h"
#pragma pop_macro("slots")
#endif


class PluginMenu
{
public:
    PluginMenu() {};
    PluginMenu& operator=(const PluginMenu& t_pluginmenu)
    {
        if (this != &t_pluginmenu)
        {
            m_menu_title = t_pluginmenu.menutitle();
            m_functionname = t_pluginmenu.functionname();;
            m_tablefilter = t_pluginmenu.tablefilter();
            m_submenu = t_pluginmenu.submenu();
            m_dataexport = t_pluginmenu.dataexport();
            m_parameter = t_pluginmenu.parameter();
        }

        return *this;
    }

    void setMenuTitle(const QString& t_menutitle) {m_menu_title = t_menutitle; }
    QString menutitle() const { return m_menu_title; }
    void setFunctionName(const QString& t_functionmame) {m_functionname = t_functionmame;}
    QString functionname() const { return m_functionname;}
    void setTableFilter(const QString& t_tablefilter) {m_tablefilter = t_tablefilter;}
    QString tablefilter() const { return m_tablefilter;}
    void setSubmenu(const QString& t_submenu) {m_submenu = t_submenu;}
    QString submenu() const {return m_submenu;}
    void setDataExport(const QString& t_dataexport) {m_dataexport = t_dataexport;}
    QString dataexport() const {return m_dataexport;}
    void setParameter(const QString& t_parameter) {m_parameter = t_parameter;}
    QString parameter() const {return m_parameter;}

private:
    QString m_menu_title;
    QString m_functionname;
    QString m_tablefilter;
    QString m_submenu;
    QString m_dataexport;
    QString m_parameter;
};

class PythonPlugin
{
public:
    PythonPlugin() { };
    PythonPlugin& operator=(const PythonPlugin& t_original)
    {
        if (this != &t_original)
        {
            m_name = t_original.name();
            m_description = t_original.description();
            m_members = t_original.members();
            m_submenu = t_original.submenu();
            m_menus = t_original.menus();
            m_imported_modules = t_original.imports();
        }

        return *this;
    }

    void setName(const QString& t_name) { m_name = t_name; }
    QString name() const { return m_name; }
    void setDescription(const QString& t_description) { m_description = t_description; }
    QString description() const { return m_description; }
    void addMember(const QString& t_member ) { m_members.append(t_member); }
    QStringList members() const { return m_members; }
    bool hasMember(const QString& t_member) const { return m_members.contains(t_member); }
    void setSubmenu(const QString& t_submenu) { m_submenu = t_submenu; }
    QString submenu() const { return m_submenu; }
    void setTimerDelay(const int t_timerdelay) { m_timerdelay = t_timerdelay; }
    int timerdelay() { return m_timerdelay; }
    QList<PluginMenu> menus() const { return m_menus; }
    void addMenu(const QString& t_menutitle, const QString& t_functionname, const QString& t_tablefilter, const QString& t_submenu, const QString& t_dataexport, const QString& t_parameter)
    {
        PluginMenu m;

        m.setMenuTitle(t_menutitle);
        m.setFunctionName(t_functionname);
        m.setTableFilter(t_tablefilter);
        m.setSubmenu(t_submenu);
        m.setDataExport(t_dataexport);
        m.setParameter(t_parameter);

        m_menus.append(m);
    }

    QStringList imports() const { return m_imported_modules; }
    void addImport(const QString& t_import)
    {
        m_imported_modules.append(t_import);
    }

    void clearMenu()
    {
        m_menus.clear();
    }

private:
    QString m_name;
    QString m_description;
    int m_timerdelay;
    QStringList m_members;
    QString m_submenu;

    QList<PluginMenu> m_menus;
    QStringList m_imported_modules;

};

Q_DECLARE_METATYPE(PythonPlugin)
Q_DECLARE_METATYPE(PluginMenu)

class PythonWorker : public QObject
{
    Q_OBJECT

public:
    explicit PythonWorker(QObject *parent = nullptr);
    ~PythonWorker();

    void emitError();
    QMutex& loadingMutex() { return m_loadingmutex; }
    void checkForMember(const QString& t_member);
    bool isLoaded() { return m_isloaded; }
private:
    QString m_PluginLocation;
    PyObject* m_PNPluginModule;

    // these events should happen from the event loop
    int setPythonVariable(const QString& t_variablename, const QString& t_value);
    QString getPythonVariable(const QString& t_variablename);
    QStringList getPythonStringList(const QString& t_variablename);

signals:
    // signals to caller
    void returnXml(const QString& t_xml);
    void loadComplete(const PythonPlugin& t_plugin);
    void unloadComplete();

public slots:
    void loadModule(const QString& t_modulepath);
    void unloadModule();
    void reloadModule();
    bool isLoading() { return m_isloading; }

    void sendMethodXml(const QString& t_method, const QString& t_xml, const QString& t_parameter);
    void sendMethod(const QString& t_method, const QString& t_parameter);

private slots:
    void timerUpdate();

private:
    QMutex m_loadingmutex;
    QWaitCondition m_loadwait;
    bool m_isloading = false;
    bool m_isloaded = false;

    QString m_modulename;
    QString m_modulepath;

    PythonPlugin m_plugin;

    QTimer* m_timer = nullptr;
    long m_minute_counter = 0;
};

#endif // PYTHONWORKER_H
