// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "AppController.h"
#include "TextFormatter.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("ProjectNotes");
    app.setOrganizationDomain("kestermckinney.com");
    app.setApplicationName("Project Notes");
    app.setApplicationVersion("5.0.0");

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
