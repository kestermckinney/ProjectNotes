// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "columnmodel.h"
#include "databaseobjects.h"
#include <QColor>

ColumnModel::ColumnModel(DatabaseObjects* dbo) : SqlQueryModel(dbo)
{
    setObjectName("ColumnModel");
    setDeletedFilterInView(true);

    setBaseSql("select '' Column");

    setTableName("Columns", "Columns");

    addColumn("Column", tr("Column"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    setReadOnly();

    refresh();
}

void ColumnModel::setColumnModel(SqlQueryModel *columnmodel, QTableView *view)
{
    QString buildsql;

    int col_count = columnmodel->columnCount();

    for (int i = 0; i < col_count; i++)
    {
        if (columnmodel->isSearchable(i) && (view == nullptr || !view->isColumnHidden(i)))
        {
            if (!buildsql.isEmpty())
                buildsql += "union all ";

            buildsql += QString("select '%1' as Column ").arg( columnmodel->headerData(i, Qt::Horizontal).toString() );
        }
    }

    setBaseSql(buildsql);

    setTableName(columnmodel->tablename(), "Columns");

    m_columnModel = columnmodel;
}

QVariant ColumnModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ForegroundRole)
    {
        if (index.column() == 0) // column name
        {
            QString displaycolname = data(index).toString();
            QString dbcolname = m_filteringModel->getColumnName(displaycolname);

            if ( (*m_savedFilters)[dbcolname].ColumnValues.count() > 0 ||
                 !(*m_savedFilters)[dbcolname].SearchBeginValue.toString().isEmpty() ||
                 !(*m_savedFilters)[dbcolname].SearchEndValue.toString().isEmpty()||
                 !(*m_savedFilters)[dbcolname].SearchString.toString().isEmpty() )
               return QVariant(QCOLOR_BLUE);
        }
    }

    return SqlQueryModel::data(index, role);
}

