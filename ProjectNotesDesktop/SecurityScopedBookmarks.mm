// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "SecurityScopedBookmarks.h"

#include "QLogger.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

#import <Foundation/Foundation.h>

namespace SecurityScopedBookmarks {

namespace {

QString s_organization = QStringLiteral("ProjectNotes");

QString cleanPath(const QString& pathOrFileUrl)
{
    const QUrl url(pathOrFileUrl);
    const QString path = url.isLocalFile() ? url.toLocalFile() : pathOrFileUrl;
    return path.isEmpty() ? QString() : QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

// One entry per path, keyed by a hash so separators never become groups.
QString keyFor(const QString& path)
{
    return QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha1).toHex());
}

QByteArray createBookmark(const QString& path)
{
    @autoreleasepool {
        NSURL* url = [NSURL fileURLWithPath:path.toNSString()];
        NSError* error = nil;
        NSData* data = [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
                     includingResourceValuesForKeys:nil
                                      relativeToURL:nil
                                              error:&error];
        if (!data) {
            QLog_Warning(CONSOLELOG, QStringLiteral("Could not keep access to %1: %2")
                                         .arg(path, QString::fromNSString(error.localizedDescription)));
            return {};
        }
        return QByteArray::fromNSData(data);
    }
}

} // namespace

void setSettingsOrganization(const QString& organization)
{
    s_organization = organization;
}

void remember(const QString& pathOrFileUrl)
{
    const QString path = cleanPath(pathOrFileUrl);
    if (path.isEmpty() || !QFileInfo::exists(path))
        return;
    const QByteArray bookmark = createBookmark(path);
    if (bookmark.isEmpty())
        return;
    QSettings settings(s_organization, QStringLiteral("AppSettings"));
    settings.beginGroup(QStringLiteral("SecurityScopedBookmarks"));
    settings.setValue(keyFor(path), QVariantMap{{QStringLiteral("path"), path}, {QStringLiteral("bookmark"), bookmark}});
    settings.endGroup();

    // The open panel already granted this session; hold the access the same
    // way a restored bookmark does.
    @autoreleasepool {
        BOOL stale = NO;
        NSURL* url = [NSURL URLByResolvingBookmarkData:bookmark.toNSData()
                                               options:NSURLBookmarkResolutionWithSecurityScope
                                         relativeToURL:nil
                                   bookmarkDataIsStale:&stale
                                                 error:nil];
        [url startAccessingSecurityScopedResource];
    }
}

void restoreAll()
{
    QSettings settings(s_organization, QStringLiteral("AppSettings"));
    settings.beginGroup(QStringLiteral("SecurityScopedBookmarks"));
    const QStringList keys = settings.childKeys();
    int restored = 0;
    for (const QString& key : keys) {
        const QVariantMap entry = settings.value(key).toMap();
        const QByteArray bookmark = entry.value(QStringLiteral("bookmark")).toByteArray();
        const QString path = entry.value(QStringLiteral("path")).toString();
        bool ok = false;
        bool stale = false;
        @autoreleasepool {
            BOOL isStale = NO;
            NSError* error = nil;
            NSURL* url = [NSURL URLByResolvingBookmarkData:bookmark.toNSData()
                                                   options:NSURLBookmarkResolutionWithSecurityScope
                                             relativeToURL:nil
                                       bookmarkDataIsStale:&isStale
                                                     error:&error];
            ok = url && [url startAccessingSecurityScopedResource];
            stale = isStale;
        }
        if (!ok) {
            // The folder is gone or the user revoked access: forget it.
            settings.remove(key);
            QLog_Info(CONSOLELOG, QStringLiteral("Lost access to %1; pick it again to restore it.").arg(path));
            continue;
        }
        ++restored;
        if (stale) {
            const QByteArray fresh = createBookmark(path);
            if (!fresh.isEmpty())
                settings.setValue(key, QVariantMap{{QStringLiteral("path"), path}, {QStringLiteral("bookmark"), fresh}});
        }
    }
    settings.endGroup();
    if (restored)
        QLog_Info(CONSOLELOG, QStringLiteral("Restored access to %1 location(s).").arg(restored));
}

} // namespace SecurityScopedBookmarks
