// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef FOLDERMANAGER_H
#define FOLDERMANAGER_H

#include <QObject>
#include <QVariantList>
#include <QString>
#include <QHash>
#include <QVector>

#include "databaseobjects.h"

class QQmlEngine;
class QJSEngine;

// FolderManager — user-defined project folders (e.g. "Favorites").
//
// Folders are rows in the synced project_folders table. A project belongs to
// at most one folder, recorded in projects.folder_id; NULL, empty, or the id
// of a folder deleted on another device all mean "Not Categorized".
//
// Whether a sidebar group is collapsed is a per-device preference kept in the
// local QSettings store, never in the database.
class FolderManager : public QObject
{
    Q_OBJECT

    // List of folder maps { id, name, icon, color, count, collapsed } for QML
    // sidebar + settings editor. Re-emitted on every mutation.
    Q_PROPERTY(QVariantList folders READ folders NOTIFY foldersChanged)

    // Collapsed state of the sidebar's two implicit groups (not real folders,
    // so they have no entry in `folders`).
    Q_PROPERTY(bool allProjectsCollapsed READ allProjectsCollapsed
                   WRITE setAllProjectsCollapsed NOTIFY allProjectsCollapsedChanged)
    Q_PROPERTY(bool uncategorizedCollapsed READ uncategorizedCollapsed
                   WRITE setUncategorizedCollapsed NOTIFY uncategorizedCollapsedChanged)

public:
    explicit FolderManager(QObject* parent = nullptr);
    ~FolderManager() override;

    static FolderManager* create(QQmlEngine* engine, QJSEngine* scriptEngine);
    static FolderManager* instance() { return s_instance; }

    QVariantList folders() const;

    bool allProjectsCollapsed() const;
    void setAllProjectsCollapsed(bool collapsed);
    bool uncategorizedCollapsed() const;
    void setUncategorizedCollapsed(bool collapsed);

    // ── Lifecycle ────────────────────────────────────────────────────────────
    // Re-read folders and project counts. Called once the database is open and
    // whenever external data may have changed (sync, import, plugins); signals
    // QML only when something actually differs.
    Q_INVOKABLE void reload();

    // ── Folder CRUD ──────────────────────────────────────────────────────────
    // Why `name` can't be used, or "" when it can. excludeFolderId is the
    // folder being renamed, so keeping its own name isn't a clash.
    Q_INVOKABLE QString folderNameError(const QString& name, const QString& excludeFolderId = QString()) const;
    Q_INVOKABLE QString addFolder(const QString& name,
                                  const QString& icon = QStringLiteral("folder"),
                                  const QString& color = QStringLiteral("#c98a1a"));
    Q_INVOKABLE bool    renameFolder(const QString& folderId, const QString& name);
    Q_INVOKABLE bool    setFolderIcon(const QString& folderId, const QString& icon);
    Q_INVOKABLE bool    setFolderColor(const QString& folderId, const QString& color);
    Q_INVOKABLE bool    removeFolder(const QString& folderId);
    Q_INVOKABLE bool    moveFolder(const QString& folderId, int newIndex);
    Q_INVOKABLE bool    setFolderCollapsed(const QString& folderId, bool collapsed);
    Q_INVOKABLE QString folderName(const QString& folderId) const;

    // ── Membership (one folder per project) ──────────────────────────────────
    // folderId "" moves the project to Not Categorized.
    Q_INVOKABLE bool    setProjectFolder(const QString& projectId, const QString& folderId);
    Q_INVOKABLE QString folderForProject(const QString& projectId) const;
    bool isFolder(const QString& folderId) const { return indexOfFolder(folderId) >= 0; }

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
        bool operator==(const Folder& o) const
        { return id == o.id && name == o.name && icon == o.icon && color == o.color; }
    };

    int  indexOfFolder(const QString& folderId) const;
    void load(QVector<Folder>& folders, QHash<QString, int>& counts) const;
    bool updateFolder(const QString& folderId, const char* column, const QVariant& value);
    void folderRowChanged(const QString& folderId, KeyColumnChange::OperationType operation);
    void refresh();

    QVector<Folder>      m_folders;      // in sort_order
    QHash<QString, int>  m_counts;       // folderId -> live projects in it

    static FolderManager* s_instance;
};

#endif // FOLDERMANAGER_H
