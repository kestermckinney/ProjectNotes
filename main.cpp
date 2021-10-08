#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    a.setWindowIcon(QIcon(":/icons/logo.png")); // "AppIcon.icns"
    w.show();

    return a.exec();
}
