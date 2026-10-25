// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "DirectDownloadMigration.h"

#include "QLogger.h"
#include "appidentity.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>

#import <AppKit/AppKit.h>

#include <pwd.h>
#include <unistd.h>

namespace DirectDownloadMigration {

namespace {

// Settings organization of the default profile.
const QString kOrganization = PN::Identity::settingsOrganization();
const QString kCheckedKey = QStringLiteral("Migration/DirectDownloadChecked");

// The user's home folder. Inside the sandbox QDir::homePath() is the container.
QString realHome()
{
    const passwd* pw = getpwuid(getuid());
    return pw && pw->pw_dir ? QString::fromUtf8(pw->pw_dir) : QString();
}

// The direct download shares this app's bundle ID; releases before 6.5.0 used
// com.projectnotespro.ProjectNotes. Either one, other than this process.
bool directDownloadRunning()
{
    @autoreleasepool {
        NSString* identifiers[] = {NSBundle.mainBundle.bundleIdentifier, @"com.projectnotespro.ProjectNotes"};
        for (NSString* identifier : identifiers) {
            if (!identifier)
                continue;
            for (NSRunningApplication* app in [NSRunningApplication runningApplicationsWithBundleIdentifier:identifier]) {
                if (app.processIdentifier != getpid())
                    return true;
            }
        }
        return false;
    }
}

void markChecked()
{
    QSettings(kOrganization, QStringLiteral("AppSettings")).setValue(kCheckedKey, true);
}

// Copies the data folder, leaving out logs and SQLite's shared-memory file,
// which SQLite rebuilds. The -wal file is copied with the database so changes
// not yet checkpointed come along.
bool copyDataFolder(const QString& from, const QString& to, QString* failed)
{
    const QDir source(from);
    QDirIterator it(from, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QString relative = source.relativeFilePath(path);
        if (relative.startsWith(QStringLiteral("logs/")) || relative == QStringLiteral("startup-error.log")
            || relative.endsWith(QStringLiteral("-shm")))
            continue;
        const QString destination = to + QLatin1Char('/') + relative;
        QDir().mkpath(QFileInfo(destination).absolutePath());
        QFile::remove(destination);
        if (!QFile::copy(path, destination)) {
            *failed = relative;
            return false;
        }
    }
    return true;
}

// QSettings reads a .plist path directly, so the old preferences come over
// with the same keys the direct download used.
void copySettings(const QString& plist, const QString& application)
{
    if (!QFileInfo::exists(plist))
        return;
    const QSettings from(plist, QSettings::NativeFormat);
    QSettings to(kOrganization, application);
    for (const QString& key : from.allKeys())
        to.setValue(key, from.value(key));
}

} // namespace

void run()
{
    if (QSettings(kOrganization, QStringLiteral("AppSettings")).value(kCheckedKey).toBool())
        return;

    const QString home = realHome();
    const QString source = home + QStringLiteral("/Library/Application Support/Project Notes");
    const QString target = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString database = QStringLiteral("/ProjectNotes.db");

    // Nothing to bring over, or this version already has its own data.
    if (home.isEmpty() || !QFileInfo::exists(source + database) || QFileInfo::exists(target + database)) {
        markChecked();
        return;
    }

    QMessageBox ask(QMessageBox::Question, QStringLiteral("Project Notes"),
                    QObject::tr("Bring over your data from the downloaded version?"));
    ask.setInformativeText(QObject::tr(
        "Project Notes found the projects, notes and settings of the version downloaded from "
        "projectnotespro.com. Copy them into this version? The downloaded version keeps its own copy."));
    QPushButton* copyButton = ask.addButton(QObject::tr("Copy My Data"), QMessageBox::AcceptRole);
    ask.addButton(QObject::tr("Start Empty"), QMessageBox::RejectRole);
    ask.setDefaultButton(copyButton);
    ask.exec();
    if (ask.clickedButton() != copyButton) {
        markChecked();
        return;
    }

    // Its database is in WAL mode; copying it while it is open could catch a
    // write halfway through.
    while (directDownloadRunning()) {
        QMessageBox wait(QMessageBox::Information, QStringLiteral("Project Notes"),
                         QObject::tr("Quit the downloaded version of Project Notes"));
        wait.setInformativeText(QObject::tr(
            "It is still open, so its data may be changing. Quit it, then click Continue."));
        QPushButton* continueButton = wait.addButton(QObject::tr("Continue"), QMessageBox::AcceptRole);
        wait.addButton(QObject::tr("Not Now"), QMessageBox::RejectRole);
        wait.setDefaultButton(continueButton);
        wait.exec();
        if (wait.clickedButton() != continueButton)
            return; // ask again on the next start
    }

    QString failed;
    QDir().mkpath(target);
    if (!copyDataFolder(source, target, &failed)) {
        // Leave no partial database behind, so the next start offers again.
        QFile::remove(target + database);
        QFile::remove(target + database + QStringLiteral("-wal"));
        QLog_Warning(CONSOLELOG, QStringLiteral("Copying the downloaded version's data stopped at %1").arg(failed));
        QMessageBox::warning(nullptr, QStringLiteral("Project Notes"),
            QObject::tr("Project Notes could not copy %1 from the downloaded version. It will start "
                        "empty this time and offer to copy your data again on the next start.")
                .arg(failed));
        return;
    }
    copySettings(home + QStringLiteral("/Library/Preferences/com.projectnotes.AppSettings.plist"),
                 QStringLiteral("AppSettings"));
    copySettings(home + QStringLiteral("/Library/Preferences/com.projectnotes.PluginSettings.plist"),
                 QStringLiteral("PluginSettings"));
    markChecked();
    QLog_Info(CONSOLELOG, QStringLiteral("Copied the downloaded version's data from %1").arg(source));

    QMessageBox::information(nullptr, QStringLiteral("Project Notes"),
        QObject::tr("Your data was copied.\n\n"
                    "This version can open only the folders you choose, so pick your project folders "
                    "again (Files & Folders > Browse Folder) and your File Finder locations "
                    "(Settings > File Finder). If cloud sync asks you to sign in, sign in again."));
}

} // namespace DirectDownloadMigration
