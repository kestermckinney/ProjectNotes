#include "valueselectmodel.h"

ValueSelectModel::ValueSelectModel(QObject *parent) : PNSqlQueryModel(parent)
{
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

    QString fieldnm = m_FilteringModel->emptyrecord().fieldName(col);
    setType(0, m_FilteringModel->getType(col));
    QString sql = "select distinct " + fieldnm + " from " + m_FilteringModel->tablename() + m_FilteringModel->ConstructWhereClause() + " and " + fieldnm + " is not null";

    setBaseSql(sql);

    Refresh();
}
