#include "clientsmodel.h"

#include <QRegularExpression>

ClientsModel::ClientsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ClientsModel");

    setBaseSql("SELECT client_id, client_name FROM clients");

    setTableName("clients", "Clients");


    addColumn(0, tr("Client ID"), DB_STRING, false, true, false, true);
    addColumn(1, tr("Client Name"), DB_STRING, true, true, true, true);

    addRelatedTable("people", "client_id", "People");

    setOrderBy("client_name");

}

bool ClientsModel::newRecord()
{
    QSqlQuery select;
    select.prepare("select max(client_name) from clients where client_name like '[%'");
    QString maxnum;

    select.exec();
    if (select.next())
    {
        maxnum = select.value(0).toString();
        maxnum.remove(QRegularExpression("[^0-9]+"));
    }

    int num = maxnum.toInt() + 1;

    QSqlRecord qr = emptyrecord();
    qr.setValue(1, QString("[New Client %1]").arg(num, 2, 10, QLatin1Char('0')));

    addRecord(qr);

    return true;
}
