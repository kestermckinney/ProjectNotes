// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

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
    QList<FileFinderRule> rules;
    bool enabled = false;
    bool office365Enabled = false;
    QString accessToken;
};

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
};

struct FileFinderScanSummary
{
    int projects = 0;
    int files = 0;
    int matched = 0;
    int inserted = 0;
    int updated = 0;
    int unchanged = 0;
    qint64 elapsedMs = 0;
    QString warning;
    QString error;
};

Q_DECLARE_METATYPE(FileFinderRule)
Q_DECLARE_METATYPE(QList<FileFinderRule>)
Q_DECLARE_METATYPE(FileFinderConfiguration)
Q_DECLARE_METATYPE(FileFinderScanSummary)
