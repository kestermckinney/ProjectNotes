// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

// The Mac App Store build runs in the App Sandbox, where a file or folder
// outside the app's container is reachable only while the user's permission is
// held. remember() saves a security-scoped bookmark for a location the user
// picked or dropped; restoreAll() reopens every saved one at startup, so
// project folders, File Finder locations and plugin folders keep working
// after a restart. In every other build both functions do nothing.
namespace SecurityScopedBookmarks {

// QSettings organization holding the bookmarks ("ProjectNotes" + profile).
void setSettingsOrganization(const QString& organization);
void remember(const QString& pathOrFileUrl);
void restoreAll();

} // namespace SecurityScopedBookmarks
