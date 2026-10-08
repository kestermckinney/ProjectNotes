// SPDX-License-Identifier: GPL-3.0-only
#pragma once

// Idempotent cleanup; false leaves the database version unchanged.
bool db_UpgradeStep_v6_4_0();
