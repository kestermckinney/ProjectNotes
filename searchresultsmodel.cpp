#include "searchresultsmodel.h"

SearchResultsModel::SearchResultsModel(QObject* parent): PNSqlQueryModel(parent)
{
    setBaseSql("SELECT dataid, datatype, dataname, datadescription, internal_item, client_id, project_status, project_number, project_name, item_number, item_name, note_date, note_title, fk_id, (dataid || dataname) keyval FROM database_search");

    setTableName("database_search", "Search Results");

    AddColumn(0, tr("Data ID"), DB_STRING, false, false, false, false);
    AddColumn(1, tr("Type"), DB_STRING, false, false, false, false);
    AddColumn(2, tr("Name"), DB_STRING, false, false, false, false);
    AddColumn(3, tr("Description"), DB_STRING, false, false, false, false);
    AddColumn(4, tr("Internal"), DB_STRING, false, false, false, false);
    AddColumn(5, tr("Client ID"), DB_STRING, false, false, false, false);
    AddColumn(6, tr("Project Status"), DB_STRING, false, false, false, false);

    AddColumn(7, tr("Project Number"), DB_STRING, false, false, false, false);
    AddColumn(8, tr("Project Name"), DB_STRING, false, false, false, false);
    AddColumn(9, tr("Item Number"), DB_STRING, false, false, false, false);
    AddColumn(10, tr("Item Name"), DB_STRING, false, false, false, false);
    AddColumn(11, tr("Meeting"), DB_STRING, false, false, false, false);
    AddColumn(12, tr("Title"), DB_STRING, false, false, false, false);
    AddColumn(13, tr("Foreign Key"), DB_STRING, false, false, false, false);
    AddColumn(14, tr("keyval"), DB_STRING, false, false, false, false);
 }
