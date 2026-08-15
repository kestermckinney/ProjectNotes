// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "AppController.h"
#include "MobileSettings.h"
#include "TextFormatter.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <cstdio>

static void pnmobileMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex outMutex;
    static QFile logFile;
    static bool initialized = false;
    QMutexLocker locker(&outMutex);

    if (!initialized) {
        initialized = true;
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        const QString path = dir + "/pnmobile.log";
        logFile.setFileName(path);
        logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        QTextStream(&logFile) << "==== pnmobile log start "
                              << QDateTime::currentDateTime().toString(Qt::ISODate)
                              << " at " << path << "\n";
        logFile.flush();
    }

    const QString level = [type]() {
        switch (type) {
        case QtDebugMsg: return "D"; case QtInfoMsg: return "I";
        case QtWarningMsg: return "W"; case QtCriticalMsg: return "C";
        case QtFatalMsg: return "F"; default: return "?";
        }
    }();
    const QString line = QString("[%1] %2 %3:%4 %5\n")
        .arg(level)
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"))
        .arg(context.file ? context.file : "")
        .arg(context.line)
        .arg(msg);

    if (logFile.isOpen()) {
        QTextStream(&logFile) << line;
        logFile.flush();
    }
    // Mirror Qt's default handler so stderr still gets everything too.
    fprintf(stderr, "%s", qPrintable(line));
    fflush(stderr);
}

int main(int argc, char *argv[])
{
#ifdef QT_DEBUG
    qputenv("QML_IMPORT_TRACE", "1");
#endif

    qInstallMessageHandler(pnmobileMessageHandler);
    qInfo("ProjectNotesMobile starting (pid=%d)", int(QCoreApplication::applicationPid()));

    QGuiApplication app(argc, argv);
    app.setOrganizationName("ProjectNotes");
    app.setOrganizationDomain("projectnotespro.com");
    app.setApplicationName("Project Notes");
    app.setApplicationVersion("5.0.0");

    QCommandLineParser parser;
    QCommandLineOption testSupabaseOption(
        "test-supabase",
        "Use the test Supabase instance instead of production.");
    parser.addOption(testSupabaseOption);
    parser.parse(QCoreApplication::arguments());

    if (parser.isSet(testSupabaseOption))
        MobileSettings::setTestSupabase(true);

    // Use the iOS-native look by default
    QQuickStyle::setStyle("iOS");

    // Register QML singletons — the create() factory is called by the engine on first use.
    qmlRegisterSingletonType<AppController>(
        "ProjectNotesMobile", 1, 0, "AppController",
        &AppController::create);
    qmlRegisterSingletonType<TextFormatter>(
        "ProjectNotesMobile", 1, 0, "TextFormatter",
        &TextFormatter::create);

    QQmlApplicationEngine engine;

    const QUrl url("qrc:/qt/qml/ProjectNotesMobile/qml/Main.qml");
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app,    [&]() { app.exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
