// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef FOLDERMANAGER_H
#define FOLDERMANAGER_H

#include <QObject>
#include <QVariantList>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QVector>

class QQmlEngine;
class QJSEngine;

// FolderManager — user-defined project folders (e.g. "Favorites").
//
// A project can belong to multiple folders. Folder definitions and the
// project→folder memberships are persisted as a single JSON document in the
// application_settings table (parameter_name = "project_folders") via
// DatabaseObjects::saveParameter / loadParameter, so they sync through the
// existing SqliteSyncPro plumbing like any other setting.
//
// JSON shape:
//   { "folders":     [ {"id","name","icon","color","order"} ],
//     "memberships": { "<projectId>": ["<folderId>", ...] } }
//
// Projects that are members of no folder are shown by QML under an implicit
// "All Projects" group; that group is not stored here.
class FolderManager : public QObject
{
    Q_OBJECT

    // List of folder maps { id, name, icon, color, count, collapsed } for QML
    // sidebar + settings editor. Re-emitted on every mutation.
    Q_PROPERTY(QVariantList folders READ folders NOTIFY foldersChanged)

    // Collapsed state of the sidebar's two implicit groups (not real folders,
    // so they have no entry in `folders`). Persisted alongside folders/
    // memberships so collapse state survives a restart and syncs like the
    // rest of this document.
    Q_PROPERTY(bool allProjectsCollapsed READ allProjectsCollapsed
                   WRITE setAllProjectsCollapsed NOTIFY allProjectsCollapsedChanged)
    Q_PROPERTY(bool uncategorizedCollapsed READ uncategorizedCollapsed
                   WRITE setUncategorizedCollapsed NOTIFY uncategorizedCollapsedChanged)

public:
    explicit FolderManager(QObject* parent = nullptr);

    static FolderManager* create(QQmlEngine* engine, QJSEngine* scriptEngine);
    static FolderManager* instance() { return s_instance; }

    QVariantList folders() const;

    bool allProjectsCollapsed() const { return m_allProjectsCollapsed; }
    void setAllProjectsCollapsed(bool collapsed);
    bool uncategorizedCollapsed() const { return m_uncategorizedCollapsed; }
    void setUncategorizedCollapsed(bool collapsed);

    // ── Lifecycle ────────────────────────────────────────────────────────────
    // Load the stored JSON from application_settings. Call once the database is
    // open (from QML after AppController.openOrCreateDatabase()).
    Q_INVOKABLE void reload();

    // ── Folder CRUD ──────────────────────────────────────────────────────────
    Q_INVOKABLE QString addFolder(const QString& name,
                                  const QString& icon = QStringLiteral("folder"),
                                  const QString& color = QStringLiteral("#c98a1a"));
    Q_INVOKABLE bool    renameFolder(const QString& folderId, const QString& name);
    Q_INVOKABLE bool    setFolderIcon(const QString& folderId, const QString& icon);
    Q_INVOKABLE bool    setFolderColor(const QString& folderId, const QString& color);
    Q_INVOKABLE bool    removeFolder(const QString& folderId);
    Q_INVOKABLE bool    moveFolder(const QString& folderId, int newIndex);
    Q_INVOKABLE bool    setFolderCollapsed(const QString& folderId, bool collapsed);

    // ── Membership (multi-folder) ────────────────────────────────────────────
    Q_INVOKABLE bool        addProjectToFolder(const QString& projectId, const QString& folderId);
    Q_INVOKABLE bool        removeProjectFromFolder(const QString& projectId, const QString& folderId);
    Q_INVOKABLE void        removeProjectFromAllFolders(const QString& projectId);
    Q_INVOKABLE bool        isProjectInFolder(const QString& projectId, const QString& folderId) const;
    Q_INVOKABLE QStringList foldersForProject(const QString& projectId) const;
    Q_INVOKABLE QStringList projectIdsInFolder(const QString& folderId) const;

signals:
    void foldersChanged();
    void allProjectsCollapsedChanged();
    void uncategorizedCollapsedChanged();

private:
    struct Folder {
        QString id;
        QString name;
        QString icon;
        QString color;
        bool    collapsed = false;
    };

    int  indexOfFolder(const QString& folderId) const;
    void load();
    void save();

    QVector<Folder>              m_folders;
    QHash<QString, QStringList>  m_memberships;   // projectId -> [folderId,...]
    bool                         m_allProjectsCollapsed  = false;
    bool                         m_uncategorizedCollapsed = false;

    // The exact JSON string currently reflected in m_folders/m_memberships (as
    // last loaded from or saved to application_settings). reload() compares the
    // stored document against this so a no-op reload — e.g. after a sync cycle
    // that didn't touch project_folders — skips the foldersChanged churn.
    QString                      m_rawJson;

    static FolderManager* s_instance;

    static const QString kSettingKey;
};

#endif // FOLDERMANAGER_H
