// Copyright (C) 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DATABASEUPGRADE_V5_0_3_H
#define DATABASEUPGRADE_V5_0_3_H

// Database upgrade from v5.0.2 to v5.0.3
// Replace inline UNIQUE constraints with partial unique indexes (WHERE deleted = 0)
// so that soft-deleted records do not block re-insertion of records with the same values.
void db_UpgradeStep_v5_0_3();

#endif // DATABASEUPGRADE_V5_0_3_H
