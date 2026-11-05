// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>
#include <QUrl>

// Which project_locations rows may be a project's primary project folder.
//
// A primary folder is either a local "File Folder" row or a Microsoft Teams
// channel folder. File Finder records a Teams channel folder as a "Web Link"
// row whose full_path is the folder's SharePoint URL (Teams keeps channel
// files in the team's SharePoint document library), so a Web Link counts only
// when it points at a SharePoint host. Writing into a Teams folder goes
// through Microsoft Graph; see PN::Comm::ProjectFolderStore.
namespace PN::ProjectFolder {

inline bool isTeamsFolderUrl(const QString &path)
{
    const QUrl url(path.trimmed());
    if (url.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) != 0)
        return false;
    // tenant.sharepoint.com, plus the national clouds (sharepoint.us,
    // sharepoint.de, sharepoint.cn) and *-my.sharepoint.com.
    const QString host = url.host().toLower();
    return host.contains(QLatin1String(".sharepoint."));
}

inline bool isEligibleLocation(const QString &locationType, const QString &path)
{
    if (locationType == QLatin1String("File Folder"))
        return !path.trimmed().isEmpty();
    return locationType == QLatin1String("Web Link") && isTeamsFolderUrl(path);
}

// The same rule as SQL, for a project_locations alias ("l" or "").
inline QString eligibleLocationSql(const QString &alias = QString())
{
    const QString p = alias.isEmpty() ? QString() : alias + QLatin1Char('.');
    return QStringLiteral("(%1location_type = 'File Folder' OR (%1location_type = 'Web Link' "
                          "AND %1full_path LIKE 'https://%.sharepoint.%'))").arg(p);
}

} // namespace PN::ProjectFolder
