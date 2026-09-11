// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only
//
// Test runner for the QML desktop frontend. Sets up an isolated, throwaway data
// profile and the same runtime type registrations the shipping app installs,
// then runs the two test groups (functional parity + UI smoke) in one process
// so they share one database and one DesktopAppController singleton.

#include <QApplication>
#include <QDir>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QtQml/qqml.h>
#include <QtTest/QtTest>

#include "DesktopAppController.h"
#include "FolderManager.h"
#include "HelpController.h"
#include "LogViewerController.h"
#include "SpellCheck.h"
#include "TextFormatter.h"

// Defined in the two test translation units.
int runParityTests(int argc, char** argv);
int runUiSmokeTests(int argc, char** argv);

int main(int argc, char* argv[])
{
    // QApplication (Widgets) — matches the shipping app; core surfaces some
    // errors through QMessageBox (guarded off at runtime). Platform is forced to
    // offscreen so the whole suite runs headless.
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    // Identical identifiers to the shipping app so AppDataLocation resolves the
    // same way — but test mode + a dedicated profile keep it fully isolated.
    QApplication::setOrganizationDomain("projectnotespro.com");
    QApplication::setApplicationName("projectnotes");
    QApplication::setApplicationDisplayName("Project Notes");
    QApplication::setApplicationVersion("6.1.0");

    QStandardPaths::setTestModeEnabled(true);
    DesktopAppController::setDeveloperProfile("qmltest");
    DesktopAppController::setUpdateChecksEnabled(false);   // no network in tests

    // Start every run from a clean database/settings.
    const QString dataDir = DesktopAppController::dataLocation();
    QDir(dataDir).removeRecursively();
    QDir().mkpath(dataDir);
    qInfo("Test data location: %s", qUtf8Printable(dataDir));

    QQuickStyle::setStyle("Basic");

    // Same singleton/type registrations as ProjectNotesDesktop/main.cpp.
    qmlRegisterSingletonType<DesktopAppController>(
        "ProjectNotesDesktop", 1, 0, "DesktopAppController", &DesktopAppController::create);
    qmlRegisterSingletonType<FolderManager>(
        "ProjectNotesDesktop", 1, 0, "FolderManager", &FolderManager::create);
    qmlRegisterSingletonType<LogViewerController>(
        "ProjectNotesDesktop", 1, 0, "LogViewerController", &LogViewerController::create);
    qmlRegisterSingletonType<HelpController>(
        "ProjectNotesDesktop", 1, 0, "HelpController", &HelpController::create);
    qmlRegisterSingletonType<TextFormatter>(
        "ProjectNotesDesktop", 1, 0, "TextFormatter", &TextFormatter::create);
    qmlRegisterType<SpellCheck>("ProjectNotesDesktop", 1, 0, "SpellCheck");

    int failed = 0;
    failed += runParityTests(argc, argv);   // functional parity first — seeds data
    failed += runUiSmokeTests(argc, argv);  // then drive the real UI over that data

    if (failed == 0)
        qInfo("\n==== ALL QML DESKTOP TESTS PASSED ====");
    else
        qWarning("\n==== %d QML DESKTOP TEST GROUP(S) FAILED ====", failed);
    return failed == 0 ? 0 : 1;
}
