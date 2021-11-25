#include "pncolumnmodel.h"

PNColumnModel::PNColumnModel(QObject *parent) : PNSqlQueryModel(parent)
{
    setBaseSql("select '' Column");

    setTableName("Columns", "Columns");

    AddColumn(0, tr("Column"), DB_STRING, false, true, false);
    setReadOnly();

    Refresh();
}

void PNColumnModel::setColumnModel(PNSqlQueryModel *columnmodel)
{
    QString buildsql;

    int col_count = columnmodel->columnCount();

    for (int i = 0; i < col_count; i++)
    {
        if (columnmodel->isSearchable(i))
        {
            if (!buildsql.isEmpty())
                buildsql += "union all ";

            buildsql += QString("select '%1' as Column ").arg( columnmodel->headerData(i, Qt::Horizontal).toString() );
        }
    }

    setBaseSql(buildsql);

    setTableName(columnmodel->tablename(), "Columns");

    m_ColumnModel = columnmodel;
}
