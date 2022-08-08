#include "projectsmodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>

ProjectsModel::ProjectsModel(QObject* t_parent) : PNSqlQueryModel(t_parent)
{
    setObjectName("ProjectsModel");

    setBaseSql("SELECT project_id, project_number, project_name, last_status_date, last_invoice_date, primary_contact, budget, actual,"
        " bcwp, bcws, bac, invoicing_period, status_report_period, client_id, project_status, "
        " (case when budget > 0 then (actual / budget) * 100.0 else NULL end) pct_consumed, "
        " (case when actual > 0 and bcws > 0 then actual + (bac - bcwp) / (bcwp/actual*bcwp/bcws) else NULL end) eac, "
        " (case when bcwp > 0 then (actual -  bcwp) / bcwp * 100.0 else NULL end) cv, "
        " (case when bcws > 0 then (bcwp -  bcws) / bcws * 100.0 else NULL end) sv, "
        " (case when bac > 0 then bcwp / bac * 100.0 else NULL end) pct_complete, "
        " (case when actual > 0 then round(bcwp / actual, 2) else NULL end) cpi "
        " FROM projects");

    setTableName("projects", "Project");

    addColumn(0, tr("Project ID"), DB_STRING, false, true, false);
    addColumn(1, tr("Number"), DB_STRING, true, true, true, true);
    addColumn(2, tr("Project Name"), DB_STRING, true, true, true, true);
    addColumn(3, tr("Status Date"), DB_DATE, true, false, true);
    addColumn(4, tr("Invoice Date"), DB_DATE, true, false, true);
    addColumn(5, tr("Primary Contact"), DB_STRING, true, false, true);
    addColumn(6, tr("Budget"), DB_USD, true, false, true);
    addColumn(7, tr("Actual"), DB_USD, true, false, true);
    addColumn(8, tr("BCWP"), DB_USD, true, false, true);
    addColumn(9, tr("BCWS"), DB_USD, true, false, true);
    addColumn(10, tr("BAC"), DB_USD, true, false, true);
    addColumn(11, tr("Invoice Period"), DB_STRING, true, false, true);
    associateLookupValues(11, &PNDatabaseObjects::invoicing_period);
    addColumn(12, tr("Report Period"), DB_STRING, true, false, true);
    associateLookupValues(12, &PNDatabaseObjects::status_report_period);
    addColumn(13, tr("Client"), DB_STRING, true, false, true);
    addColumn(14, tr("Status"), DB_STRING, true, false, true);
    associateLookupValues(14, &PNDatabaseObjects::project_status);
    addColumn(15, tr("Consumed"), DB_PERCENT, true, false, false);
    addColumn(16, tr("EAC"), DB_USD, true, false, false);
    addColumn(17, tr("CV"), DB_PERCENT, true, false, false);
    addColumn(18, tr("SV"), DB_PERCENT, true, false, false);
    addColumn(19, tr("Completed"), DB_PERCENT, true, false, false);
    addColumn(20, tr("CPI"), DB_REAL, true, false, false);

    addRelatedTable("project_notes", "project_id", "Meeting");
    addRelatedTable("item_tracker", "project_id", "Action/Tracker Item");
    addRelatedTable("project_locations", "project_id", "Project Location");
    addRelatedTable("project_people", "project_id", "Project People");
    addRelatedTable("status_report_items", "project_id", "Status Report Item");

    setOrderBy("project_number");
}

bool ProjectsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value1);
    Q_UNUSED(t_fk_value2);

    QSqlQuery select;
    select.prepare("select max(project_number) from projects where project_number like '[%'");
    QString maxnum;

    select.exec();
    if (select.next())
    {
        maxnum = select.value(0).toString();
        maxnum.remove(QRegularExpression("[^0-9]+"));
    }

    int num = maxnum.toInt() + 1;

    QSqlRecord qr = emptyrecord();
    qr.setValue(1, QString("[%1]").arg(num, 4, 10, QLatin1Char('0')));
    qr.setValue(2, QString("[New Project %1]").arg(num, 2, 10, QLatin1Char('0')));
    qr.setValue(11, tr("Monthly"));
    qr.setValue(12, tr("Bi-Weekly"));
    qr.setValue(14, tr("Active"));

    addRecord(qr);

    return true;
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
                    return QVariant(QColor(Qt::darkRed));
                }
                else if (dif == 7)
                {
                    return QVariant(QColor(Qt::darkYellow));
                }
            }
            else if (period == "Bi-Weekly")
            {
                if (dif > 14)
                {
                    return QVariant(QColor(Qt::darkRed));
                }
                else if (dif > 12)
                {
                    return QVariant(QColor(Qt::darkYellow));
                }
            }
            else if (period == "Monthly")
            {
                if (dif >= 31)
                {
                    return QVariant(QColor(Qt::darkRed));
                }
                else if (dif > 25)
                {
                    return QVariant(QColor(Qt::darkYellow));
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
                    return QVariant(QColor(Qt::darkYellow));
                }
            }
            else if (period == "Monthly")
            {
                if (QDate::currentDate() > nextdate)
                {
                    return QVariant(QColor(Qt::darkRed));
                }
                else if (dif > 25)
                {
                    return QVariant(QColor(Qt::darkYellow));
                }
            }

        }
        else if (t_index.column() == 15)  // percent consumed
        {
            double t_value = data(t_index).toDouble();

            if (t_value >= 95.0)
                return QVariant(QColor(Qt::darkRed));
            else if (t_value >= 90.0)
                return QVariant(QColor(Qt::darkYellow));
        }
        else if (t_index.column() == 17) // cost variance
        {
            double t_value = data(t_index).toDouble();

            if (t_value >= 10.0)
                return QVariant(QColor(Qt::darkRed));
            else if (t_value >= 5.0)
                return QVariant(QColor(Qt::darkYellow));
        }
        else if (t_index.column() == 18)  // schedule variance
        {
            double t_value = data(t_index).toDouble();

            if (t_value >= 10.0)
                return QVariant(QColor(Qt::darkRed));
            else if (t_value >= 05.0)
                return QVariant(QColor(Qt::darkYellow));
        }
        else if (t_index.column() == 19)  // percent complete
        {
            double t_value = data(t_index).toDouble();

            if (t_value >= 95.0)
                return QVariant(QColor(Qt::darkRed));
            else if (t_value >= 90.0)
                return QVariant(QColor(Qt::darkYellow));
        }
        else if (t_index.column() == 20)  // CPI
        {
            double t_value = data(t_index).toDouble();

            if (t_value <= 0.8)
                return QVariant(QColor(Qt::darkRed));
            else if (t_value < 1.0)
                return QVariant(QColor(Qt::darkYellow));
        }
    }

    return PNSqlQueryModel::data(t_index, t_role);
}
