// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

// Builds without the App Sandbox can reach any path the user can, so there is
// nothing to remember. The App Store build uses SecurityScopedBookmarks.mm.

#include "SecurityScopedBookmarks.h"

namespace SecurityScopedBookmarks {

void setSettingsOrganization(const QString&) {}
void remember(const QString&) {}
void restoreAll() {}

} // namespace SecurityScopedBookmarks
