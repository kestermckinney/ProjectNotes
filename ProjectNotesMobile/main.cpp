// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "AppController.h"
#include "MobileSettings.h"
#include "TextFormatter.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>

int main(int argc, char *argv[])
{
#ifdef QT_DEBUG
    qputenv("QML_IMPORT_TRACE", "1");
#endif

    QGuiApplication app(argc, argv);
    app.setOrganizationName("ProjectNotes");
    app.setOrganizationDomain("projectnotespro.com");
    app.setApplicationName("Project Notes");
    app.setApplicationVersion(QStringLiteral(PROJECTNOTES_VERSION));

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

    // Rasterize text with Core Text instead of Qt's distance fields, so SF Pro
    // gets the same stroke weight it has in UIKit apps such as the App Store.
    // Safe here because the mobile UI never scales text after layout.
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

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
