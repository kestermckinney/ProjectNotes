// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "projectlocationsmodel.h"
#include "databaseobjects.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QUrlQuery>

ProjectLocationsModel::ProjectLocationsModel(DatabaseObjects* dbo): SqlQueryModel(dbo)
{
    setObjectName("ProjectLocationsModel");

    // note you can't use aliases for column names it will mess up query builer when it adds fundamental colums
    setBaseSql("SELECT project_locations.id, project_id, location_type, location_description, full_path, (select p.project_number from projects p where p.id=project_locations.project_id) project_number, (select p.project_name from projects p where p.id=project_locations.project_id) project_name FROM project_locations ");

    setTableName("project_locations", "Project Locations");

    addColumn("id", tr("Location ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("project_id", tr("Project ID"), DBString, DBNotSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "id", "project_number");
    addColumn("location_type", tr("Location Type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique, &DatabaseObjects::file_types);
    addColumn("location_description", tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("full_path", tr("Full Path"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("project_number", tr("Project Number"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);
    addColumn("project_name", tr("Project Name"), DBString, DBNotSearchable, DBNotRequired, DBReadOnly, DBNotUnique);

//    QStringList key1 = {"project_id", "full_path"};

//    addUniqueKeys(key1, "Full Path");

    QStringList key2 = {"project_id", "location_description"};

    addUniqueKeys(key2, "Description");

    setOrderBy("project_id");
}


const QModelIndex ProjectLocationsModel::newRecord(const QVariant* fkValue1, const QVariant* fkValue2)
{
    Q_UNUSED(fkValue2);

    QVector<QVariant> qr = emptyrecord();

    // let system generate id qr[0, QString("%1").arg(itemnumber_int, 4, 10, QLatin1Char('0')));  // Need to make a counter that looks good for items
    qr[1] = *fkValue1;
    qr[2] = "Generic File (System Identified)";

    return addRecord(qr);
}

bool ProjectLocationsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QString store_val = value.toString(); // may be replaced with protocol-prefixed URL

    if (index.column() == 4)
    {
        QModelIndex qmi_file_type = this->index(index.row(), 2);
        QVariant file_type = "Generic File (System Identified)";
        QString test_val = store_val;

        QModelIndex qmi_desc = this->index(index.row(), 3);
        QVariant desc_val = data(qmi_desc, role);

        bool isUrl = test_val.contains("http:", Qt::CaseInsensitive)
                  || test_val.contains("https:", Qt::CaseInsensitive)
                  || test_val.contains("www.", Qt::CaseInsensitive);

        // For SharePoint-style URLs the document name is in a 'file=' query param,
        // e.g. …/Doc.aspx?sourcedoc={GUID}&file=Report.docx&action=default
        // Extract it so extension detection and description both use the real name.
        QString fileParam;
        if (isUrl)
        {
            QUrlQuery query(QUrl(test_val).query());
            fileParam = query.queryItemValue("file", QUrl::FullyDecoded);
        }

        // Determine what to check for the file extension:
        //   SharePoint → filename from file= param
        //   Other URL  → base URL (path only, no query string)
        //   Local path → the path as-is
        QString pathPart = isUrl ? test_val.split('?').first() : test_val;
        QString suffix = fileParam.isEmpty() ? pathPart.right(5) : fileParam.right(5);

        if (suffix.contains(".docx", Qt::CaseInsensitive) || suffix.contains(".doc", Qt::CaseInsensitive) || suffix.contains(".dot", Qt::CaseInsensitive) || suffix.contains(".odt", Qt::CaseInsensitive) || suffix.contains(".rtf", Qt::CaseInsensitive))
        {
            file_type = "Word Document";
            if (isUrl && !test_val.startsWith("ms-word:", Qt::CaseInsensitive))
                store_val = "ms-word:ofe|u|" + test_val;
        }
        else if (suffix.contains(".xlsx", Qt::CaseInsensitive) || suffix.contains(".xls", Qt::CaseInsensitive) || suffix.contains(".ods", Qt::CaseInsensitive) || suffix.contains(".xlt", Qt::CaseInsensitive))
        {
            file_type = "Excel Document";
            if (isUrl && !test_val.startsWith("ms-excel:", Qt::CaseInsensitive))
                store_val = "ms-excel:ofe|u|" + test_val;
        }
        else if (suffix.contains(".mpp", Qt::CaseInsensitive) || suffix.contains(".mpt", Qt::CaseInsensitive))
        {
            file_type = "Microsoft Project";
            if (isUrl && !test_val.startsWith("ms-project:", Qt::CaseInsensitive))
                store_val = "ms-project:ofe|u|" + test_val;
        }
        else if (suffix.contains(".pptx", Qt::CaseInsensitive) || suffix.contains(".ppt", Qt::CaseInsensitive) || suffix.contains(".odp", Qt::CaseInsensitive) || suffix.contains(".pps", Qt::CaseInsensitive) || suffix.contains(".pot", Qt::CaseInsensitive))
        {
            file_type = "PowerPoint Document";
            if (isUrl && !test_val.startsWith("ms-powerpoint:", Qt::CaseInsensitive))
                store_val = "ms-powerpoint:ofe|u|" + test_val;
        }
        else if (suffix.contains(".pdf", Qt::CaseInsensitive))
        {
            file_type = "PDF File";
        }
        else if (isUrl)
        {
            file_type = "Web Link";
        }
        else if (!suffix.contains(".", Qt::CaseInsensitive))
        {
            file_type = "File Folder";
        }

        SqlQueryModel::setData(qmi_file_type, file_type, role);

        if (!desc_val.isValid() || desc_val.toString().isEmpty())
        {
            QString fileName;
            if (!fileParam.isEmpty())
            {
                // SharePoint: the file= param already contains the decoded filename
                fileName = fileParam;
            }
            else
            {
                // Local file / plain URL: extract last path component
                QString nameSource = pathPart;
                while (nameSource.endsWith('/') || nameSource.endsWith('\\'))
                    nameSource.chop(1);
                fileName = QFileInfo(nameSource).fileName();
                if (isUrl && !fileName.isEmpty())
                    fileName = QUrl::fromPercentEncoding(fileName.toUtf8());
            }

            SqlQueryModel::setData(qmi_desc, fileName.isEmpty() ? test_val : fileName, role);
        }
    }

    return SqlQueryModel::setData(index, QVariant(store_val), role);
}

void ProjectLocationsModel::prepareCopiedRecord(QVector<QVariant>& newrecord, const QModelIndex& sourceIndex)
{
    // Description already exists for this project, prepend "Copy of "
    newrecord[3] = QString("Copy of %1").arg(newrecord[3].toString());
}

