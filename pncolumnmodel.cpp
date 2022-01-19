#include "pncolumnmodel.h"
#include <QColor>

PNColumnModel::PNColumnModel(QObject *t_parent) : PNSqlQueryModel(t_parent)
{
    setObjectName("PNColumnModel");

    setBaseSql("select '' Column");

    setTableName("Columns", "Columns");

    addColumn(0, tr("Column"), DB_STRING, false, true, false);
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
               return QVariant(QColor(Qt::darkBlue));
        }
    }

    return PNSqlQueryModel::data(t_index, t_role);
}

