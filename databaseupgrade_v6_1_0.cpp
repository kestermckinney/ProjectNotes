// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseupgrade_v6_1_0.h"
#include "databaseobjects.h"

void db_UpgradeStep_v6_1_0()
{
    // File Finder scans can legitimately re-describe a location that shares
    // its description with another row for the same project (a description
    // is a human-readable label, not an identity key). Recreate the index
    // without the UNIQUE constraint so a scan can no longer fail with
    // "UNIQUE constraint failed: project_locations.project_id,
    // project_locations.location_description".
    global_DBObjects.execute(R"(DROP INDEX IF EXISTS idx_project_locations_proj_desc;)");
    global_DBObjects.execute(R"(
        CREATE INDEX idx_project_locations_proj_desc
            ON project_locations (project_id, location_description) WHERE deleted = 0;
    )");
}
