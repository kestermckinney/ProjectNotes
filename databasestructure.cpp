#include "databasestructure.h"
#include "mainwindow.h"
#include "pndatabaseobjects.h"

bool DatabaseStructure::CreateDatabase()
{

    global_DBObjects.execute(R"(
        CREATE TABLE application_settings(
            parameter_id    TEXT PRIMARY KEY
            UNIQUE
            NOT NULL,
            parameter_name  TEXT,
            parameter_value TEXT
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX app_set_name on application_settings (parameter_name);
    )");

    global_DBObjects.execute(R"(
        CREATE TABLE application_version(
            current_version TEXT PRIMARY KEY
            UNIQUE
            NOT NULL
        );
    )");

    global_DBObjects.execute(R"(
        CREATE TABLE clients (
            client_id   TEXT PRIMARY KEY
                             UNIQUE
                             NOT NULL,
            client_name TEXT UNIQUE
                             NOT NULL
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX client_name on clients (client_name);
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
        CREATE TABLE item_tracker_updates (
            tracker_updated_id TEXT    PRIMARY KEY
                                       UNIQUE
                                       NOT NULL,
            item_id            TEXT    NOT NULL,
            lastupdated_date   INTEGER,
            update_note        TEXT,
            updated_by         TEXT
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX traker_update_item on item_tracker_updates (item_id);
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

    global_DBObjects.execute(R"(
        CREATE TABLE people (
            people_id    TEXT NOT NULL
                              PRIMARY KEY
                              UNIQUE,
            name         TEXT NOT NULL,
            email        TEXT,
            office_phone TEXT,
            cell_phone   TEXT,
            client_id    TEXT,
            role         TEXT,
            UNIQUE (
                name ASC
            )
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX person_name on people (name);
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
        CREATE TABLE project_notes (
            note_id       TEXT    PRIMARY KEY
                                  UNIQUE
                                  NOT NULL,
            project_id    TEXT,
            note_title    TEXT    NOT NULL,
            note_date     INTEGER NOT NULL,
            note          TEXT,
            internal_item INTEGER
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX note_project on project_notes (project_id);
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX note_internal on project_notes (internal_item);
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
        CREATE INDEX proj_people_project on project_people (project_id);
    )");

    global_DBObjects.execute(R"(
        CREATE TABLE project_risks (
            risk_id               TEXT    PRIMARY KEY
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
            risk_response_actions TEXT
        );
    )");

    global_DBObjects.execute(R"(
        CREATE TABLE projects (
            project_id           TEXT    PRIMARY KEY
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
            UNIQUE (
                project_number ASC
            )
        );
    )");

    global_DBObjects.execute(R"(
        CREATE INDEX proj_status on projects (project_status);
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

    QString search_view = R"(
        CREATE VIEW database_search AS select 'Client' as datatype, 'Client Name' as dataname, client_name as datadescription, client_id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, client_id as datakey from clients
        -- list all the people data
        union all
        select 'People' as datatype, 'Person Name' as dataname, name as datadescription, people_id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, people_id as datakey from people
        union all
        select 'People' as datatype, 'Person Email' as dataname, email as datadescription, people_id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from people
        union all
        select 'People' as datatype, 'Person Office Phone' as dataname, office_phone as datadescription, people_id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from people
        union all
        select 'People' as datatype, 'Person Cell Phone' as dataname, cell_phone as datadescription, people_id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from people
        union all
        select 'People' as datatype, 'Person Client Name' as dataname, clients.client_name as datadescription, people_id as dataid, '0' as internal_item, people.client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, people.client_id as fk_id, people.client_id as datakey from people join clients on clients.client_id=people.client_id
        -- list all project data
        union all
        select 'Project' as datatype, 'Project Number' as dataname, project_number as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, project_id as datakey from projects
        union all
        select 'Project' as datatype, 'Project Name' as dataname, project_name as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, project_id as datakey from projects
        union all
        select 'Project' as datatype, 'Last Status Date' as dataname, strftime('%m/%d/%Y', datetime(last_status_date, 'unixepoch')) as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey  from projects
        union all
        select 'Project' as datatype, 'Last Invoice Date' as dataname, strftime('%m/%d/%Y', datetime(last_invoice_date, 'unixepoch')) as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey  from projects
        union all
        select 'Project' as datatype, 'Primary Contact' as dataname, people.name as datadescription, project_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, people.people_id as fk_id, primary_contact as datakey  from projects join people on primary_contact=people_id
        union all
        select 'Project' as datatype, 'Invoicing Period' as dataname, invoicing_period as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey  from projects
        union all
        select 'Project' as datatype, 'Client' as dataname, clients.client_name as datadescription, project_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.client_id as fk_id, projects.client_id as datakey  from projects join clients on clients.client_id=projects.client_id
        union all
        select 'Project' as datatype, 'Project Status' as dataname, project_status as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from projects
        -- list all meeting notes
        union all
        select 'Project Notes' as datatype, 'Project Number' as dataname, project_number as datadescription, note_id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.project_id as fk_id, '' as datakey from project_notes join projects on project_notes.project_id=projects.project_id
        union all
        select 'Project Notes' as datatype, 'Title' as dataname, note_title as datadescription, note_id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.project_id as fk_id, '' as datakey  from project_notes join projects on project_notes.project_id=projects.project_id
        union all
        select 'Project Notes' as datatype, 'Date' as dataname, strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) as datadescription, note_id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.project_id as fk_id, '' as datakey  from project_notes join projects on project_notes.project_id=projects.project_id
        union all
        select 'Project Notes' as datatype, 'Note' as dataname, note as datadescription, note_id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.project_id as fk_id, '' as datakey  from project_notes join projects on project_notes.project_id=projects.project_id
        -- list all meeting attendees
        union all
        select 'Meeting Attendees' as datatype, 'Attendee' as dataname, people.name as datadescription, attendee_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, project_notes.note_id as fk_id, people.people_id as datakey from meeting_attendees join people on meeting_attendees.person_id=people.people_id join project_notes on project_notes.note_id=meeting_attendees.note_id join projects on projects.project_id=project_notes.project_id
        -- list all project locations
        union all
        select 'Project Locations' as datatype, 'Project Number' as dataname, projects.project_number as datadescription, location_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.project_id
        union all
        select 'Project Locations' as datatype, 'Location Type' as dataname, location_type as datadescription, location_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.project_id
        union all
        select 'Project Locations' as datatype, 'Description' as dataname, location_description as datadescription, location_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.project_id
        union all)";

    search_view += R"(
        select 'Project Locations' as datatype, 'Full Path' as dataname, full_path as datadescription, location_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.project_id
        -- list all project team members
        union all
        select 'Project Team' as datatype, 'Member Name' as dataname, people.name as datadescription, teammember_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, people.people_id as datakey from project_people join people on project_people.people_id=people.people_id join projects on project_people.project_id=projects.project_id
        union all
        select 'Project Team' as datatype, 'Role' as dataname, role as datadescription, teammember_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from project_people join projects on project_people.project_id=projects.project_id
        -- list all status report items
        union all
        select 'Status Report Item' as datatype, 'Project Number' as dataname, project_number as datadescription, status_item_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, projects.project_id as datakey from status_report_items left join projects on status_report_items.project_id=projects.project_id
        union all
        select 'Status Report Item' as datatype, 'Category' as dataname, task_category as datadescription, status_item_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from status_report_items left join projects on status_report_items.project_id=projects.project_id
        union all
        select 'Status Report Item' as datatype, 'Description' as dataname, task_description as datadescription, status_item_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from status_report_items left join projects on status_report_items.project_id=projects.project_id
        -- list all item tracker
        union all
        select 'Item Tracker' as datatype, 'Item Number' as dataname, item_number as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Item Type' as dataname, item_type as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Item Name' as dataname, item_name as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Identified By' as dataname, people.name as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, people.people_id as datakey from item_tracker join people on item_tracker.identified_by=people.people_id left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Date Identified' as dataname, strftime('%m/%d/%Y', datetime(date_identified, 'unixepoch')) as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Item Number' as dataname, item_number as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Item Type' as dataname, item_type as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Item Name' as dataname, item_name as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Identified By' as dataname, people.name as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, people.people_id as datakey from item_tracker join people on item_tracker.identified_by=people.people_id left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Date Identified' as dataname, strftime('%m/%d/%Y', datetime(date_identified, 'unixepoch')) as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Description' as dataname, description as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Assigned To' as dataname, people.name as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, people.people_id as datakey from item_tracker join people on item_tracker.assigned_to=people.people_id left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Priority' as dataname, priority as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Status' as dataname, status as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Due Date' as dataname, strftime('%m/%d/%Y', datetime(date_due, 'unixepoch')) as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Last Updated' as dataname, strftime('%m/%d/%Y', datetime(last_update, 'unixepoch')) as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Date Resolved' as dataname, strftime('%m/%d/%Y', datetime(date_resolved, 'unixepoch')) as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Item Tracker' as datatype, 'Project Number' as dataname, projects.project_number as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, projects.project_id as datakey from item_tracker join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
        union all
        select 'Tracker Update' as datatype, 'Comments' as dataname, item_tracker_updates.update_note as datadescription, tracker_updated_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(lastupdated_date, 'unixepoch')) as note_date, note_title, item_tracker.project_id as fk_id, item_tracker.item_id as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id left join item_tracker_updates on item_tracker.item_id=item_tracker_updates.item_id
        )";

    global_DBObjects.execute(search_view);

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
            (select c.client_id from projects c where c.project_id=project_id) client_id
        FROM item_tracker
        )");

    global_DBObjects.execute(R"(
            CREATE VIEW projects_view AS SELECT
                project_id,
                project_number,
                project_name,
                last_status_date,
                last_invoice_date,
                primary_contact,
                budget,
                actual,
                bcwp,
                bcws,
                bac,
                invoicing_period,
                status_report_period,
                client_id,
                project_status,
                (case when budget > 0 then round((actual / budget) * 100.0, 2) else NULL end) pct_consumed,
                (case when actual > 0 and bcws > 0 then round(actual + (bac - bcwp) / (bcwp/actual*bcwp/bcws), 2) else NULL end) eac,
                (case when bcwp > 0 then round((actual -  bcwp) / bcwp * 100.0, 2) else NULL end) cv,
                (case when bcws > 0 then round((bcwp -  bcws) / bcws * 100.0, 2) else NULL end) sv,
                (case when bac > 0 then round(bcwp / bac * 100.0, 2) else NULL end) pct_complete,
                (case when actual > 0 then round(bcwp / actual, 2) else NULL end) cpi
            FROM projects
    )");

    global_DBObjects.execute(QString("INSERT INTO application_version ( current_version ) VALUES( '%1.%2.%3' ); ").arg(PNMajorVersion).arg(PNMinorVersion).arg(PNFixVersion));

    return true;
}

bool DatabaseStructure::UpgradeDatabase()
{

    QString currentversion = global_DBObjects.execute("select current_version from application_version");

    if (currentversion == "1.0.0")
    {
        global_DBObjects.execute(R"(
            CREATE TABLE sqlitestudio_temp_table AS SELECT*
                FROM people;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE people;
        )");

        global_DBObjects.execute(R"(
            CREATE TABLE people(
                people_id    TEXT NOT NULL
                PRIMARY KEY
                UNIQUE,
                name         TEXT NOT NULL,
                email        TEXT,
                office_phone TEXT,
                cell_phone   TEXT,
                client_id    TEXT,
                role         TEXT,
                UNIQUE(
                    name ASC
                )
            );
        )");

        global_DBObjects.execute(R"(
            INSERT INTO people(
                people_id,
                name,
                email,
                office_phone,
                cell_phone,
                client_id
            )
                SELECT people_id,
                name,
                email,
                office_phone,
                cell_phone,
                client_id
                FROM sqlitestudio_temp_table;
        )");

        global_DBObjects.execute(R"(
            DROP TABLE sqlitestudio_temp_table;
        )");
    }

    if (currentversion == "1.2.0" || currentversion == "1.0.0")
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
            (select c.client_id from projects c where c.project_id=project_id) client_id
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

        global_DBObjects.execute(R"(
            DROP VIEW IF EXISTS database_search;
        )");

        QString search_view = R"(
            CREATE VIEW database_search AS select 'Client' as datatype, 'Client Name' as dataname, client_name as datadescription, client_id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, client_id as datakey from clients
            -- list all the people data
            union all
            select 'People' as datatype, 'Person Name' as dataname, name as datadescription, people_id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, people_id as datakey from people
            union all
            select 'People' as datatype, 'Person Email' as dataname, email as datadescription, people_id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from people
            union all
            select 'People' as datatype, 'Person Office Phone' as dataname, office_phone as datadescription, people_id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from people
            union all
            select 'People' as datatype, 'Person Cell Phone' as dataname, cell_phone as datadescription, people_id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from people
            union all
            select 'People' as datatype, 'Person Client Name' as dataname, clients.client_name as datadescription, people_id as dataid, '0' as internal_item, people.client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, people.client_id as fk_id, people.client_id as datakey from people join clients on clients.client_id=people.client_id
            -- list all project data
            union all
            select 'Project' as datatype, 'Project Number' as dataname, project_number as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, project_id as datakey from projects
            union all
            select 'Project' as datatype, 'Project Name' as dataname, project_name as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, project_id as datakey from projects
            union all
            select 'Project' as datatype, 'Last Status Date' as dataname, strftime('%m/%d/%Y', datetime(last_status_date, 'unixepoch')) as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey  from projects
            union all
            select 'Project' as datatype, 'Last Invoice Date' as dataname, strftime('%m/%d/%Y', datetime(last_invoice_date, 'unixepoch')) as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey  from projects
            union all
            select 'Project' as datatype, 'Primary Contact' as dataname, people.name as datadescription, project_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, people.people_id as fk_id, primary_contact as datakey  from projects join people on primary_contact=people_id
            union all
            select 'Project' as datatype, 'Invoicing Period' as dataname, invoicing_period as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey  from projects
            union all
            select 'Project' as datatype, 'Client' as dataname, clients.client_name as datadescription, project_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.client_id as fk_id, projects.client_id as datakey  from projects join clients on clients.client_id=projects.client_id
            union all
            select 'Project' as datatype, 'Project Status' as dataname, project_status as datadescription, project_id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from projects
            -- list all meeting notes
            union all
            select 'Project Notes' as datatype, 'Project Number' as dataname, project_number as datadescription, note_id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.project_id as fk_id, '' as datakey from project_notes join projects on project_notes.project_id=projects.project_id
            union all
            select 'Project Notes' as datatype, 'Title' as dataname, note_title as datadescription, note_id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.project_id as fk_id, '' as datakey  from project_notes join projects on project_notes.project_id=projects.project_id
            union all
            select 'Project Notes' as datatype, 'Date' as dataname, strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) as datadescription, note_id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.project_id as fk_id, '' as datakey  from project_notes join projects on project_notes.project_id=projects.project_id
            union all
            select 'Project Notes' as datatype, 'Note' as dataname, note as datadescription, note_id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.project_id as fk_id, '' as datakey  from project_notes join projects on project_notes.project_id=projects.project_id
            -- list all meeting attendees
            union all
            select 'Meeting Attendees' as datatype, 'Attendee' as dataname, people.name as datadescription, attendee_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, project_notes.note_id as fk_id, people.people_id as datakey from meeting_attendees join people on meeting_attendees.person_id=people.people_id join project_notes on project_notes.note_id=meeting_attendees.note_id join projects on projects.project_id=project_notes.project_id
            -- list all project locations
            union all
            select 'Project Locations' as datatype, 'Project Number' as dataname, projects.project_number as datadescription, location_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.project_id
            union all
            select 'Project Locations' as datatype, 'Location Type' as dataname, location_type as datadescription, location_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.project_id
            union all
            select 'Project Locations' as datatype, 'Description' as dataname, location_description as datadescription, location_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.project_id
            union all)";

        search_view += R"(
            select 'Project Locations' as datatype, 'Full Path' as dataname, full_path as datadescription, location_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.project_id
            -- list all project team members
            union all
            select 'Project Team' as datatype, 'Member Name' as dataname, people.name as datadescription, teammember_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, people.people_id as datakey from project_people join people on project_people.people_id=people.people_id join projects on project_people.project_id=projects.project_id
            union all
            select 'Project Team' as datatype, 'Role' as dataname, role as datadescription, teammember_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from project_people join projects on project_people.project_id=projects.project_id
            -- list all status report items
            union all
            select 'Status Report Item' as datatype, 'Project Number' as dataname, project_number as datadescription, status_item_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, projects.project_id as datakey from status_report_items left join projects on status_report_items.project_id=projects.project_id
            union all
            select 'Status Report Item' as datatype, 'Category' as dataname, task_category as datadescription, status_item_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from status_report_items left join projects on status_report_items.project_id=projects.project_id
            union all
            select 'Status Report Item' as datatype, 'Description' as dataname, task_description as datadescription, status_item_id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.project_id as fk_id, '' as datakey from status_report_items left join projects on status_report_items.project_id=projects.project_id
            -- list all item tracker
            union all
            select 'Item Tracker' as datatype, 'Item Number' as dataname, item_number as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Item Type' as dataname, item_type as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Item Name' as dataname, item_name as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Identified By' as dataname, people.name as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, people.people_id as datakey from item_tracker join people on item_tracker.identified_by=people.people_id left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Date Identified' as dataname, strftime('%m/%d/%Y', datetime(date_identified, 'unixepoch')) as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Item Number' as dataname, item_number as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Item Type' as dataname, item_type as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Item Name' as dataname, item_name as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Identified By' as dataname, people.name as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, people.people_id as datakey from item_tracker join people on item_tracker.identified_by=people.people_id left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Date Identified' as dataname, strftime('%m/%d/%Y', datetime(date_identified, 'unixepoch')) as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Description' as dataname, description as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Assigned To' as dataname, people.name as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, people.people_id as datakey from item_tracker join people on item_tracker.assigned_to=people.people_id left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Priority' as dataname, priority as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Status' as dataname, status as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Due Date' as dataname, strftime('%m/%d/%Y', datetime(date_due, 'unixepoch')) as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Last Updated' as dataname, strftime('%m/%d/%Y', datetime(last_update, 'unixepoch')) as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Date Resolved' as dataname, strftime('%m/%d/%Y', datetime(date_resolved, 'unixepoch')) as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Item Tracker' as datatype, 'Project Number' as dataname, projects.project_number as datadescription, item_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.project_id as fk_id, projects.project_id as datakey from item_tracker join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id
            union all
            select 'Tracker Update' as datatype, 'Comments' as dataname, item_tracker_updates.update_note as datadescription, tracker_updated_id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(lastupdated_date, 'unixepoch')) as note_date, note_title, item_tracker.project_id as fk_id, item_tracker.item_id as datakey from item_tracker left join projects on item_tracker.project_id=projects.project_id left join project_notes on project_notes.note_id=item_tracker.note_id left join item_tracker_updates on item_tracker.item_id=item_tracker_updates.item_id
            )";

        global_DBObjects.execute(search_view);

        global_DBObjects.execute(R"(
            CREATE VIEW projects_view AS SELECT
                project_id,
                project_number,
                project_name,
                last_status_date,
                last_invoice_date,
                primary_contact,
                budget,
                actual,
                bcwp,
                bcws,
                bac,
                invoicing_period,
                status_report_period,
                client_id,
                project_status,
                (case when budget > 0 then round((actual / budget) * 100.0, 2) else NULL end) pct_consumed,
                (case when actual > 0 and bcws > 0 then round(actual + (bac - bcwp) / (bcwp/actual*bcwp/bcws), 2) else NULL end) eac,
                (case when bcwp > 0 then round((actual -  bcwp) / bcwp * 100.0, 2) else NULL end) cv,
                (case when bcws > 0 then round((bcwp -  bcws) / bcws * 100.0, 2) else NULL end) sv,
                (case when bac > 0 then round(bcwp / bac * 100.0, 2) else NULL end) pct_complete,
                (case when actual > 0 then round(bcwp / actual, 2) else NULL end) cpi
            FROM projects
        )");
    }

    global_DBObjects.execute(QString("update application_version set current_version = '%1.%2.%3' where current_version = '%4'; ").arg(PNMajorVersion).arg(PNMinorVersion).arg(PNFixVersion).arg(currentversion));

    return true;
}
