// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectteammembersmodel.h"
#include "pndatabaseobjects.h"

//#include <QDebug>

ProjectTeamMembersModel::ProjectTeamMembersModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectTeamMembersModel");
    setOrderKey(17);

    setBaseSql("SELECT teammember_id, project_id, pp.people_id, name, receive_status_report, pp.role, email, (select pr2.project_number from projects pr2 where pr2.project_id=pp.project_id) project_number, (select pr.project_name from projects pr where pr.project_id=pp.project_id) project_name, (select client_name from clients c where c.client_id=p.client_id) client_name FROM project_people pp left join people p on p.people_id=pp.people_id");

    setTableName("project_people", "Project People");

    addColumn(0, tr("Team Member ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
            "projects", "project_id", "project_number");
    addColumn(2, tr("Name"), DBString, DBNotSearchable, DBNotRequired, DBEditable, DBNotUnique,
            "people", "people_id", "name");
    addColumn(3, tr("Name"), DBString, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn(4, tr("Receive Status"), DBBool, DBSearchable, DBNotRequired, DBEditable);
    addColumn(5, tr("Role"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(6, tr("Email"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);
    addColumn(7, tr("Project Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);
    addColumn(8, tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);
    addColumn(9, tr("Client Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);

    QStringList key1 = {"project_id", "people_id"};

    addUniqueKeys(key1, "Name");

    QStringList rel_col4 = { "project_id", "people_id" };
    QStringList rel_fk4 = { "project_id", "primary_contact" };
    addRelatedTable("projects", rel_fk4, rel_col4, "Primary Contact");

    QStringList rel_col1 = { "project_id", "people_id" };
    QStringList rel_fk1 = { "project_id", "identified_by" };
    addRelatedTable("item_tracker", rel_fk1, rel_col1, "Identified By");

    QStringList rel_col2 = { "project_id", "people_id" };
    QStringList rel_fk2 = { "project_id", "assigned_to" };
    addRelatedTable("item_tracker", rel_fk2, rel_col2, "Assigned To");

    QStringList rel_col3 = { "project_id", "people_id" };
    QStringList rel_fk3 = { "project_id", "person_id" };
    addRelatedTable("meeting_attendees", rel_fk3, rel_col3, "Attendee");

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
    if (PNSqlQueryModel::setData(t_index, t_value, t_role))
    {
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

        return true;
    }

    return false;
}

bool ProjectTeamMembersModel::openRecord(QModelIndex t_index)
{
    Q_UNUSED(t_index);

    global_DBObjects.peoplemodel()->deactivateUserFilter(global_DBObjects.peoplemodel()->objectName());

    return true;
}
