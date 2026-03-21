// Copyright (C) 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseupgrade_v5_0_1.h"
#include "databaseobjects.h"

void db_UpgradeStep_v5_0_1()
{
    // Drop old triggers that reset syncdate = NULL on every UPDATE (including when
    // SqliteSyncPro writes syncdate to mark a row as synced, causing an infinite loop).
    // Recreate them with WHEN NEW.syncdate IS OLD.syncdate so they only fire on
    // normal data changes, not when syncdate itself is being stamped.

    const QStringList triggers = {
        "trg_application_settings_updated",
        "trg_application_version_updated",
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
