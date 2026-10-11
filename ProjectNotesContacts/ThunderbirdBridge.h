// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>
#include <QStringList>

namespace PN::Contacts {
// Use the same absolute socket path in desktop and Flatpak processes. Relative
// QLocalSocket names use TMPDIR, which Thunderbird's Flatpak deliberately changes.
inline QString thunderbirdBridgeSocket()
{
    const QString name = "projectnotes-contacts-" + QString::fromLatin1(
        QCryptographicHash::hash(QDir::homePath().toUtf8(), QCryptographicHash::Sha256).toHex()).left(20);
#ifdef Q_OS_LINUX
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (!runtime.isEmpty())
        return runtime + "/app/com.projectnotespro.ProjectNotes/contacts/" + name.right(20);
#endif
    return name;
}

QStringList thunderbirdFlatpakIds(const QString &home);
struct ThunderbirdBridgeInstallation {
    QString error;
    QString addonPath;
    QStringList flatpakIds;
};
// Paths are explicit so installation can be verified without changing the real
// user's native-messaging registration or Flatpak permissions.
ThunderbirdBridgeInstallation installThunderbirdBridge(const QString &home,
    const QString &applicationDirectory, const QString &nativeManifestDirectory,
    const QString &projectNotesFlatpakId = {});
}
