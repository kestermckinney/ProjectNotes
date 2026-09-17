// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

// Builds a native Office URI scheme deep link (ms-word:, ms-excel:,
// ms-powerpoint:, ms-project:) from a project_locations full_path, per
// Microsoft's documented "Office URI Schemes":
//   ms-<app>:ofe|u|<direct document URL>
//
// Office can only open a URL that points directly at the document, such as
// https://tenant.sharepoint.com/sites/X/Shared%20Documents/Budget.xlsx (the
// form Microsoft Graph returns as webDavUrl). SharePoint's browser viewer
// page (…/_layouts/15/Doc.aspx?sourcedoc={GUID}&file=Budget.xlsx) and short
// sharing links (…/:x:/r/…, …/:f:/g/…) are not documents: handed to Excel,
// they produce "Office doesn't recognize the command it was given". Those
// return an empty string so the caller opens them in the browser instead.
//
// Called at open time; full_path itself is never rewritten. Returns an empty
// string when the path isn't an http(s) URL or its path doesn't end in an
// extension a Microsoft Office desktop/mobile app owns.
QString officeDeepLinkFor(const QString& fullPath);

// Returns fullPath rewritten to force SharePoint/OneDrive's browser-based
// Office Online viewer, for the same document types officeDeepLinkFor()
// above knows how to hand to a native Office app. Appending "web=1" to a
// document URL is Microsoft's documented way to make the browser show the
// Office Online viewer instead of downloading the file or prompting to open
// the desktop app.
//
// Used when the user has turned off "Open links in desktop apps when
// available": the caller opens the returned URL directly rather than trying
// officeDeepLinkFor()'s ms-<app>: scheme first. Returns an empty string for
// anything officeDeepLinkFor() would also refuse, so the caller can fall back
// to opening fullPath verbatim.
QString officeWebViewerUrlFor(const QString& fullPath);

// Rows saved by the old save-time rewrite in ProjectLocationsModel::setData()
// hold an ms-<app>:ofe|u|<url> (or ofv|u|) value. Returns the embedded <url>
// for those, and fullPath unchanged for everything else.
QString unwrapOfficeDeepLink(const QString& fullPath);
