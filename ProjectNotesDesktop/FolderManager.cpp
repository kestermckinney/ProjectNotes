// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "FolderManager.h"

#include "databaseobjects.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QVariantMap>

FolderManager* FolderManager::s_instance = nullptr;
const QString  FolderManager::kSettingKey = QStringLiteral("project_folders");

FolderManager* FolderManager::create(QQmlEngine* /*engine*/, QJSEngine* /*scriptEngine*/)
{
    if (!s_instance)
        s_instance = new FolderManager();
    return s_instance;
}

FolderManager::FolderManager(QObject* parent)
    : QObject(parent)
{
    if (!s_instance)
        s_instance = this;
}

// ── QML property ─────────────────────────────────────────────────────────────

QVariantList FolderManager::folders() const
{
    QVariantList out;
    out.reserve(m_folders.size());
    for (const Folder& f : m_folders) {
        int count = 0;
        for (auto it = m_memberships.constBegin(); it != m_memberships.constEnd(); ++it)
            if (it.value().contains(f.id)) ++count;

        QVariantMap m;
        m.insert(QStringLiteral("id"),    f.id);
        m.insert(QStringLiteral("name"),  f.name);
        m.insert(QStringLiteral("icon"),  f.icon);
        m.insert(QStringLiteral("color"), f.color);
        m.insert(QStringLiteral("count"), count);
        out.append(m);
    }
    return out;
}

// ── Lifecycle ────────────────────────────────────────────────────────────────

void FolderManager::reload()
{
    load();
    emit foldersChanged();
}

void FolderManager::load()
{
    m_folders.clear();
    m_memberships.clear();

    const QString raw = global_DBObjects.loadParameter(kSettingKey);
    if (raw.isEmpty())
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    if (!doc.isObject())
        return;

    const QJsonObject root = doc.object();

    const QJsonArray folders = root.value(QStringLiteral("folders")).toArray();
    for (const QJsonValue& v : folders) {
        const QJsonObject o = v.toObject();
        Folder f;
        f.id    = o.value(QStringLiteral("id")).toString();
        f.name  = o.value(QStringLiteral("name")).toString();
        f.icon  = o.value(QStringLiteral("icon")).toString(QStringLiteral("folder"));
        f.color = o.value(QStringLiteral("color")).toString(QStringLiteral("#c98a1a"));
        if (!f.id.isEmpty())
            m_folders.append(f);
    }

    const QJsonObject memberships = root.value(QStringLiteral("memberships")).toObject();
    for (auto it = memberships.constBegin(); it != memberships.constEnd(); ++it) {
        QStringList folderIds;
        for (const QJsonValue& fv : it.value().toArray()) {
            const QString fid = fv.toString();
            if (!fid.isEmpty())
                folderIds.append(fid);
        }
        if (!folderIds.isEmpty())
            m_memberships.insert(it.key(), folderIds);
    }
}

void FolderManager::save()
{
    QJsonArray folders;
    for (int i = 0; i < m_folders.size(); ++i) {
        const Folder& f = m_folders.at(i);
        QJsonObject o;
        o.insert(QStringLiteral("id"),    f.id);
        o.insert(QStringLiteral("name"),  f.name);
        o.insert(QStringLiteral("icon"),  f.icon);
        o.insert(QStringLiteral("color"), f.color);
        o.insert(QStringLiteral("order"), i);
        folders.append(o);
    }

    QJsonObject memberships;
    for (auto it = m_memberships.constBegin(); it != m_memberships.constEnd(); ++it) {
        if (it.value().isEmpty())
            continue;
        QJsonArray arr;
        for (const QString& fid : it.value())
            arr.append(fid);
        memberships.insert(it.key(), arr);
    }

    QJsonObject root;
    root.insert(QStringLiteral("folders"), folders);
    root.insert(QStringLiteral("memberships"), memberships);

    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Compact);
    global_DBObjects.saveParameter(kSettingKey, QString::fromUtf8(json));

    emit foldersChanged();
}

// ── Helpers ──────────────────────────────────────────────────────────────────

int FolderManager::indexOfFolder(const QString& folderId) const
{
    for (int i = 0; i < m_folders.size(); ++i)
        if (m_folders.at(i).id == folderId)
            return i;
    return -1;
}

// ── Folder CRUD ──────────────────────────────────────────────────────────────

QString FolderManager::addFolder(const QString& name, const QString& icon, const QString& color)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        return {};

    Folder f;
    f.id    = QUuid::createUuid().toString(QUuid::WithoutBraces);
    f.name  = trimmed;
    f.icon  = icon.isEmpty() ? QStringLiteral("folder") : icon;
    f.color = color.isEmpty() ? QStringLiteral("#c98a1a") : color;
    m_folders.append(f);
    save();
    return f.id;
}

bool FolderManager::renameFolder(const QString& folderId, const QString& name)
{
    const int i = indexOfFolder(folderId);
    if (i < 0 || name.trimmed().isEmpty())
        return false;
    m_folders[i].name = name.trimmed();
    save();
    return true;
}

bool FolderManager::setFolderIcon(const QString& folderId, const QString& icon)
{
    const int i = indexOfFolder(folderId);
    if (i < 0 || icon.isEmpty())
        return false;
    m_folders[i].icon = icon;
    save();
    return true;
}

bool FolderManager::setFolderColor(const QString& folderId, const QString& color)
{
    const int i = indexOfFolder(folderId);
    if (i < 0 || color.isEmpty())
        return false;
    m_folders[i].color = color;
    save();
    return true;
}

bool FolderManager::removeFolder(const QString& folderId)
{
    const int i = indexOfFolder(folderId);
    if (i < 0)
        return false;
    m_folders.remove(i);

    // Drop this folder from every project's membership list.
    for (auto it = m_memberships.begin(); it != m_memberships.end(); ) {
        it.value().removeAll(folderId);
        if (it.value().isEmpty())
            it = m_memberships.erase(it);
        else
            ++it;
    }
    save();
    return true;
}

bool FolderManager::moveFolder(const QString& folderId, int newIndex)
{
    const int i = indexOfFolder(folderId);
    if (i < 0)
        return false;
    newIndex = qBound(0, newIndex, m_folders.size() - 1);
    if (newIndex == i)
        return true;
    const Folder f = m_folders.at(i);
    m_folders.remove(i);
    m_folders.insert(newIndex, f);
    save();
    return true;
}

// ── Membership ───────────────────────────────────────────────────────────────

bool FolderManager::addProjectToFolder(const QString& projectId, const QString& folderId)
{
    if (projectId.isEmpty() || indexOfFolder(folderId) < 0)
        return false;

    QStringList& ids = m_memberships[projectId];
    if (ids.contains(folderId))
        return true;   // already a member — no-op, still success
    ids.append(folderId);
    save();
    return true;
}

bool FolderManager::removeProjectFromFolder(const QString& projectId, const QString& folderId)
{
    auto it = m_memberships.find(projectId);
    if (it == m_memberships.end())
        return false;
    if (it.value().removeAll(folderId) == 0)
        return false;
    if (it.value().isEmpty())
        m_memberships.erase(it);
    save();
    return true;
}

void FolderManager::removeProjectFromAllFolders(const QString& projectId)
{
    if (m_memberships.remove(projectId) > 0)
        save();
}

bool FolderManager::isProjectInFolder(const QString& projectId, const QString& folderId) const
{
    return m_memberships.value(projectId).contains(folderId);
}

QStringList FolderManager::foldersForProject(const QString& projectId) const
{
    return m_memberships.value(projectId);
}

QStringList FolderManager::projectIdsInFolder(const QString& folderId) const
{
    QStringList out;
    for (auto it = m_memberships.constBegin(); it != m_memberships.constEnd(); ++it)
        if (it.value().contains(folderId))
            out.append(it.key());
    return out;
}
