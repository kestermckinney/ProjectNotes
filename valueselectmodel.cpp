
#include "valueselectmodel.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

ValueSelectModel::ValueSelectModel(PNDatabaseObjects* t_dbo) : PNSqlQueryModel(t_dbo)
{
    setObjectName("ValueSelectModel");

    setBaseSql("select '' Vals");

    setTableName("Values", "Values");

    addColumn("Vals", tr("Values"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    setReadOnly();

    refresh();
}

void ValueSelectModel::setValuesColumn(QString t_column)
{
    int ccount = m_filtering_model->columnCount();
    int col = 0;

    for (col = 0; col < ccount;  col++)
    {
        QString header = m_filtering_model->headerData(col, Qt::Horizontal).toString();
        if (header == t_column)
            break;
    }

    if (col >= ccount)
        return; // nothing can be done if the incorrect colum was specified

    QString where = m_filtering_model->constructWhereClause(false);

    if (!where.isEmpty())
        where += " and ";
    else
        where = " where ";

    QString fieldnm = m_filtering_model->emptyrecord()[col].toString();

    setType(0, m_filtering_model->getType(col));
    QString sql = "select distinct " + fieldnm + " from ( " + m_filtering_model->BaseSQL() + where + fieldnm + " is not null )";
    //qDebug() << "Value Select: " << sql << "\n";

    setBaseSql(sql);

    refresh();
}
