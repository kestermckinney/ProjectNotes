// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "DesktopAppController.h"
#include "FolderManager.h"
#include "SpellCheck.h"
#include "TextFormatter.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>

int main(int argc, char* argv[])
{
    // QApplication (not QGuiApplication): ProjectNotesCore surfaces database
    // errors through QMessageBox, which requires the Widgets application object.
    QApplication app(argc, argv);

    // These identifiers MUST match the Widgets app so that
    // QStandardPaths::AppDataLocation resolves to the same directory and both
    // apps open the same database file (see DesktopAppController::openOrCreateDatabase).
    //
    // The Widgets app never calls setApplicationName(), so its data location is
    // derived from its executable basename, which its CMake sets per platform:
    //   Linux  -> "projectnotes"   (OUTPUT_NAME)
    //   Win/mac-> "Project Notes"  (OUTPUT_NAME / bundle name)
    // We set applicationName explicitly to reproduce the identical path.
    QApplication::setOrganizationDomain("projectnotespro.com");
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    QApplication::setApplicationName("Project Notes");
#else
    QApplication::setApplicationName("projectnotes");
#endif
    QApplication::setApplicationDisplayName("Project Notes");
    QApplication::setApplicationVersion("5.2.3");

    // --developer-profile PROFILENAME: use a separate data directory (same
    // behavior as the Widgets app), so the QML app can open the same dev DB.
    QCommandLineParser parser;
    QCommandLineOption devProfileOption(
        "developer-profile",
        "Use a separate settings/data profile (matches the Widgets app).",
        "PROFILENAME");
    parser.addOption(devProfileOption);
    QCommandLineOption testSupabaseOption(
        "test-supabase",
        "Route cloud sync at the TEST Supabase instance instead of production.");
    parser.addOption(testSupabaseOption);
    parser.parse(QCoreApplication::arguments());
    if (parser.isSet(devProfileOption)) {
        const QString profile = parser.value(devProfileOption);
        QApplication::setOrganizationDomain(profile + ".projectnotespro.com");
        DesktopAppController::setDeveloperProfile(profile);
    }
    if (parser.isSet(testSupabaseOption))
        DesktopAppController::setTestSupabase(true);

    // Note: no single-instance guard. The QML app is designed to run alongside
    // the Widgets app on the same WAL SQLite database, which handles concurrent
    // access; a RunGuard/QSystemSemaphore can also hang a restart if the process
    // is force-killed mid-critical-section, so it is intentionally omitted.
    app.setWindowIcon(QIcon(":/qt/qml/ProjectNotesDesktop/icons/projectnotes.ico"));

    // Basic style is fully themeable (no platform palette overrides), which the
    // custom light/dark design system in Theme.qml relies on.
    QQuickStyle::setStyle("Basic");

    // Register the C++ singletons into the QML module (same runtime-registration
    // approach the mobile app uses). Doing this explicitly — rather than relying
    // on QML_ELEMENT compile-time registration — avoids a URI clash where the
    // runtime TextFormatter registration would otherwise pre-empt the module's
    // auto-registered types, leaving them "not defined" in QML.
    qmlRegisterSingletonType<DesktopAppController>(
        "ProjectNotesDesktop", 1, 0, "DesktopAppController", &DesktopAppController::create);
    qmlRegisterSingletonType<FolderManager>(
        "ProjectNotesDesktop", 1, 0, "FolderManager", &FolderManager::create);
    qmlRegisterSingletonType<TextFormatter>(
        "ProjectNotesDesktop", 1, 0, "TextFormatter", &TextFormatter::create);
    // Creatable (non-singleton) inline spell-check attach type.
    qmlRegisterType<SpellCheck>("ProjectNotesDesktop", 1, 0, "SpellCheck");

    QQmlApplicationEngine engine;

    const QUrl url("qrc:/qt/qml/ProjectNotesDesktop/qml/Main.qml");
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app,    [] { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
