#include "teamsmodel.h"

TeamsModel::TeamsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("TeamsModel");

    setBaseSql("SELECT teammember_id, b.name, project_id, a.people_id, b.client_id FROM project_people a join people b on a.people_id=b.people_id ");

    setTableName("project_people", "Project People");

    addColumn(0, tr("Team Member ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    addColumn(1, tr("Name"), DBString, DBSearchable, DBRequired);
    addColumn(2, tr("Project ID"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn(3, tr("People ID"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name");
    addColumn(4, tr("Client ID"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "clients", "client_id", "client_name");

//    addRelatedTable("people", "people_id", "People");
//    addRelatedTable("clients", "client_id", "Clients");
//    addRelatedTable("projects", "project_id", "Projects");
    // I don't think the related table is needed because this is just used for drop down lists

    setOrderBy("name");

    setShowBlank(); // add a blank line to the selection
    setNoExport();  // this table is only for drop downs don't export it
}
