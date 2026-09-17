// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "EmailSettingsStore.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>

namespace PN::Comm {
namespace {

QString defaultExportSubfolder(Workflow workflow)
{
    switch (workflow) {
    case Workflow::TrackerItemsReport: return QStringLiteral("Project Management/Issues List");
    case Workflow::StatusReport: return QStringLiteral("Project Management/Status Reports");
    case Workflow::MeetingNotesReport: return QStringLiteral("Project Management/Meeting Minutes");
    case Workflow::SendMeetingNotes: return {};
    }
    return {};
}

} // namespace

EmailSettingsStore::EmailSettingsStore(QSettings &settings) : m_settings(settings)
{
    m_settings.setFallbacksEnabled(false);
}

QString EmailSettingsStore::databaseKeyForPath(const QString &databasePath)
{
    QFileInfo info(databasePath);
    const QString normalized = info.exists() ? info.canonicalFilePath() : info.absoluteFilePath();
    return QString::fromLatin1(QCryptographicHash::hash(QDir::cleanPath(normalized).toUtf8(),
                                                         QCryptographicHash::Sha256).toHex());
}

BackendId EmailSettingsStore::preferredBackend() const
{
    return backendIdFromStableString(m_settings.value(QStringLiteral("Email/v1/PreferredBackend")).toString())
        .value_or(BackendId::Mailto);
}

void EmailSettingsStore::setPreferredBackend(BackendId backend)
{
    m_settings.setValue(QStringLiteral("Email/v1/PreferredBackend"), toStableString(backend));
    m_settings.sync();
}

QString EmailSettingsStore::thunderbirdPath() const
{
    return m_settings.value(QStringLiteral("Email/v1/ThunderbirdPath")).toString();
}

void EmailSettingsStore::setThunderbirdPath(const QString &path)
{
    m_settings.setValue(QStringLiteral("Email/v1/ThunderbirdPath"), path);
    m_settings.sync();
}

QString EmailSettingsStore::reportKey(Workflow workflow, const QString &databaseKey) const
{
    const QString base = databaseKey.isEmpty()
        ? QStringLiteral("Reports/v1/")
        : QStringLiteral("Reports/v1/Databases/") + databaseKey + QLatin1Char('/');
    return base + toStableString(workflow) + QStringLiteral("/ExportSubFolder");
}

QString EmailSettingsStore::exportSubfolder(Workflow workflow, const QString &databaseKey) const
{
    const QString scopedKey = reportKey(workflow, databaseKey);
    if (!databaseKey.isEmpty() && m_settings.contains(scopedKey))
        return m_settings.value(scopedKey).toString();
    const QString globalKey = reportKey(workflow, {});
    if (m_settings.contains(globalKey))
        return m_settings.value(globalKey).toString();
    return defaultExportSubfolder(workflow);
}

void EmailSettingsStore::setExportSubfolder(Workflow workflow, const QString &value, const QString &databaseKey)
{
    m_settings.setValue(reportKey(workflow, databaseKey), value);
    m_settings.sync();
}

} // namespace PN::Comm
