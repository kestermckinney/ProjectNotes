#include "projectlocationsmodel.h"

ProjectLocationsModel::ProjectLocationsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjedtLocationsModel");

    setBaseSql("SELECT location_id, project_id, location_type, location_description, full_path FROM project_locations");

    setTableName("project_locations", "Project Locations");

    addColumn(0, tr("Location ID"), DB_STRING, false, true, true, true);
    addColumn(1, tr("Project ID"), DB_STRING, true, true, true, false);
    addColumn(2, tr("Location t_type"), DB_STRING, true, true, true, false);
    addColumn(3, tr("Description"), DB_STRING, true, false, true, false);

    //addRelatedTable("item_tracker", "assigned_to", "Assigned Item");

    setOrderBy("Description");
}
