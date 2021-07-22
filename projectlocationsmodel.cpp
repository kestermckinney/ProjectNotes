#include "projectlocationsmodel.h"

ProjectLocationsModel::ProjectLocationsModel(QObject* parent): PNSqlQueryModel(parent)
{
    setBaseSql("SELECT location_id, project_id, location_type, location_description, full_path FROM project_locations");

    setTableName("project_locations", "Project Locations");

    AddColumn(0, tr("Location ID"), DB_STRING, false, true, true, true);
    AddColumn(1, tr("Project ID"), DB_STRING, true, true, true, false);
    AddColumn(2, tr("Location Type"), DB_STRING, true, true, true, false);
    AddColumn(3, tr("Description"), DB_STRING, true, false, true, false);

    //AddRelatedTable("item_tracker", "assigned_to", "Assigned Item");

    SetOrderBy("Description");
}
