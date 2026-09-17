// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "ReportDestination.h"

#include <QDir>
#include <QFileInfo>

namespace PN::Comm {
namespace {

bool safeFileName(const QString &value)
{
    return !value.isEmpty() && value != QLatin1String(".") && value != QLatin1String("..")
        && !value.contains(u'/') && !value.contains(u'\\') && !value.contains(QStringLiteral(".."))
        && !value.contains(u'\r') && !value.contains(u'\n');
}

bool safeRelativeDirectory(const QString &value)
{
    const QString normalized = QDir::fromNativeSeparators(value.trimmed());
    if (normalized.isEmpty())
        return true;
    if (QDir::isAbsolutePath(normalized) || normalized.startsWith(u'/')
        || normalized.startsWith(u'\\') || normalized.contains(QStringLiteral("://"))
        || (normalized.size() >= 2 && normalized[0].isLetter() && normalized[1] == u':'))
        return false;
    for (const QString &part : normalized.split(u'/', Qt::SkipEmptyParts)) {
        if (part == QLatin1String(".") || part == QLatin1String("..")
            || part.contains(u'\r') || part.contains(u'\n'))
            return false;
    }
    return true;
}

} // namespace

ReportDestination resolveReportDestination(const QString &projectFolderPath,
                                           const QString &exportSubfolder,
                                           const QString &fileName)
{
    ReportDestination result;
    const QString root = QDir::cleanPath(QDir::fromNativeSeparators(projectFolderPath.trimmed()));
    if (root.isEmpty() || !QDir::isAbsolutePath(root) || root.contains(QStringLiteral("://"))) {
        result.error.code = QStringLiteral("project-folder-unavailable");
        return result;
    }
    if (!safeRelativeDirectory(exportSubfolder)) {
        result.error.code = QStringLiteral("export-subfolder-invalid");
        return result;
    }
    if (!safeFileName(fileName.trimmed())) {
        result.error.code = QStringLiteral("report-filename-invalid");
        return result;
    }
    const QString relative = QDir::cleanPath(QDir::fromNativeSeparators(exportSubfolder.trimmed()));
    result.rootPath = root;
    result.directoryPath = relative.isEmpty() ? root : QDir(root).filePath(relative);
    result.filePath = QDir(result.directoryPath).filePath(fileName.trimmed());
    return result;
}

ReportDestination ensureReportDestination(const ReportDestination &destination)
{
    ReportDestination result = destination;
    if (!result.ok())
        return result;

    const QFileInfo rootInfo(result.rootPath);
    if (!rootInfo.exists() || !rootInfo.isDir() || rootInfo.isSymLink()) {
        result.error.code = QStringLiteral("project-folder-unavailable");
        return result;
    }
    const QString canonicalRoot = rootInfo.canonicalFilePath();
    if (canonicalRoot.isEmpty()) {
        result.error.code = QStringLiteral("project-folder-unavailable");
        return result;
    }

    const QString relative = QDir(canonicalRoot).relativeFilePath(result.directoryPath);
    if (!safeRelativeDirectory(relative)) {
        result.error.code = QStringLiteral("export-subfolder-invalid");
        return result;
    }
    QString current = canonicalRoot;
    for (const QString &part : QDir::fromNativeSeparators(relative).split(u'/', Qt::SkipEmptyParts)) {
        const QString child = QDir(current).filePath(part);
        QFileInfo childInfo(child);
        if (childInfo.exists()) {
            if (!childInfo.isDir() || childInfo.isSymLink()) {
                result.error.code = QStringLiteral("export-subfolder-unavailable");
                return result;
            }
        } else if (!QDir(current).mkdir(part)) {
            result.error.code = QStringLiteral("export-subfolder-unavailable");
            return result;
        }
        childInfo = QFileInfo(child);
        const QString canonicalChild = childInfo.canonicalFilePath();
        if (canonicalChild.isEmpty() || (canonicalChild != canonicalRoot
            && !canonicalChild.startsWith(canonicalRoot + QDir::separator()))) {
            result.error.code = QStringLiteral("export-subfolder-unavailable");
            return result;
        }
        current = canonicalChild;
    }
    result.rootPath = canonicalRoot;
    result.directoryPath = current;
    result.filePath = QDir(current).filePath(QFileInfo(result.filePath).fileName());
    return result;
}

} // namespace PN::Comm
