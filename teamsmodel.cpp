#include "teamsmodel.h"

TeamsModel::TeamsModel(PNDatabaseObjects* t_dbo, bool t_gui): PNSqlQueryModel(t_dbo, t_gui)
{
    setObjectName("TeamsModel");
    setOrderKey(15);

    setBaseSql("SELECT teammember_id, b.name, project_id, a.people_id, b.client_id FROM project_people a join people b on a.people_id=b.people_id ");

    setTableName("project_people", "Project People");

    addColumn(0, tr("Team Member ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly);
    addColumn(1, tr("Name"), DBString, DBSearchable, DBRequired);
    addColumn(2, tr("Project ID"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "projects", "project_id", "project_number");
    addColumn(3, tr("People ID"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "people", "people_id", "name");
    addColumn(4, tr("Client ID"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique,
              "clients", "client_id", "client_name");

    QStringList key1 = {"project_id", "people_id"};

    addUniqueKeys(key1, "Name");

    QStringList rel_col4 = { "project_id", "people_id" };
    QStringList rel_fk4 = { "project_id", "primary_contact" };
    addRelatedTable("projects", rel_fk4, rel_col4, "Primary Contact");

    QStringList rel_col1 = { "project_id", "people_id" };
    QStringList rel_fk1 = { "project_id", "identified_by" };
    addRelatedTable("item_tracker", rel_fk1, rel_col1, "Identified By");

    QStringList rel_col2 = { "project_id", "people_id" };
    QStringList rel_fk2 = { "project_id", "assigned_to" };
    addRelatedTable("item_tracker", rel_fk2, rel_col2, "Assigned To");

    QStringList rel_col3 = { "project_id", "people_id" };
    QStringList rel_fk3 = { "project_id", "person_id" };
    addRelatedTable("meeting_attendees", rel_fk3, rel_col3, "Attendee");

    setOrderBy("name");

    setShowBlank(); // add a blank line to the selection
    setNoExport();  // this table is only for drop downs don't export it
}
