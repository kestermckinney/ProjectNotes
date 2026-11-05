// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QHash>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>

struct FileFinderRule
{
    QString classification;
    QString pattern;
};

struct FileFinderConfiguration
{
    QStringList roots;
    QStringList folderExclusions;
    QHash<QString, QString> graphFolderState;
    QList<FileFinderRule> rules;
    bool enabled = false;
    bool office365Enabled = false;
    // When a project has both a local project folder and a Microsoft Teams
    // channel folder, which one File Finder makes the primary project folder.
    bool preferTeamsProjectFolder = false;
    QString accessToken;
};

// The description File Finder gives each kind of project folder row. They
// differ so a project can list both its local folder and its Teams folder.
inline QString localProjectFolderDescription() { return QStringLiteral("Project Folder"); }
inline QString teamsProjectFolderDescription() { return QStringLiteral("Teams Project Folder"); }

struct ActiveProject
{
    QString id;
    QString number;
};

struct DiscoveredLocation
{
    QString projectId;
    QString locationType;
    QString description;
    QString fullPath;
    // True when this location came from a remote source (e.g. Microsoft
    // Teams/SharePoint via Graph) rather than the local/system filesystem.
    // Used by FileFinderWorker::commitLocations() to prefer local discoveries
    // over remote ones when both describe the same file.
    bool isRemote = false;
};

struct FileFinderScanSummary
{
    int projects = 0;
    int files = 0;
    int matched = 0;
    int inserted = 0;
    int updated = 0;
    int unchanged = 0;
    int primaryFolders = 0;   // projects whose primary project folder this scan set
    qint64 elapsedMs = 0;
    QString warning;
    QString error;
};

Q_DECLARE_METATYPE(FileFinderRule)
Q_DECLARE_METATYPE(QList<FileFinderRule>)
Q_DECLARE_METATYPE(FileFinderConfiguration)
Q_DECLARE_METATYPE(FileFinderScanSummary)
