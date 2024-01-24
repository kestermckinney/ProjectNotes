// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "searchresultsmodel.h"
#include "pndatabaseobjects.h"

SearchResultsModel::SearchResultsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("SearchResultsModel");
    setOrderKey(200);

    setBaseSql("SELECT dataid, datatype, dataname, datadescription, internal_item, client_id, project_status, project_number, project_name, item_number, item_name, note_date, note_title, fk_id, datakey FROM database_search");

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
    addColumn(14, tr("Data Key"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    setReadOnly();
}

void SearchResultsModel::PerformSearch(const QString& t_search_value)
{
    clearAllUserSearches();

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

void SearchResultsModel::PerformKeySearch(const QStringList& t_search_fields, const QStringList& t_search_values)
{
    clearAllUserSearches();

    if ( t_search_values.isEmpty())
    {
        setUserSearchString(0, "FIND NOTHING");
        activateUserFilter(QString());
    }
    else
    {
        // if specific search column passed use it, if not a column use the data value
        for (int c = 0; c < t_search_fields.count(); c++)
        {
            QString col_name = t_search_fields.at(c);
            QString col_val = t_search_values.at(c);

            int col_num = getColumnNumber(col_name);

            if (col_num == -1)
                setUserSearchString(14, col_val);
            else
                setUserSearchString(col_num, col_val);
        }

        activateUserFilter(QString());
    }
}

