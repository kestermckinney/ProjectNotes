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
#include "databaseupgrade_v6_1_0.h"
#include "databaseupgrade_v6_2_0.h"
#include "databaseupgrade_v6_4_0.h"
#include "databaseupgrade_v6_5_0.h"
#include <QVersionNumber>

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

    if (QVersionNumber::fromString(currentversion) > QVersionNumber::fromString(targetversion)) {
        global_DBObjects.setLastSaveError(QString("Database version %1 is newer than this application (%2).")
                                             .arg(currentversion, targetversion));
        return false;
    }

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

        if (currentversion == "6.0.0" || currentversion == "5.0.0" || currentversion == "4.1.0"
            || currentversion == "1.2.0" || currentversion == "1.0.0")
            db_UpgradeStep_v6_1_0();
    }

    // Runs on every launch, outside the version check: every statement is
    // guarded, so pre-6.2 databases receive the audience table and databases
    // created by early 6.2.0 builds pick up the shared-audience columns.
    db_UpgradeStep_v6_2_0();

    // Run cleanup even at the target version: an older synced client can
    // reintroduce retired settings. On failure, leave the version unchanged.
    if (!db_UpgradeStep_v6_4_0()) {
        db_CreateAllViews();
        return false;
    }

    // Also every launch: an older synced client can push the retired
    // project_folders setting back, and its folders still need moving.
    if (!db_UpgradeStep_v6_5_0()) {
        db_CreateAllViews();
        return false;
    }
    if (currentversion != targetversion) {
        QSqlQuery version(global_DBObjects.getDb());
        version.prepare("UPDATE application_version SET current_version = ?");
        version.addBindValue(targetversion);
        if (!version.exec()) {
            global_DBObjects.setLastSaveError(version.lastError().text());
            db_CreateAllViews();
            return false;
        }
    }

    // Recreate all views as final step (always uses current column names)
    db_CreateAllViews();

    return true;
}
