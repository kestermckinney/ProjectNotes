// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectteammembersmodel.h"
#include "databaseobjects.h"

#include "QLogger.h"
#include "QLoggerWriter.h"

#include <QMessageBox>

using namespace QLogger;


ProjectTeamMembersModel::ProjectTeamMembersModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("ProjectTeamMembersModel");
    setOrderKey(17);

    setBaseSql("SELECT teammember_id, project_id, pp.people_id, name, receive_status_report, pp.role, email, (select pr2.project_number from projects pr2 where pr2.project_id=pp.project_id) project_number, (select pr.project_name from projects pr where pr.project_id=pp.project_id) project_name, (select client_name from clients c where c.client_id=p.client_id) client_name FROM project_people pp left join people p on p.people_id=pp.people_id");

    setTableName("project_people", "Project People");

    addColumn("teammember_id", tr("Team Member ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
            "projects", "project_id", "project_number");
    addColumn("people_id", tr("Name"), DBString, DBNotSearchable, DBNotRequired, DBEditable, DBNotUnique,
            "people", "people_id", "name");
    addColumn("name", tr("Name"), DBString, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn("receive_status_report", tr("Receive Status"), DBBool, DBSearchable, DBNotRequired, DBEditable);
    addColumn("role", tr("Role"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn("email", tr("Email"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);
    addColumn("project_number", tr("Project Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);
    addColumn("project_name", tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);
    addColumn("client_name", tr("Client Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly);

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

QVariant ProjectTeamMembersModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column())
        {
        case 4:
            return Qt::AlignCenter;
            break;
        }
    }

    return SqlQueryModel::data(index, role);
}

const QModelIndex ProjectTeamMembersModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue1);
    Q_UNUSED(fkValue2);

    QVector<QVariant> qr = emptyrecord();

    qr[getColumnNumber("project_id")] = *fkValue1;

    return addRecord(qr);
}

bool ProjectTeamMembersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // prevent changing the person if they are referenced as a meeting attendee for this project
    // (meeting_attendees has no project_id column, so the base class columnChangeCheck cannot handle it;
    //  we must check via a subquery through project_notes)
    if (role == Qt::EditRole && index.column() == 2)
    {
        QVariant current_people_id = data(this->index(index.row(), 2));
        QVariant current_project_id = data(this->index(index.row(), 1));

        if (!current_people_id.isNull() && !current_people_id.toString().isEmpty())
        {
            DB_LOCK;
            QSqlQuery qry(getDBOs()->getDb());
            qry.prepare(QString("SELECT count(*) FROM meeting_attendees WHERE person_id = '%1' AND note_id IN (SELECT note_id FROM project_notes WHERE project_id = '%2')")
                        .arg(current_people_id.toString(), current_project_id.toString()));
            qry.exec();

            int count = 0;
            if (qry.next())
                count = qry.value(0).toInt();

            DB_UNLOCK;

            if (count > 0)
            {
                // look up the project number for the key search
                QString project_number_key;
                DB_LOCK;
                QSqlQuery projqry(getDBOs()->getDb());
                projqry.prepare(QString("SELECT project_number FROM projects WHERE project_id = '%1'").arg(current_project_id.toString()));
                projqry.exec();
                if (projqry.next())
                    project_number_key = projqry.value(0).toString();
                DB_UNLOCK;

                QString message = tr("Team Member is referenced in the following item(s):\n\n") +
                    QString::number(count) + tr(" Meetings(s)\n\n") +
                    tr("You cannot change Team Member until they are no longer assocated with the following items. Would you like to run a search for all related items?");

                if ( QMessageBox::question(nullptr, tr("Cannot Change Team member"),
                     message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes )
                {
                    QStringList key_columns;
                    QStringList key_values;
                    key_columns.append("project_number");
                    key_values.append(project_number_key);
                    key_columns.append("datakey");
                    key_values.append(current_people_id.toString());

                    getDBOs()->searchresultsmodel()->PerformKeySearch(key_columns, key_values);
                    emit callKeySearch();
                }

                emit dataChanged(index, index);
                return false;
            }
        }
    }

    // if setting team member and no role is available grab the default
    if (SqlQueryModel::setData(index, value, role))
    {
        if (index.column() == 2)
        {
            QModelIndex qi = this->index(index.row(), 5);

            if (data(qi).isNull())
            {
                QModelIndex qi_key = this->index(index.row(), 2);
                DB_LOCK;


                // get the default
                QSqlQuery qry(getDBOs()->getDb());
                qry.prepare(QString("select role from people where people_id='%1'").arg( data(qi_key).toString()));
                //qDebug() << QString("select role from people where people_id='%1'").arg( data(qi_key).toString() );

                qry.exec();

                if (qry.next())
                {

                    DB_UNLOCK;
                    setData(qi, qry.record().value(0), role);
                }
                else
                {

                    DB_UNLOCK;
                }
            }
        }

        return true;
    }

    return false;
}

