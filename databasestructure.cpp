// Copyright (C) 2022, 2023, 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include <version.h>
#include "databasestructure.h"
#include "databaseobjects.h"
#include "databasecreate.h"
#include "databaseviews.h"
#include "databaseupgrade_v1_0_0.h"
#include "databaseupgrade_v1_2_0.h"
#include "databaseupgrade_v5_0_0.h"

bool DatabaseStructure::CreateDatabase()
{
    // Create all tables and triggers
    db_CreateNewDatabase();

    // Create all views as final step
    db_CreateAllViews();

    // Insert current version
    global_DBObjects.execute(QString("INSERT INTO application_version (id) VALUES('%1.%2.%3');")
        .arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH));

    return true;
}

bool DatabaseStructure::UpgradeDatabase()
{
    // Try the v5+ column name first; fall back to the pre-v5 name for older databases
    QString currentversion = global_DBObjects.execute("select id from application_version");
    if (currentversion.isEmpty())
        currentversion = global_DBObjects.execute("select current_version from application_version");

    if (currentversion.isEmpty())
        return false;

    QString targetversion = QString("%1.%2.%3")
        .arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH);

    if (currentversion == targetversion)
        return true;  // Already current, nothing to do

    // Drop all views before any schema changes
    db_DropAllViews();

    // Run applicable upgrade steps in version order
    if (currentversion == "1.0.0")
        db_UpgradeStep_v1_0_0();

    if (currentversion == "1.2.0" || currentversion == "1.0.0")
        db_UpgradeStep_v1_2_0();

    if (currentversion == "4.1.0" || currentversion == "1.2.0" || currentversion == "1.0.0")
        db_UpgradeStep_v5_0_0();

    // Recreate all views as final step (always uses current column names)
    db_CreateAllViews();

    // Update version to target
    global_DBObjects.execute(QString("update application_version set id = '%1' where id = '%2';")
        .arg(targetversion, currentversion));

    return true;
}
