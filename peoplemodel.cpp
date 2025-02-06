// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "peoplemodel.h"
#include "pndatabaseobjects.h"

#include <QRegularExpression>

PeopleModel::PeopleModel(PNDatabaseObjects* t_dbo, bool t_gui): PNSqlQueryModel(t_dbo, t_gui)
{
    setObjectName("PeopleModel");
    setOrderKey(17);

    setBaseSql("SELECT people_id, name, email, office_phone, cell_phone, client_id, role FROM people");

    setTableName("people", "People");

    addColumn("people_id", tr("People ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn("name", tr("Name"), DBString, DBSearchable, DBRequired, DBEditable, DBUnique);
    addColumn("email", tr("Email"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("office_phone", tr("Office Phone"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("cell_phone", tr("Cell Phone"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);
    addColumn("client_id", tr("Client"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique,
            "clients", "client_id", "client_name");
    addColumn("role", tr("Role"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);

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
