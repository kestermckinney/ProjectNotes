// Copyright (C) 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseupgrade_v1_2_0.h"
#include "databaseobjects.h"

void db_UpgradeStep_v1_2_0()
{

        global_DBObjects.execute(R"(
            CREATE INDEX app_set_name on application_settings (parameter_name);
        )");

        global_DBObjects.execute(R"(
            CREATE INDEX client_name on clients (client_name);
        )");


        // fix duplicate item_numbers
        global_DBObjects.execute(R"(
            update item_tracker set item_number = ( item_number || '-' || item_id)  where item_id in
            (select item_id from item_tracker i where (select count(p.item_id) from item_tracker p where i.project_id=p.project_id and i.item_number=p.item_number) > 1)
        )");

        // fix duplicate names
        global_DBObjects.execute(R"(
            update item_tracker set item_name = ( item_name || '-' || item_id)  where item_id in
            (select item_id from item_tracker i where (select count(p.item_id) from item_tracker p where i.project_id=p.project_id and i.item_name=p.item_name) > 1)
        )");

        global_DBObjects.execute(R"(
            CREATE TABLE tmp_item_tracker as select * from item_tracker;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE item_tracker;
        )");

        global_DBObjects.execute(R"(
            CREATE TABLE item_tracker(
                item_id         TEXT    NOT NULL
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

        global_DBObjects.execute(R"(
            insert into item_tracker
            (
                item_id,
                item_number,
                item_type,
                item_name,
                identified_by,
                date_identified,
                description,
                assigned_to,
                priority,
                status,
                date_due,
                last_update,
                date_resolved,
                note_id,
                project_id,
                internal_item
            )
            select
                item_id,
                item_number,
                item_type,
                item_name,
                identified_by,
                date_identified,
                description,
                assigned_to,
                priority,
                status,
                date_due,
                last_update,
                date_resolved,
                note_id,
                project_id,
                internal_item
            from tmp_item_tracker;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE tmp_item_tracker;
        )");

        global_DBObjects.execute(R"(
            CREATE INDEX traker_update_item on item_tracker_updates (item_id);
        )");

        global_DBObjects.execute(R"(
            CREATE VIEW item_tracker_view AS SELECT
            item_id,
            item_number,
            item_type,
            item_name,
            identified_by,
            date_identified,
            description,
            assigned_to,
            priority,
            status,
            date_due,
            last_update,
            date_resolved,
            note_id,
            project_id,
            internal_item,
            (select GROUP_CONCAT(update_note, ',') from item_tracker_updates where item_tracker.item_id=item_tracker_updates.item_id ) comments,
            (select project_status from projects p where p.project_id=item_tracker.project_id) project_status,
            (select c.client_id from projects c where c.project_id=project_id) client_id,
            (select project_name from projects p where p.project_id=item_tracker.project_id) project_name,
            (select project_number from projects p where p.project_id=item_tracker.project_id) project_number
            FROM item_tracker;
        )");


        global_DBObjects.execute(R"(
            CREATE TABLE tmp_meeting_attendees as select * from meeting_attendees;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE meeting_attendees;
        )");

        global_DBObjects.execute(R"(
            CREATE TABLE meeting_attendees(
                attendee_id TEXT PRIMARY KEY
                UNIQUE
                NOT NULL,
                note_id     TEXT,
                person_id   TEXT,
                UNIQUE("note_id","person_id")
            );
        )");

        global_DBObjects.execute(R"(
            CREATE INDEX meeting_attend_note on meeting_attendees (note_id);
        )");

        // clean up duplicate attendees
        global_DBObjects.execute(R"(
            insert into meeting_attendees(
                attendee_id,
                note_id,
                person_id
            )
            select
                min(attendee_id) attendee_id,
                note_id,
                person_id
            from tmp_meeting_attendees
            where person_id is not null
            group by note_id, person_id;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE tmp_meeting_attendees;
        )");

        global_DBObjects.execute(R"(
            CREATE INDEX person_name on people (name);
        )");

        global_DBObjects.execute(R"(
            CREATE TABLE tmp_project_locations as select * from project_locations;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE project_locations;
        )");

        global_DBObjects.execute(R"(
            CREATE TABLE project_locations(
                location_id          TEXT PRIMARY KEY
                NOT NULL
                UNIQUE,
                project_id           TEXT,
                location_type        TEXT,
                location_description TEXT,
                full_path            TEXT,
                UNIQUE("project_id","location_id"),
                UNIQUE("project_id","location_description")
            );
        )");

        global_DBObjects.execute(R"(
            CREATE INDEX location_project on project_locations (project_id);
        )");

        global_DBObjects.execute(R"(
            insert into project_locations(
                location_id,
                project_id,
                location_type,
                location_description,
                full_path
            )
            select
                location_id,
                project_id,
                location_type,
                location_description,
                full_path
            from tmp_project_locations;
        )");


        global_DBObjects.execute(R"(
            DROP TABLE tmp_project_locations;
        )");

        global_DBObjects.execute(R"(
            CREATE TABLE tmp_project_people AS select * from project_people;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE project_people;
        )");

        global_DBObjects.execute(R"(
            CREATE TABLE project_people (
                teammember_id         TEXT    PRIMARY KEY
                                              UNIQUE
                                              NOT NULL,
                people_id             TEXT    NOT NULL,
                project_id            TEXT    NOT NULL,
                role                  TEXT,
                receive_status_report INTEGER,
                UNIQUE("project_id","people_id")
            );
        )");

        global_DBObjects.execute(R"(
            insert into project_people (
                teammember_id,
                people_id,
                project_id,
                role,
                receive_status_report
            )
            select
                teammember_id,
                people_id,
                project_id,
                role,
                receive_status_report
            from tmp_project_people;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE tmp_project_people;
        )");

        global_DBObjects.execute(R"(
            CREATE INDEX note_project on project_notes (project_id);
        )");

        global_DBObjects.execute(R"(
            CREATE INDEX note_internal on project_notes (internal_item);
        )");

        global_DBObjects.execute(R"(
            CREATE INDEX proj_people_project on project_people (project_id);
        )");

        global_DBObjects.execute(R"(
            CREATE INDEX proj_status on projects (project_status);
        )");

        global_DBObjects.execute(R"(
            CREATE TABLE tmp_status_report_items AS select * from status_report_items;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE status_report_items;
        )");

        global_DBObjects.execute(R"(
            CREATE TABLE status_report_items (
                status_item_id   TEXT UNIQUE
                                      PRIMARY KEY
                                      NOT NULL,
                project_id       TEXT NOT NULL,
                task_category    TEXT,
                task_description TEXT NOT NULL,
                UNIQUE("task_description","project_id")
            );
        )");

        global_DBObjects.execute(R"(
            CREATE INDEX stat_project on status_report_items (project_id);
        )");

        global_DBObjects.execute(R"(
            insert into status_report_items (
                status_item_id,
                project_id,
                task_category,
                task_description
            )
            select
                status_item_id,
                project_id,
                task_category,
                task_description
            from tmp_status_report_items;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE tmp_status_report_items;
        )");
}
