// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "TransientCacheCleaner.h"

#include "ArtifactStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace PN::Comm {

TransientCacheCleaner::TransientCacheCleaner(QString cacheDirectory)
    : m_cacheDirectory(std::move(cacheDirectory))
{
}

QStringList TransientCacheCleaner::transientFilePatterns()
{
    // Legacy iCloud JSON holds identity links, not disposable downloads. Keep
    // it available until native contacts has migrated and validated those links.
    return {QStringLiteral("*.html"), QStringLiteral("*.htm")};
}

QStringList TransientCacheCleaner::clean() const
{
    QStringList failed;
    if (m_cacheDirectory.isEmpty())
        return failed;
    const QFileInfo root(m_cacheDirectory);
    if (!root.exists() || !root.isDir() || root.isSymLink())
        return failed;

    ArtifactStore store(m_cacheDirectory);
    if (!store.cleanupAll())
        failed.append(store.stagingRoot());

    QDir directory(root.absoluteFilePath());
    // JavaScript plug-ins can generate reports/downloads of any type inside
    // their per-run tree. Contact drag files are temporary as well.
    for (const QString &name : {QStringLiteral("plugin-temp"), QStringLiteral("vcard-drag")}) {
        const QString path = directory.filePath(name);
        const QFileInfo entry(path);
        if (!entry.exists() && !entry.isSymLink())
            continue;
        const bool removed = entry.isSymLink() || !entry.isDir()
            ? QFile::remove(path) : QDir(path).removeRecursively();
        if (!removed)
            failed.append(path);
    }
    const QFileInfoList files = directory.entryInfoList(transientFilePatterns(),
                                                        QDir::Files | QDir::Hidden | QDir::System);
    for (const QFileInfo &file : files) {
        // A link could point outside the cache; remove the link, never its target.
        if (!QFile::remove(file.absoluteFilePath()))
            failed.append(file.absoluteFilePath());
    }
    return failed;
}

} // namespace PN::Comm
