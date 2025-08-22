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
    QCoreApplication::addLibraryPath("./site-packages/PyQt6/Qt6/plugins");

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
    if ( a.style()->name().contains("windows11") ) // windows11 style is broken in 6.7
        qApp->setStyle(QStyleFactory::create("windowsvista"));
#endif

    MainWindow w;

    QObject::connect(&a, &QCoreApplication::aboutToQuit, &w, &MainWindow::aboutToQuit);


    a.setWindowIcon(QIcon(":/icons/logo.png")); // "AppIcon.icns"
    w.show();

    return a.exec();
}
