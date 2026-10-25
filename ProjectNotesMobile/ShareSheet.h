// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <QString>

class ShareSheet
{
public:
    // Presents the system share sheet (UIActivityViewController) for a local
    // file, so the user can save it to Files, AirDrop it, or hand it to any
    // app that accepts it (Messages, Contacts, a third-party mail app).
    // Returns false when the file doesn't exist or there's no view controller
    // to present from.
    static bool shareFile(const QString& path);
};
