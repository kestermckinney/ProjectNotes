// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "mainwindow.h"
#include "runguard.h"
#include "plugin.h"

#include <QApplication>
#include <QStyleFactory>
#include <QSharedMemory>

int main(int argc, char *argv[])
{
    QCoreApplication::addLibraryPath("./site-packages/PyQt6/Qt6/bin");
    QCoreApplication::addLibraryPath("./site-packages/PyQt6/Qt6/plugins");
    QCoreApplication::addLibraryPath("./site-packages/PyQt6/Qt6/plugins/bin");

    // Set the attribute before creating QApplication this was needed for QWebView would work
    QCoreApplication::setAttribute(Qt::ApplicationAttribute::AA_ShareOpenGLContexts);

    QApplication a(argc, argv);

    qRegisterMetaType<PythonPlugin>();
    qRegisterMetaType<PluginMenu>();

    RunGuard guard( "62d60669-bb94-4a94-88bb-b964890a71f5" ); // modified for 4.0 beta last digit was 4
    if ( !guard.tryToRun() )
    {
        QMessageBox::critical(nullptr, "Only One Instance", QString("Only one instance of Project Notes can be running at a time."));
        return 0;
    }

#ifdef Q_OS_WIN
    QString processPath = QCoreApplication::applicationDirPath() + "/site-packages/PyQt6/Qt6/bin/QtWebEngineProcess.exe";   // adjust if you moved it
    QString resourcePath = QCoreApplication::applicationDirPath() + "/site-packages/PyQt6/Qt6/resources";   // adjust if you moved it
    QString localesPath = QCoreApplication::applicationDirPath() + "/site-packages/PyQt6/Qt6/translations/qtwebengine_locales";   // adjust if you moved it

    qDebug() << "Looking for file " << processPath;

    if (QFileInfo::exists(processPath))
    {
        QLog_Info(CONSOLELOG,QString("Setting application copy of QtWebEngineProcess.exe to be the default."));
        qputenv("QTWEBENGINEPROCESS_PATH", processPath.toUtf8());
        qputenv("QTWEBENGINE_RESOURCES_PATH", resourcePath.toUtf8());
        qputenv("QTWEBENGINE_LOCALES_PATH", localesPath.toUtf8());

        // try to get more debugging information
        qputenv("QT_LOGGING_RULES", "qt.webenginecontext.debug=true");
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--enable-logging=stderr --v=1");

    }
#endif

    MainWindow w;

    QObject::connect(&a, &QCoreApplication::aboutToQuit, &w, &MainWindow::aboutToQuit);


    a.setWindowIcon(QIcon(":/icons/logo.png")); // "AppIcon.icns"
    w.show();

    return a.exec();
}
