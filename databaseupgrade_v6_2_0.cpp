// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseupgrade_v6_2_0.h"
#include "databaseobjects.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QSqlQuery>

// Folds report-scoped audience rows into one project-wide row per name. The
// most recently updated row survives and collects every report it was the
// default for; the others are soft-deleted so the removal syncs.
static void db_ShareProjectEmailAudiences()
{
    QSqlDatabase &db = global_DBObjects.getDb();
    QSqlQuery legacy(db);
    if (!legacy.exec(QStringLiteral(
            "SELECT id, project_id, audience_name, workflow, is_default, default_workflows_json "
            "FROM project_email_audiences WHERE workflow <> 'project' AND deleted = 0 "
            "ORDER BY project_id, audience_name COLLATE NOCASE, COALESCE(updateddate, 0) DESC, id")))
        return;

    struct Group { QString survivor; QStringList duplicates; QStringList defaults; };
    QMap<QString, Group> groups;
    QStringList order;
    while (legacy.next()) {
        const QString key = legacy.value(1).toString() + QChar(0x1f) + legacy.value(2).toString().toCaseFolded();
        Group &group = groups[key];
        if (group.survivor.isEmpty()) {
            group.survivor = legacy.value(0).toString();
            order.append(key);
        } else {
            group.duplicates.append(legacy.value(0).toString());
        }
        for (const QJsonValue &value : QJsonDocument::fromJson(legacy.value(5).toString().toUtf8()).array())
            if (value.isString() && !group.defaults.contains(value.toString()))
                group.defaults.append(value.toString());
        if (legacy.value(4).toInt() == 1 && !group.defaults.contains(legacy.value(3).toString()))
            group.defaults.append(legacy.value(3).toString());
    }
    if (order.isEmpty() || !db.transaction())
        return;

    // The name index is keyed by workflow, so it is dropped while rows move to
    // the shared value and recreated by the caller.
    bool ok = QSqlQuery(db).exec(QStringLiteral("DROP INDEX IF EXISTS idx_project_email_audiences_name"));
    for (const QString &key : order) {
        const Group &group = groups.value(key);
        for (const QString &id : group.duplicates) {
            QSqlQuery remove(db);
            remove.prepare(QStringLiteral("UPDATE project_email_audiences SET deleted = 1, is_default = 0 WHERE id = ?"));
            remove.addBindValue(id);
            ok = remove.exec() && ok;
        }
        QSqlQuery share(db);
        share.prepare(QStringLiteral("UPDATE project_email_audiences SET workflow = 'project', is_default = 0, "
                                     "default_workflows_json = ? WHERE id = ?"));
        share.addBindValue(QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(group.defaults))
                                                 .toJson(QJsonDocument::Compact)));
        share.addBindValue(group.survivor);
        ok = share.exec() && ok;
    }
    if (!ok || !db.commit())
        db.rollback();
}

void db_UpgradeStep_v6_2_0()
{
    global_DBObjects.execute(R"(
        CREATE TABLE IF NOT EXISTS project_email_audiences (
            id                          TEXT PRIMARY KEY NOT NULL,
            project_id                  TEXT NOT NULL,
            workflow                    TEXT NOT NULL,
            audience_name               TEXT NOT NULL,
            people_source               TEXT NOT NULL,
            company_filter              TEXT NOT NULL,
            selected_company_ids_json   TEXT NOT NULL DEFAULT '[]',
            selected_person_ids_json    TEXT NOT NULL DEFAULT '[]',
            recipient_distribution_json TEXT NOT NULL DEFAULT '[]',
            is_default                  INTEGER NOT NULL DEFAULT 0,
            default_workflows_json      TEXT NOT NULL DEFAULT '[]',
            settings_version            INTEGER NOT NULL DEFAULT 1,
            updateddate                 INTEGER,
            syncdate                    INTEGER,
            deleted                     INTEGER NOT NULL DEFAULT 0
        );
    )");
    // Early 6.2.0 builds scoped audiences to one report; they are now shared by
    // every report of the project with a default recorded per report.
    bool hasDefaultWorkflows = false;
    QSqlQuery columns(global_DBObjects.getDb());
    if (columns.exec(QStringLiteral("PRAGMA table_info(project_email_audiences)")))
        while (columns.next())
            if (columns.value(1).toString() == QLatin1String("default_workflows_json"))
                hasDefaultWorkflows = true;
    if (!hasDefaultWorkflows)
        global_DBObjects.execute(R"(ALTER TABLE project_email_audiences ADD COLUMN default_workflows_json TEXT NOT NULL DEFAULT '[]';)");
    global_DBObjects.execute(R"(DROP INDEX IF EXISTS idx_project_email_audiences_default;)");
    global_DBObjects.execute(R"(DROP INDEX IF EXISTS idx_project_email_audiences_project;)");
    global_DBObjects.execute(R"(CREATE INDEX IF NOT EXISTS idx_project_email_audiences_project_shared ON project_email_audiences(project_id, deleted);)");
    db_ShareProjectEmailAudiences();
    global_DBObjects.execute(R"(CREATE UNIQUE INDEX IF NOT EXISTS idx_project_email_audiences_name ON project_email_audiences(project_id, workflow, audience_name COLLATE NOCASE) WHERE deleted = 0;)");
    global_DBObjects.execute(R"(
        CREATE TRIGGER IF NOT EXISTS trg_project_email_audiences_updated
        AFTER UPDATE ON project_email_audiences
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_email_audiences
            SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");
}
