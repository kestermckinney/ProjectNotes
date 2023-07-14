#include "peoplemodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>

PeopleModel::PeopleModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("PeopleModel");

    setBaseSql("SELECT people_id, name, email, office_phone, cell_phone, client_id, role FROM people");

    setTableName("people", "People");

    addColumn(0, tr("People ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Name"), DBString, DBSearchable, DBRequired, DBEditable, DBUnique);
    addColumn(2, tr("Email"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(3, tr("Office Phone"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(4, tr("Cell Phone"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn(5, tr("Client"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
            "clients", "client_id", "client_name");
    addColumn(6, tr("Role"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);

    addRelatedTable("item_tracker", "assigned_to", "people_id", "Assigned Item");
    addRelatedTable("item_tracker", "identified_by", "people_id", "Identified By Item");
    addRelatedTable("item_tracker_updates", "updated_by", "people_id", "Item Updated By");
    addRelatedTable("meeting_attendees", "person_id", "people_id", "Meeting Attendee");
    addRelatedTable("project_people", "people_id", "people_id", "Project Team");
    addRelatedTable("projects", "primary_contact", "people_id", "Project Primary Contact");

    QStringList key1 = {"name"};

    addUniqueKeys(key1, "Name");

    setOrderBy("name");
}
