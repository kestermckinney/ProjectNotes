#include "peoplemodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>

PeopleModel::PeopleModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("PeopleModel");

    setBaseSql("SELECT people_id, name, email, office_phone, cell_phone, client_id, t_role FROM people");

    setTableName("people", "People");

    AddColumn(0, tr("People ID"), DB_STRING, false);
    AddColumn(1, tr("Name"), DB_STRING, true, true, false);
    AddColumn(2,  tr("Email"), DB_STRING, true, false, true, false);
    AddColumn(3, tr("Office Phone"), DB_STRING, true, false, true, false);
    AddColumn(4, tr("Cell Phone"), DB_STRING, true, false, true, false);
    // this should be a lookup column
    AddColumn(5, tr("Client"), DB_STRING, true, true); //clients, tr("client_name"), tr("client_id"));
    AddColumn(6, tr("Role"), DB_STRING, true, false, true, false);

    AddRelatedTable("item_tracker", "assigned_to", "Assigned Item");
    AddRelatedTable("item_tracker", "identified_by", "Identified By Item");
    AddRelatedTable("item_tracker_updates", "updated_by", "Item Updated By");
    AddRelatedTable("meeting_attendees", "person_id", "Meeting Attendee");
    AddRelatedTable("project_people", "people_id", "Project Team");
    AddRelatedTable("projects", "primary_contact", "Project Primary Contact");

    SetOrderBy("name");
}
