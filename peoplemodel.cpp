#include "peoplemodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>

PeopleModel::PeopleModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("PeopleModel");

    setBaseSql("SELECT people_id, name, email, office_phone, cell_phone, client_id, role FROM people");

    setTableName("people", "People");

    addColumn(0, tr("People ID"), DB_STRING, false);
    addColumn(1, tr("Name"), DB_STRING, true, true, false);
    addColumn(2,  tr("Email"), DB_STRING, true, false, true, false);
    addColumn(3, tr("Office Phone"), DB_STRING, true, false, true, false);
    addColumn(4, tr("Cell Phone"), DB_STRING, true, false, true, false);
    // this should be a lookup column
    addColumn(5, tr("Client"), DB_STRING, true, true); //clients, tr("client_name"), tr("client_id"));
    addColumn(6, tr("Role"), DB_STRING, true, false, true, false);

    addRelatedTable("item_tracker", "assigned_to", "Assigned Item");
    addRelatedTable("item_tracker", "identified_by", "Identified By Item");
    addRelatedTable("item_tracker_updates", "updated_by", "Item Updated By");
    addRelatedTable("meeting_attendees", "person_id", "Meeting Attendee");
    addRelatedTable("project_people", "people_id", "Project Team");
    addRelatedTable("projects", "primary_contact", "Project Primary Contact");

    setOrderBy("name");
}
