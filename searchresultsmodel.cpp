#include "searchresultsmodel.h"

SearchResultsModel::SearchResultsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("SearchResultsModel");

    setBaseSql("SELECT dataid, datatype, dataname, datadescription, internal_item, client_id, project_status, project_number, project_name, item_number, item_name, note_date, note_title, fk_id, (dataid || dataname) keyval FROM database_search");

    setTableName("database_search", "Search Results");

    addColumn(0, tr("Data ID"), DB_STRING, false, false, false, false);
    addColumn(1, tr("t_type"), DB_STRING, false, false, false, false);
    addColumn(2, tr("Name"), DB_STRING, false, false, false, false);
    addColumn(3, tr("Description"), DB_STRING, false, false, false, false);
    addColumn(4, tr("Internal"), DB_STRING, false, false, false, false);
    addColumn(5, tr("Client ID"), DB_STRING, false, false, false, false);
    addColumn(6, tr("Project Status"), DB_STRING, false, false, false, false);

    addColumn(7, tr("Project Number"), DB_STRING, false, false, false, false);
    addColumn(8, tr("Project Name"), DB_STRING, false, false, false, false);
    addColumn(9, tr("Item Number"), DB_STRING, false, false, false, false);
    addColumn(10, tr("Item Name"), DB_STRING, false, false, false, false);
    addColumn(11, tr("Meeting"), DB_STRING, false, false, false, false);
    addColumn(12, tr("Title"), DB_STRING, false, false, false, false);
    addColumn(13, tr("Foreign Key"), DB_STRING, false, false, false, false);
    addColumn(14, tr("keyval"), DB_STRING, false, false, false, false);
 }
