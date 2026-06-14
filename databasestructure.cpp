// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include <version.h>
#include <QUuid>
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

    // Insert version with a GUID id
    QString versionGuid = QUuid::createUuid().toString();
    QString versionString = QString("%1.%2.%3")
        .arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH);

    global_DBObjects.execute(QString("INSERT INTO application_version (id, current_version) VALUES('%1', '%2');")
        .arg(versionGuid, versionString));

    return true;
}

bool DatabaseStructure::UpgradeDatabase()
{
    // Read current version from application_version table
    QString currentversion = global_DBObjects.execute("select current_version from application_version");

    if (currentversion.isEmpty())
        return false;

    QString targetversion = QString("%1.%2.%3")
        .arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH);

    // Always drop and recreate views, even when the schema version is unchanged.
    // Views are derived objects with no data, so rebuilding them on every launch
    // is cheap and guarantees they match the current code. Without this a view
    // definition fix would never reach a database already on the target version.
    db_DropAllViews();

    if (currentversion != targetversion)
    {
        // Run applicable upgrade steps in version order
        if (currentversion == "1.0.0")
            db_UpgradeStep_v1_0_0();

        if (currentversion == "1.2.0" || currentversion == "1.0.0")
            db_UpgradeStep_v1_2_0();

        if (currentversion == "4.1.0" || currentversion == "1.2.0" || currentversion == "1.0.0")
            db_UpgradeStep_v5_0_0();

        // Update version to target (only update current_version, never modify id)
        global_DBObjects.execute(QString("update application_version set current_version = '%1';")
            .arg(targetversion));
    }

    // Recreate all views as final step (always uses current column names)
    db_CreateAllViews();

    return true;
}
