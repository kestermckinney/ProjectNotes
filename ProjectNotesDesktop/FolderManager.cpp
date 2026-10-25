// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "FolderManager.h"

#include "DesktopAppController.h"

#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariantMap>

FolderManager* FolderManager::s_instance = nullptr;

namespace {

// Collapse state is a per-device preference, so it lives in the local store
// rather than the synced database.
QSettings localSettings()
{
    return QSettings(DesktopAppController::localSettingsOrganization(), QStringLiteral("AppSettings"));
}

QString collapsedKey(const QString& group)
{
    return QStringLiteral("ProjectFolders/Collapsed/") + group;
}

} // namespace

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

FolderManager::~FolderManager()
{
    if (s_instance == this)
        s_instance = nullptr;
}

// ── QML property ─────────────────────────────────────────────────────────────

QVariantList FolderManager::folders() const
{
    const QSettings settings = localSettings();
    QVariantList out;
    out.reserve(m_folders.size());
    for (const Folder& f : m_folders) {
        QVariantMap m;
        m.insert(QStringLiteral("id"),        f.id);
        m.insert(QStringLiteral("name"),      f.name);
        m.insert(QStringLiteral("icon"),      f.icon);
        m.insert(QStringLiteral("color"),     f.color);
        m.insert(QStringLiteral("count"),     m_counts.value(f.id));
        m.insert(QStringLiteral("collapsed"), settings.value(collapsedKey(f.id), false).toBool());
        out.append(m);
    }
    return out;
}

bool FolderManager::allProjectsCollapsed() const
{
    return localSettings().value(collapsedKey(QStringLiteral("AllProjects")), false).toBool();
}

void FolderManager::setAllProjectsCollapsed(bool collapsed)
{
    if (allProjectsCollapsed() == collapsed)
        return;
    localSettings().setValue(collapsedKey(QStringLiteral("AllProjects")), collapsed);
    emit allProjectsCollapsedChanged();
}

bool FolderManager::uncategorizedCollapsed() const
{
    return localSettings().value(collapsedKey(QStringLiteral("Uncategorized")), false).toBool();
}

void FolderManager::setUncategorizedCollapsed(bool collapsed)
{
    if (uncategorizedCollapsed() == collapsed)
        return;
    localSettings().setValue(collapsedKey(QStringLiteral("Uncategorized")), collapsed);
    emit uncategorizedCollapsedChanged();
}

// ── Lifecycle ────────────────────────────────────────────────────────────────

void FolderManager::reload()
{
    // Callers (sync completion, XML/plugin import) invoke this whenever external
    // data may have changed. Only signal QML when something differs, so an
    // unrelated sync never resets the sidebar's rows.
    QVector<Folder> folders;
    QHash<QString, int> counts;
    load(folders, counts);
    if (folders == m_folders && counts == m_counts)
        return;
    m_folders = folders;
    m_counts = counts;
    emit foldersChanged();
}

void FolderManager::refresh()
{
    load(m_folders, m_counts);
    emit foldersChanged();
}

void FolderManager::load(QVector<Folder>& folders, QHash<QString, int>& counts) const
{
    folders.clear();
    counts.clear();
    if (!global_DBObjects.getDb().isOpen())
        return;

    QSqlQuery q(global_DBObjects.getDb());
    if (q.exec(QStringLiteral("SELECT id, folder_name, icon, color FROM project_folders WHERE deleted = 0 "
                              "ORDER BY sort_order, folder_name COLLATE NOCASE"))) {
        while (q.next())
            folders.append({q.value(0).toString(), q.value(1).toString(),
                            q.value(2).toString(), q.value(3).toString()});
    }
    if (q.exec(QStringLiteral("SELECT folder_id, COUNT(*) FROM projects "
                              "WHERE deleted = 0 AND COALESCE(folder_id, '') <> '' GROUP BY folder_id"))) {
        while (q.next())
            counts.insert(q.value(0).toString(), q.value(1).toInt());
    }
}

// ── Helpers ──────────────────────────────────────────────────────────────────

int FolderManager::indexOfFolder(const QString& folderId) const
{
    for (int i = 0; i < m_folders.size(); ++i)
        if (m_folders.at(i).id == folderId)
            return i;
    return -1;
}

void FolderManager::folderRowChanged(const QString& folderId, KeyColumnChange::OperationType operation)
{
    // Tells the projects proxies their cached folder names are stale.
    global_DBObjects.pushRowChange(QStringLiteral("project_folders"), folderId, operation);
    global_DBObjects.updateDisplayData();
}

bool FolderManager::updateFolder(const QString& folderId, const char* column, const QVariant& value)
{
    if (indexOfFolder(folderId) < 0)
        return false;
    QSqlQuery q(global_DBObjects.getDb());
    q.prepare(QStringLiteral("UPDATE project_folders SET %1 = ? WHERE id = ?").arg(QLatin1String(column)));
    q.addBindValue(value);
    q.addBindValue(folderId);
    if (!q.exec())
        return false;
    folderRowChanged(folderId, KeyColumnChange::Update);
    refresh();
    return true;
}

// ── Folder CRUD ──────────────────────────────────────────────────────────────

QString FolderManager::folderNameError(const QString& name, const QString& excludeFolderId) const
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        return tr("Enter a folder name.");
    for (const Folder& f : m_folders)
        if (f.id != excludeFolderId && f.name.compare(trimmed, Qt::CaseInsensitive) == 0)
            return tr("A folder named \"%1\" already exists.").arg(f.name);
    return {};
}

QString FolderManager::folderName(const QString& folderId) const
{
    const int i = indexOfFolder(folderId);
    return i < 0 ? QString() : m_folders.at(i).name;
}

QString FolderManager::addFolder(const QString& name, const QString& icon, const QString& color)
{
    if (!folderNameError(name).isEmpty())
        return {};

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlQuery q(global_DBObjects.getDb());
    q.prepare(QStringLiteral("INSERT INTO project_folders(id, folder_name, icon, color, sort_order, updateddate, deleted) "
                             "VALUES(?, ?, ?, ?, (SELECT COALESCE(MAX(sort_order), -1) + 1 FROM project_folders), "
                             "CAST(strftime('%s', 'now') AS INTEGER), 0)"));
    q.addBindValue(id);
    q.addBindValue(name.trimmed());
    q.addBindValue(icon.isEmpty() ? QStringLiteral("folder") : icon);
    q.addBindValue(color.isEmpty() ? QStringLiteral("#c98a1a") : color);
    if (!q.exec())
        return {};
    folderRowChanged(id, KeyColumnChange::Insert);
    refresh();
    return id;
}

bool FolderManager::renameFolder(const QString& folderId, const QString& name)
{
    if (!folderNameError(name, folderId).isEmpty())
        return false;
    return updateFolder(folderId, "folder_name", name.trimmed());
}

bool FolderManager::setFolderIcon(const QString& folderId, const QString& icon)
{
    return !icon.isEmpty() && updateFolder(folderId, "icon", icon);
}

bool FolderManager::setFolderColor(const QString& folderId, const QString& color)
{
    return !color.isEmpty() && updateFolder(folderId, "color", color);
}

bool FolderManager::removeFolder(const QString& folderId)
{
    if (indexOfFolder(folderId) < 0)
        return false;

    // Its projects move to Not Categorized; each one is its own synced change.
    QStringList members;
    QSqlQuery q(global_DBObjects.getDb());
    q.prepare(QStringLiteral("SELECT id FROM projects WHERE folder_id = ?"));
    q.addBindValue(folderId);
    if (q.exec())
        while (q.next())
            members.append(q.value(0).toString());

    QSqlDatabase& db = global_DBObjects.getDb();
    if (!db.transaction())
        return false;
    QSqlQuery clear(db);
    clear.prepare(QStringLiteral("UPDATE projects SET folder_id = NULL WHERE folder_id = ?"));
    clear.addBindValue(folderId);
    QSqlQuery remove(db);
    remove.prepare(QStringLiteral("UPDATE project_folders SET deleted = 1 WHERE id = ?"));
    remove.addBindValue(folderId);
    if (!clear.exec() || !remove.exec() || !db.commit()) {
        db.rollback();
        return false;
    }

    localSettings().remove(collapsedKey(folderId));
    for (const QString& projectId : std::as_const(members))
        global_DBObjects.pushRowChange(QStringLiteral("projects"), projectId);
    folderRowChanged(folderId, KeyColumnChange::Delete);
    refresh();
    return true;
}

bool FolderManager::setFolderCollapsed(const QString& folderId, bool collapsed)
{
    if (indexOfFolder(folderId) < 0)
        return false;
    QSettings settings = localSettings();
    if (settings.value(collapsedKey(folderId), false).toBool() == collapsed)
        return true;
    settings.setValue(collapsedKey(folderId), collapsed);
    emit foldersChanged();
    return true;
}

bool FolderManager::moveFolder(const QString& folderId, int newIndex)
{
    const int i = indexOfFolder(folderId);
    if (i < 0)
        return false;
    newIndex = qBound(0, newIndex, int(m_folders.size()) - 1);
    if (newIndex == i)
        return true;
    QVector<Folder> order = m_folders;
    order.move(i, newIndex);

    QSqlDatabase& db = global_DBObjects.getDb();
    if (!db.transaction())
        return false;
    for (int n = 0; n < order.size(); ++n) {
        QSqlQuery q(db);
        q.prepare(QStringLiteral("UPDATE project_folders SET sort_order = ? WHERE id = ? AND sort_order IS NOT ?"));
        q.addBindValue(n);
        q.addBindValue(order.at(n).id);
        q.addBindValue(n);
        if (!q.exec()) {
            db.rollback();
            return false;
        }
    }
    if (!db.commit()) {
        db.rollback();
        return false;
    }
    refresh();
    return true;
}

// ── Membership ───────────────────────────────────────────────────────────────

bool FolderManager::setProjectFolder(const QString& projectId, const QString& folderId)
{
    if (projectId.isEmpty() || (!folderId.isEmpty() && indexOfFolder(folderId) < 0))
        return false;
    if (folderForProject(projectId) == folderId)
        return true;

    QSqlQuery q(global_DBObjects.getDb());
    q.prepare(QStringLiteral("UPDATE projects SET folder_id = ? WHERE id = ?"));
    q.addBindValue(folderId.isEmpty() ? QVariant() : QVariant(folderId));
    q.addBindValue(projectId);
    if (!q.exec())
        return false;

    global_DBObjects.pushRowChange(QStringLiteral("projects"), projectId);
    global_DBObjects.updateDisplayData();
    refresh();
    return true;
}

QString FolderManager::folderForProject(const QString& projectId) const
{
    QSqlQuery q(global_DBObjects.getDb());
    q.prepare(QStringLiteral("SELECT folder_id FROM projects WHERE id = ?"));
    q.addBindValue(projectId);
    if (!q.exec() || !q.next())
        return {};
    // A folder deleted on another device leaves the project uncategorized.
    const QString folderId = q.value(0).toString();
    return indexOfFolder(folderId) < 0 ? QString() : folderId;
}
