#include "projectteammembersmodel.h"

ProjectTeamMembersModel::ProjectTeamMembersModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectTeamMembersModel");

    setBaseSql("SELECT teammember_id, project_id, project_people.people_id, name, receive_status_report, project_people.role FROM project_people left join people on people.people_id=project_people.people_id");

    setTableName("project_people", "Project People");

    addColumn(0, tr("Team Member ID"), DB_STRING, false, true, true, true);
    addColumn(1, tr("Project ID"), DB_STRING, false, true, true, false);
    addColumn(2, tr("Name"), DB_STRING, false, false, true, false);
    addColumn(3, tr("Name"), DB_STRING, true, false, false, false);
    addColumn(4, tr("Receive Status"), DB_BOOL, true, false, true, false);
    addColumn(5, tr("Role"), DB_STRING, true, false, true, false);

    addRelatedTable("projects", "project_id", "Projects");

    setOrderBy("name");
}

QVariant ProjectTeamMembersModel::data(const QModelIndex &t_index, int t_role) const
{
    if (t_role == Qt::TextAlignmentRole)
    {
        switch (t_index.column())
        {
        case 4:
            return Qt::AlignCenter;
            break;
        }
    }

    return PNSqlQueryModel::data(t_index, t_role);
}

bool ProjectTeamMembersModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value1);
    Q_UNUSED(t_fk_value2);

    QSqlRecord qr = emptyrecord();

    qr.setValue("project_id", *t_fk_value1);

    return addRecord(qr);
}

