// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once
#include <QString>

class NativeUrlOpener
{
public:
    // Opens uri via UIApplication, bypassing QUrl entirely. Needed for
    // ms-word:ofe|u|<url>-style Office deep links: QUrl percent-encodes the
    // literal "|" command delimiter those rely on when it's round-tripped
    // through QDesktopServices::openUrl(), which breaks them (Office reports
    // "doesn't recognize the command it was given") — see officeDeepLinkFor()
    // in officedeeplink.h. Building the NSURL directly from the raw NSString
    // avoids that. Returns false (without prompting) when no app can open it,
    // or when the string doesn't form a valid NSURL.
    static bool openRawUrl(const QString& uri);
};
