// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

// The Mac App Store build runs in the App Sandbox, so it starts with an empty
// container and cannot see the data of the direct download, which is not
// sandboxed and keeps it in ~/Library. On the first start,
// run() offers to copy that version's database, plugins and settings into the
// container. It copies and never moves: the downloaded version keeps working.
// Reading the old locations needs the read-only temporary exception in
// packaging/macos/ProjectNotes-AppStore.entitlements.
//
// Store builds only (DirectDownloadMigration.mm).
namespace DirectDownloadMigration {

// Call after the application identifiers are set and before anything opens
// the database or reads settings. Asks once; does nothing on later starts.
void run();

} // namespace DirectDownloadMigration
