#include "clientsmodel.h"

ClientsModel::ClientsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ClientsModel");

    setBaseSql("SELECT client_id, client_name FROM clients");

    setTableName("clients", "Clients");


    AddColumn(0, tr("Client ID"), DB_STRING, true, true, false, true);
    AddColumn(1, tr("Client Name"), DB_STRING, true, true, true, true);

    AddRelatedTable("people", "client_id", "People");

    SetOrderBy("client_name");

}
