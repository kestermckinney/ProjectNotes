#include "valueselectmodel.h"

ValueSelectModel::ValueSelectModel(QObject *parent) : PNSqlQueryModel(parent)
{
    setBaseSql("select '' Values");

    setTableName("Values", "Values");

    AddColumn(0, tr("Values"), DB_STRING, false, true, false);

    Refresh();
}

void ValueSelectModel::setValuesModel(PNSqlQueryModel* model, QString Column)
{
    int ccount = model->columnCount();
    int col = 0;

    for (col = 0; col < ccount;  col++)
    {
        QString header = model->headerData(col, Qt::Horizontal).toString();
        if (header == Column)
            break;
    }

    if (col >= ccount)
        return; // nothing can be done if the incorrect colum was specified

    QString fieldnm = model->emptyrecord().fieldName(col);
    setType(0, model->getType(col));
    QString sql = "select " + fieldnm + " from " + model->tablename() + model->ConstructWhereClause();

    setBaseSql(sql);

    Refresh();
}
