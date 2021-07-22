#include "teamsmodel.h"

TeamsModel::TeamsModel(QObject* parent): PNSqlQueryModel(parent)
{
    setBaseSql("SELECT teammember_id, b.name, project_id, a.people_id, b.client_id FROM project_people a join people b on a.people_id=b.people_id ");

    setTableName("project_people", "Project People");

    AddColumn(0, tr("Team Member ID"), DB_STRING, false, true, true);
    AddColumn(1, tr("Name"), DB_STRING, true, true, false, false);
    AddColumn(2, tr("Project ID"), DB_STRING, true, true, false, false);
    AddColumn(3, tr("People ID"), DB_STRING, true, true, false, false);
    AddColumn(4, tr("Client ID"), DB_STRING, true, true, false, false);

    AddRelatedTable("people", "people_id", "People");
    AddRelatedTable("clients", "client_id", "Clients");
    AddRelatedTable("projects", "project_id", "Projects");

    SetOrderBy("name");

}
