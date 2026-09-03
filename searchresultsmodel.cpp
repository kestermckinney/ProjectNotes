// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "searchresultsmodel.h"
#include "databaseobjects.h"

#include <QTextDocument>

namespace {

// Collapse a note body to a single line of context around the search term.
// When the text is already short, or the term can't be located, it's returned
// (trimmed / head-clipped) unchanged. This keeps the global-search list showing
// readable prose instead of a whole meeting note.
QString snippetAround(const QString& text, const QString& search)
{
    const int radius = 80;             // characters of context on each side
    const int maxKeep = 2 * radius;

    if (text.length() <= maxKeep + 20)
        return text;

    int pos = -1;
    int matchLen = 0;

    if (!search.isEmpty())
    {
        // The search itself is a single LIKE '%value%', so the whole string is
        // what matched; fall back to individual words only if that isn't found
        // (e.g. the note's whitespace differs from the query's).
        pos = text.indexOf(search, 0, Qt::CaseInsensitive);
        if (pos >= 0)
        {
            matchLen = search.length();
        }
        else
        {
            const QStringList tokens = search.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
            for (const QString& tok : tokens)
            {
                pos = text.indexOf(tok, 0, Qt::CaseInsensitive);
                if (pos >= 0) { matchLen = tok.length(); break; }
            }
        }
    }

    if (pos < 0)
        return text.left(maxKeep).trimmed() + QStringLiteral(" …");

    int start = qMax(0, pos - radius);
    int end = qMin(text.length(), pos + matchLen + radius);

    // Snap outward to whitespace so the snippet doesn't slice through a word.
    while (start > 0 && !text.at(start - 1).isSpace())
        --start;
    while (end < text.length() && !text.at(end).isSpace())
        ++end;

    QString snippet = text.mid(start, end - start).trimmed();
    if (start > 0)
        snippet.prepend(QStringLiteral("… "));
    if (end < text.length())
        snippet.append(QStringLiteral(" …"));
    return snippet;
}

} // namespace

SearchResultsModel::SearchResultsModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("SearchResultsModel");

    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
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

    // datadescription is column 3 and can hold a rich-text note body. The Widgets
    // TableView reaches it as (column 3, DisplayRole/EditRole); the QML ListView
    // always passes column 0 and encodes the column in the role as
    // Qt::UserRole + colIndex (see SqlQueryModel::roleNames()). Handle both.
    int col = index.column();
    if (role >= Qt::UserRole && role < Qt::UserRole + 1000)
        col = role - Qt::UserRole;

    const bool contentRole = (role == Qt::DisplayRole || role == Qt::EditRole
                              || role == Qt::UserRole + 3);

    if (col == 3 && contentRole)
    {
        QString text = val.toString();

        // Render any HTML as plain text so formatting markup never shows.
        if (Qt::mightBeRichText(text))
        {
            QTextDocument doc;
            doc.setHtml(text);
            text = doc.toPlainText();
        }

        text = text.simplified();
        return snippetAround(text, m_lastSearchValue);
    }

    return val;
}

void SearchResultsModel::PerformSearch(const QString& searchValue)
{
    clearAllUserSearches();
    clearFilter(7);   // clear any previous key search on project_number
    clearFilter(14);  // clear any previous key search on datakey

    m_lastSearchValue = searchValue.trimmed();

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
    clearFilter(7);   // clear any previous key search on project_number
    clearFilter(14);  // clear any previous key search on datakey

    m_lastSearchValue.clear();  // key/ID searches aren't free-text, so no snippet

    if ( searchValues.isEmpty())
    {
        setUserSearchString(0, "FIND NOTHING");
        activateUserFilter(QString());
    }
    else
    {
        // use exact matching for key/ID fields (not LIKE substring search)
        for (int c = 0; c < searchFields.count(); c++)
        {
            QString col_name = searchFields.at(c);
            QString col_val = searchValues.at(c);

            int col_num = getColumnNumber(col_name);

            if (col_num == -1)
                setFilter(14, col_val, DBCompareType::Equals);
            else
                setFilter(col_num, col_val, DBCompareType::Equals);
        }

        activateUserFilter(QString());
    }
}

