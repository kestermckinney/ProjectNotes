// Copyright (C) 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DATABASEUPGRADE_V5_0_0_H
#define DATABASEUPGRADE_V5_0_0_H

// Database upgrade from v4.1.0 to v5.0.0 — PK rename to id, add updateddate/syncdate columns and triggers
void db_UpgradeStep_v5_0_0();

#endif // DATABASEUPGRADE_V5_0_0_H
