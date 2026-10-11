// SPDX-License-Identifier: GPL-3.0-only
#include "legacywidgetsettings.h"
#include <QSettings>

namespace PN::LegacyWidgetSettings {
QStringList filterKeys()
{
    const QStringList models{
        "ClientsModel", "PeopleModel", "ProjectsListModel", "ProjectsModel",
        "TrackerItemsModel", "ProjectNotesModel", "ProjectTeamMembersModel",
        "ProjectLocationsModel", "StatusReportItemsModel", "MeetingAttendeesModel",
        "TrackerItemCommentsModel", "ActionItemProjectNotesModel",
        "ActionItemsDetailsMeetingsModel", "ItemDetailTeamListModel",
        "NotesActionItemsModel", "SearchResultsModel", "TeamsModel"
    };
    QStringList keys;
    for (const QString &model : models)
        keys << "UserFilter:" + model << "UserFilter:" + model + ":IsActive";
    return keys;
}

QStringList databaseKeys()
{
    return filterKeys() + QStringList{
        "ViewFilter:ShowResolvedTrackerItems", "UserFilter:ShowClosedProjects",
        "UserFilter:ShowInternalItems"
    };
}

void cleanupLocal(QSettings &settings)
{
    // Named legacy controls only; preserve current view options, QML geometry,
    // dictionaries, credentials, sync settings, and separate PluginSettings.
    for (const QString &key : filterKeys()) settings.remove(key);
    const QStringList views{
        "tableViewProjects", "tableViewAllItems", "tableViewPeople", "tableViewClients",
        "tableViewProjectNotes", "tableViewAtendees", "tableViewActionItems",
        "tableViewLocations", "tableViewTeam", "tableViewStatusReportItems",
        "tableViewTrackerItems", "tableViewComments", "tableViewSearchResults"
    };
    for (const QString &view : views) {
        for (const QString &name : QStringList{view, "tableViewColumnName_" + view,
                                               "tableViewFilterValues_" + view}) {
            for (const QString &suffix : QStringList{"Columns", "SortColumn", "SortDirection"})
                settings.remove(name + suffix);
        }
    }
    const QStringList windows{"MainWindow", "CloudSyncSettingsDialog", "FilterDataDialog",
                              "FindReplaceDialog", "PreferencesDialog", "SpellCheckDialog"};
    for (const QString &window : windows)
        for (const QString &suffix : QStringList{"X", "Y", "Width", "Height", "Maximized", "StatusBar"})
            settings.remove(window + '/' + suffix);
    settings.remove("ZoomFactor");
}
}
