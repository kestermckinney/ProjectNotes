// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "FileFinderTypes.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QUrl>
#include <functional>

class MicrosoftGraphSource
{
public:
    MicrosoftGraphSource(QString bearerToken, QNetworkAccessManager *network,
                         QUrl endpoint = {}, // Empty uses the public Graph v1.0 endpoint.
                         std::function<void(const QString &)> diagnostic = {},
                         const QStringList &folderExclusions = {},
                         const QHash<QString, QString> &folderState = {});

    QList<DiscoveredLocation> discover(const QList<ActiveProject> &projects,
                                       const QList<FileFinderRule> &rules,
                                       int *filesExamined, int *matchedFiles,
                                       QString *error);
    QHash<QString, QString> folderState() const { return m_folderState; }

private:
    using CompiledRules = QList<QPair<QString, QRegularExpression>>;
    QJsonObject getObject(const QUrl &url, QString *error);
    QJsonArray getCollection(const QUrl &url, QString *error);
    QJsonObject getRelativeObject(const QString &relative, QString *error);
    bool appendChildren(const QString &driveId, const QString &itemId,
                        const QString &parentPath, const ActiveProject &project,
                        const CompiledRules &rules,
                        QList<DiscoveredLocation> *locations,
                        int *filesExamined, int *matchedFiles, QString *error);
    static QString locationType(const QString &path);
    bool isFolderExcluded(const QString &path, const QString &name) const;
    static QString folderStateKey(const QString &driveId, const QString &itemId);

    QString m_token;
    QNetworkAccessManager *m_network = nullptr;
    QUrl m_endpoint;
    std::function<void(const QString &)> m_diagnostic;
    QList<QRegularExpression> m_folderExclusions;
    QHash<QString, QString> m_previousFolderState;
    QHash<QString, QString> m_folderState;
};
