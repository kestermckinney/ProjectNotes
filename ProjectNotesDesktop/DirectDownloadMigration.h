// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

// The Mac App Store build runs in the App Sandbox, so it starts with an empty
// container and cannot see the data of the direct download, which is not
// sandboxed and keeps it in ~/Library. On the first start, run() offers to
// copy that version's database, plugins and settings into the container. It
// copies and never moves: the downloaded version keeps working.
//
// No sandbox exception is involved. The user picks the direct download's data
// folder in an Open dialog, which grants access to that folder alone. The
// sandbox cannot read ~/Library/Preferences, so the direct download keeps a
// copy of its settings inside its data folder (exportSettings()), and run()
// reads them from there.
namespace DirectDownloadMigration {

// Folder, inside the direct download's data folder, holding the copies of its
// AppSettings and PluginSettings as AppSettings.plist and PluginSettings.plist.
inline constexpr char kSettingsFolder[] = "AppStoreMigration";

// App Store build (DirectDownloadMigration.mm). Call after the application
// identifiers are set and before anything opens the database or reads
// settings. Asks once; does nothing on later starts.
void run();

// Direct download on macOS (DirectDownloadSettingsExport.cpp). Writes the
// default profile's settings into kSettingsFolder of its data folder.
void exportSettings();

} // namespace DirectDownloadMigration
