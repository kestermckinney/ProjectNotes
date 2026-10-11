// SPDX-License-Identifier: GPL-3.0-only
#include "ThunderbirdBridge.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace PN::Contacts {
namespace {
QString shellQuote(QString value)
{
    value.replace('\'', "'\\''");
    return "'" + value + "'";
}
bool save(const QString &path, const QByteArray &bytes, bool executable = false)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) return false;
    if (!file.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                             | (executable ? QFile::ExeOwner : QFile::Permissions{}))) return false;
    return file.commit();
}
QByteArray manifest(const QString &executable)
{
    return QJsonDocument(QJsonObject{{"name", "org.projectnotes.contacts"},
        {"description", "Project Notes contact bridge"}, {"path", executable},
        {"type", "stdio"}, {"allowed_extensions", QJsonArray{"contacts@projectnotes.org"}}}).toJson();
}
}
QStringList thunderbirdFlatpakIds(const QString &home)
{
    QStringList ids;
#ifdef Q_OS_LINUX
    for (const auto &id : QStringList{"org.mozilla.thunderbird", "org.mozilla.Thunderbird",
                                     "org.mozilla.thunderbird_esr", "org.mozilla.thunderbird_beta"})
        if (QFileInfo(home + "/.var/app/" + id).isDir()) ids << id;
#endif
    return ids;
}
ThunderbirdBridgeInstallation installThunderbirdBridge(const QString &home,
    const QString &applicationDirectory, const QString &nativeManifestDirectory,
    const QString &projectNotesFlatpakId)
{
    ThunderbirdBridgeInstallation result;
    QString helper = applicationDirectory + "/ProjectNotesThunderbirdContacts";
#ifdef Q_OS_WIN
    helper += ".exe";
#endif
    result.addonPath = applicationDirectory + "/projectnotes-contacts.xpi";
    if (!QFileInfo(helper).isFile() || !QFileInfo(helper).isExecutable()) {
        result.error = "The Thunderbird native host is missing or not executable in this installation.";
        return result;
    }
    if (!QFileInfo(result.addonPath).isFile() || !QFileInfo(result.addonPath).isReadable()) {
        result.error = "The Thunderbird add-on projectnotes-contacts.xpi is missing from this installation.";
        return result;
    }
    QString hostCommand = shellQuote(helper);
    QString nativeHost = helper;
#ifdef Q_OS_LINUX
    if (!projectNotesFlatpakId.isEmpty()) {
        hostCommand = "/usr/bin/flatpak run --command=ProjectNotesThunderbirdContacts "
            + shellQuote(projectNotesFlatpakId);
        nativeHost = nativeManifestDirectory + "/projectnotes-contacts-host.sh";
        if (!save(nativeHost, ("#!/bin/sh\nexec " + hostCommand + " \"$@\"\n").toUtf8(), true)) {
            result.error = "Cannot install the Project Notes Flatpak host launcher.";
            return result;
        }
    }
#endif
    if (!save(nativeManifestDirectory + "/org.projectnotes.contacts.json", manifest(nativeHost))) {
        result.error = "Cannot install Thunderbird native-host manifest.";
        return result;
    }
    result.flatpakIds = thunderbirdFlatpakIds(home);
    for (const auto &id : result.flatpakIds) {
        const QString directory = home + "/.var/app/" + id + "/.mozilla/native-messaging-hosts";
        const QString launcher = directory + "/projectnotes-contacts-host.sh";
        if (!save(launcher, ("#!/bin/sh\nexec /usr/bin/flatpak-spawn --host "
                            + hostCommand + " \"$@\"\n").toUtf8(), true)
            || !save(directory + "/org.projectnotes.contacts.json", manifest(launcher))) {
            result.error = "Cannot install the contact bridge for Flatpak Thunderbird (" + id + ").";
            return result;
        }
        // Thunderbird can select this file without access to the application's
        // installation directory (or Project Notes' separate /app mount).
        const QString addon = home + "/.var/app/" + id + "/data/projectnotes-contacts.xpi";
        QFile source(result.addonPath);
        if (!source.open(QIODevice::ReadOnly) || !save(addon, source.readAll())) {
            result.error = "Cannot copy the Thunderbird add-on into its Flatpak data folder.";
            return result;
        }
        if (id == result.flatpakIds.first()) result.addonPath = addon;
    }
    return result;
}
}
