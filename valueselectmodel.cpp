#include <QDebug>
#include "valueselectmodel.h"

ValueSelectModel::ValueSelectModel(QObject *parent) : PNSqlQueryModel(parent)
{
    setObjectName("ValueSelectModel");

    setBaseSql("select '' Vals");

    setTableName("Values", "Values");

    AddColumn(0, tr("Values"), DB_STRING, false, true, false);
    setReadOnly();

    Refresh();
}

void ValueSelectModel::setValuesColumn(QString Column)
{
    int ccount = m_FilteringModel->columnCount();
    int col = 0;

    for (col = 0; col < ccount;  col++)
    {
        QString header = m_FilteringModel->headerData(col, Qt::Horizontal).toString();
        if (header == Column)
            break;
    }

    if (col >= ccount)
        return; // nothing can be done if the incorrect colum was specified

    QString where = m_FilteringModel->ConstructWhereClause(false);

    if (!where.isEmpty())
        where += " and ";
    else
        where = " where ";

    QString fieldnm = m_FilteringModel->emptyrecord().fieldName(col);

    setType(0, m_FilteringModel->getType(col));
    QString sql = "select distinct " + fieldnm + " from " + m_FilteringModel->tablename() + where + fieldnm + " is not null";
    qDebug() << "Value Select: " << sql << "\n";

    setBaseSql(sql);

    Refresh();
}
