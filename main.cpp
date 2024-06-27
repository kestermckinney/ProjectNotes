// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "mainwindow.h"
#include "runguard.h"

#include <QApplication>
#include <QStyleFactory>
#include <QSharedMemory>

int main(int argc, char *argv[])
{
    QCoreApplication::addLibraryPath("./site-packages/PyQt6/Qt6/plugins");

    QApplication a(argc, argv);

    RunGuard guard( "62d60669-bb94-4a94-88bb-b964890a71f4" );
    if ( !guard.tryToRun() )
    {
        QMessageBox::critical(nullptr, "Only One Instance", QString("Only one instance of Project Notes can be running at a time."));
        return 0;
    }

    MainWindow w;

    a.setWindowIcon(QIcon(":/icons/logo.png")); // "AppIcon.icns"
    w.show();

    return a.exec();
}
