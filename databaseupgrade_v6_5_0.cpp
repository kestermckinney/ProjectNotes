// SPDX-License-Identifier: GPL-3.0-only
#include "databaseupgrade_v6_5_0.h"
#include "databaseobjects.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>

static bool db_HasColumn(QSqlDatabase &db, const QString &table, const QString &column)
{
    QSqlQuery columns(db);
    if (columns.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table)))
        while (columns.next())
            if (columns.value(1).toString().compare(column, Qt::CaseInsensitive) == 0)
                return true;
    return false;
}

bool db_UpgradeStep_v6_5_0()
{
    QWriteLocker lock(&db_rwlock);
    QSqlDatabase &db = global_DBObjects.getDb();
    if (!db.transaction()) {
        global_DBObjects.setLastSaveError(db.lastError().text());
        return false;
    }
    auto fail = [&](const QSqlError &error) {
        global_DBObjects.setLastSaveError(error.text());
        db.rollback();
        return false;
    };
    auto run = [&](const QString &sql) {
        QSqlQuery q(db);
        return q.exec(sql) ? QSqlError() : q.lastError();
    };

    const QStringList schema {
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS project_folders (
                id          TEXT PRIMARY KEY NOT NULL,
                folder_name TEXT NOT NULL,
                icon        TEXT NOT NULL DEFAULT 'folder',
                color       TEXT NOT NULL DEFAULT '#c98a1a',
                sort_order  INTEGER NOT NULL DEFAULT 0,
                updateddate INTEGER,
                syncdate    INTEGER,
                deleted     INTEGER NOT NULL DEFAULT 0
            ))"),
        QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idx_project_folders_name "
                       "ON project_folders(folder_name COLLATE NOCASE) WHERE deleted = 0"),
        QStringLiteral(R"(
            CREATE TRIGGER IF NOT EXISTS trg_project_folders_updated AFTER UPDATE ON project_folders
            WHEN NEW.syncdate IS OLD.syncdate
            BEGIN
                UPDATE project_folders SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
                WHERE id = NEW.id;
            END)"),
    };
    for (const QString &sql : schema) {
        const QSqlError error = run(sql);
        if (error.isValid()) return fail(error);
    }
    if (!db_HasColumn(db, QStringLiteral("projects"), QStringLiteral("folder_id"))) {
        const QSqlError error = run(QStringLiteral("ALTER TABLE projects ADD COLUMN folder_id TEXT"));
        if (error.isValid()) return fail(error);
    }
    const QSqlError indexError = run(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_projects_folder ON projects(folder_id)"));
    if (indexError.isValid()) return fail(indexError);

    // Folders and memberships used to live in one JSON setting. An older synced
    // client can push it back, so every live copy is merged and then retired.
    QSqlQuery legacy(db);
    if (!legacy.exec(QStringLiteral("SELECT id, parameter_value FROM application_settings "
                                    "WHERE parameter_name = 'project_folders' AND deleted = 0 "
                                    "ORDER BY COALESCE(updateddate, 0) DESC, id")))
        return fail(legacy.lastError());
    QStringList settingIds;
    QStringList documents;
    while (legacy.next()) {
        settingIds.append(legacy.value(0).toString());
        documents.append(legacy.value(1).toString());
    }
    legacy.finish();

    for (const QString &raw : documents) {
        const QJsonObject root = QJsonDocument::fromJson(raw.toUtf8()).object();

        // Legacy folder id -> project_folders id. Folder ids are kept so devices
        // that migrate the same synced document independently converge; a name
        // already taken by another folder maps onto that folder instead.
        QHash<QString, QString> folderIds;
        const QJsonArray folders = root.value(QStringLiteral("folders")).toArray();
        for (int i = 0; i < folders.size(); ++i) {
            const QJsonObject o = folders.at(i).toObject();
            const QString id = o.value(QStringLiteral("id")).toString();
            const QString name = o.value(QStringLiteral("name")).toString().trimmed();
            if (id.isEmpty() || name.isEmpty())
                continue;

            QSqlQuery existing(db);
            existing.prepare(QStringLiteral("SELECT id, deleted FROM project_folders WHERE id = ? "
                                            "UNION ALL SELECT id, deleted FROM project_folders "
                                            "WHERE folder_name = ? COLLATE NOCASE AND deleted = 0 AND id <> ?"));
            existing.addBindValue(id);
            existing.addBindValue(name);
            existing.addBindValue(id);
            if (!existing.exec()) return fail(existing.lastError());
            if (existing.next()) {
                // A folder deleted since the document was written stays deleted.
                if (existing.value(1).toInt() == 0)
                    folderIds.insert(id, existing.value(0).toString());
                continue;
            }
            existing.finish();

            QSqlQuery insert(db);
            insert.prepare(QStringLiteral("INSERT INTO project_folders(id, folder_name, icon, color, sort_order, updateddate, deleted) "
                                          "VALUES(?, ?, ?, ?, ?, CAST(strftime('%s', 'now') AS INTEGER), 0)"));
            insert.addBindValue(id);
            insert.addBindValue(name);
            insert.addBindValue(o.value(QStringLiteral("icon")).toString(QStringLiteral("folder")));
            insert.addBindValue(o.value(QStringLiteral("color")).toString(QStringLiteral("#c98a1a")));
            insert.addBindValue(i);
            if (!insert.exec()) return fail(insert.lastError());
            folderIds.insert(id, id);
        }

        // A project now lives in one folder: the first one it listed.
        const QJsonObject memberships = root.value(QStringLiteral("memberships")).toObject();
        for (auto it = memberships.constBegin(); it != memberships.constEnd(); ++it) {
            QString folderId;
            for (const QJsonValue &value : it.value().toArray()) {
                folderId = folderIds.value(value.toString());
                if (!folderId.isEmpty())
                    break;
            }
            if (folderId.isEmpty())
                continue;
            QSqlQuery assign(db);
            assign.prepare(QStringLiteral("UPDATE projects SET folder_id = ? WHERE id = ? AND folder_id IS NULL"));
            assign.addBindValue(folderId);
            assign.addBindValue(it.key());
            if (!assign.exec()) return fail(assign.lastError());
        }
    }

    for (const QString &id : settingIds) {
        QSqlQuery retire(db);
        retire.prepare(QStringLiteral("UPDATE application_settings SET deleted = 1 WHERE id = ?"));
        retire.addBindValue(id);
        if (!retire.exec()) return fail(retire.lastError());
    }

    if (!db.commit()) return fail(db.lastError());
    return true;
}
