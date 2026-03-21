// Copyright (C) 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseupgrade_v5_0_0.h"
#include "databaseobjects.h"

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
    global_DBObjects.execute(R"(ALTER TABLE application_version ADD COLUMN syncdate INTEGER;)");
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
    global_DBObjects.execute(R"(ALTER TABLE application_version ADD COLUMN deleted INTEGER DEFAULT 0;)");
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
        CREATE TRIGGER trg_application_version_updated AFTER UPDATE ON application_version
        WHEN NEW.syncdate IS OLD.syncdate
        BEGIN
            UPDATE application_version SET updateddate = CAST(strftime('%s', 'now') AS INTEGER), syncdate = NULL
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
