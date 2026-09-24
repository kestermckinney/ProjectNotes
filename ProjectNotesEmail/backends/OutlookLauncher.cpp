// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "OutlookLauncher.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>

#include <algorithm>
#include <functional>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <shellapi.h>
#endif

namespace PN::Comm::OutlookLauncher {

#if defined(Q_OS_WIN)
namespace {

// The ProgID the new Outlook's package registered for `extension`, such as
// "AppX18q1gk8kdws5gt2g5c62cxc6qydq7tsw" for .ics.  The package name changes
// with every update, so it is found by its family prefix.
QString newOutlookProgId(const QString &extension)
{
    QSettings packages(QStringLiteral("HKEY_CLASSES_ROOT\\Local Settings\\Software\\Microsoft\\Windows\\"
                                      "CurrentVersion\\AppModel\\Repository\\Packages"),
                       QSettings::NativeFormat);
    QStringList names = packages.childGroups().filter(QStringLiteral("Microsoft.OutlookForWindows_"),
                                                      Qt::CaseInsensitive);
    // Newest version first, should an old one linger.
    std::sort(names.begin(), names.end(), std::greater<>());
    for (const QString &name : std::as_const(names)) {
        const QString progId = packages.value(name + QStringLiteral("/Microsoft.OutlookforWindows/Capabilities/FileAssociations/")
                                              + extension).toString();
        if (!progId.isEmpty())
            return progId;
    }
    return {};
}

} // namespace

bool openWithNewOutlook(const QString &path)
{
    // The new Outlook is a full-trust packaged app, which
    // IApplicationActivationManager::ActivateForFile cannot start, so the
    // file is opened through the new Outlook's own ProgID instead of the
    // one the extension is associated with.
    const QString progId = newOutlookProgId(QLatin1Char('.') + QFileInfo(path).suffix().toLower());
    if (progId.isEmpty())
        return false;
    const std::wstring file = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath()).toStdWString();
    const std::wstring progIdText = progId.toStdWString();
    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_CLASSNAME | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"open";
    info.lpFile = file.c_str();
    info.lpClass = progIdText.c_str();
    info.nShow = SW_SHOWNORMAL;
    return ShellExecuteExW(&info);
}

QString classicExecutable()
{
    for (const char *root : {"HKEY_LOCAL_MACHINE", "HKEY_CURRENT_USER"}) {
        const QSettings appPath(QStringLiteral("%1\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\OUTLOOK.EXE")
                                    .arg(QLatin1String(root)),
                                QSettings::NativeFormat);
        const QString executable = appPath.value(QStringLiteral("Default")).toString().trimmed();
        if (!executable.isEmpty() && QFileInfo::exists(executable))
            return executable;
    }
    return {};
}

bool openWithClassicOutlook(const QString &path, const QString &fileSwitch)
{
    const QString executable = classicExecutable();
    if (executable.isEmpty())
        return false;
    // Outlook is often already running; let it bring its window forward.
    AllowSetForegroundWindow(ASFW_ANY);
    return QProcess::startDetached(executable, {fileSwitch, QDir::toNativeSeparators(path)});
}

#else

bool openWithNewOutlook(const QString &) { return false; }
QString classicExecutable() { return {}; }
bool openWithClassicOutlook(const QString &, const QString &) { return false; }

#endif

} // namespace PN::Comm::OutlookLauncher
