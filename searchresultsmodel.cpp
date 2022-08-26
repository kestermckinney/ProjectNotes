#include "searchresultsmodel.h"

SearchResultsModel::SearchResultsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("SearchResultsModel");

    setBaseSql("SELECT dataid, datatype, dataname, datadescription, internal_item, client_id, project_status, project_number, project_name, item_number, item_name, note_date, note_title, fk_id, (dataid || dataname) keyval FROM database_search");

    setTableName("database_search", "Search Results");

    addColumn(0, tr("Data ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(1, tr("t_type"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(2, tr("Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(3, tr("Description"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(4, tr("Internal"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(6, tr("Project Status"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    addColumn(7, tr("Project Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(8, tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(9, tr("Item Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(10, tr("Item Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(11, tr("Meeting"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(12, tr("Title"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(13, tr("Foreign Key"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn(14, tr("keyval"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
 }
