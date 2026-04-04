// Copyright (C) 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databasecreate.h"
#include "databaseobjects.h"

void db_CreateNewDatabase()
{
    // Application settings table
    global_DBObjects.execute(R"(
        CREATE TABLE application_settings(
            id              TEXT PRIMARY KEY
            UNIQUE
            NOT NULL,
            parameter_name  TEXT,
            parameter_value TEXT,
            updateddate     INTEGER,
            syncdate        INTEGER,
            deleted         INTEGER DEFAULT 0
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX app_set_name on application_settings (parameter_name);
    )");

    // Application version table — no syncdate/deleted so SqliteSyncPro does not sync it
    global_DBObjects.execute(R"(
        CREATE TABLE application_version(
            id              TEXT PRIMARY KEY
            UNIQUE
            NOT NULL,
            current_version TEXT,
            updateddate     INTEGER
        );
    )");

    // Clients table
    global_DBObjects.execute(R"(
        CREATE TABLE clients (
            id          TEXT PRIMARY KEY
                             UNIQUE
                             NOT NULL,
            client_name TEXT NOT NULL,
            updateddate INTEGER,
            syncdate    INTEGER,
            deleted     INTEGER DEFAULT 0
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX client_name on clients (client_name);
    )");

    global_DBObjects.execute(R"(
        CREATE UNIQUE INDEX idx_clients_client_name ON clients (client_name) WHERE deleted = 0;
    )");

    // Item tracker table
    global_DBObjects.execute(R"(
        CREATE TABLE item_tracker(
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
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX traker_status on item_tracker (status, priority);
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX traker_project on item_tracker (project_id);
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX traker_note on item_tracker (note_id);
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX traker_internal on item_tracker (internal_item);
    )");

    global_DBObjects.execute(R"(
        CREATE UNIQUE INDEX idx_item_tracker_proj_num ON item_tracker (project_id, item_number) WHERE deleted = 0;
    )");

    global_DBObjects.execute(R"(
        CREATE UNIQUE INDEX idx_item_tracker_proj_name ON item_tracker (project_id, item_name) WHERE deleted = 0;
    )");

    // Item tracker updates table
    global_DBObjects.execute(R"(
        CREATE TABLE item_tracker_updates (
            id                 TEXT    PRIMARY KEY
                                       UNIQUE
                                       NOT NULL,
            item_id            TEXT    NOT NULL,
            lastupdated_date   INTEGER,
            update_note        TEXT,
            updated_by         TEXT,
            updateddate        INTEGER,
            syncdate           INTEGER,
            deleted            INTEGER DEFAULT 0
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX traker_update_item on item_tracker_updates (item_id);
    )");

    // Meeting attendees table
    global_DBObjects.execute(R"(
        CREATE TABLE meeting_attendees(
            id          TEXT PRIMARY KEY
            UNIQUE
            NOT NULL,
            note_id     TEXT,
            person_id   TEXT,
            updateddate INTEGER,
            syncdate    INTEGER,
            deleted     INTEGER DEFAULT 0
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX meeting_attend_note on meeting_attendees (note_id);
    )");

    global_DBObjects.execute(R"(
        CREATE UNIQUE INDEX idx_meeting_attendees_note_person ON meeting_attendees (note_id, person_id) WHERE deleted = 0;
    )");

    // People table
    global_DBObjects.execute(R"(
        CREATE TABLE people (
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
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX person_name on people (name);
    )");

    global_DBObjects.execute(R"(
        CREATE UNIQUE INDEX idx_people_name ON people (name) WHERE deleted = 0;
    )");

    // Project locations table
    global_DBObjects.execute(R"(
        CREATE TABLE project_locations(
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
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX location_project on project_locations (project_id);
    )");

    global_DBObjects.execute(R"(
        CREATE UNIQUE INDEX idx_project_locations_proj_desc ON project_locations (project_id, location_description) WHERE deleted = 0;
    )");

    // Project notes table
    global_DBObjects.execute(R"(
        CREATE TABLE project_notes (
            id            TEXT    PRIMARY KEY
                                  UNIQUE
                                  NOT NULL,
            project_id    TEXT,
            note_title    TEXT    NOT NULL,
            note_date     INTEGER NOT NULL,
            note          TEXT,
            internal_item INTEGER,
            updateddate   INTEGER,
            syncdate      INTEGER,
            deleted       INTEGER DEFAULT 0
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX note_project on project_notes (project_id);
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX note_internal on project_notes (internal_item);
    )");

    // Project people table
    global_DBObjects.execute(R"(
        CREATE TABLE project_people (
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
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX proj_people_project on project_people (project_id);
    )");

    global_DBObjects.execute(R"(
        CREATE UNIQUE INDEX idx_project_people_proj_person ON project_people (project_id, people_id) WHERE deleted = 0;
    )");

    // Project risks table
    global_DBObjects.execute(R"(
        CREATE TABLE project_risks (
            id                    TEXT    PRIMARY KEY
                                          UNIQUE
                                          NOT NULL,
            project_id            TEXT,
            internal_risk         INTEGER,
            risk_number           TEXT    NOT NULL,
            risk_event            TEXT    NOT NULL,
            risk_cause            TEXT,
            risk_effect           TEXT,
            risk_type             TEXT,
            risk_probability      TEXT,
            risk_impact           TEXT,
            risk_strategy         TEXT,
            risk_response_actions TEXT,
            updateddate           INTEGER,
            syncdate              INTEGER,
            deleted               INTEGER DEFAULT 0
        );
    )");

    // Projects table
    global_DBObjects.execute(R"(
        CREATE TABLE projects (
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
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX proj_status on projects (project_status);
    )");

    global_DBObjects.execute(R"(
        CREATE UNIQUE INDEX idx_projects_name ON projects (project_name) WHERE deleted = 0;
    )");

    global_DBObjects.execute(R"(
        CREATE UNIQUE INDEX idx_projects_number ON projects (project_number) WHERE deleted = 0;
    )");

    // Status report items table
    global_DBObjects.execute(R"(
        CREATE TABLE status_report_items (
            id               TEXT UNIQUE
                                  PRIMARY KEY
                                  NOT NULL,
            project_id       TEXT NOT NULL,
            task_category    TEXT,
            task_description TEXT NOT NULL,
            updateddate      INTEGER,
            syncdate         INTEGER,
            deleted          INTEGER DEFAULT 0
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX stat_project on status_report_items (project_id);
    )");

    global_DBObjects.execute(R"(
        CREATE UNIQUE INDEX idx_status_report_items_desc_proj ON status_report_items (task_description, project_id) WHERE deleted = 0;
    )");

    // Triggers — on data changes: stamp updateddate and clear syncdate so the row is queued for sync.
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
}
