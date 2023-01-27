#include "projectlocationsmodel.h"
#include "pndatabaseobjects.h"

#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>

ProjectLocationsModel::ProjectLocationsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjedtLocationsModel");

    setBaseSql("SELECT location_id, project_id, location_type, location_description, full_path FROM project_locations");

    setTableName("project_locations", "Project Locations");

    addColumn(0, tr("Location ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn(2, tr("Location Type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &PNDatabaseObjects::file_types);
    addColumn(3, tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(4, tr("Full Path"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);

    setOrderBy("location_id");
}


bool ProjectLocationsModel::newRecord(const QVariant* t_fk_value1, const QVariant* t_fk_value2)
{
    Q_UNUSED(t_fk_value2);

    QSqlRecord qr = emptyrecord();

    // let system generate id qr.setValue(0, QString("%1").arg(itemnumber_int, 4, 10, QLatin1Char('0')));  // Need to make a counter that looks good for items
    qr.setValue(1, *t_fk_value1);
    qr.setValue(2, "Generic File (System Identified)");
    qr.setValue(3, QVariant());
    qr.setValue(4, QVariant());

    return addRecord(qr);
}

bool ProjectLocationsModel::setData(const QModelIndex &t_index, const QVariant &t_value, int t_role)
{
    // if the issue was changed to resolved them change the resolved date
    if (t_index.column() == 4)
    {
        QModelIndex qmi_file_type = index(t_index.row(), 2);
        QVariant file_type = "Generic File (System Identified)";
        QString test_val = t_value.toString();

        QModelIndex qmi_desc = index(t_index.row(), 3);
        QVariant desc_val = data(qmi_desc, t_role);

        if ( test_val.right(5).contains(".doc", Qt::CaseInsensitive) || test_val.right(5).contains(".dot", Qt::CaseInsensitive) || test_val.right(5).contains(".odt", Qt::CaseInsensitive) || test_val.right(5).contains(".rtf", Qt::CaseInsensitive) )
        {
            file_type = "Word Document";
        }
        else if ( test_val.right(5).contains(".xls", Qt::CaseInsensitive) || test_val.right(5).contains(".ods", Qt::CaseInsensitive) || test_val.right(5).contains(".xlt", Qt::CaseInsensitive) )
        {
            file_type = "Excel Document";
        }
        else if ( test_val.right(5).contains(".mpp", Qt::CaseInsensitive) || test_val.right(5).contains(".mpt", Qt::CaseInsensitive) )
        {
            file_type = "Microsoft Project";
        }
        else if ( test_val.right(5).contains(".ppt", Qt::CaseInsensitive) || test_val.right(5).contains(".odp", Qt::CaseInsensitive) || test_val.right(5).contains(".pps", Qt::CaseInsensitive) || test_val.right(5).contains(".pot", Qt::CaseInsensitive) )
        {
            file_type = "PowerPoint Document";
        }
        else if ( test_val.right(5).contains(".pdf", Qt::CaseInsensitive) || test_val.right(5).contains(".odp", Qt::CaseInsensitive) || test_val.right(5).contains(".pps", Qt::CaseInsensitive) || test_val.right(5).contains(".pot", Qt::CaseInsensitive) )
        {
            file_type = "PDF File";
        }
        else if ( test_val.contains("http:", Qt::CaseInsensitive) || test_val.contains("https:", Qt::CaseInsensitive) || test_val.contains("www.", Qt::CaseInsensitive) )
        {
            file_type = "Web Link";
        }
        else if ( !test_val.right(5).contains(".", Qt::CaseInsensitive) )
        {
            file_type = "File Folder";
        }

        PNSqlQueryModel::setData(qmi_file_type, file_type, t_role);

        if ( !desc_val.isValid() || desc_val.toString().isEmpty() )
        {
            if ( file_type != "File Folder" && file_type != "Web Link")
            {
                QFileInfo fi(test_val);
                PNSqlQueryModel::setData(qmi_desc, fi.completeBaseName(), t_role);
            }
            else
            {
                PNSqlQueryModel::setData(qmi_desc, t_value, t_role);
            }
        }
    }


    return PNSqlQueryModel::setData(t_index, t_value, t_role);
}

bool ProjectLocationsModel::openRecord(QModelIndex t_index)
{
    QVariant location = data(index(t_index.row(), 4));
    QVariant location_type = data(index(t_index.row(), 2));

    if ( location_type == "Web Link" )
    {
        QDesktopServices::openUrl(QUrl(location.toString(), QUrl::TolerantMode));
    }
    else
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(location.toString()));
    }

    return true;
}
