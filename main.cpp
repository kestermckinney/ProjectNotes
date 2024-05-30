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

#ifdef Q_OS_WIN
    if ( a.style()->name().contains("windows11") ) // windows11 style is broken in 6.7
        qApp->setStyle(QStyleFactory::create("windowsvista"));

    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",QSettings::NativeFormat);
    if(settings.value("AppsUseLightTheme")==0){
        qApp->setStyle(QStyleFactory::create("Fusion"));
        QPalette darkPalette;
        QColor darkColor = QColor(45,45,45);
        QColor disabledColor = QColor(127,127,127);
        darkPalette.setColor(QPalette::Window, darkColor);
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(18,18,18));
        darkPalette.setColor(QPalette::AlternateBase, darkColor);
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
        darkPalette.setColor(QPalette::Button, darkColor);
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

        qApp->setPalette(darkPalette);

        qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
    }
#endif

    a.setWindowIcon(QIcon(":/icons/logo.png")); // "AppIcon.icns"
    w.show();

    return a.exec();
}
