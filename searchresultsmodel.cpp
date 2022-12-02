#include "searchresultsmodel.h"
#include "pndatabaseobjects.h"

SearchResultsModel::SearchResultsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("SearchResultsModel");

    setBaseSql("SELECT dataid, datatype, dataname, datadescription, internal_item, client_id, project_status, project_number, project_name, item_number, item_name, note_date, note_title, fk_id, (dataid || dataname) keyval FROM database_search");

    setTableName("database_search", "Search Results");

    addColumn(0, tr("Data ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(1, tr("Type"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(2, tr("Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(3, tr("Content"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(4, tr("Internal"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(5, tr("Client ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(6, tr("Project Status"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(7, tr("Project"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(8, tr("Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(9, tr("Item"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(10, tr("Item Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(11, tr("Meeting"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(12, tr("Title"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(13, tr("Foreign Key"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(14, tr("keyval"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique); //TODO: FIX unused columns
}

bool SearchResultsModel::openRecord(QModelIndex t_index)
{
    QVariant data_type = data(index(t_index.row(), 1));

    if (data_type == tr("Client"))
    {
        global_DBObjects.clientsmodel()->deactivateUserFilter(global_DBObjects.clientsmodel()->objectName());
    }
    else if (data_type == tr("People"))
    {
        global_DBObjects.peoplemodel()->deactivateUserFilter(global_DBObjects.peoplemodel()->objectName());
    }
    else if (data_type == tr("Project"))
    {
        QVariant record_id = data(index(t_index.row(), 0));

        // only select the records another event will be fired to open the window to show them
        global_DBObjects.projectinformationmodel()->setFilter(0, record_id.toString());
        global_DBObjects.projectinformationmodel()->refresh();

        // filter team members by project
        global_DBObjects.projectteammembersmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectteammembersmodel()->refresh();

        // filter project status items
        global_DBObjects.statusreportitemsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.statusreportitemsmodel()->refresh();

        // filter team members by project for members list
        global_DBObjects.teamsmodel()->setFilter(2, record_id.toString());
        global_DBObjects.teamsmodel()->refresh();

        // filter tracker items by project
        global_DBObjects.trackeritemsmodel()->setFilter(14, record_id.toString());
        global_DBObjects.trackeritemsmodel()->refresh();

        // filter tracker items by project
        global_DBObjects.trackeritemsmodel()->setFilter(14, record_id.toString());
        global_DBObjects.trackeritemsmodel()->refresh();

        global_DBObjects.trackeritemsmeetingsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.trackeritemsmeetingsmodel()->refresh();

        global_DBObjects.projectlocationsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectlocationsmodel()->refresh();

        global_DBObjects.projectnotesmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectnotesmodel()->refresh();
    }
    else if (data_type == tr("Project Notes"))
    {
        QVariant note_id = data(index(t_index.row(), 0));
        QVariant project_id = data(index(t_index.row(), 13));

        global_DBObjects.projecteditingnotesmodel()->setFilter(0, note_id.toString());
        global_DBObjects.projecteditingnotesmodel()->refresh();

        // only select the records another event will be fired to open the window to show them
        global_DBObjects.meetingattendeesmodel()->setFilter(1, note_id.toString());
        global_DBObjects.meetingattendeesmodel()->refresh();

        global_DBObjects.notesactionitemsmodel()->setFilter(13, note_id.toString());
        global_DBObjects.notesactionitemsmodel()->refresh();

        // filter team members by project for members list
        global_DBObjects.teamsmodel()->setFilter(2, project_id.toString());
        global_DBObjects.teamsmodel()->refresh();  //TODO: Non-HTML notes aren't showing return characters and tabs

    }
    else if (data_type == tr("Meeting Attendees"))
    {
        QVariant note_id = data(index(t_index.row(), 13));

        QVariant project_number = data(index(t_index.row(), 7));
        QVariant project_id = global_DBObjects.execute(QString("select project_id from projects where project_number='%1'").arg(project_number.toString()));

        global_DBObjects.projecteditingnotesmodel()->setFilter(0, note_id.toString());
        global_DBObjects.projecteditingnotesmodel()->refresh();

        // only select the records another event will be fired to open the window to show them
        global_DBObjects.meetingattendeesmodel()->setFilter(1, note_id.toString());
        global_DBObjects.meetingattendeesmodel()->refresh();

        global_DBObjects.notesactionitemsmodel()->setFilter(13, note_id.toString());
        global_DBObjects.notesactionitemsmodel()->refresh();

        // filter team members by project for members list
        global_DBObjects.teamsmodel()->setFilter(2, project_id.toString());
        global_DBObjects.teamsmodel()->refresh();  //TODO: Non-HTML notes aren't showing return characters and tabs
    }
    else if (data_type == tr("Project Locations") || data_type == tr("Project Team") || data_type == tr("Status Report Item") || data_type == tr("Item Tracker") )
    {
        QVariant record_id = data(index(t_index.row(), 13));

        // only select the records another event will be fired to open the window to show them
        global_DBObjects.projectinformationmodel()->setFilter(0, record_id.toString());
        global_DBObjects.projectinformationmodel()->refresh();

        // filter team members by project
        global_DBObjects.projectteammembersmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectteammembersmodel()->refresh();

        // filter project status items
        global_DBObjects.statusreportitemsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.statusreportitemsmodel()->refresh();

        // filter team members by project for members list
        global_DBObjects.teamsmodel()->setFilter(2, record_id.toString());
        global_DBObjects.teamsmodel()->refresh();

        // filter tracker items by project
        global_DBObjects.trackeritemsmodel()->setFilter(14, record_id.toString());
        global_DBObjects.trackeritemsmodel()->refresh();

        // filter tracker items by project
        global_DBObjects.trackeritemsmodel()->setFilter(14, record_id.toString());
        global_DBObjects.trackeritemsmodel()->refresh();

        global_DBObjects.trackeritemsmeetingsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.trackeritemsmeetingsmodel()->refresh();

        global_DBObjects.projectlocationsmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectlocationsmodel()->refresh();

        global_DBObjects.projectnotesmodel()->setFilter(1, record_id.toString());
        global_DBObjects.projectnotesmodel()->refresh();
    }

    return true;
}

void SearchResultsModel::PerformSearch(const QString& t_search_value)
{
    clearAllUserSearches();
    deactivateUserFilter(QString());

    if ( t_search_value.isEmpty())
    {
        setUserSearchString(0, "FIND NOTHING");
        activateUserFilter(QString());
    }
    else
    {
        setUserSearchString(3, t_search_value);
        activateUserFilter(QString());
    }
}

//TODO: Add search query to database updates
