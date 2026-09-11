// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DATABASEUPGRADE_V6_1_0_H
#define DATABASEUPGRADE_V6_1_0_H

// Database upgrade step for v6.1.0 — drop the UNIQUE constraint on
// idx_project_locations_proj_desc; a shared description is not an identity
// conflict and should not block a File Finder scan from writing a row.
void db_UpgradeStep_v6_1_0();

#endif // DATABASEUPGRADE_V6_1_0_H
