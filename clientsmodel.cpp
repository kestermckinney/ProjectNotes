#include "clientsmodel.h"

ClientsModel::ClientsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ClientsModel");

    setBaseSql("SELECT client_id, client_name FROM clients");

    setTableName("clients", "Clients");


    addColumn(0, tr("Client ID"), DB_STRING, true, true, false, true);
    addColumn(1, tr("Client Name"), DB_STRING, true, true, true, true);

    addRelatedTable("people", "client_id", "People");

    setOrderBy("client_name");

}
