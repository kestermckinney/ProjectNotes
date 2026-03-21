// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "searchresultsmodel.h"
#include "databaseobjects.h"

#include <QTextDocument>

SearchResultsModel::SearchResultsModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("SearchResultsModel");
    setOrderKey(200);

    setBaseSql("SELECT dataid, datatype, dataname, datadescription, internal_item, client_id, project_status, project_number, project_name, item_number, item_name, note_date, note_title, fk_id, datakey FROM database_search");
    setDeletedFilterInView(true);  // view filters deleted rows internally

    setTableName("database_search", "Search Results");

    addColumn("dataid", tr("Data ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("datatype", tr("Type"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("dataname", tr("Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("datadescription", tr("Content"), DBString, DBSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("internal_item", tr("Internal"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("client_id", tr("Client ID"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_status", tr("Project Status"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_number", tr("Project"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_name", tr("Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("item_number", tr("Item"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("item_name", tr("Item Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("note_date", tr("Meeting"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("note_title", tr("Title"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("fk_id", tr("Foreign Key"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("datakey", tr("Data Key"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

    setReadOnly();
}

QVariant SearchResultsModel::data(const QModelIndex &index, int role) const
{
    QVariant val = SqlQueryModel::data(index, role);

    // datadescription is column 3; strip HTML for display if the content contains markup
    if (index.column() == 3 && role == Qt::DisplayRole)
    {
        QString text = val.toString();
        if (text.contains('<'))
        {
            QTextDocument doc;
            doc.setHtml(text);
            return doc.toPlainText().trimmed();
        }
    }

    return val;
}

void SearchResultsModel::PerformSearch(const QString& searchValue)
{
    clearAllUserSearches();

    if ( searchValue.isEmpty())
    {
        setUserSearchString(0, "FIND NOTHING");
        activateUserFilter(QString());
    }
    else
    {
        setUserSearchString(3, searchValue);
        activateUserFilter(QString());
    }
}

void SearchResultsModel::PerformKeySearch(const QStringList& searchFields, const QStringList& searchValues)
{
    clearAllUserSearches();

    if ( searchValues.isEmpty())
    {
        setUserSearchString(0, "FIND NOTHING");
        activateUserFilter(QString());
    }
    else
    {
        // if specific search column passed use it, if not a column use the data value
        for (int c = 0; c < searchFields.count(); c++)
        {
            QString col_name = searchFields.at(c);
            QString col_val = searchValues.at(c);

            int col_num = getColumnNumber(col_name);

            if (col_num == -1)
                setUserSearchString(14, col_val);
            else
                setUserSearchString(col_num, col_val);
        }

        activateUserFilter(QString());
    }
}

