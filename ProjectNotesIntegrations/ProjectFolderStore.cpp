// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectFolderStore.h"

#include "Office365Service.h"
#include "projectfolderlocation.h"
#include "QLogger.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUuid>

#include <algorithm>
#include <memory>

namespace PN::Comm {

namespace {

const char kGraphRoot[] = "https://graph.microsoft.com/v1.0/";

QString encodeSegment(const QString &segment)
{
    return QString::fromLatin1(QUrl::toPercentEncoding(segment));
}

ServiceError makeError(const QString &code, const QString &text)
{
    ServiceError error;
    error.code = code;
    error.displayText = text;
    return error;
}

bool isOfficeDocument(const QString &name)
{
    static const QStringList extensions = {
        QStringLiteral("doc"), QStringLiteral("docx"), QStringLiteral("xls"), QStringLiteral("xlsx"),
        QStringLiteral("ppt"), QStringLiteral("pptx"), QStringLiteral("mpp")};
    return extensions.contains(QFileInfo(name).suffix().toLower());
}

QString localPath(const ProjectFolder &folder, const QStringList &segments)
{
    QString path = QDir::cleanPath(QDir::fromNativeSeparators(folder.path));
    for (const QString &segment : segments)
        path += QLatin1Char('/') + segment;
    return path;
}

ProjectFileEntry localEntry(const QFileInfo &info)
{
    return {info.fileName(), info.isDir(), info.isDir() ? 0 : info.size(),
            QDir::cleanPath(info.absoluteFilePath())};
}

} // namespace

ProjectFolder ProjectFolder::fromLocation(const QString &locationType, const QString &fullPath)
{
    if (!PN::ProjectFolder::isEligibleLocation(locationType, fullPath))
        return {};
    return {locationType == QLatin1String("File Folder") ? Kind::Local : Kind::Teams, fullPath.trimmed()};
}

ProjectFolderStore::ProjectFolderStore(Office365Service *office, QObject *parent)
    : QObject(parent), m_office(office)
{
}

bool ProjectFolderStore::canWrite(const ProjectFolder &folder) const
{
    if (folder.kind == ProjectFolder::Kind::Local)
        return true;
    return folder.kind == ProjectFolder::Kind::Teams && m_office && m_office->teamsFilesWritable();
}

ServiceError ProjectFolderStore::readOnlyError(const QString &what)
{
    if (!m_office || !m_office->teamsFileWriteEnabled())
        return makeError(QStringLiteral("teams-read-only"),
                         tr("Could not save %1: saving into Microsoft Teams folders is turned off in Settings > "
                            "Office 365 Integration. Save to a local folder instead.").arg(what));
    // Saving is on but the sign-in only allows reading: ask for the write
    // scope on the next sign-in.
    m_office->requestAdditionalScopes({m_office->teamsFileScope()});
    return makeError(QStringLiteral("teams-access-denied"),
                     tr("Could not save %1: the Office 365 sign-in does not allow saving into Microsoft Teams. "
                        "Sign in again from Settings > Office 365 Integration and allow Project Notes to save "
                        "files (Files.ReadWrite.All).").arg(what));
}

bool ProjectFolderStore::splitRelative(const QString &relative, QStringList *segments, bool allowEmpty)
{
    segments->clear();
    // Plugin paths are the same on every platform, so a backslash separates
    // folders on macOS and Linux too (QDir::fromNativeSeparators converts it
    // only on Windows). Teams and SharePoint refuse it in names anyway.
    QString value = relative.trimmed();
    value.replace(QLatin1Char('\\'), QLatin1Char('/'));
    static const QRegularExpression drive(QStringLiteral("^[A-Za-z]:"));
    if (value.startsWith(QLatin1Char('/')) || drive.match(value).hasMatch())
        return false;
    for (const QString &part : value.split(QLatin1Char('/'), Qt::SkipEmptyParts)) {
        const QString segment = part.trimmed();
        if (segment.isEmpty() || segment == QLatin1String("."))
            continue;
        if (segment == QLatin1String(".."))
            return false;
        segments->append(segment);
    }
    return allowEmpty || !segments->isEmpty();
}

QString ProjectFolderStore::shareId(const QString &url)
{
    return QStringLiteral("u!") + QString::fromLatin1(url.trimmed().toUtf8().toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

// ── Requests ────────────────────────────────────────────────────────────────

void ProjectFolderStore::graph(const QByteArray &method, const QUrl &url, const QByteArray &body,
                               const QHash<QByteArray, QByteArray> &headers, ResponseCallback done)
{
    if (!m_office) {
        HttpResponse response;
        response.error = makeError(QStringLiteral("office365-unavailable"),
                                   tr("Sign in to Office 365 in Settings > Office 365 Integration to use Microsoft Teams folders."));
        done(std::move(response));
        return;
    }
    HttpRequest request;
    request.operationId = QUuid::createUuid();
    request.method = method;
    request.url = url;
    request.body = body;
    request.headers = headers;
    request.headers.insert("Accept", "application/json");
    m_office->sendGraphRequest(std::move(request), std::move(done));
}

void ProjectFolderStore::unauthenticated(const QByteArray &method, const QUrl &url, const QByteArray &body,
                                         const QHash<QByteArray, QByteArray> &headers, ResponseCallback done)
{
    if (!m_office) {
        HttpResponse response;
        response.error = makeError(QStringLiteral("office365-unavailable"), tr("Office 365 is not available."));
        done(std::move(response));
        return;
    }
    HttpRequest request;
    request.operationId = QUuid::createUuid();
    request.method = method;
    request.url = url;
    request.body = body;
    request.headers = headers;
    m_office->sendUploadRequest(std::move(request), std::move(done));
}

ServiceError ProjectFolderStore::responseError(const HttpResponse &response, const QString &what)
{
    const int status = response.statusCode;
    QString detail = QJsonDocument::fromJson(response.body).object()
        .value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
    if (status < 200 || status >= 300) {
        const QJsonObject graphError = QJsonDocument::fromJson(response.body).object()
            .value(QStringLiteral("error")).toObject();
        QLog_Warning(CONSOLELOG, QStringLiteral("Teams request for %1 failed: HTTP %2 %3 %4 (granted scopes: %5)")
                         .arg(what).arg(status)
                         .arg(graphError.value(QStringLiteral("code")).toString(), detail,
                              m_office ? m_office->account().grantedScopes.join(u' ') : QString()));
    }
    if (status == 401 || status == 403) {
        // A session signed in before Project Notes wrote to Teams holds only
        // Files.Read.All. Ask for the write scope on the next sign-in, unless
        // saving into Teams is turned off: then asking could fail the sign-in.
        if (m_office && status == 403 && m_office->teamsFileWriteEnabled())
            m_office->requestAdditionalScopes({m_office->teamsFileScope()});
        return makeError(QStringLiteral("teams-access-denied"),
                         tr("Microsoft Teams refused access to %1. Sign in again from Settings > Office 365 "
                            "Integration and allow Project Notes to save files (Files.ReadWrite.All).").arg(what));
    }
    if (status == 0 && !response.error.code.isEmpty())
        return response.error;
    if (detail.isEmpty())
        detail = response.error.displayText.isEmpty() ? tr("HTTP %1").arg(status) : response.error.displayText;
    return makeError(QStringLiteral("teams-request-failed"),
                     tr("Microsoft Teams could not complete the request for %1: %2").arg(what, detail));
}

QUrl ProjectFolderStore::itemUrl(const Root &root, const QString &itemId, const QStringList &segments,
                                 const QString &suffix, const QString &query)
{
    QString url = QString::fromLatin1(kGraphRoot) + QStringLiteral("drives/") + encodeSegment(root.driveId)
        + QStringLiteral("/items/") + encodeSegment(itemId);
    if (!segments.isEmpty()) {
        QStringList encoded;
        for (const QString &segment : segments)
            encoded.append(encodeSegment(segment));
        url += QStringLiteral(":/") + encoded.join(QLatin1Char('/')) + QLatin1Char(':');
    }
    if (!suffix.isEmpty())
        url += QLatin1Char('/') + suffix;
    if (!query.isEmpty())
        url += QLatin1Char('?') + query;
    return QUrl::fromEncoded(url.toLatin1());
}

QString ProjectFolderStore::itemLocation(const QJsonObject &item)
{
    const QString name = item.value(QStringLiteral("name")).toString();
    if (!item.contains(QStringLiteral("folder")) && isOfficeDocument(name)) {
        const QString webDav = item.value(QStringLiteral("webDavUrl")).toString();
        if (!webDav.isEmpty())
            return webDav;
    }
    return item.value(QStringLiteral("webUrl")).toString();
}

ProjectFileEntry ProjectFolderStore::entryFor(const QJsonObject &item)
{
    return {item.value(QStringLiteral("name")).toString(), item.contains(QStringLiteral("folder")),
            static_cast<qint64>(item.value(QStringLiteral("size")).toDouble()), itemLocation(item)};
}

void ProjectFolderStore::resolveRoot(const QString &folderUrl, RootCallback done)
{
    const auto cached = m_roots.constFind(folderUrl);
    if (cached != m_roots.constEnd()) {
        done(*cached, {});
        return;
    }
    const QUrl url = QUrl::fromEncoded((QString::fromLatin1(kGraphRoot) + QStringLiteral("shares/")
        + shareId(folderUrl) + QStringLiteral("/driveItem?$select=id,name,folder,parentReference")).toLatin1());
    QPointer<ProjectFolderStore> self(this);
    graph("GET", url, {}, {}, [self, folderUrl, done](HttpResponse response) {
        if (!self)
            return;
        if (response.statusCode < 200 || response.statusCode >= 300) {
            done(std::nullopt, response.statusCode == 404
                 ? makeError(QStringLiteral("teams-folder-missing"),
                             tr("The Microsoft Teams project folder %1 no longer exists.").arg(folderUrl))
                 : self->responseError(response, folderUrl));
            return;
        }
        const QJsonObject item = QJsonDocument::fromJson(response.body).object();
        const Root root{item.value(QStringLiteral("parentReference")).toObject()
                            .value(QStringLiteral("driveId")).toString(),
                        item.value(QStringLiteral("id")).toString()};
        if (root.driveId.isEmpty() || root.itemId.isEmpty() || !item.contains(QStringLiteral("folder"))) {
            done(std::nullopt, makeError(QStringLiteral("teams-not-a-folder"),
                                         tr("%1 is not a Microsoft Teams or SharePoint folder.").arg(folderUrl)));
            return;
        }
        self->m_roots.insert(folderUrl, root);
        done(root, {});
    });
}

void ProjectFolderStore::teamsItem(const Root &root, const QStringList &segments, ItemCallback done)
{
    QPointer<ProjectFolderStore> self(this);
    const QString what = segments.join(QLatin1Char('/'));
    graph("GET", itemUrl(root, root.itemId, segments), {}, {}, [self, what, done](HttpResponse response) {
        if (!self)
            return;
        if (response.statusCode == 404) {
            done(std::nullopt, {});
            return;
        }
        if (response.statusCode < 200 || response.statusCode >= 300) {
            done(std::nullopt, self->responseError(response, what));
            return;
        }
        done(QJsonDocument::fromJson(response.body).object(), {});
    });
}

void ProjectFolderStore::ensureFolders(const Root &root, QString parentId, QStringList remaining, IdCallback done)
{
    if (remaining.isEmpty()) {
        done(parentId, {});
        return;
    }
    const QString name = remaining.takeFirst();
    QPointer<ProjectFolderStore> self(this);
    auto next = [self, root, remaining, done](QString id, ServiceError error) {
        if (!self)
            return;
        if (!error.code.isEmpty()) {
            done({}, error);
            return;
        }
        self->ensureFolders(root, id, remaining, done);
    };
    auto created = [self, root, parentId, name, next](HttpResponse response) {
        if (!self)
            return;
        const QJsonObject item = QJsonDocument::fromJson(response.body).object();
        if (response.statusCode >= 200 && response.statusCode < 300) {
            next(item.value(QStringLiteral("id")).toString(), {});
            return;
        }
        if (response.statusCode != 409) {
            next({}, self->responseError(response, name));
            return;
        }
        // Someone created it meanwhile; read it back.
        self->graph("GET", itemUrl(root, parentId, {name}), {}, {}, [self, name, next](HttpResponse again) {
            if (!self)
                return;
            if (again.statusCode < 200 || again.statusCode >= 300) {
                next({}, self->responseError(again, name));
                return;
            }
            next(QJsonDocument::fromJson(again.body).object().value(QStringLiteral("id")).toString(), {});
        });
    };
    graph("GET", itemUrl(root, parentId, {name}, {}, QStringLiteral("$select=id,name,folder")), {}, {},
          [self, root, parentId, name, next, created](HttpResponse response) {
        if (!self)
            return;
        if (response.statusCode >= 200 && response.statusCode < 300) {
            const QJsonObject item = QJsonDocument::fromJson(response.body).object();
            if (!item.contains(QStringLiteral("folder"))) {
                next({}, makeError(QStringLiteral("teams-not-a-folder"),
                                   tr("%1 is a file, not a folder.").arg(name)));
                return;
            }
            next(item.value(QStringLiteral("id")).toString(), {});
            return;
        }
        if (response.statusCode != 404) {
            next({}, self->responseError(response, name));
            return;
        }
        const QByteArray body = QJsonDocument(QJsonObject{
            {QStringLiteral("name"), name}, {QStringLiteral("folder"), QJsonObject{}},
            {QStringLiteral("@microsoft.graph.conflictBehavior"), QStringLiteral("fail")}}).toJson(QJsonDocument::Compact);
        self->graph("POST", itemUrl(root, parentId, {}, QStringLiteral("children")), body,
                    {{"Content-Type", "application/json"}}, created);
    });
}

// ── Operations ──────────────────────────────────────────────────────────────

void ProjectFolderStore::list(const ProjectFolder &folder, const QString &relative, EntriesCallback done)
{
    QStringList segments;
    if (!folder.isValid() || !splitRelative(relative, &segments)) {
        done({}, makeError(QStringLiteral("validation-failed"), tr("%1 is not a path inside the project folder.").arg(relative)));
        return;
    }
    if (folder.kind == ProjectFolder::Kind::Local) {
        QList<ProjectFileEntry> entries;
        QDirIterator it(localPath(folder, segments), QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            it.next();
            entries.append(localEntry(it.fileInfo()));
        }
        std::sort(entries.begin(), entries.end(), [](const ProjectFileEntry &a, const ProjectFileEntry &b) {
            return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
        });
        done(entries, {});
        return;
    }
    QPointer<ProjectFolderStore> self(this);
    resolveRoot(folder.path, [self, segments, done](std::optional<Root> root, ServiceError error) {
        if (!self)
            return;
        if (!root) {
            done({}, error);
            return;
        }
        auto entries = std::make_shared<QList<ProjectFileEntry>>();
        auto page = std::make_shared<std::function<void(QUrl)>>();
        const QString what = segments.join(QLatin1Char('/'));
        *page = [self, entries, page, what, done](QUrl url) {
            if (!self)
                return;
            self->graph("GET", url, {}, {}, [self, entries, page, what, done](HttpResponse response) {
                if (!self) {
                    *page = nullptr;
                    return;
                }
                if (response.statusCode == 404) {
                    *page = nullptr;
                    done({}, {});
                    return;
                }
                if (response.statusCode < 200 || response.statusCode >= 300) {
                    *page = nullptr;
                    done({}, self->responseError(response, what.isEmpty() ? tr("the project folder") : what));
                    return;
                }
                const QJsonObject object = QJsonDocument::fromJson(response.body).object();
                for (const QJsonValue &value : object.value(QStringLiteral("value")).toArray())
                    entries->append(entryFor(value.toObject()));
                const QString next = object.value(QStringLiteral("@odata.nextLink")).toString();
                if (!next.isEmpty()) {
                    (*page)(QUrl(next));
                    return;
                }
                std::sort(entries->begin(), entries->end(), [](const ProjectFileEntry &a, const ProjectFileEntry &b) {
                    return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                });
                const QList<ProjectFileEntry> result = *entries;
                *page = nullptr;   // break the self-reference
                done(result, {});
            });
        };
        (*page)(itemUrl(*root, root->itemId, segments, QStringLiteral("children"),
                        QStringLiteral("$select=id,name,size,folder,file,webUrl,webDavUrl&$top=999")));
    });
}

void ProjectFolderStore::exists(const ProjectFolder &folder, const QString &relative, BoolCallback done)
{
    location(folder, relative, [done](QString found, ServiceError error) {
        done(!found.isEmpty(), error);
    });
}

void ProjectFolderStore::location(const ProjectFolder &folder, const QString &relative, LocationCallback done)
{
    QStringList segments;
    if (!folder.isValid() || !splitRelative(relative, &segments)) {
        done({}, makeError(QStringLiteral("validation-failed"), tr("%1 is not a path inside the project folder.").arg(relative)));
        return;
    }
    if (folder.kind == ProjectFolder::Kind::Local) {
        const QFileInfo info(localPath(folder, segments));
        done(info.exists() ? QDir::cleanPath(info.absoluteFilePath()) : QString(), {});
        return;
    }
    if (segments.isEmpty()) {
        const QString url = folder.path;
        resolveRoot(folder.path, [url, done](std::optional<Root> root, ServiceError error) {
            const bool missing = error.code == QLatin1String("teams-folder-missing");
            done(root ? url : QString(), root.has_value() || missing ? ServiceError() : error);
        });
        return;
    }
    QPointer<ProjectFolderStore> self(this);
    resolveRoot(folder.path, [self, segments, done](std::optional<Root> root, ServiceError error) {
        if (!self)
            return;
        if (!root) {
            done({}, error);
            return;
        }
        self->teamsItem(*root, segments, [done](std::optional<QJsonObject> item, ServiceError itemError) {
            done(item ? itemLocation(*item) : QString(), itemError);
        });
    });
}

void ProjectFolderStore::mkdir(const ProjectFolder &folder, const QString &relative, LocationCallback done)
{
    QStringList segments;
    if (!folder.isValid() || !splitRelative(relative, &segments)) {
        done({}, makeError(QStringLiteral("validation-failed"), tr("%1 is not a path inside the project folder.").arg(relative)));
        return;
    }
    if (folder.kind == ProjectFolder::Kind::Local) {
        const QString path = localPath(folder, segments);
        if (!QDir().mkpath(path)) {
            done({}, makeError(QStringLiteral("io-error"), tr("Could not create %1.").arg(path)));
            return;
        }
        done(path, {});
        return;
    }
    if (!canWrite(folder)) {
        done({}, readOnlyError(relative));
        return;
    }
    QPointer<ProjectFolderStore> self(this);
    resolveRoot(folder.path, [self, segments, done](std::optional<Root> root, ServiceError error) {
        if (!self)
            return;
        if (!root) {
            done({}, error);
            return;
        }
        const Root r = *root;
        self->ensureFolders(r, r.itemId, segments, [self, r, done](QString id, ServiceError ensureError) {
            if (!self)
                return;
            if (!ensureError.code.isEmpty()) {
                done({}, ensureError);
                return;
            }
            self->graph("GET", itemUrl(r, id, {}, {}, QStringLiteral("$select=id,name,folder,webUrl")), {}, {},
                        [self, done](HttpResponse response) {
                if (!self)
                    return;
                if (response.statusCode < 200 || response.statusCode >= 300) {
                    done({}, self->responseError(response, tr("the new folder")));
                    return;
                }
                done(itemLocation(QJsonDocument::fromJson(response.body).object()), {});
            });
        });
    });
}

void ProjectFolderStore::read(const ProjectFolder &folder, const QString &relative, BytesCallback done)
{
    QStringList segments;
    if (!folder.isValid() || !splitRelative(relative, &segments, false)) {
        done(std::nullopt, makeError(QStringLiteral("validation-failed"), tr("%1 is not a file inside the project folder.").arg(relative)));
        return;
    }
    if (folder.kind == ProjectFolder::Kind::Local) {
        QFile file(localPath(folder, segments));
        if (!file.exists()) {
            done(std::nullopt, {});
            return;
        }
        if (!file.open(QIODevice::ReadOnly)) {
            done(std::nullopt, makeError(QStringLiteral("io-error"), file.errorString()));
            return;
        }
        done(file.readAll(), {});
        return;
    }
    QPointer<ProjectFolderStore> self(this);
    resolveRoot(folder.path, [self, segments, done](std::optional<Root> root, ServiceError error) {
        if (!self)
            return;
        if (!root) {
            done(std::nullopt, error);
            return;
        }
        const QString what = segments.join(QLatin1Char('/'));
        self->teamsItem(*root, segments, [self, what, done](std::optional<QJsonObject> item, ServiceError itemError) {
            if (!self)
                return;
            if (!item) {
                done(std::nullopt, itemError);
                return;
            }
            // The pre-authenticated download URL must not carry the token.
            const QString download = item->value(QStringLiteral("@microsoft.graph.downloadUrl")).toString();
            if (download.isEmpty()) {
                done(std::nullopt, makeError(QStringLiteral("teams-request-failed"),
                                             tr("Microsoft Teams did not offer %1 for download.").arg(what)));
                return;
            }
            self->unauthenticated("GET", QUrl(download), {}, {}, [self, what, done](HttpResponse response) {
                if (!self)
                    return;
                if (response.statusCode < 200 || response.statusCode >= 300) {
                    done(std::nullopt, self->responseError(response, what));
                    return;
                }
                done(response.body, {});
            });
        });
    });
}

void ProjectFolderStore::write(const ProjectFolder &folder, const QString &relative, QByteArray bytes,
                               bool overwrite, LocationCallback done)
{
    QStringList segments;
    if (!folder.isValid() || !splitRelative(relative, &segments, false)) {
        done({}, makeError(QStringLiteral("validation-failed"), tr("%1 is not a file inside the project folder.").arg(relative)));
        return;
    }
    if (folder.kind == ProjectFolder::Kind::Local) {
        const QString path = localPath(folder, segments);
        if (QFileInfo::exists(path)) {
            if (!overwrite) {
                done({}, makeError(QStringLiteral("exists"), tr("%1 already exists.").arg(path)));
                return;
            }
        }
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(bytes) != bytes.size()) {
            done({}, makeError(QStringLiteral("io-error"), tr("Could not write %1: %2").arg(path, file.errorString())));
            return;
        }
        file.close();
        done(path, {});
        return;
    }
    if (!canWrite(folder)) {
        done({}, readOnlyError(relative));
        return;
    }
    const QString name = segments.takeLast();
    QPointer<ProjectFolderStore> self(this);
    resolveRoot(folder.path, [self, segments, name, bytes = std::move(bytes), overwrite, done]
                (std::optional<Root> root, ServiceError error) mutable {
        if (!self)
            return;
        if (!root) {
            done({}, error);
            return;
        }
        const Root r = *root;
        self->ensureFolders(r, r.itemId, segments, [self, r, name, bytes = std::move(bytes), overwrite, done]
                            (QString parentId, ServiceError ensureError) mutable {
            if (!self)
                return;
            if (!ensureError.code.isEmpty()) {
                done({}, ensureError);
                return;
            }
            if (bytes.size() <= kSimpleUploadLimit)
                self->uploadSimple(r, parentId, name, std::move(bytes), overwrite, done);
            else
                self->uploadSession(r, parentId, name, std::move(bytes), overwrite, done);
        });
    });
}

void ProjectFolderStore::uploadSimple(const Root &root, const QString &parentId, const QString &name,
                                      QByteArray bytes, bool overwrite, LocationCallback done)
{
    QPointer<ProjectFolderStore> self(this);
    const QString query = QStringLiteral("@microsoft.graph.conflictBehavior=")
        + (overwrite ? QStringLiteral("replace") : QStringLiteral("fail"));
    graph("PUT", itemUrl(root, parentId, {name}, QStringLiteral("content"), query), bytes,
          {{"Content-Type", "application/octet-stream"}}, [self, root, name, done](HttpResponse response) {
        if (!self)
            return;
        if (response.statusCode == 409) {
            done({}, makeError(QStringLiteral("exists"), tr("%1 already exists.").arg(name)));
            return;
        }
        if (response.statusCode < 200 || response.statusCode >= 300) {
            done({}, self->responseError(response, name));
            return;
        }
        self->finishUpload(root, QJsonDocument::fromJson(response.body).object(), done);
    });
}

void ProjectFolderStore::uploadSession(const Root &root, const QString &parentId, const QString &name,
                                       QByteArray bytes, bool overwrite, LocationCallback done)
{
    QPointer<ProjectFolderStore> self(this);
    const QByteArray body = QJsonDocument(QJsonObject{{QStringLiteral("item"), QJsonObject{
        {QStringLiteral("@microsoft.graph.conflictBehavior"),
         overwrite ? QStringLiteral("replace") : QStringLiteral("fail")}}}}).toJson(QJsonDocument::Compact);
    graph("POST", itemUrl(root, parentId, {name}, QStringLiteral("createUploadSession")), body,
          {{"Content-Type", "application/json"}},
          [self, root, name, bytes = std::move(bytes), done](HttpResponse response) mutable {
        if (!self)
            return;
        if (response.statusCode == 409) {
            done({}, makeError(QStringLiteral("exists"), tr("%1 already exists.").arg(name)));
            return;
        }
        const QString uploadUrl = QJsonDocument::fromJson(response.body).object()
            .value(QStringLiteral("uploadUrl")).toString();
        if (response.statusCode < 200 || response.statusCode >= 300 || uploadUrl.isEmpty()) {
            done({}, self->responseError(response, name));
            return;
        }
        self->uploadChunk(QUrl(uploadUrl), std::move(bytes), 0, root, done);
    });
}

void ProjectFolderStore::uploadChunk(QUrl uploadUrl, QByteArray bytes, qsizetype offset, const Root &root,
                                     LocationCallback done)
{
    const qsizetype length = qMin(kUploadChunk, bytes.size() - offset);
    const QByteArray chunk = bytes.mid(offset, length);
    const QByteArray range = "bytes " + QByteArray::number(offset) + '-' + QByteArray::number(offset + length - 1)
        + '/' + QByteArray::number(bytes.size());
    QPointer<ProjectFolderStore> self(this);
    unauthenticated("PUT", uploadUrl, chunk, {{"Content-Range", range}, {"Content-Type", "application/octet-stream"}},
                    [self, uploadUrl, bytes = std::move(bytes), offset, length, root, done](HttpResponse response) mutable {
        if (!self)
            return;
        if (response.statusCode == 409) {
            done({}, makeError(QStringLiteral("exists"), tr("The file already exists.")));
            return;
        }
        if (response.statusCode < 200 || response.statusCode >= 300) {
            done({}, self->responseError(response, tr("the upload")));
            return;
        }
        if (offset + length < bytes.size() && response.statusCode == 202) {
            self->uploadChunk(uploadUrl, std::move(bytes), offset + length, root, done);
            return;
        }
        self->finishUpload(root, QJsonDocument::fromJson(response.body).object(), done);
    });
}

void ProjectFolderStore::finishUpload(const Root &root, const QJsonObject &item, LocationCallback done)
{
    const QString name = item.value(QStringLiteral("name")).toString();
    const QString id = item.value(QStringLiteral("id")).toString();
    const bool needsWebDav = isOfficeDocument(name) && item.value(QStringLiteral("webDavUrl")).toString().isEmpty();
    if (!needsWebDav || id.isEmpty()) {
        done(itemLocation(item), {});
        return;
    }
    QPointer<ProjectFolderStore> self(this);
    graph("GET", itemUrl(root, id, {}, {}, QStringLiteral("$select=id,name,webUrl,webDavUrl")), {}, {},
          [self, item, done](HttpResponse response) {
        if (!self)
            return;
        if (response.statusCode >= 200 && response.statusCode < 300)
            done(itemLocation(QJsonDocument::fromJson(response.body).object()), {});
        else
            done(itemLocation(item), {});   // the upload itself succeeded
    });
}

} // namespace PN::Comm
