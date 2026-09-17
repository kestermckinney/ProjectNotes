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

    // Opens uri in an in-app Safari view (SFSafariViewController) instead of
    // handing it to UIApplication. A plain https:// open — whether via
    // openRawUrl() above or QDesktopServices::openUrl() — goes through
    // UIApplication's normal URL routing, which honors Universal Links: the
    // installed Word/Excel/PowerPoint apps register as handlers for
    // SharePoint/OneDrive domains, so a document link opened that way lands
    // back in the native app regardless of the "Open links in desktop apps"
    // setting being off. Presenting it in SFSafariViewController instead
    // bypasses that routing entirely, so the browser-based Office Online
    // viewer link actually opens in the browser. Returns false (without
    // presenting anything) when the string doesn't form a valid NSURL or
    // there's no view controller to present from.
    static bool openInBrowser(const QString& uri);
};
