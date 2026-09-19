// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseupgrade_v6_2_0.h"
#include "databaseobjects.h"

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
            settings_version            INTEGER NOT NULL DEFAULT 1,
            updateddate                 INTEGER,
            syncdate                    INTEGER,
            deleted                     INTEGER NOT NULL DEFAULT 0
        );
    )");
    global_DBObjects.execute(R"(CREATE INDEX IF NOT EXISTS idx_project_email_audiences_project ON project_email_audiences(project_id, workflow, deleted);)");
    global_DBObjects.execute(R"(CREATE UNIQUE INDEX IF NOT EXISTS idx_project_email_audiences_name ON project_email_audiences(project_id, workflow, audience_name COLLATE NOCASE) WHERE deleted = 0;)");
    global_DBObjects.execute(R"(CREATE UNIQUE INDEX IF NOT EXISTS idx_project_email_audiences_default ON project_email_audiences(project_id, workflow) WHERE deleted = 0 AND is_default = 1;)");
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
