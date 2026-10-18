// SPDX-License-Identifier: GPL-3.0-only
#pragma once

// Creates project_folders and projects.folder_id, then moves the legacy
// "project_folders" setting into them. Idempotent; false leaves the database
// version unchanged.
bool db_UpgradeStep_v6_5_0();
