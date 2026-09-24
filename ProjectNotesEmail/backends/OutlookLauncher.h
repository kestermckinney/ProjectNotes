// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

namespace PN::Comm::OutlookLauncher {

// Windows keeps two Outlooks side by side, and the file association only names
// one of them.  These open a staged .eml/.ics file in a specific one, whatever
// the association says.  Both return false on other hosts.

// The new Outlook for Windows (olk.exe), a packaged app.  The file is opened
// through the ProgID its package registered for that type (.eml and .ics).
bool openWithNewOutlook(const QString &path);

// OUTLOOK.EXE as registered under App Paths; empty when it is not installed.
QString classicExecutable();

// Starts OUTLOOK.EXE with `fileSwitch` (/eml or /ical, as its own file
// associations do) and the file.
bool openWithClassicOutlook(const QString &path, const QString &fileSwitch);

} // namespace PN::Comm::OutlookLauncher
