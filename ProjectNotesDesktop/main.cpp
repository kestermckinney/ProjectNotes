// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "DesktopAppController.h"
#include "FolderManager.h"
#include "HelpController.h"
#include "LogViewerController.h"
#include "SpellCheck.h"
#include "TextFormatter.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QWindow>

#if defined(_WIN32)
#include <windows.h>
#include <shobjidl.h>
#include <string>

namespace {

// The AppUserModel property keys (fmtid {9F4C2855-9F79-4B39-A8D0-E1D42DE1D5F3}).
// Defined locally so we don't have to pull in propkey.h + INITGUID/Propsys.lib.
constexpr PROPERTYKEY kPkeyAppUserModelId =
    { { 0x9F4C2855, 0x9F79, 0x4B39, { 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 } }, 5 };
constexpr PROPERTYKEY kPkeyRelaunchCommand =
    { { 0x9F4C2855, 0x9F79, 0x4B39, { 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 } }, 2 };
constexpr PROPERTYKEY kPkeyRelaunchIconResource =
    { { 0x9F4C2855, 0x9F79, 0x4B39, { 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 } }, 3 };
constexpr PROPERTYKEY kPkeyRelaunchDisplayNameResource =
    { { 0x9F4C2855, 0x9F79, 0x4B39, { 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 } }, 4 };

void setStringProperty(IPropertyStore* store, const PROPERTYKEY& key, const wchar_t* value)
{
    PROPVARIANT pv;
    PropVariantInit(&pv);
    const size_t len = wcslen(value) + 1;
    pv.pwszVal = static_cast<LPWSTR>(CoTaskMemAlloc(len * sizeof(wchar_t)));
    if (pv.pwszVal) {
        pv.vt = VT_LPWSTR;
        wcscpy_s(pv.pwszVal, len, value);
        store->SetValue(key, pv);
    }
    PropVariantClear(&pv);
}

// Give the top-level window an explicit taskbar identity. Because this build sets
// a custom AppUserModelID (see SetCurrentProcessExplicitAppUserModelID in main),
// Windows no longer draws the taskbar button from the window's own icon — it
// resolves the icon (and pin/relaunch behavior) from whatever carries that AUMID.
// A dev/build-dir run has no Start-menu shortcut carrying it, so the taskbar button
// comes up blank. Stamping RelaunchIconResource (plus command/name) onto the
// window's property store points Windows straight at the exe's embedded icon, so
// the taskbar button and any pinned shortcut render correctly without a shortcut.
void applyWindowsTaskbarIdentity(QWindow* window)
{
    if (!window)
        return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd)
        return;

    IPropertyStore* store = nullptr;
    if (FAILED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&store))) || !store)
        return;

    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    const std::wstring iconResource = std::wstring(exePath) + L",0";
    const std::wstring relaunchCommand = L"\"" + std::wstring(exePath) + L"\"";

    setStringProperty(store, kPkeyAppUserModelId, L"ProjectNotesPro.ProjectNotes.Desktop");
    setStringProperty(store, kPkeyRelaunchIconResource, iconResource.c_str());
    setStringProperty(store, kPkeyRelaunchCommand, relaunchCommand.c_str());
    setStringProperty(store, kPkeyRelaunchDisplayNameResource, L"Project Notes");

    store->Commit();
    store->Release();
}

} // namespace
#endif

int main(int argc, char* argv[])
{
#if defined(_WIN32)
    // Give the QML desktop app its own taskbar / Task Manager identity. Both this
    // app and the legacy Widgets build ship as "Project Notes.exe"; without a
    // distinct AppUserModelID, Windows 11 groups them under a single app identity
    // and shows the Widgets icon for this process in the taskbar and Task Manager
    // (the window title-bar icon comes from setWindowIcon and is unaffected).
    // Setting an explicit AUMID before any window is created decouples them so the
    // process uses its own embedded/window icon. Must be called early in main().
    SetCurrentProcessExplicitAppUserModelID(L"ProjectNotesPro.ProjectNotes.Desktop");
#endif

    // Must be set before QApplication is constructed, or any Python plugin that
    // imports PyQt6.QtWebEngineWidgets (exportnotes_plugin.py,
    // exportstatusreport_plugin.py, exporttrackeritems_plugin.py) fails to load
    // with "QtWebEngineWidgets must be imported or Qt.AA_ShareOpenGLContexts
    // must be set before a QCoreApplication instance is created" — matches the
    // Widgets app's main.cpp, which sets this for the same reason.
    QCoreApplication::setAttribute(Qt::ApplicationAttribute::AA_ShareOpenGLContexts);

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
    QApplication::setApplicationVersion("6.0.0");

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
    qmlRegisterSingletonType<LogViewerController>(
        "ProjectNotesDesktop", 1, 0, "LogViewerController", &LogViewerController::create);
    qmlRegisterSingletonType<HelpController>(
        "ProjectNotesDesktop", 1, 0, "HelpController", &HelpController::create);
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

    if (engine.rootObjects().isEmpty())
        return -1;
    auto* window = qobject_cast<QWindow*>(engine.rootObjects().first());

#if defined(_WIN32)
    // Must run before the window is shown: the taskbar resolves a window's
    // identity (and icon) from this property store the moment the window is
    // first mapped, and won't re-query it afterward. Main.qml keeps the
    // ApplicationWindow's `visible` false for exactly this reason — showing it
    // is deferred to the explicit window->show() below.
    applyWindowsTaskbarIdentity(window);
#endif

    if (window)
        window->show();

    return app.exec();
}
