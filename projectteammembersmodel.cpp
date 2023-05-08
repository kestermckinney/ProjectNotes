#include "projectteammembersmodel.h"

ProjectTeamMembersModel::ProjectTeamMembersModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectTeamMembersModel");

    setBaseSql("SELECT teammember_id, project_id, project_people.people_id, name, receive_status_report, project_people.role FROM project_people left join people on people.people_id=project_people.people_id");

    setTableName("project_people", "Project People");

    addColumn(0, tr("Team Member ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name");
    addColumn(2, tr("Name"), DBString, DBNotSearchable, DBNotRequired, DBEditable);
    addColumn(3, tr("Name"), DBString, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn(4, tr("Receive Status"), DBBool, DBSearchable, DBNotRequired, DBEditable);
    addColumn(5, tr("Role"), DBString, DBSearchable, DBNotRequired, DBEditable);

    addRelatedTable("projects", "primary_contact", "Primary Contact");
    addRelatedTable("item_tracker", "identified_by", "Identified By");
    addRelatedTable("item_tracker","assigned_to", "Assigned To");

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

