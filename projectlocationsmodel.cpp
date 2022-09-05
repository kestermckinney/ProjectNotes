#include "projectlocationsmodel.h"

ProjectLocationsModel::ProjectLocationsModel(QObject* t_parent): PNSqlQueryModel(t_parent)
{
    setObjectName("ProjedtLocationsModel");

    setBaseSql("SELECT location_id, project_id, location_type, location_description, full_path FROM project_locations");

    setTableName("project_locations", "Project Locations");

    addColumn(0, tr("Location ID"), DBString, DBNotSearchable, DBRequired, DBReadOnly, DBUnique);
    addColumn(1, tr("Project ID"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(2, tr("Location t_type"), DBString, DBSearchable, DBRequired, DBEditable, DBNotUnique);
    addColumn(3, tr("Description"), DBString, DBSearchable, DBNotRequired, DBEditable, DBNotUnique);

    //addRelatedTable("item_tracker", "assigned_to", "Assigned Item");

    setOrderBy("Description");
}
