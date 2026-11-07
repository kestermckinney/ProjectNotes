// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "CommunicationTypes.h"
#include "HttpTransport.h"

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QStringList>

#include <functional>
#include <optional>

namespace PN::Comm {

class Office365Service;

// A project's primary project folder: a local folder, or a Microsoft Teams
// channel folder identified by its SharePoint URL.
struct ProjectFolder {
    enum class Kind { None, Local, Teams };
    Kind kind = Kind::None;
    QString path;

    [[nodiscard]] bool isValid() const { return kind != Kind::None && !path.trimmed().isEmpty(); }
    [[nodiscard]] QString kindName() const
    {
        return kind == Kind::Local ? QStringLiteral("local")
             : kind == Kind::Teams ? QStringLiteral("teams") : QString();
    }
    // From a project_locations row; Kind::None when the row can't be a folder.
    static ProjectFolder fromLocation(const QString &locationType, const QString &fullPath);
};

struct ProjectFileEntry {
    QString name;
    bool isDir = false;
    qint64 size = 0;
    // What a project_locations row stores for it: a local path, or for a
    // Teams item its URL (the direct document URL for Office files, which
    // the Office URI schemes can open; see officedeeplink.h).
    QString location;
};

// Reads and writes files beneath a project folder, wherever it lives. Paths
// are relative to the project folder, use '/' or '\' as separators, and may
// not climb out of it. Callbacks run on the store's thread, possibly before
// the call returns.
//
// Teams folders go through Microsoft Graph with the signed-in Office 365
// account: the folder URL is resolved once through /shares, then items are
// addressed by path beneath it. Writing needs Files.ReadWrite.All and the
// Office 365 "save files into Microsoft Teams folders" setting; without them
// a Teams folder is read-only and write() and mkdir() fail at once with
// "teams-read-only" or "teams-access-denied".
class ProjectFolderStore final : public QObject {
    Q_OBJECT
public:
    using EntriesCallback = std::function<void(QList<ProjectFileEntry>, ServiceError)>;
    using BoolCallback = std::function<void(bool, ServiceError)>;
    using LocationCallback = std::function<void(QString, ServiceError)>;
    using BytesCallback = std::function<void(std::optional<QByteArray>, ServiceError)>;

    explicit ProjectFolderStore(Office365Service *office, QObject *parent = nullptr);

    // Whether write() and mkdir() can work in `folder`: always for a local
    // folder, and for a Teams folder when the Office 365 account may save there.
    [[nodiscard]] bool canWrite(const ProjectFolder &folder) const;

    // The files and folders directly inside `relative`; empty when it is missing.
    void list(const ProjectFolder &folder, const QString &relative, EntriesCallback done);
    void exists(const ProjectFolder &folder, const QString &relative, BoolCallback done);
    // Creates `relative` and any missing parents; returns the folder's location.
    void mkdir(const ProjectFolder &folder, const QString &relative, LocationCallback done);
    // nullopt (and no error) when the file is missing.
    void read(const ProjectFolder &folder, const QString &relative, BytesCallback done);
    // Writes a file, creating missing parent folders. Without `overwrite` an
    // existing file fails with the "exists" code. Returns its location.
    void write(const ProjectFolder &folder, const QString &relative, QByteArray bytes,
               bool overwrite, LocationCallback done);
    // The location of an existing file or folder; empty when it is missing.
    void location(const ProjectFolder &folder, const QString &relative, LocationCallback done);

    // Splits a relative path into segments, refusing "..", drive letters and
    // empty input when `allowEmpty` is false.
    static bool splitRelative(const QString &relative, QStringList *segments, bool allowEmpty = true);
    // The /shares ID Graph accepts for a sharing or document URL.
    static QString shareId(const QString &url);
    // Graph's simple upload takes up to 4 MB; larger files use an upload
    // session in chunks that are a multiple of 320 KiB.
    static constexpr qsizetype kSimpleUploadLimit = 4 * 1024 * 1024;
    static constexpr qsizetype kUploadChunk = 16 * 320 * 1024;

private:
    struct Root { QString driveId; QString itemId; };
    using ResponseCallback = std::function<void(HttpResponse)>;
    using RootCallback = std::function<void(std::optional<Root>, ServiceError)>;
    using ItemCallback = std::function<void(std::optional<QJsonObject>, ServiceError)>;
    using IdCallback = std::function<void(QString, ServiceError)>;

    void graph(const QByteArray &method, const QUrl &url, const QByteArray &body,
               const QHash<QByteArray, QByteArray> &headers, ResponseCallback done);
    void unauthenticated(const QByteArray &method, const QUrl &url, const QByteArray &body,
                         const QHash<QByteArray, QByteArray> &headers, ResponseCallback done);
    void resolveRoot(const QString &folderUrl, RootCallback done);
    void teamsItem(const Root &root, const QStringList &segments, ItemCallback done);
    void ensureFolders(const Root &root, QString parentId, QStringList remaining, IdCallback done);
    void uploadSimple(const Root &root, const QString &parentId, const QString &name,
                      QByteArray bytes, bool overwrite, LocationCallback done);
    void uploadSession(const Root &root, const QString &parentId, const QString &name,
                       QByteArray bytes, bool overwrite, LocationCallback done);
    void uploadChunk(QUrl uploadUrl, QByteArray bytes, qsizetype offset, const Root &root,
                     LocationCallback done);
    void finishUpload(const Root &root, const QJsonObject &item, LocationCallback done);
    ServiceError responseError(const HttpResponse &response, const QString &what);
    ServiceError readOnlyError(const QString &what);

    static QUrl itemUrl(const Root &root, const QString &itemId, const QStringList &segments,
                        const QString &suffix = {}, const QString &query = {});
    static QString itemLocation(const QJsonObject &item);
    static ProjectFileEntry entryFor(const QJsonObject &item);

    QPointer<Office365Service> m_office;
    QHash<QString, Root> m_roots;
};

} // namespace PN::Comm
