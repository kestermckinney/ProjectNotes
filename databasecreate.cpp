// Copyright (C) 2024 Paul McKinney
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
            syncdate        INTEGER
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX app_set_name on application_settings (parameter_name);
    )");

    // Application version table
    global_DBObjects.execute(R"(
        CREATE TABLE application_version(
            id          TEXT PRIMARY KEY
            UNIQUE
            NOT NULL,
            updateddate INTEGER,
            syncdate    INTEGER
        );
    )");

    // Clients table
    global_DBObjects.execute(R"(
        CREATE TABLE clients (
            id          TEXT PRIMARY KEY
                             UNIQUE
                             NOT NULL,
            client_name TEXT UNIQUE
                             NOT NULL,
            updateddate INTEGER,
            syncdate    INTEGER
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX client_name on clients (client_name);
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
            UNIQUE("project_id","item_number"),
            UNIQUE("project_id","item_name")
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
            syncdate           INTEGER
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
            UNIQUE("note_id","person_id")
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX meeting_attend_note on meeting_attendees (note_id);
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
            UNIQUE (
                name ASC
            )
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX person_name on people (name);
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
            UNIQUE("project_id","id"),
            UNIQUE("project_id","location_description")
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX location_project on project_locations (project_id);
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
            syncdate      INTEGER
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
            UNIQUE("project_id","people_id")
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX proj_people_project on project_people (project_id);
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
            syncdate              INTEGER
        );
    )");

    // Projects table
    global_DBObjects.execute(R"(
        CREATE TABLE projects (
            id                   TEXT    PRIMARY KEY
                                         UNIQUE
                                         NOT NULL,
            project_number       TEXT,
            project_name         TEXT    NOT NULL
                                         UNIQUE,
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
            UNIQUE (
                project_number ASC
            )
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX proj_status on projects (project_status);
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
            UNIQUE("task_description","project_id")
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX stat_project on status_report_items (project_id);
    )");

    // Triggers — set updateddate timestamp whenever a record is updated, and reset syncdate to NULL
    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_application_settings_updated AFTER UPDATE ON application_settings
        BEGIN
            UPDATE application_settings SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_application_version_updated AFTER UPDATE ON application_version
        BEGIN
            UPDATE application_version SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_clients_updated AFTER UPDATE ON clients
        BEGIN
            UPDATE clients SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_item_tracker_updated AFTER UPDATE ON item_tracker
        BEGIN
            UPDATE item_tracker SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_item_tracker_updates_updated AFTER UPDATE ON item_tracker_updates
        BEGIN
            UPDATE item_tracker_updates SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_meeting_attendees_updated AFTER UPDATE ON meeting_attendees
        BEGIN
            UPDATE meeting_attendees SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_people_updated AFTER UPDATE ON people
        BEGIN
            UPDATE people SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_locations_updated AFTER UPDATE ON project_locations
        BEGIN
            UPDATE project_locations SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_notes_updated AFTER UPDATE ON project_notes
        BEGIN
            UPDATE project_notes SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_people_updated AFTER UPDATE ON project_people
        BEGIN
            UPDATE project_people SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_project_risks_updated AFTER UPDATE ON project_risks
        BEGIN
            UPDATE project_risks SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_projects_updated AFTER UPDATE ON projects
        BEGIN
            UPDATE projects SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");

    global_DBObjects.execute(R"(
        CREATE TRIGGER trg_status_report_items_updated AFTER UPDATE ON status_report_items
        BEGIN
            UPDATE status_report_items SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
            WHERE id = NEW.id;
        END;
    )");
}
