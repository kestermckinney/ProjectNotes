// Copyright (C) 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "databaseupgrade_v1_0_0.h"
#include "databaseobjects.h"

void db_UpgradeStep_v1_0_0()
{
    // Rebuild people table to add missing columns and update structure
    global_DBObjects.execute(R"(
        CREATE TABLE sqlitestudio_temp_table AS SELECT*
            FROM people;
    )");

    global_DBObjects.execute(R"(
        DROP TABLE people;
    )");

    global_DBObjects.execute(R"(
        CREATE TABLE people(
            people_id    TEXT NOT NULL
            PRIMARY KEY
            UNIQUE,
            name         TEXT NOT NULL,
            email        TEXT,
            office_phone TEXT,
            cell_phone   TEXT,
            client_id    TEXT,
            role         TEXT,
            UNIQUE(
                name ASC
            )
        );
    )");

    global_DBObjects.execute(R"(
        INSERT INTO people(
            people_id,
            name,
            email,
            office_phone,
            cell_phone,
            client_id
        )
            SELECT people_id,
            name,
            email,
            office_phone,
            cell_phone,
            client_id
            FROM sqlitestudio_temp_table;
    )");

    global_DBObjects.execute(R"(
        DROP TABLE sqlitestudio_temp_table;
    )");
}
