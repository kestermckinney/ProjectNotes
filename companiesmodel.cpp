// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "companiesmodel.h"
#include "databaseobjects.h"

#include <QRegularExpression>

CompaniesModel::CompaniesModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("CompaniesModel");

    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
    setBaseSql("SELECT id, client_name FROM clients");

    setTableName("clients", "Companies");

    addColumn("id", tr("Company ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("client_name", tr("Company Name"), DBString, DBSearchable, DBRequired, DBEditable, DBUnique);

    addRelatedTable("people", "client_id", "id", "Contacts");
    addRelatedTable("projects", "client_id", "id", "Projects");

    QStringList key1 = {"client_name"};

    addUniqueKeys(key1, "Company Name");

    setOrderBy("client_name");
}

const QModelIndex CompaniesModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue1);
    Q_UNUSED(fkValue2);


    QVector<QVariant> qr = emptyrecord();

    return addRecord(qr);
}
