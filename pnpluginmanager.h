#ifndef PNPLUGINMANAGER_H
#define PNPLUGINMANAGER_H

#include "pnplugin.h"

#include <QMenu>

typedef std::function<void(std::string)> stdout_write_type;
void set_stdout(stdout_write_type write);
void reset_stdout();

class PNPluginManager
{
public:
    PNPluginManager();
    ~PNPluginManager();

    QList<PNPlugin*> findStartupEvents();
    QList<PNPlugin*> findShutdownEvents();
    QList<PNPlugin*> findEveryMinuteEvents();
    QList<PNPlugin*> findEvery5MinutesEvents();
    QList<PNPlugin*> findEvery10MinutesEvents();
    QList<PNPlugin*> findEvery15MinutesEvents();
    QList<PNPlugin*> findEvery30MinutesEvents();
    QList<PNPlugin*> findPluginMenuEvents();

    QList<PNPlugin*> findDataRightClickEvents(const QString& t_tablename);
    QList<PNPlugin*> getPlugins() { return m_PNPlugins;};

private:
    QList<PNPlugin*> m_PNPlugins;
};

#endif // PNPLUGINMANAGER_H
