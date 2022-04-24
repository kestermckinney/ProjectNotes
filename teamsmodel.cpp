#include "teamsmodel.h"

TeamsModel::TeamsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("TeamsModel");

    setBaseSql("SELECT teammember_id, b.name, project_id, a.people_id, b.client_id FROM project_people a join people b on a.people_id=b.people_id ");

    setTableName("project_people", "Project People");

    addColumn(0, tr("Team Member ID"), DB_STRING, false, true, true);
    addColumn(1, tr("Name"), DB_STRING, true, true, false, false);
    addColumn(2, tr("Project ID"), DB_STRING, true, true, false, false);
    addColumn(3, tr("People ID"), DB_STRING, true, true, false, false);
    addColumn(4, tr("Client ID"), DB_STRING, true, true, false, false);

    addRelatedTable("people", "people_id", "People");
    addRelatedTable("clients", "client_id", "Clients");
    addRelatedTable("projects", "project_id", "Projects");

    setOrderBy("name");

}
