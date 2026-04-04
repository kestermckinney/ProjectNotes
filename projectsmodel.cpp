// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseobjects.h"
#include "projectsmodel.h"

#include <QRegularExpression>
#include <QUuid>
#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

ProjectsModel::ProjectsModel(DatabaseObjects* dbo) : SqlQueryModel(dbo)
{
    setObjectName("ProjectsModel");

    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
    //setBaseSql("select * from projects_view");
    //setDeletedFilterInView(true);  // view filters deleted rows internally

    setBaseSql(R"(SELECT
        id,
        project_number,
        project_name,
        last_status_date,
        last_invoice_date,
        primary_contact,
        budget,
        actual,
        bcwp,
        bcws,
        bac,
        invoicing_period,
        status_report_period,
        client_id,
        project_status,
        (case when budget > 0 then round((actual / budget) * 100.0, 2) else NULL end) pct_consumed,
        (case when actual > 0 and bcws > 0 then round(actual + (bac - bcwp) / (bcwp/actual*bcwp/bcws), 2) else NULL end) eac,
        (case when bcwp > 0 then round((actual -  bcwp) / bcwp * 100.0, 2) else NULL end) cv,
        (case when bcws > 0 then round((bcwp -  bcws) / bcws * 100.0, 2) else NULL end) sv,
        (case when bac > 0 then round(bcwp / bac * 100.0, 2) else NULL end) pct_complete,
        (case when actual > 0 then round(bcwp / actual, 2) else NULL end) cpi
        FROM projects
        )");


    setTableName("projects", "Project");

    addColumn("id", tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    addColumn("project_number", tr("Number"), DBString, DBSearchable, DBRequired, DBEditable, DBUnique);
    addColumn("project_name", tr("Project Name"), DBString, DBSearchable, DBRequired, DBEditable, DBUnique);
    addColumn("last_status_date", tr("Status Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn("last_invoice_date", tr("Invoice Date"), DBDate, DBSearchable, DBNotRequired, DBEditable);
    addColumn("primary_contact", tr("Primary Contact"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "people", "id", "name");
    addColumn("budget", tr("Budget"), DBUSD, DBSearchable, DBNotRequired, DBEditable);
    addColumn("actual", tr("Actual"), DBUSD, DBSearchable, DBNotRequired, DBEditable);
    addColumn("bcwp", tr("BCWP"), DBUSD, DBSearchable, DBNotRequired, DBEditable);
    addColumn("bcws", tr("BCWS"), DBUSD, DBSearchable, DBNotRequired, DBEditable);
    addColumn("bac", tr("BAC"), DBUSD, DBSearchable, DBNotRequired, DBEditable);
    addColumn("invoicing_period", tr("Invoice Period"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn("status_report_period", tr("Report Period"), DBString, DBSearchable, DBNotRequired, DBEditable);
    addColumn("client_id", tr("Client"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
              "clients", "id", "client_name");
    addColumn("project_status", tr("Status"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique, &DatabaseObjects::project_status);
    addColumn("pct_consumed", tr("Consumed"), DBPercent, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn("eac", tr("EAC"), DBUSD, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn("cv", tr("CV"), DBPercent, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn("sv", tr("SV"), DBPercent, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn("pct_complete", tr("Completed"), DBPercent, DBSearchable, DBNotRequired, DBReadOnly);
    addColumn("cpi", tr("CPI"), DBReal, DBSearchable, DBNotRequired, DBReadOnly);

    addRelatedTable("project_notes", "project_id", "id", "Meeting", DBExportable);
    addRelatedTable("item_tracker", "project_id", "id", "Action/Tracker Item", DBExportable);
    addRelatedTable("project_locations", "project_id", "id", "Project Location", DBExportable);
    addRelatedTable("project_people", "project_id", "id", "Project People", DBExportable);
    addRelatedTable("status_report_items", "project_id", "id", "Status Report Item", DBExportable);

    QStringList key1 = {"project_number"};

    addUniqueKeys(key1, "Number");

    setOrderBy("project_number");
}

const QModelIndex ProjectsModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue1);
    Q_UNUSED(fkValue2);

    QString unique_stamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");

    QVector<QVariant> qr = emptyrecord();
    qr[1] = QString("[%1]").arg(unique_stamp);
    qr[2] = QString("[New Project %1]").arg(unique_stamp);
    qr[11] = tr("Monthly");
    qr[12] = tr("Bi-Weekly");
    qr[14] = tr("Active");

    return addRecord(qr);
}

QVariant ProjectsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ForegroundRole)
    {
        if (index.column() == 3) // status date
        {
            QVariant value = data(index);

            QDateTime datecol = parseDateTime(value.toString());
            qint64 dif = datecol.daysTo(QDateTime::currentDateTime());

            QString period = data( this->index(index.row(), 12)).toString();
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
        else if (index.column() == 4) // invoice date
        {
            QVariant value = data(index);

            QDateTime datecol = parseDateTime(value.toString());
            QDate nextdate = datecol.date();
            nextdate = nextdate.addMonths(1);
            nextdate.setDate(nextdate.year(), nextdate.month(), 1); // set to the first of the next month

            qint64 dif = datecol.daysTo(QDateTime::currentDateTime());

            QString period = data( this->index(index.row(), 11)).toString();

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
        else if (index.column() == 15)  // percent consumed
        {
            double value = data(index).toDouble();

            if (value >= 95.0)
                return QVariant(QCOLOR_RED);
            else if (value >= 90.0)
                return QVariant(QCOLOR_YELLOW);
        }
        else if (index.column() == 17) // cost variance
        {
            double value = data(index).toDouble();

            if (value >= 10.0)
                return QVariant(QCOLOR_RED);
            else if (value >= 5.0)
                return QVariant(QCOLOR_YELLOW);
        }
        else if (index.column() == 18)  // schedule variance
        {
            double value = data(index).toDouble();

            if (value >= 10.0)
                return QVariant(QCOLOR_RED);
            else if (value >= 5.0)
                return QVariant(QCOLOR_YELLOW);
        }
        else if (index.column() == 19)  // percent complete
        {
            double value = data(index).toDouble();

            if (value >= 95.0)
                return QVariant(QCOLOR_RED);
            else if (value >= 90.0)
                return QVariant(QCOLOR_YELLOW);
        }
        else if (index.column() == 20)  // CPI
        {
            double value = data(index).toDouble();

            if (value <= 0.8)
                return QVariant(QCOLOR_RED);
            else if (value < 1.0)
                return QVariant(QCOLOR_YELLOW);
        }
    }

    return SqlQueryModel::data(index, role);
}

bool ProjectsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool wasnew = isNewRecord(index);
    bool result = SqlQueryModel::setData(index, value, role);

    if (wasnew && result)
    {
        QString project_id = data(this->index(index.row(), 0)).toString();
        getDBOs()->addDefaultPMToProject(project_id);
    }

    return result;
}

void ProjectsModel::prepareCopiedRecord(QVector<QVariant>& newrecord, const QModelIndex& sourceIndex)
{
    // Copy columns 5-14; let base class handle making columns 1-2 unique
    for (int col = 5; col <= 14; ++col)
        newrecord[col] = data(this->index(sourceIndex.row(), col));
}

const QModelIndex ProjectsModel::copyRecord(QModelIndex index)
{
    const int row = index.row();
    const QString oldid = data(this->index(row, 0)).toString();

    QModelIndex qi = SqlQueryModel::copyRecord(index);

    if (qi.isValid())
    {
        const QString newid = data(this->index(qi.row(), 0)).toString();

        // Copy project_people records with new GUIDs
        DB_LOCK;
        QSqlQuery query(getDBOs()->getDb());
        query.prepare("SELECT people_id, role, receive_status_report FROM project_people WHERE project_id = ? and deleted = 0 "
                      "AND people_id NOT IN (SELECT people_id FROM project_people WHERE project_id = ? and deleted = 0)");
        query.addBindValue(oldid);
        query.addBindValue(newid);

        if (query.exec())
        {
            QSqlQuery insert(getDBOs()->getDb());
            insert.prepare("INSERT INTO project_people (id, project_id, people_id, role, receive_status_report) VALUES (?, ?, ?, ?, ?)");

            while (query.next())
            {
                QString pid = QUuid::createUuid().toString();

                insert.addBindValue(pid);
                insert.addBindValue(newid);
                insert.addBindValue(query.value(0));
                insert.addBindValue(query.value(1));
                insert.addBindValue(query.value(2));
                insert.exec();

                getDBOs()->pushRowChange("project_people", pid, KeyColumnChange::Insert);
            }
        }
        DB_UNLOCK;

        getDBOs()->updateDisplayData();
    }

    return qi;
}
