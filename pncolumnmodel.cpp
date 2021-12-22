#include "pncolumnmodel.h"
#include <QColor>

PNColumnModel::PNColumnModel(QObject *parent) : PNSqlQueryModel(parent)
{
    setObjectName("PNColumnModel");

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

QVariant PNColumnModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ForegroundRole)
    {
        if (index.column() == 0) // column name
        {
            QString displaycolname = data(index).toString();
            QString dbcolname = m_FilteringModel->getColumnName(displaycolname);

            if ( (*savedFilters)[dbcolname].ColumnValues.count() > 0 ||
                 !(*savedFilters)[dbcolname].SearchBeginValue.toString().isEmpty() ||
                 !(*savedFilters)[dbcolname].SearchEndValue.toString().isEmpty()||
                 !(*savedFilters)[dbcolname].SearchString.toString().isEmpty() )
               return QVariant(QColor(Qt::darkBlue));
        }
    }

    return PNSqlQueryModel::data(index, role);
}

