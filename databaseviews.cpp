// Copyright (C) 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseviews.h"
#include "databaseobjects.h"

void db_DropAllViews()
{
    global_DBObjects.execute("DROP VIEW IF EXISTS database_search;");
    global_DBObjects.execute("DROP VIEW IF EXISTS item_tracker_view;");
    global_DBObjects.execute("DROP VIEW IF EXISTS projects_view;");
}

void db_CreateAllViews()
{
    // database_search view - comprehensive union of all searchable content
    // Each branch filters out soft-deleted rows from its primary table.
    QString search_view_a = R"(
        CREATE VIEW database_search AS
        select 'Client' as datatype, 'Client Name' as dataname, client_name as datadescription, id as dataid, '0' as internal_item, id as client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, id as datakey from clients WHERE (clients.deleted IS NULL OR clients.deleted = 0)
        -- list all the people data
        union all
        select 'People' as datatype, 'Person Name' as dataname, name as datadescription, id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, id as datakey from people WHERE (people.deleted IS NULL OR people.deleted = 0)
        union all
        select 'People' as datatype, 'Person Email' as dataname, email as datadescription, id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from people WHERE (people.deleted IS NULL OR people.deleted = 0)
        union all
        select 'People' as datatype, 'Person Office Phone' as dataname, office_phone as datadescription, id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from people WHERE (people.deleted IS NULL OR people.deleted = 0)
        union all
        select 'People' as datatype, 'Person Cell Phone' as dataname, cell_phone as datadescription, id as dataid, '0' as internal_item, client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from people WHERE (people.deleted IS NULL OR people.deleted = 0)
        union all
        select 'People' as datatype, 'Person Client Name' as dataname, clients.client_name as datadescription, people.id as dataid, '0' as internal_item, people.client_id, 'Active' as project_status, '' as project_number, '' as project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, people.client_id as fk_id, people.client_id as datakey from people join clients on clients.id=people.client_id WHERE (people.deleted IS NULL OR people.deleted = 0)
        -- list all project data
        union all
        select 'Project' as datatype, 'Project Number' as dataname, project_number as datadescription, id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, id as datakey from projects WHERE (projects.deleted IS NULL OR projects.deleted = 0)
        union all
        select 'Project' as datatype, 'Project Name' as dataname, project_name as datadescription, id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, id as datakey from projects WHERE (projects.deleted IS NULL OR projects.deleted = 0)
        union all
        select 'Project' as datatype, 'Last Status Date' as dataname, strftime('%m/%d/%Y', datetime(last_status_date, 'unixepoch')) as datadescription, id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey  from projects WHERE (projects.deleted IS NULL OR projects.deleted = 0)
        union all
        select 'Project' as datatype, 'Last Invoice Date' as dataname, strftime('%m/%d/%Y', datetime(last_invoice_date, 'unixepoch')) as datadescription, id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey  from projects WHERE (projects.deleted IS NULL OR projects.deleted = 0)
        union all
        select 'Project' as datatype, 'Primary Contact' as dataname, people.name as datadescription, projects.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, people.id as fk_id, primary_contact as datakey  from projects join people on primary_contact=people.id WHERE (projects.deleted IS NULL OR projects.deleted = 0) AND (people.deleted IS NULL OR people.deleted = 0)
        union all
        select 'Project' as datatype, 'Invoicing Period' as dataname, invoicing_period as datadescription, id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey  from projects WHERE (projects.deleted IS NULL OR projects.deleted = 0)
        union all
        select 'Project' as datatype, 'Client' as dataname, clients.client_name as datadescription, projects.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.client_id as fk_id, projects.client_id as datakey  from projects join clients on clients.id=projects.client_id WHERE (projects.deleted IS NULL OR projects.deleted = 0)
        union all
        select 'Project' as datatype, 'Project Status' as dataname, project_status as datadescription, id as dataid, '0' as internal_item, client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, '' as fk_id, '' as datakey from projects WHERE (projects.deleted IS NULL OR projects.deleted = 0)
        -- list all meeting notes
        union all
        select 'Project Notes' as datatype, 'Project Number' as dataname, project_number as datadescription, project_notes.id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.id as fk_id, '' as datakey from project_notes join projects on project_notes.project_id=projects.id WHERE (project_notes.deleted IS NULL OR project_notes.deleted = 0)
        union all
        select 'Project Notes' as datatype, 'Title' as dataname, note_title as datadescription, project_notes.id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.id as fk_id, '' as datakey  from project_notes join projects on project_notes.project_id=projects.id WHERE (project_notes.deleted IS NULL OR project_notes.deleted = 0)
        union all
        select 'Project Notes' as datatype, 'Date' as dataname, strftime('%m/%d/%Y', datetime(note_date, 'unixepoch')) as datadescription, project_notes.id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.id as fk_id, '' as datakey  from project_notes join projects on project_notes.project_id=projects.id WHERE (project_notes.deleted IS NULL OR project_notes.deleted = 0)
        union all
        select 'Project Notes' as datatype, 'Note' as dataname, note as datadescription, project_notes.id as dataid, project_notes.internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, projects.id as fk_id, '' as datakey  from project_notes join projects on project_notes.project_id=projects.id WHERE (project_notes.deleted IS NULL OR project_notes.deleted = 0)
        -- list all meeting attendees
        union all
        select 'Meeting Attendees' as datatype, 'Attendee' as dataname, people.name as datadescription, meeting_attendees.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) note_date, note_title, project_notes.id as fk_id, people.id as datakey from meeting_attendees join people on meeting_attendees.person_id=people.id join project_notes on project_notes.id=meeting_attendees.note_id join projects on projects.id=project_notes.project_id WHERE (meeting_attendees.deleted IS NULL OR meeting_attendees.deleted = 0) AND (people.deleted IS NULL OR people.deleted = 0)
        -- list all project locations
        union all
        select 'Project Locations' as datatype, 'Project Number' as dataname, projects.project_number as datadescription, project_locations.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.id WHERE (project_locations.deleted IS NULL OR project_locations.deleted = 0)
        union all
        select 'Project Locations' as datatype, 'Location Type' as dataname, location_type as datadescription, project_locations.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.id WHERE (project_locations.deleted IS NULL OR project_locations.deleted = 0)
        union all
        select 'Project Locations' as datatype, 'Description' as dataname, location_description as datadescription, project_locations.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.id WHERE (project_locations.deleted IS NULL OR project_locations.deleted = 0)
        union all
        select 'Project Locations' as datatype, 'Full Path' as dataname, full_path as datadescription, project_locations.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.id as fk_id, '' as datakey from project_locations join projects on project_locations.project_id=projects.id WHERE (project_locations.deleted IS NULL OR project_locations.deleted = 0)
        -- list all project team members
        union all
        select 'Project Team' as datatype, 'Member Name' as dataname, people.name as datadescription, project_people.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.id as fk_id, people.id as datakey from project_people join people on project_people.people_id=people.id join projects on project_people.project_id=projects.id WHERE (project_people.deleted IS NULL OR project_people.deleted = 0) AND (people.deleted IS NULL OR people.deleted = 0)
        union all
        select 'Project Team' as datatype, 'Role' as dataname, role as datadescription, project_people.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.id as fk_id, '' as datakey from project_people join projects on project_people.project_id=projects.id WHERE (project_people.deleted IS NULL OR project_people.deleted = 0)
        -- list all status report items
        union all
        select 'Status Report Item' as datatype, 'Project Number' as dataname, project_number as datadescription, status_report_items.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.id as fk_id, projects.id as datakey from status_report_items left join projects on status_report_items.project_id=projects.id WHERE (status_report_items.deleted IS NULL OR status_report_items.deleted = 0)
        union all
        select 'Status Report Item' as datatype, 'Category' as dataname, task_category as datadescription, status_report_items.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.id as fk_id, '' as datakey from status_report_items left join projects on status_report_items.project_id=projects.id WHERE (status_report_items.deleted IS NULL OR status_report_items.deleted = 0)
        union all
        select 'Status Report Item' as datatype, 'Description' as dataname, task_description as datadescription, status_report_items.id as dataid, '0' as internal_item, projects.client_id, project_status, project_number, project_name, '' as item_number, '' as item_name, '' as note_date, '' as note_title, projects.id as fk_id, '' as datakey from status_report_items left join projects on status_report_items.project_id=projects.id WHERE (status_report_items.deleted IS NULL OR status_report_items.deleted = 0)
        -- list all item tracker
        union all
        select 'Item Tracker' as datatype, 'Item Number' as dataname, item_number as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        )";

    QString search_view_b = R"(
        union all
        select 'Item Tracker' as datatype, 'Item Type' as dataname, item_type as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Item Name' as dataname, item_name as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Identified By' as dataname, people.name as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, people.id as datakey from item_tracker join people on item_tracker.identified_by=people.id left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0) AND (people.deleted IS NULL OR people.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Date Identified' as dataname, strftime('%m/%d/%Y', datetime(date_identified, 'unixepoch')) as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Description' as dataname, description as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Assigned To' as dataname, people.name as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, people.id as datakey from item_tracker join people on item_tracker.assigned_to=people.id left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0) AND (people.deleted IS NULL OR people.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Priority' as dataname, priority as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Status' as dataname, status as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Due Date' as dataname, strftime('%m/%d/%Y', datetime(date_due, 'unixepoch')) as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Last Updated' as dataname, strftime('%m/%d/%Y', datetime(last_update, 'unixepoch')) as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Date Resolved' as dataname, strftime('%m/%d/%Y', datetime(date_resolved, 'unixepoch')) as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, '' as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        union all
        select 'Item Tracker' as datatype, 'Project Number' as dataname, projects.project_number as datadescription, item_tracker.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(project_notes.note_date, 'unixepoch')) as note_date, note_title, projects.id as fk_id, projects.id as datakey from item_tracker join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
        union all
        select 'Tracker Update' as datatype, 'Comments' as dataname, item_tracker_updates.update_note as datadescription, item_tracker_updates.id as dataid, item_tracker.internal_item, projects.client_id, project_status, project_number, project_name, item_number, item_name, strftime('%m/%d/%Y', datetime(lastupdated_date, 'unixepoch')) as note_date, note_title, item_tracker.project_id as fk_id, item_tracker.id as datakey from item_tracker left join projects on item_tracker.project_id=projects.id left join project_notes on project_notes.id=item_tracker.note_id left join item_tracker_updates on item_tracker.id=item_tracker_updates.item_id WHERE (item_tracker_updates.deleted IS NULL OR item_tracker_updates.deleted = 0)
        )";

    global_DBObjects.execute(search_view_a + search_view_b);

    // item_tracker_view — filters soft-deleted rows
    global_DBObjects.execute(R"(
        CREATE VIEW item_tracker_view AS SELECT
        id,
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
        (select GROUP_CONCAT(update_note, ',') from item_tracker_updates where item_tracker.id=item_tracker_updates.item_id ) comments,
        (select project_status from projects p where p.id=item_tracker.project_id) project_status,
        (select id from projects c where c.id=project_id) client_id,
        (select project_name from projects p where p.id=item_tracker.project_id) project_id_name
    FROM item_tracker
    WHERE (item_tracker.deleted IS NULL OR item_tracker.deleted = 0)
    )");

    // projects_view — filters soft-deleted rows
    global_DBObjects.execute(R"(
        CREATE VIEW projects_view AS SELECT
            id,
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
        WHERE (projects.deleted IS NULL OR projects.deleted = 0)
    )");
}
