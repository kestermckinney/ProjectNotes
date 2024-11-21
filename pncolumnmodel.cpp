// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pncolumnmodel.h"
#include "pndatabaseobjects.h"
#include <QColor>

PNColumnModel::PNColumnModel(PNDatabaseObjects* t_dbo, bool t_gui) : PNSqlQueryModel(t_dbo, t_gui)
{
    setObjectName("PNColumnModel");

    setBaseSql("select '' Column");

    setTableName("Columns", "Columns");

    addColumn(0, tr("Column"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    setReadOnly();

    refresh();
}

void PNColumnModel::setColumnModel(PNSqlQueryModel *t_columnmodel)
{
    QString buildsql;

    int col_count = t_columnmodel->columnCount();

    for (int i = 0; i < col_count; i++)
    {
        if (t_columnmodel->isSearchable(i))
        {
            if (!buildsql.isEmpty())
                buildsql += "union all ";

            buildsql += QString("select '%1' as Column ").arg( t_columnmodel->headerData(i, Qt::Horizontal).toString() );
        }
    }

    setBaseSql(buildsql);

    setTableName(t_columnmodel->tablename(), "Columns");

    m_column_model = t_columnmodel;
}

QVariant PNColumnModel::data(const QModelIndex &t_index, int t_role) const
{
    if (t_role == Qt::ForegroundRole)
    {
        if (t_index.column() == 0) // column name
        {
            QString displaycolname = data(t_index).toString();
            QString dbcolname = m_filtering_model->getColumnName(displaycolname);

            if ( (*m_saved_filters)[dbcolname].ColumnValues.count() > 0 ||
                 !(*m_saved_filters)[dbcolname].SearchBeginValue.toString().isEmpty() ||
                 !(*m_saved_filters)[dbcolname].SearchEndValue.toString().isEmpty()||
                 !(*m_saved_filters)[dbcolname].SearchString.toString().isEmpty() )
               return QVariant(QCOLOR_BLUE);
        }
    }

    return PNSqlQueryModel::data(t_index, t_role);
}

