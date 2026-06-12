// Copyright (C) 2026 Paul McKinney
#include "supportbundle.h"

#include "logviewer.h"
#include "QLogger.h"
#include "version.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QMessageBox>
#include <QWidget>

#include <private/qzipwriter_p.h>

using namespace QLogger;

namespace
{
    // Write all of `logFiles` into a new zip at `zipPath`. Returns false (and
    // sets `errorOut`) on failure. Uses Qt's built-in QZipWriter so no external
    // archive library is needed.
    bool writeZip(const QString &zipPath,
                  const QFileInfoList &logFiles,
                  QString &errorOut)
    {
        QZipWriter zip(zipPath);
        if (zip.status() != QZipWriter::NoError) {
            errorOut = QStringLiteral("could not create archive (status %1)")
                           .arg(static_cast<int>(zip.status()));
            return false;
        }
        zip.setCompressionPolicy(QZipWriter::AlwaysCompress);

        bool addedAny = false;
        for (const QFileInfo &fi : logFiles) {
            QFile in(fi.absoluteFilePath());
            if (!in.open(QIODevice::ReadOnly)) {
                // Skip a log we cannot read (e.g. locked) rather than aborting
                // the whole bundle, but note it for diagnostics.
                QLog_Warning(SYNCERRORLOG,
                    QString("Support bundle: skipped unreadable log '%1'").arg(fi.fileName()));
                continue;
            }
            zip.addFile(fi.fileName(), in.readAll());
            in.close();

            if (zip.status() != QZipWriter::NoError) {
                errorOut = QStringLiteral("could not add '%1' to archive (status %2)")
                               .arg(fi.fileName())
                               .arg(static_cast<int>(zip.status()));
                zip.close();
                return false;
            }
            addedAny = true;
        }

        zip.close();
        if (zip.status() != QZipWriter::NoError) {
            errorOut = QStringLiteral("archive finalization failed (status %1)")
                           .arg(static_cast<int>(zip.status()));
            return false;
        }
        if (!addedAny) {
            errorOut = QStringLiteral("no readable log files to archive");
            return false;
        }
        return true;
    }
}

namespace SupportBundle
{
    QString supportEmailAddress()
    {
        return QStringLiteral("admin@projectnotespro.com");
    }

    bool sendLogsToSupport(QWidget *parent)
    {
        const QString logDir = LogViewer::getLogFileLocation();

        QDir dir(logDir);
        const QFileInfoList logFiles =
            dir.entryInfoList({QStringLiteral("*.log")}, QDir::Files, QDir::Name);

        if (logFiles.isEmpty()) {
            QMessageBox::information(parent, QObject::tr("Send Logs to Support"),
                QObject::tr("No log files were found to send.\n\nLog files are stored in:\n%1")
                    .arg(QDir::toNativeSeparators(logDir)));
            return false;
        }

        // Prefer the Desktop; fall back to the temp dir if it isn't writable.
        QString destDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        if (destDir.isEmpty() || !QFileInfo(destDir).isWritable())
            destDir = QDir::tempPath();

        const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
        const QString zipPath = QDir(destDir).filePath(
            QStringLiteral("ProjectNotes-logs-%1.zip").arg(stamp));

        QString zipError;
        if (!writeZip(zipPath, logFiles, zipError)) {
            QLog_Error(SYNCERRORLOG,
                QString("Support bundle creation failed: %1").arg(zipError));
            QMessageBox::warning(parent, QObject::tr("Send Logs to Support"),
                QObject::tr("Could not create the log archive: %1").arg(zipError));
            return false;
        }

        // Open a pre-addressed mail draft. mailto cannot carry attachments, so
        // the body gives the user the full path to the zip to attach.
        const QString version = QStringLiteral("%1.%2.%3")
            .arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH);

        QUrlQuery query;
        query.addQueryItem(QStringLiteral("subject"),
            QStringLiteral("ProjectNotes Support Logs (v%1)").arg(version));
        query.addQueryItem(QStringLiteral("body"),
            QObject::tr("Please describe the problem you are seeing below.\n\n"
                        "IMPORTANT: attach the log archive located at:\n%1\n\n"
                        "--- describe your issue here ---\n")
                .arg(QDir::toNativeSeparators(zipPath)));

        QUrl mailUrl;
        mailUrl.setScheme(QStringLiteral("mailto"));
        mailUrl.setPath(supportEmailAddress());
        mailUrl.setQuery(query);
        QDesktopServices::openUrl(mailUrl);

        QMessageBox::information(parent, QObject::tr("Send Logs to Support"),
            QObject::tr("A log archive was created here:\n\n%1\n\n"
                        "An email to %2 has been started. Please attach that "
                        "zip file to the email and send it.")
                .arg(QDir::toNativeSeparators(zipPath), supportEmailAddress()));

        return true;
    }
}
