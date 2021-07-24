#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pndatabaseobjects.h"
#include "pnsettings.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //const PNDatabaseObjects* DBObjects() { return m_DBObjects; };

private slots:
    void handleNewProjectClicked();
    void handleDeleteProjectClicked();

private:
    Ui::MainWindow *ui;
};

static PNSettings global_Settings;

#endif // MAINWINDOW_H
