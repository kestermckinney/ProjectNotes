// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "clientsmodel.h"
#include "databaseobjects.h"

#include <QRegularExpression>

ClientsModel::ClientsModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("ClientsModel");

    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
    setBaseSql("SELECT id, client_name FROM clients");

    setTableName("clients", "Clients");

    addColumn("id", tr("Client ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("client_name", tr("Client Name"), DBString, DBSearchable, DBRequired, DBEditable, DBUnique);

    addRelatedTable("people", "client_id", "id", "People");
    addRelatedTable("projects", "client_id", "id", "Projects");

    QStringList key1 = {"client_name"};

    addUniqueKeys(key1, "Client Name");

    setOrderBy("client_name");
}

const QModelIndex ClientsModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue1);
    Q_UNUSED(fkValue2);

    DB_LOCK;
    QSqlQuery select(getDBOs()->getDb());
    select.prepare("select max(client_name) from clients where client_name like '[%' and deleted = 0");
    QString maxnum;

    select.exec();
    if (select.next())
    {
        maxnum = select.value(0).toString();
        maxnum.remove(QRegularExpression("[^0-9]+"));
    }
    DB_UNLOCK;

    int num = maxnum.toInt() + 1;

    QVector<QVariant> qr = emptyrecord();
    qr[1] = QString("[New Client %1]").arg(num, 2, 10, QLatin1Char('0'));

    return addRecord(qr);
}
