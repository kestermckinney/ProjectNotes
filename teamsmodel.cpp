#include "teamsmodel.h"

TeamsModel::TeamsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("TeamsModel");

    setBaseSql("SELECT teammember_id, b.name, project_id, a.people_id, b.client_id FROM project_people a join people b on a.people_id=b.people_id ");

    setTableName("project_people", "Project People");

    addColumn(0, tr("Team Member ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    addColumn(1, tr("Name"), DBString, DBSearchable, DBRequired);
    addColumn(2, tr("Project ID"), DBString, DBSearchable, DBRequired);
    addColumn(3, tr("People ID"), DBString, DBSearchable, DBRequired);
    addColumn(4, tr("Client ID"), DBString, DBSearchable, DBRequired);

    addRelatedTable("people", "people_id", "People");
    addRelatedTable("clients", "client_id", "Clients");
    addRelatedTable("projects", "project_id", "Projects");

    setOrderBy("name");

}
