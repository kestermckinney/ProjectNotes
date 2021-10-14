#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pndatabaseobjects.h"
#include "pnsettings.h"

#include <QMainWindow>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    //void handleNewProjectClicked();
    //void handleDeleteProjectClicked();
    void setButtonAndMenuStates();
    void OpenDatabase(QString dbfile);

    void on_actionExit_triggered();

    void on_actionOpen_Database_triggered();

    void on_actionClose_Database_triggered();

    void on_actionClosed_Projects_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
