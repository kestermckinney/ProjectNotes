#include "projectteammembersmodel.h"
#include <QDebug>

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

    QStringList key1 = {"project_id", "people_id"};

    addUniqueKeys(key1, "Name");

    addRelatedTable("projects", "primary_contact", "people_id", "Primary Contact");
    addRelatedTable("item_tracker", "identified_by", "people_id", "Identified By");
    addRelatedTable("item_tracker","assigned_to", "people_id", "Assigned To");
    addRelatedTable("meeting_attendees", "person_id", "people_id", "Attendee");

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

bool ProjectTeamMembersModel::setData(const QModelIndex &t_index, const QVariant &t_value, int t_role)
{
    // if setting team member and no role is available grab the default
    if (t_index.column() == 2)
    {
        QModelIndex qi = index(t_index.row(), 5);

        if (data(qi).isNull())
        {
            QModelIndex qi_key = index(t_index.row(), 2);

            // get the default
            QSqlQuery qry(QString("select role from people where people_id='%1'").arg( data(qi_key).toString() ));
            //qDebug() << QString("select role from people where people_id='%1'").arg( data(qi_key).toString() );
            qry.exec();

            if (qry.next())
            {
                setData(qi, qry.record().value(0), t_role);
            }
        }
    }

    return PNSqlQueryModel::setData(t_index, t_value, t_role);
}
