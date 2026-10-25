// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APPIDENTITY_H
#define APPIDENTITY_H

#include <QStandardPaths>
#include <QString>

// The names Project Notes stores things under. Each spelling has one job, and
// none may change: installed copies find their data, settings and saved
// passwords through them.
//
//   Windows  data %APPDATA%\Project Notes              settings HKCU\Software\ProjectNotes
//   macOS    data ~/Library/Application Support/Project Notes
//                                                      settings com.projectnotes.*.plist
//   Linux    data ~/.local/share/projectnotes           settings ~/.config/ProjectNotes
//   iOS      data Application Support/ProjectNotes/Project Notes
//                                                      settings com.projectnotes.MobileSettings
namespace PN::Identity {

// What people see: windows and dialogs, and the data and cache folder on
// Windows, macOS and iOS.
inline QString displayName() { return QStringLiteral("Project Notes"); }

// Machine-facing name: the data and cache folder and executable on Linux, and
// files in the system temp folder. Lowercase without spaces, as Linux expects.
inline QString machineName() { return QStringLiteral("projectnotes"); }

// QCoreApplication::applicationName(), which names the data and cache folder.
inline QString applicationName()
{
#if defined(Q_OS_WIN) || defined(Q_OS_DARWIN)
    return displayName();
#else
    return machineName();
#endif
}

// QSettings organization: the registry key on Windows, the com.projectnotes.*
// preference domains on macOS and iOS, ~/.config/ProjectNotes on Linux.
inline QString settingsOrganization() { return QStringLiteral("ProjectNotes"); }

// A developer profile (--developer-profile NAME) keeps its own settings, data
// and cache. Its settings organization is the name appended without a
// separator ("ProjectNotesNAME"), as existing profiles were created; its
// folders are a NAME subfolder of the data and cache folders.
inline QString settingsOrganization(const QString& profile) { return settingsOrganization() + profile; }

// The profile in an organization made by settingsOrganization(profile).
inline QString profileOf(const QString& organization)
{
    return organization.startsWith(settingsOrganization()) ? organization.mid(settingsOrganization().size()) : QString();
}

inline QString dataLocation(const QString& profile = {})
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return profile.isEmpty() ? base : base + QLatin1Char('/') + profile;
}

inline QString cacheLocation(const QString& profile = {})
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return profile.isEmpty() ? base : base + QLatin1Char('/') + profile;
}

// Start of every file or folder name in the system temp folder.
inline QString tempPrefix() { return machineName() + QLatin1Char('-'); }

// Prefix of every saved password's key (Keychain, Windows Credential Manager,
// Secret Service). It is the bundle ID the macOS app had before 6.5.0 and is
// now only a namespace; changing it would lose every saved password.
inline QString credentialNamespace() { return QStringLiteral("com.projectnotespro.ProjectNotes"); }

} // namespace PN::Identity

#endif // APPIDENTITY_H
