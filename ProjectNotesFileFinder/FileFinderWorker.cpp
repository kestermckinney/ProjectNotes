// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "FileFinderWorker.h"
#include "MicrosoftGraphSource.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QRegularExpression>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QWriteLocker>

namespace {
constexpr int kReconciliationIntervalMs = 5 * 60 * 1000;
const QChar kKeySeparator(0x1f);

QString locationKey(const QString &projectId, const QString &value)
{
    return projectId + kKeySeparator + value;
}

}

FileFinderWorker::FileFinderWorker(QObject *parent) : QObject(parent)
{
}

void FileFinderWorker::initializeDatabase(const QString &databasePath,
                                          QReadWriteLock *databaseLock)
{
    closeDatabase();
    m_databaseLock = databaseLock;
    m_connectionName = QStringLiteral("ProjectNotesFileFinder-%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16);
    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setDatabaseName(databasePath);
    m_database.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
    if (!m_database.open())
        emit diagnostic(tr("File Finder could not open the Project Notes database: %1")
                            .arg(m_database.lastError().text()));
}

void FileFinderWorker::configure(const FileFinderConfiguration &configuration)
{
    m_configuration = configuration;
    if (!m_configuration.enabled && m_timer)
        m_timer->stop();
    else if (m_running)
        QTimer::singleShot(0, this, &FileFinderWorker::scanNow);
}

void FileFinderWorker::start()
{
    if (m_running)
        return;
    m_running = true;
    if (!m_timer) {
        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        m_timer->setTimerType(Qt::VeryCoarseTimer);
        connect(m_timer, &QTimer::timeout, this, &FileFinderWorker::scanNow);
    }
    if (m_configuration.enabled)
        scanNow();
}

void FileFinderWorker::stop()
{
    m_running = false;
    m_rescanPending = false;
    if (m_timer)
        m_timer->stop();
}

void FileFinderWorker::scanNow()
{
    if (!m_configuration.enabled || !m_database.isOpen())
        return;
    if (m_scanning) {
        m_rescanPending = true;
        return;
    }

    m_scanning = true;
    m_rescanPending = false;
    emit scanStarted();
    QElapsedTimer elapsed;
    elapsed.start();
    FileFinderScanSummary summary;
    QString error;

    const QList<ActiveProject> projects = activeProjects(&error);
    summary.projects = projects.size();
    if (error.isEmpty()) {
        const QHash<QString, QString> folders = findLocalProjectFolders(projects);
        QList<DiscoveredLocation> locations = scanLocalFolders(
            projects, folders, &summary.files, &summary.matched);

        if (m_configuration.office365Enabled && !m_configuration.accessToken.isEmpty()
            && !QThread::currentThread()->isInterruptionRequested()) {
            if (!m_network)
                m_network = new QNetworkAccessManager(this);
            int remoteFiles = 0;
            int remoteMatches = 0;
            QString graphError;
            MicrosoftGraphSource graph(m_configuration.accessToken, m_network);
            const QList<DiscoveredLocation> remote = graph.discover(
                projects, m_configuration.rules, &remoteFiles, &remoteMatches, &graphError);
            summary.files += remoteFiles;
            summary.matched += remoteMatches;
            locations.append(remote);
            if (!graphError.isEmpty()) {
                summary.warning = graphError;
                emit diagnostic(graphError);
            }
        }

        if (!QThread::currentThread()->isInterruptionRequested())
            commitLocations(locations, &summary, &error);
        else
            error = tr("File Finder scan was cancelled.");
    }

    summary.error = error;
    summary.elapsedMs = elapsed.elapsed();
    if (!error.isEmpty())
        emit diagnostic(error);
    emit scanFinished(summary);
    m_scanning = false;

    if (m_rescanPending && m_running)
        QTimer::singleShot(0, this, &FileFinderWorker::scanNow);
    else
        scheduleNextScan();
}

void FileFinderWorker::closeDatabase()
{
    if (!m_connectionName.isEmpty()) {
        m_database.close();
        m_database = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
    }
}

QList<ActiveProject> FileFinderWorker::activeProjects(QString *error)
{
    QList<ActiveProject> projects;
    if (!m_database.isOpen() || !m_databaseLock) {
        if (error)
            *error = tr("File Finder database connection is unavailable.");
        return projects;
    }
    QReadLocker locker(m_databaseLock);
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, project_number FROM projects "
        "WHERE deleted = 0 AND project_status = 'Active' "
        "AND trim(coalesce(project_number, '')) <> ''"));
    if (!query.exec()) {
        if (error)
            *error = query.lastError().text();
        return projects;
    }
    while (query.next())
        projects.append({query.value(0).toString(), query.value(1).toString().trimmed()});
    return projects;
}

QHash<QString, QString> FileFinderWorker::findLocalProjectFolders(
    const QList<ActiveProject> &projects)
{
    QHash<QString, QString> folders;
    if (projects.isEmpty())
        return folders;

    struct ProjectMatcher {
        QString id;
        QString number;
        QRegularExpression expression;
    };
    QList<ProjectMatcher> matchers;
    matchers.reserve(projects.size());
    for (const ActiveProject &project : projects) {
        matchers.append({project.id, project.number,
            QRegularExpression(
                QStringLiteral("(?<![A-Za-z0-9])%1(?![A-Za-z0-9])")
                    .arg(QRegularExpression::escape(project.number)),
                QRegularExpression::CaseInsensitiveOption)});
    }

    QStringList roots = m_configuration.roots;
    roots.removeDuplicates();
    for (const QString &rootValue : roots) {
        if (QThread::currentThread()->isInterruptionRequested())
            break;
        const QString root = normalizedPath(rootValue);
        if (!QFileInfo(root).isDir()) {
            emit diagnostic(tr("File Finder search root is not available: %1").arg(root));
            continue;
        }

        auto considerDirectory = [&](const QString &path, const QString &name) {
            for (const ProjectMatcher &project : matchers) {
                if (!folders.contains(project.id)
                    && name.contains(project.number, Qt::CaseInsensitive)
                    && project.expression.match(name).hasMatch())
                    folders.insert(project.id, path);
            }
        };
        considerDirectory(root, QFileInfo(root).fileName());
        QDirIterator iterator(root, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext() && folders.size() < projects.size()) {
            if (QThread::currentThread()->isInterruptionRequested())
                break;
            iterator.next();
            considerDirectory(iterator.filePath(), iterator.fileName());
        }
        if (folders.size() == projects.size())
            break;
    }
    return folders;
}

QList<DiscoveredLocation> FileFinderWorker::scanLocalFolders(
    const QList<ActiveProject> &projects, const QHash<QString, QString> &folders,
    int *filesExamined, int *matchedFiles)
{
    struct CompiledRule { QString classification; QRegularExpression expression; };
    QList<CompiledRule> rules;
    for (const FileFinderRule &rule : m_configuration.rules) {
        QRegularExpression expression(rule.pattern, QRegularExpression::CaseInsensitiveOption);
        if (expression.isValid() && !rule.pattern.trimmed().isEmpty())
            rules.append({rule.classification.trimmed(), expression});
        else if (!expression.isValid())
            emit diagnostic(tr("Invalid File Finder expression '%1': %2")
                                .arg(rule.pattern, expression.errorString()));
    }

    QList<DiscoveredLocation> locations;
    for (const ActiveProject &project : projects) {
        if (QThread::currentThread()->isInterruptionRequested())
            break;
        const QString folder = folders.value(project.id);
        if (folder.isEmpty())
            continue;
        locations.append({project.id, QStringLiteral("File Folder"),
                          QStringLiteral("File Finder: Project Folder"), folder});
        const QDir base(folder);
        QDirIterator iterator(folder, QDir::Files | QDir::Readable,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            if (QThread::currentThread()->isInterruptionRequested())
                break;
            iterator.next();
            if (filesExamined)
                ++*filesExamined;
            const QFileInfo info = iterator.fileInfo();
            const QString normalized = QDir::fromNativeSeparators(info.absoluteFilePath());
            const QString relative = QDir::fromNativeSeparators(base.relativeFilePath(info.filePath()));
            for (const CompiledRule &rule : rules) {
                if (!rule.expression.match(normalized).hasMatch()
                    && !rule.expression.match(info.fileName()).hasMatch())
                    continue;
                if (matchedFiles)
                    ++*matchedFiles;
                locations.append({project.id, locationType(info.filePath()),
                    QStringLiteral("File Finder: %1 : %2").arg(rule.classification, relative),
                    normalizedPath(info.filePath())});
                break;
            }
        }
    }
    return locations;
}

bool FileFinderWorker::commitLocations(const QList<DiscoveredLocation> &locations,
                                       FileFinderScanSummary *summary, QString *error)
{
    if (locations.isEmpty())
        return true;
    QWriteLocker locker(m_databaseLock);
    if (!m_database.transaction()) {
        if (error)
            *error = m_database.lastError().text();
        return false;
    }

    struct ExistingLocation {
        QString id;
        QString type;
        QString description;
        QString path;
    };
    QHash<QString, ExistingLocation> byPath;
    QHash<QString, ExistingLocation> byDescription;
    QSqlQuery existing(m_database);
    if (!existing.exec(QStringLiteral(
        "SELECT l.id, l.project_id, l.location_type, l.location_description, l.full_path "
        "FROM project_locations l JOIN projects p ON p.id = l.project_id "
        "WHERE l.deleted = 0 AND p.deleted = 0 AND p.project_status = 'Active'"))) {
        m_database.rollback();
        if (error)
            *error = existing.lastError().text();
        return false;
    }
    while (existing.next()) {
        ExistingLocation value{existing.value(0).toString(), existing.value(2).toString(),
                               existing.value(3).toString(), existing.value(4).toString()};
        const QString projectId = existing.value(1).toString();
        byPath.insert(locationKey(projectId, normalizedPath(value.path)), value);
        byDescription.insert(locationKey(projectId, value.description), value);
    }

    QSqlQuery update(m_database);
    update.prepare(QStringLiteral(
        "UPDATE project_locations SET location_type = ?, location_description = ?, "
        "full_path = ? WHERE id = ?"));
    QSqlQuery insert(m_database);
    insert.prepare(QStringLiteral(
        "INSERT INTO project_locations "
        "(id, project_id, location_type, location_description, full_path, updateddate, syncdate, deleted) "
        "VALUES (?, ?, ?, ?, ?, ?, NULL, 0)"));

    QSet<QString> seen;
    for (const DiscoveredLocation &location : locations) {
        const QString path = normalizedPath(location.fullPath);
        if (location.projectId.isEmpty() || location.description.isEmpty() || path.isEmpty())
            continue;
        const QString uniqueKey = locationKey(location.projectId, location.description);
        if (seen.contains(uniqueKey))
            continue;
        seen.insert(uniqueKey);
        ExistingLocation old = byPath.value(locationKey(location.projectId, path));
        if (old.id.isEmpty())
            old = byDescription.value(uniqueKey);
        if (!old.id.isEmpty()) {
            if (old.type == location.locationType && old.description == location.description
                && normalizedPath(old.path) == path) {
                ++summary->unchanged;
                continue;
            }
            update.bindValue(0, location.locationType);
            update.bindValue(1, location.description);
            update.bindValue(2, path);
            update.bindValue(3, old.id);
            if (!update.exec()) {
                m_database.rollback();
                if (error)
                    *error = update.lastError().text();
                return false;
            }
            ++summary->updated;
            continue;
        }

        insert.bindValue(0, QUuid::createUuid().toString());
        insert.bindValue(1, location.projectId);
        insert.bindValue(2, location.locationType);
        insert.bindValue(3, location.description);
        insert.bindValue(4, path);
        insert.bindValue(5, QDateTime::currentSecsSinceEpoch());
        if (!insert.exec()) {
            m_database.rollback();
            if (error)
                *error = insert.lastError().text();
            return false;
        }
        ++summary->inserted;
    }

    if (!m_database.commit()) {
        if (error)
            *error = m_database.lastError().text();
        return false;
    }
    if (summary->inserted || summary->updated)
        emit locationsCommitted(summary->inserted, summary->updated);
    return true;
}

void FileFinderWorker::scheduleNextScan()
{
    if (m_running && m_configuration.enabled && m_timer)
        m_timer->start(kReconciliationIntervalMs);
}

QString FileFinderWorker::locationType(const QString &path)
{
    const QString extension = QFileInfo(path).suffix().toLower();
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

QString FileFinderWorker::normalizedPath(const QString &path)
{
    if (path.trimmed().isEmpty())
        return {};
    if (path.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
        || path.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive))
        return path;
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return QDir::fromNativeSeparators(canonical.isEmpty() ? info.absoluteFilePath() : canonical);
}
