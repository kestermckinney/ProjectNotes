// Copyright (C) 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseupgrade_v5_0_0.h"
#include "databaseobjects.h"

// Helper: recreate a table without its inline UNIQUE constraints, preserving all data.
// SQLite does not support DROP CONSTRAINT, so the standard rename-copy-drop approach is used.
static void recreateWithoutUniqueConstraints(const QString& table, const QString& newSchema)
{
    global_DBObjects.execute(QString("CREATE TABLE %1_new %2;").arg(table, newSchema));
    global_DBObjects.execute(QString("INSERT INTO %1_new SELECT * FROM %1;").arg(table));
    global_DBObjects.execute(QString("DROP TABLE %1;").arg(table));
    global_DBObjects.execute(QString("ALTER TABLE %1_new RENAME TO %1;").arg(table));
}


void db_UpgradeStep_v5_0_0()
{
    // Rename column 0 (primary key) to id on every table
    global_DBObjects.execute(R"(ALTER TABLE application_settings RENAME COLUMN parameter_id TO id;)");

    // For application_version, rebuild the table with id (GUID) as PK and keep current_version for version string
    global_DBObjects.execute(R"(
        CREATE TABLE application_version_new(
            id              TEXT PRIMARY KEY UNIQUE NOT NULL,
            current_version TEXT
        );
    )");

    // Copy data from old table, generating UUIDs for the id column
    global_DBObjects.execute(R"(
        INSERT INTO application_version_new (id, current_version)
        SELECT lower(hex(randomblob(16))), current_version
        FROM application_version;
    )");

    // Drop old table and rename new one
    global_DBObjects.execute(R"(DROP TABLE application_version;)");
    global_DBObjects.execute(R"(ALTER TABLE application_version_new RENAME TO application_version;)");

    global_DBObjects.execute(R"(ALTER TABLE clients RENAME COLUMN client_id TO id;)");
    global_DBObjects.execute(R"(ALTER TABLE item_tracker RENAME COLUMN item_id TO id;)");
    global_DBObjects.execute(R"(ALTER TABLE item_tracker_updates RENAME COLUMN tracker_updated_id TO id;)");
    global_DBObjects.execute(R"(ALTER TABLE meeting_attendees RENAME COLUMN attendee_id TO id;)");
    global_DBObjects.execute(R"(ALTER TABLE people RENAME COLUMN people_id TO id;)");
    global_DBObjects.execute(R"(ALTER TABLE project_locations RENAME COLUMN location_id TO id;)");
    global_DBObjects.execute(R"(ALTER TABLE project_notes RENAME COLUMN note_id TO id;)");
    global_DBObjects.execute(R"(ALTER TABLE project_people RENAME COLUMN teammember_id TO id;)");
    global_DBObjects.execute(R"(ALTER TABLE project_risks RENAME COLUMN risk_id TO id;)");
    global_DBObjects.execute(R"(ALTER TABLE projects RENAME COLUMN project_id TO id;)");
    global_DBObjects.execute(R"(ALTER TABLE status_report_items RENAME COLUMN status_item_id TO id;)");

    // Add updateddate and syncdate columns to every table
    global_DBObjects.execute(R"(ALTER TABLE application_settings ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE application_settings ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE application_version ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE clients ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE clients ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE item_tracker ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE item_tracker ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE item_tracker_updates ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE item_tracker_updates ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE meeting_attendees ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE meeting_attendees ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE people ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE people ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE project_locations ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE project_locations ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE project_notes ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE project_notes ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE project_people ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE project_people ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE project_risks ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE project_risks ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE projects ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE projects ADD COLUMN syncdate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE status_report_items ADD COLUMN updateddate INTEGER;)");
    global_DBObjects.execute(R"(ALTER TABLE status_report_items ADD COLUMN syncdate INTEGER;)");

    // Add deleted column to every table
    global_DBObjects.execute(R"(ALTER TABLE application_settings ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE clients ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE item_tracker ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE item_tracker_updates ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE meeting_attendees ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE people ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE project_locations ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE project_notes ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE project_people ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE project_risks ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE projects ADD COLUMN deleted INTEGER DEFAULT 0;)");
    global_DBObjects.execute(R"(ALTER TABLE status_report_items ADD COLUMN deleted INTEGER DEFAULT 0;)");

    // Create updateddate triggers for every table.
    // WHEN NEW.syncdate IS OLD.syncdate: skip if the UPDATE is only writing syncdate itself
    // (i.e. SqliteSyncPro marking the row as synced), so syncdate is not immediately reset to NULL.
    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_application_settings_updated AFTER UPDATE ON application_settings
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE application_settings SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_clients_updated AFTER UPDATE ON clients
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE clients SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_item_tracker_updated AFTER UPDATE ON item_tracker
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE item_tracker SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_item_tracker_updates_updated AFTER UPDATE ON item_tracker_updates
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE item_tracker_updates SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_meeting_attendees_updated AFTER UPDATE ON meeting_attendees
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE meeting_attendees SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_people_updated AFTER UPDATE ON people
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE people SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_locations_updated AFTER UPDATE ON project_locations
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_locations SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_notes_updated AFTER UPDATE ON project_notes
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_notes SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_people_updated AFTER UPDATE ON project_people
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_people SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_risks_updated AFTER UPDATE ON project_risks
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_risks SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_projects_updated AFTER UPDATE ON projects
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE projects SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_status_report_items_updated AFTER UPDATE ON status_report_items
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE status_report_items SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    // Drop old triggers that reset syncdate = NULL on every UPDATE (including when
    // SqliteSyncPro writes syncdate to mark a row as synced, causing an infinite loop).
    // Recreate them with WHEN NEW.syncdate IS OLD.syncdate so they only fire on
    // normal data changes, not when syncdate itself is being stamped.

    const QStringList triggers = {
        "trg_application_settings_updated",
        "trg_clients_updated",
        "trg_item_tracker_updated",
        "trg_item_tracker_updates_updated",
        "trg_meeting_attendees_updated",
        "trg_people_updated",
        "trg_project_locations_updated",
        "trg_project_notes_updated",
        "trg_project_people_updated",
        "trg_project_risks_updated",
        "trg_projects_updated",
        "trg_status_report_items_updated",
    };

    for (const QString& name : triggers)
        global_DBObjects.execute(QString("DROP TRIGGER IF EXISTS %1;").arg(name));

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_application_settings_updated AFTER UPDATE ON application_settings
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE application_settings SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_clients_updated AFTER UPDATE ON clients
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE clients SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_item_tracker_updated AFTER UPDATE ON item_tracker
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE item_tracker SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_item_tracker_updates_updated AFTER UPDATE ON item_tracker_updates
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE item_tracker_updates SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_meeting_attendees_updated AFTER UPDATE ON meeting_attendees
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE meeting_attendees SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_people_updated AFTER UPDATE ON people
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE people SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_locations_updated AFTER UPDATE ON project_locations
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_locations SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_notes_updated AFTER UPDATE ON project_notes
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_notes SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_people_updated AFTER UPDATE ON project_people
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_people SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_risks_updated AFTER UPDATE ON project_risks
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_risks SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_projects_updated AFTER UPDATE ON projects
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE projects SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_status_report_items_updated AFTER UPDATE ON status_report_items
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE status_report_items SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    // -------------------------------------------------------------------------
    // Remove syncdate and deleted from application_version so SqliteSyncPro
    // does not sync it. Drop its trigger as it references syncdate.
    // -------------------------------------------------------------------------

    global_DBObjects.execute("DROP TRIGGER IF EXISTS trg_application_version_updated;");

    global_DBObjects.execute(R"(CREATE TABLE application_version_new (
        id              TEXT PRIMARY KEY
        UNIQUE
        NOT NULL,
        current_version TEXT,
        updateddate     INTEGER
    );)");
    global_DBObjects.execute("INSERT INTO application_version_new (id, current_version, updateddate) SELECT id, current_version, updateddate FROM application_version;");
    global_DBObjects.execute("DROP TABLE application_version;");
    global_DBObjects.execute("ALTER TABLE application_version_new RENAME TO application_version;");

    // -------------------------------------------------------------------------
    // Recreate tables without inline UNIQUE constraints (except PRIMARY KEY).
    // Partial unique indexes (WHERE deleted = 0) are created afterwards so that
    // soft-deleted rows do not block re-insertion of logically new records.
    // -------------------------------------------------------------------------

    recreateWithoutUniqueConstraints("clients", R"((
        id          TEXT PRIMARY KEY
                         UNIQUE
                         NOT NULL,
        client_name TEXT NOT NULL,
        updateddate INTEGER,
        syncdate    INTEGER,
        deleted     INTEGER DEFAULT 0
    ))");
    global_DBObjects.execute("CREATE INDEX client_name ON clients (client_name);");
    global_DBObjects.execute("CREATE UNIQUE INDEX idx_clients_client_name ON clients (client_name) WHERE deleted = 0;");

    recreateWithoutUniqueConstraints("item_tracker", R"((
        id              TEXT    NOT NULL
        UNIQUE
        PRIMARY KEY,
        item_number     TEXT,
        item_type       TEXT,
        item_name       TEXT,
        identified_by   TEXT,
        date_identified INTEGER,
        description     TEXT,
        assigned_to     TEXT,
        priority        TEXT,
        status          TEXT,
        date_due        INTEGER,
        last_update     INTEGER,
        date_resolved   INTEGER,
        note_id         TEXT,
        project_id      TEXT,
        internal_item   INTEGER,
        updateddate     INTEGER,
        syncdate        INTEGER,
        deleted         INTEGER DEFAULT 0
    ))");
    global_DBObjects.execute("CREATE INDEX traker_status ON item_tracker (status, priority);");
    global_DBObjects.execute("CREATE INDEX traker_project ON item_tracker (project_id);");
    global_DBObjects.execute("CREATE INDEX traker_note ON item_tracker (note_id);");
    global_DBObjects.execute("CREATE INDEX traker_internal ON item_tracker (internal_item);");
    global_DBObjects.execute("CREATE UNIQUE INDEX idx_item_tracker_proj_num ON item_tracker (project_id, item_number) WHERE deleted = 0;");
    global_DBObjects.execute("CREATE UNIQUE INDEX idx_item_tracker_proj_name ON item_tracker (project_id, item_name) WHERE deleted = 0;");

    recreateWithoutUniqueConstraints("meeting_attendees", R"((
        id          TEXT PRIMARY KEY
        UNIQUE
        NOT NULL,
        note_id     TEXT,
        person_id   TEXT,
        updateddate INTEGER,
        syncdate    INTEGER,
        deleted     INTEGER DEFAULT 0
    ))");
    global_DBObjects.execute("CREATE INDEX meeting_attend_note ON meeting_attendees (note_id);");
    global_DBObjects.execute("CREATE UNIQUE INDEX idx_meeting_attendees_note_person ON meeting_attendees (note_id, person_id) WHERE deleted = 0;");

    recreateWithoutUniqueConstraints("people", R"((
        id           TEXT NOT NULL
                          PRIMARY KEY
                          UNIQUE,
        name         TEXT NOT NULL,
        email        TEXT,
        office_phone TEXT,
        cell_phone   TEXT,
        client_id    TEXT,
        role         TEXT,
        updateddate  INTEGER,
        syncdate     INTEGER,
        deleted      INTEGER DEFAULT 0
    ))");
    global_DBObjects.execute("CREATE INDEX person_name ON people (name);");
    global_DBObjects.execute("CREATE UNIQUE INDEX idx_people_name ON people (name) WHERE deleted = 0;");

    recreateWithoutUniqueConstraints("project_locations", R"((
        id                   TEXT PRIMARY KEY
        NOT NULL
        UNIQUE,
        project_id           TEXT,
        location_type        TEXT,
        location_description TEXT,
        full_path            TEXT,
        updateddate          INTEGER,
        syncdate             INTEGER,
        deleted              INTEGER DEFAULT 0
    ))");
    global_DBObjects.execute("CREATE INDEX location_project ON project_locations (project_id);");
    global_DBObjects.execute("CREATE UNIQUE INDEX idx_project_locations_proj_desc ON project_locations (project_id, location_description) WHERE deleted = 0;");

    recreateWithoutUniqueConstraints("project_people", R"((
        id                    TEXT    PRIMARY KEY
                                      UNIQUE
                                      NOT NULL,
        people_id             TEXT    NOT NULL,
        project_id            TEXT    NOT NULL,
        role                  TEXT,
        receive_status_report INTEGER,
        updateddate           INTEGER,
        syncdate              INTEGER,
        deleted               INTEGER DEFAULT 0
    ))");
    global_DBObjects.execute("CREATE INDEX proj_people_project ON project_people (project_id);");
    global_DBObjects.execute("CREATE UNIQUE INDEX idx_project_people_proj_person ON project_people (project_id, people_id) WHERE deleted = 0;");

    recreateWithoutUniqueConstraints("projects", R"((
        id                   TEXT    PRIMARY KEY
                                     UNIQUE
                                     NOT NULL,
        project_number       TEXT,
        project_name         TEXT    NOT NULL,
        last_status_date     INTEGER,
        last_invoice_date    INTEGER,
        primary_contact      TEXT,
        budget               REAL,
        actual               REAL,
        bcwp                 REAL,
        bcws                 REAL,
        bac                  REAL,
        invoicing_period     TEXT,
        status_report_period TEXT,
        client_id            TEXT,
        project_status       TEXT,
        updateddate          INTEGER,
        syncdate             INTEGER,
        deleted              INTEGER DEFAULT 0
    ))");
    global_DBObjects.execute("CREATE INDEX proj_status ON projects (project_status);");
    global_DBObjects.execute("CREATE UNIQUE INDEX idx_projects_name ON projects (project_name) WHERE deleted = 0;");
    global_DBObjects.execute("CREATE UNIQUE INDEX idx_projects_number ON projects (project_number) WHERE deleted = 0;");

    recreateWithoutUniqueConstraints("status_report_items", R"((
        id               TEXT UNIQUE
                              PRIMARY KEY
                              NOT NULL,
        project_id       TEXT NOT NULL,
        task_category    TEXT,
        task_description TEXT NOT NULL,
        updateddate      INTEGER,
        syncdate         INTEGER,
        deleted          INTEGER DEFAULT 0
    ))");
    global_DBObjects.execute("CREATE INDEX stat_project ON status_report_items (project_id);");
    global_DBObjects.execute("CREATE UNIQUE INDEX idx_status_report_items_desc_proj ON status_report_items (task_description, project_id) WHERE deleted = 0;");

    // -------------------------------------------------------------------------
    // Recreate triggers (dropped automatically when tables were dropped above)
    // -------------------------------------------------------------------------

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_clients_updated AFTER UPDATE ON clients
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE clients SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_item_tracker_updated AFTER UPDATE ON item_tracker
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE item_tracker SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_meeting_attendees_updated AFTER UPDATE ON meeting_attendees
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE meeting_attendees SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_people_updated AFTER UPDATE ON people
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE people SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_locations_updated AFTER UPDATE ON project_locations
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_locations SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_people_updated AFTER UPDATE ON project_people
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE project_people SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_projects_updated AFTER UPDATE ON projects
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE projects SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_status_report_items_updated AFTER UPDATE ON status_report_items
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE status_report_items SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");
}
