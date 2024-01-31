// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include <QDebug>

#include "projectsmodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>


ProjectsModel::ProjectsModel(QObject* t_parent) : PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectsModel");
    setOrderKey(100);

    setBaseSql("select * from projects_view");
    setTableName("projects", "Project");

    addColumn(0, tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    addColumn(1, tr("Number"), DBString, DBSearchable, DBRequired, DBEditable, DBUnique);
    addColumn(2, tr("Project Name"), DBString, DBSearchable, DBRequired, DBEditable, DBUnique);
    addColumn(3, tr("Status Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn(4, tr("Invoice Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn(5, tr("Primary Contact"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name");
    addColumn(6, tr("Budget"), DBUSD, DBSearchable, DBNotRequired, DBEditable);
    addColumn(7, tr("Actual"), DBUSD, DBSearchable, DBNotRequired, DBEditable);
    addColumn(8, tr("BCWP"), DBUSD, DBSearchable, DBNotRequired, DBEditable);
    addColumn(9, tr("BCWS"), DBUSD, DBSearchable, DBNotRequired, DBEditable);
    addColumn(10, tr("BAC"), DBUSD, DBSearchable, DBNotRequired, DBEditable);
    addColumn(11, tr("Invoice Period"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(12, tr("Report Period"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn(13, tr("Client"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "clients", "client_id", "client_name");
    addColumn(14, tr("Status"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique, &PNDatabaseObjects::project_status);
    addColumn(15, tr("Consumed"), DBPercent, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn(16, tr("EAC"), DBUSD, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn(17, tr("CV"), DBPercent, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn(18, tr("SV"), DBPercent, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn(19, tr("Completed"), DBPercent, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn(20, tr("CPI"), DBReal, DBSearchable, DBNotRequired, DBReadOnly);

    addRelatedTable("project_notes", "project_id", "project_id", "Meeting", DBExportable);
    addRelatedTable("item_tracker", "project_id", "project_id", "Action/Tracker Item", DBExportable);
    addRelatedTable("project_locations", "project_id", "project_id", "Project Location", DBExportable);
    addRelatedTable("project_people", "project_id", "project_id", "Project People", DBExportable);
    addRelatedTable("status_report_items", "project_id", "project_id", "Status Report Item", DBExportable);

    QStringList key1 = {"project_number"};

    addUniqueKeys(key1, "Number");

    setOrderBy("project_number");
}

const QModelIndex ProjectsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value1);
    Q_UNUSED(t_fk_value2);

    QString unique_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");

    QSqlRecord qr = emptyrecord();
    qr.setValue(1, QString("[%1]").arg(unique_stamp));
    qr.setValue(2, QString("[New Project %1]").arg(unique_stamp));
    qr.setValue(3, QVariant());
    qr.setValue(4, QVariant());
    qr.setValue(11, tr("Monthly"));
    qr.setValue(12, tr("Bi-Weekly"));
    qr.setValue(14, tr("Active"));

    return addRecord(qr);
}

QVariant ProjectsModel::data(const QModelIndex &t_index, int t_role) const
{
    if (t_role == Qt::ForegroundRole)
    {
        if (t_index.column() == 3) // status date
        {
            QVariant t_value = data(t_index);

            QDateTime datecol = parseDateTime(t_value.toString());
            qint64 dif = datecol.daysTo(QDateTime::currentDateTime());

            QString period = data( this->index(t_index.row(), 12)).toString();
            if (period == "Weekly")
            {
                if (dif > 7)
                {
                    return QVariant(QCOLOR_RED);
                }
                else if (dif == 7)
                {
                    return QVariant(QCOLOR_YELLOW);
                }
            }
            else if (period == "Bi-Weekly")
            {
                if (dif > 14)
                {
                    return QVariant(QCOLOR_RED);
                }
                else if (dif > 12)
                {
                    return QVariant(QCOLOR_YELLOW);
                }
            }
            else if (period == "Monthly")
            {
                if (dif >= 31)
                {
                    return QVariant(QCOLOR_RED);
                }
                else if (dif > 25)
                {
                    return QVariant(QCOLOR_YELLOW);
                }
            }
        }
        else if (t_index.column() == 4) // invoice date
        {
            QVariant t_value = data(t_index);

            QDateTime datecol = parseDateTime(t_value.toString());
            QDate nextdate = datecol.date();
            nextdate = nextdate.addMonths(1);
            nextdate.setDate(nextdate.year(), nextdate.month(), 1); // set to the first of the next month

            qint64 dif = datecol.daysTo(QDateTime::currentDateTime());

            QString period = data( this->index(t_index.row(), 11)).toString();

            if (period == "Milestone")
            {
                if (dif > 30)
                {
                    return QVariant(QCOLOR_YELLOW);
                }
            }
            else if (period == "Monthly")
            {
                if (QDate::currentDate() > nextdate)
                {
                    return QVariant(QCOLOR_RED);
                }
                else if (dif > 25)
                {
                    return QVariant(QCOLOR_YELLOW);
                }
            }

        }
        else if (t_index.column() == 15)  // percent consumed
        {
            double t_value = data(t_index).toDouble();

            if (t_value >= 95.0)
                return QVariant(QCOLOR_RED);
            else if (t_value >= 90.0)
                return QVariant(QCOLOR_YELLOW);
        }
        else if (t_index.column() == 17) // cost variance
        {
            double t_value = data(t_index).toDouble();

            if (t_value >= 10.0)
                return QVariant(QCOLOR_RED);
            else if (t_value >= 5.0)
                return QVariant(QCOLOR_YELLOW);
        }
        else if (t_index.column() == 18)  // schedule variance
        {
            double t_value = data(t_index).toDouble();

            if (t_value >= 10.0)
                return QVariant(QCOLOR_RED);
            else if (t_value >= 05.0)
                return QVariant(QCOLOR_YELLOW);
        }
        else if (t_index.column() == 19)  // percent complete
        {
            double t_value = data(t_index).toDouble();

            if (t_value >= 95.0)
                return QVariant(QCOLOR_RED);
            else if (t_value >= 90.0)
                return QVariant(QCOLOR_YELLOW);
        }
        else if (t_index.column() == 20)  // CPI
        {
            double t_value = data(t_index).toDouble();

            if (t_value <= 0.8)
                return QVariant(QCOLOR_RED);
            else if (t_value < 1.0)
                return QVariant(QCOLOR_YELLOW);
        }
    }

    return PNSqlQueryModel::data(t_index, t_role);
}

bool ProjectsModel::setData(const QModelIndex &t_index, const QVariant &t_value, int t_role)
{
    bool wasnew = isNewRecord(t_index);
    bool result = PNSqlQueryModel::setData(t_index, t_value, t_role);

    if (wasnew && result)
    {
        QString project_id = data(index(t_index.row(), 0)).toString();
        global_DBObjects.addDefaultPMToProject(project_id);
    }

    return result;
}

const QModelIndex ProjectsModel::copyRecord(QModelIndex t_index)
{
    QSqlRecord qr = emptyrecord();
    QString unique_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");

    qr.setValue(1, QString("Copy [%2] of %1").arg(data(index(t_index.row(), 1)).toString(), unique_stamp));
    qr.setValue(2, QString("Copy [%2] of %1").arg(data(index(t_index.row(), 2)).toString(), unique_stamp));
    qr.setValue(4, QVariant());
    qr.setValue(5, data(index(t_index.row(), 5)));
    qr.setValue(6, data(index(t_index.row(), 6)));
    qr.setValue(7, data(index(t_index.row(), 7)));
    qr.setValue(8, data(index(t_index.row(), 8)));
    qr.setValue(9, data(index(t_index.row(), 9)));
    qr.setValue(10, data(index(t_index.row(), 10)));
    qr.setValue(11, data(index(t_index.row(), 11)));
    qr.setValue(12, data(index(t_index.row(), 12)));
    qr.setValue(13, data(index(t_index.row(), 13)));
    qr.setValue(14, data(index(t_index.row(), 14)));

    QModelIndex qi = addRecord(qr);
    setData( index(qi.row(), 3), QVariant(), Qt::EditRole); // force a write to the database

    QVariant oldid = data(index(t_index.row(), 0));
    QVariant newid = data(index(qi.row(), 0));

    QString insert = "insert into project_people (teammember_id, project_id, people_id, role, receive_status_report) select m.teammember_id || '-" + unique_stamp + "', '" + newid.toString() + "', m.people_id, role, receive_status_report from project_people m where m.project_id ='" + oldid.toString() + "'  and m.people_id not in (select e.people_id from project_people e where e.project_id='" + newid.toString() + "')";

    global_DBObjects.execute(insert);
    global_DBObjects.projectteammembersmodel()->setDirty();

    return qi;
}
