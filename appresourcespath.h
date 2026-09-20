// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APPRESOURCESPATH_H
#define APPRESOURCESPATH_H

#include <QCoreApplication>
#include <QDir>

// Returns the directory containing app resources (dictionary, plugins, threads).
// Release builds on macOS run from inside .app/Contents/MacOS; resources live in
// .app/Contents/Resources. Debug builds run as a plain binary (no bundle);
// resources are symlinked next to the binary.
//
// Lives in its own header (it used to sit in appsettings.h) so the embedded
// Python plugin engine — pluginmanager.cpp, pythonworker.cpp — does not have to
// pull in a frontend settings class.
inline QString appResourcesPath()
{
#ifdef Q_OS_MAC
    QString bundleResources = QCoreApplication::applicationDirPath() + "/../Resources";
    if (QDir(bundleResources).exists())
        return bundleResources;
    return QCoreApplication::applicationDirPath();
#else
    return QCoreApplication::applicationDirPath();
#endif
}

#endif // APPRESOURCESPATH_H
