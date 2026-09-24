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
    // The export plug-ins write their report HTML straight into the cache
    // folder, and the iCloud contacts import leaves its download there.
    return {QStringLiteral("*.html"), QStringLiteral("*.htm"), QStringLiteral("icloud_contacts_*.json")};
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
