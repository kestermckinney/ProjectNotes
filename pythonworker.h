#ifndef PYTHONWORKER_H
#define PYTHONWORKER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QTimer>
#include <atomic>

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
    PluginMenu& operator=(const PluginMenu& pluginmenu)
    {
        if (this != &pluginmenu)
        {
            m_menuTitle = pluginmenu.menutitle();
            m_functionname = pluginmenu.functionname();;
            m_tablefilter = pluginmenu.tablefilter();
            m_submenu = pluginmenu.submenu();
            m_dataexport = pluginmenu.dataexport();
            m_parameter = pluginmenu.parameter();
        }

        return *this;
    }

    void setMenuTitle(const QString& menutitle) {m_menuTitle = menutitle; }
    QString menutitle() const { return m_menuTitle; }
    void setFunctionName(const QString& functionmame) {m_functionname = functionmame;}
    QString functionname() const { return m_functionname;}
    void setTableFilter(const QString& tablefilter) {m_tablefilter = tablefilter;}
    QString tablefilter() const { return m_tablefilter;}
    void setSubmenu(const QString& submenu) {m_submenu = submenu;}
    QString submenu() const {return m_submenu;}
    void setDataExport(const QString& dataexport) {m_dataexport = dataexport;}
    QString dataexport() const {return m_dataexport;}
    void setParameter(const QString& parameter) {m_parameter = parameter;}
    QString parameter() const {return m_parameter;}

private:
    QString m_menuTitle;
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
    PythonPlugin& operator=(const PythonPlugin& original)
    {
        if (this != &original)
        {
            m_name = original.name();
            m_description = original.description();
            m_members = original.members();
            m_submenu = original.submenu();
            m_menus = original.menus();
            m_importedModules = original.imports();
        }

        return *this;
    }

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }
    void setDescription(const QString& description) { m_description = description; }
    QString description() const { return m_description; }
    void addMember(const QString& member ) { m_members.append(member); }
    QStringList members() const { return m_members; }
    bool hasMember(const QString& member) const { return m_members.contains(member); }
    void setSubmenu(const QString& submenu) { m_submenu = submenu; }
    QString submenu() const { return m_submenu; }
    void setTimerDelay(const int timerdelay) { m_timerdelay = timerdelay; }
    int timerdelay() { return m_timerdelay; }
    QList<PluginMenu> menus() const { return m_menus; }
    void addMenu(const QString& menutitle, const QString& functionname, const QString& tablefilter, const QString& submenu, const QString& dataexport, const QString& parameter)
    {
        PluginMenu m;

        m.setMenuTitle(menutitle);
        m.setFunctionName(functionname);
        m.setTableFilter(tablefilter);
        m.setSubmenu(submenu);
        m.setDataExport(dataexport);
        m.setParameter(parameter);

        m_menus.append(m);
    }

    QStringList imports() const { return m_importedModules; }
    void addImport(const QString& import)
    {
        m_importedModules.append(import);
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
    QStringList m_importedModules;

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
    void checkForMember(const QString& member);
    bool isLoaded() { return m_isloaded; }
private:
    QString m_PluginLocation;
    PyObject* m_PNPluginModule = nullptr;

    // these events should happen from the event loop
    int setPythonVariable(const QString& variablename, const QString& value);
    QString getPythonVariable(const QString& variablename);
    QStringList getPythonStringList(const QString& variablename);
    void findImportedModules(const QString& pythonFilePath);

signals:
    // signals to caller
    void returnXml(const QString& xml);
    void loadComplete(const PythonPlugin& plugin);
    void unloadComplete();

public slots:
    void loadModule(const QString& modulepath);
    void unloadModule();
    void reloadModule();
    bool isLoading() { return m_isloading; }

    void sendMethodXml(const QString& method, const QString& xml, const QString& parameter);
    void sendMethod(const QString& method, const QString& parameter);

private slots:
    void timerUpdate();

private:
    QMutex m_loadingmutex;
    QWaitCondition m_loadwait;
    std::atomic<bool> m_isloading{false};
    std::atomic<bool> m_isloaded{false};

    QString m_modulename;
    QString m_modulepath;

    PythonPlugin m_plugin;

    QTimer* m_timer = nullptr;
    long m_minuteCounter = 0;
};

#endif // PYTHONWORKER_H
