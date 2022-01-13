#include "projectteammembersmodel.h"

ProjectTeamMembersModel::ProjectTeamMembersModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectTeamMembersModel");

    setBaseSql("SELECT teammember_id, project_id, project_people.people_id, project_people.t_role, receive_status_report, name FROM project_people left join people on people.people_id=project_people.people_id");

    setTableName("project_people", "Project People");

    AddColumn(0, tr("Team Member ID"), DB_STRING, false, true, true, true);
    AddColumn(1, tr("Project ID"), DB_STRING, false, true, true, false);
    AddColumn(2, tr("Role"), DB_STRING, true, false, true, false);
    AddColumn(3, tr("Receive Status"), DB_BOOL, true, false, true, false);
    AddColumn(4, tr("Name"), DB_STRING, true, false, false, false);

    //AddRelatedTable("projects", "project_id", "Projecs");

    SetOrderBy("name");
}
