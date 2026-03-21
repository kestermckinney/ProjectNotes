// Copyright (C) 2024 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DATABASEUPGRADE_V5_0_1_H
#define DATABASEUPGRADE_V5_0_1_H

// Database upgrade from v5.0.0 to v5.0.1 — fix triggers to not reset syncdate when SqliteSyncPro writes it
void db_UpgradeStep_v5_0_1();

#endif // DATABASEUPGRADE_V5_0_1_H
