#include "projectteammembersmodel.h"

ProjectTeamMembersModel::ProjectTeamMembersModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectTeamMembersModel");

    setBaseSql("SELECT teammember_id, project_id, project_people.people_id, project_people.t_role, receive_status_report, name FROM project_people left join people on people.people_id=project_people.people_id");

    setTableName("project_people", "Project People");

    addColumn(0, tr("Team Member ID"), DB_STRING, false, true, true, true);
    addColumn(1, tr("Project ID"), DB_STRING, false, true, true, false);
    addColumn(2, tr("Role"), DB_STRING, true, false, true, false);
    addColumn(3, tr("Receive Status"), DB_BOOL, true, false, true, false);
    addColumn(4, tr("Name"), DB_STRING, true, false, false, false);

    //addRelatedTable("projects", "project_id", "Projecs");

    setOrderBy("name");
}
