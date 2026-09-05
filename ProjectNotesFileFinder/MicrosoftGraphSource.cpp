// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "MicrosoftGraphSource.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QThread>
#include <QTimer>

MicrosoftGraphSource::MicrosoftGraphSource(QString bearerToken,
                                           QNetworkAccessManager *network,
                                           QUrl endpoint)
    : m_token(std::move(bearerToken)), m_network(network), m_endpoint(std::move(endpoint))
{
    QString value = m_endpoint.toString();
    if (!value.endsWith(QLatin1Char('/')))
        value.append(QLatin1Char('/'));
    m_endpoint = QUrl(value);
}

QJsonObject MicrosoftGraphSource::getObject(const QUrl &url, QString *error)
{
    if (error)
        error->clear();
    if (!m_network) {
        if (error)
            *error = QStringLiteral("Microsoft Graph network manager is unavailable.");
        return {};
    }

    const QUrl requestUrl = url.isRelative() ? m_endpoint.resolved(url) : url;
    QNetworkRequest request(requestUrl);
    request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    request.setRawHeader("Accept", "application/json");
    QNetworkReply *reply = m_network->get(request);
    QEventLoop loop;
    QTimer timeout;
    QTimer interruptionPoll;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&interruptionPoll, &QTimer::timeout, &loop, [&] {
        if (QThread::currentThread()->isInterruptionRequested()) {
            reply->abort();
            loop.quit();
        }
    });
    timeout.start(30000);
    interruptionPoll.start(100);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        if (error)
            *error = QStringLiteral("Microsoft Graph request timed out: %1").arg(requestUrl.toString());
        reply->deleteLater();
        return {};
    }
    const QByteArray body = reply->readAll();
    const auto networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    reply->deleteLater();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (networkError != QNetworkReply::NoError) {
        if (error)
            *error = QStringLiteral("Microsoft Graph request failed for %1: %2")
                         .arg(requestUrl.toString(), networkErrorText);
        return {};
    }
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error)
            *error = QStringLiteral("Microsoft Graph returned invalid JSON for %1: %2")
                         .arg(requestUrl.toString(), parseError.errorString());
        return {};
    }
    return document.object();
}

QJsonObject MicrosoftGraphSource::getRelativeObject(const QString &relative, QString *error)
{
    return getObject(QUrl(relative), error);
}

QJsonArray MicrosoftGraphSource::getCollection(const QUrl &url, QString *error)
{
    QJsonArray values;
    QUrl page = url;
    while (!page.isEmpty()) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            if (error)
                *error = QStringLiteral("Microsoft Graph scan was cancelled.");
            return {};
        }
        const QJsonObject object = getObject(page, error);
        if (error && !error->isEmpty())
            return {};
        for (const QJsonValue &value : object.value(QStringLiteral("value")).toArray())
            values.append(value);
        page = QUrl(object.value(QStringLiteral("@odata.nextLink")).toString());
    }
    return values;
}

QList<DiscoveredLocation> MicrosoftGraphSource::discover(
    const QList<ActiveProject> &projects, const QList<FileFinderRule> &rules,
    int *filesExamined, int *matchedFiles, QString *error)
{
    QList<DiscoveredLocation> result;
    if (filesExamined)
        *filesExamined = 0;
    if (matchedFiles)
        *matchedFiles = 0;
    if (projects.isEmpty() || m_token.isEmpty())
        return result;

    CompiledRules compiledRules;
    for (const FileFinderRule &rule : rules) {
        QRegularExpression expression(rule.pattern, QRegularExpression::CaseInsensitiveOption);
        if (expression.isValid() && !rule.pattern.isEmpty())
            compiledRules.append({rule.classification, expression});
    }

    struct ProjectMatcher {
        ActiveProject project;
        QRegularExpression expression;
    };
    QList<ProjectMatcher> projectMatchers;
    projectMatchers.reserve(projects.size());
    for (const ActiveProject &project : projects) {
        projectMatchers.append({project,
            QRegularExpression(
                QStringLiteral("(?<![A-Za-z0-9])%1(?![A-Za-z0-9])")
                    .arg(QRegularExpression::escape(project.number)),
                QRegularExpression::CaseInsensitiveOption)});
    }

    const QJsonArray teams = getCollection(
        QUrl(QStringLiteral("me/joinedTeams?$select=id,displayName")), error);
    if (error && !error->isEmpty())
        return {};

    QSet<QString> foundProjects;
    for (const QJsonValue &teamValue : teams) {
        const QString teamId = teamValue.toObject().value(QStringLiteral("id")).toString();
        if (teamId.isEmpty())
            continue;
        const QString encodedTeam = QString::fromLatin1(QUrl::toPercentEncoding(teamId));
        const QJsonArray channels = getCollection(
            QUrl(QStringLiteral("teams/%1/channels?$select=id,displayName").arg(encodedTeam)),
            error);
        if (error && !error->isEmpty())
            return {};

        for (const QJsonValue &channelValue : channels) {
            const QJsonObject channel = channelValue.toObject();
            const QString channelName = channel.value(QStringLiteral("displayName")).toString();
            const QString channelId = channel.value(QStringLiteral("id")).toString();
            if (channelId.isEmpty())
                continue;

            for (const ProjectMatcher &matcher : projectMatchers) {
                const ActiveProject &project = matcher.project;
                if (foundProjects.contains(project.id)
                    || !channelName.contains(project.number, Qt::CaseInsensitive))
                    continue;
                if (!matcher.expression.match(channelName).hasMatch())
                    continue;

                const QString encodedChannel = QString::fromLatin1(QUrl::toPercentEncoding(channelId));
                const QJsonObject folder = getRelativeObject(
                    QStringLiteral("teams/%1/channels/%2/filesFolder")
                        .arg(encodedTeam, encodedChannel), error);
                if (error && !error->isEmpty())
                    return {};
                const QString driveId = folder.value(QStringLiteral("parentReference"))
                                            .toObject().value(QStringLiteral("driveId")).toString();
                const QString itemId = folder.value(QStringLiteral("id")).toString();
                const QString webUrl = folder.value(QStringLiteral("webUrl")).toString();
                if (driveId.isEmpty() || itemId.isEmpty() || webUrl.isEmpty())
                    continue;

                result.append({project.id, QStringLiteral("Microsoft Teams"),
                               QStringLiteral("Office 365: Project Folder"), webUrl});
                if (!appendChildren(driveId, itemId, {}, project, compiledRules, &result,
                                    filesExamined, matchedFiles, error))
                    return {};
                foundProjects.insert(project.id);
            }
            if (foundProjects.size() == projects.size())
                break;
        }
        if (foundProjects.size() == projects.size())
            break;
    }
    return result;
}

bool MicrosoftGraphSource::appendChildren(
    const QString &driveId, const QString &itemId, const QString &parentPath,
    const ActiveProject &project, const CompiledRules &rules,
    QList<DiscoveredLocation> *locations, int *filesExamined, int *matchedFiles,
    QString *error)
{
    const QString encodedDrive = QString::fromLatin1(QUrl::toPercentEncoding(driveId));
    const QString encodedItem = QString::fromLatin1(QUrl::toPercentEncoding(itemId));
    QUrl page(QStringLiteral("drives/%1/items/%2/children?"
                             "$select=id,name,size,lastModifiedDateTime,webUrl,file,folder")
                  .arg(encodedDrive, encodedItem));
    while (!page.isEmpty()) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            if (error)
                *error = QStringLiteral("Microsoft Graph scan was cancelled.");
            return false;
        }
        const QJsonObject object = getObject(page, error);
        if (error && !error->isEmpty())
            return false;
        for (const QJsonValue &value : object.value(QStringLiteral("value")).toArray()) {
            const QJsonObject item = value.toObject();
            const QString name = item.value(QStringLiteral("name")).toString();
            const QString id = item.value(QStringLiteral("id")).toString();
            const QString relative = parentPath.isEmpty() ? name : parentPath + QLatin1Char('/') + name;
            if (name.isEmpty() || id.isEmpty())
                continue;
            if (item.contains(QStringLiteral("folder"))) {
                if (!appendChildren(driveId, id, relative, project, rules, locations,
                                    filesExamined, matchedFiles, error))
                    return false;
                continue;
            }
            if (filesExamined)
                ++*filesExamined;
            for (const auto &rule : rules) {
                if (!rule.second.match(relative).hasMatch()
                    && !rule.second.match(name).hasMatch())
                    continue;
                if (matchedFiles)
                    ++*matchedFiles;
                locations->append({project.id, locationType(name),
                    QStringLiteral("Office 365: %1 : %2").arg(rule.first, relative),
                    item.value(QStringLiteral("webUrl")).toString()});
                break;
            }
        }
        page = QUrl(object.value(QStringLiteral("@odata.nextLink")).toString());
    }
    return true;
}

QString MicrosoftGraphSource::locationType(const QString &path)
{
    const QString extension = path.section(QLatin1Char('.'), -1).toLower();
    if (extension == QLatin1String("xlsx") || extension == QLatin1String("xls"))
        return QStringLiteral("Excel Document");
    if (extension == QLatin1String("docx") || extension == QLatin1String("doc"))
        return QStringLiteral("Word Document");
    if (extension == QLatin1String("pdf"))
        return QStringLiteral("PDF File");
    if (extension == QLatin1String("mpp"))
        return QStringLiteral("Microsoft Project");
    if (extension == QLatin1String("pptx") || extension == QLatin1String("ppt"))
        return QStringLiteral("PowerPoint Document");
    return QStringLiteral("Generic File (System Identified)");
}
