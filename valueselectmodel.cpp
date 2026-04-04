// Copyright (C) 2021, 2022, 2023, 2024, 2025, 2026 Paul McKinney

#include "valueselectmodel.h"
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

ValueSelectModel::ValueSelectModel(DatabaseObjects* dbo) : SqlQueryModel(dbo)
{
    setObjectName("ValueSelectModel");

    setBaseSql("select '' Vals");

    setTableName("Values", "Values");

    addColumn("Vals", tr("Values"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    setReadOnly();

    refresh();
}

void ValueSelectModel::setValuesColumn(QString column)
{
    int ccount = m_filteringModel->columnCount();
    int col = 0;

    for (col = 0; col < ccount;  col++)
    {
        QString header = m_filteringModel->headerData(col, Qt::Horizontal).toString();
        if (header == column)
            break;
    }

    if (col >= ccount)
        return; // nothing can be done if the incorrect colum was specified

    QString where = m_filteringModel->constructWhereClause(false);

    if (!where.isEmpty())
        where += " and ";
    else
        where = " where ";

    QString fieldnm = m_filteringModel->getColumnName(col);

    setType(0, m_filteringModel->getType(col));
    QString sql;

    if (m_filteringModel->getDeletedFilterInView())
        sql = "select distinct " + fieldnm + " from ( " + m_filteringModel->BaseSQL() + where + fieldnm + " is not null)";
    else
        sql = "select distinct " + fieldnm + " from ( " + m_filteringModel->BaseSQL() + where + fieldnm + " is not null and (deleted is Null or deleted = 0) )";

    setDeletedFilterInView(true);

    setBaseSql(sql);

    refresh();
}

