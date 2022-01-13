#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pndatabaseobjects.h"
#include "pnsettings.h"
#include "filterdatadialog.h"

#include <QMainWindow>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *t_parent = nullptr);
    ~MainWindow();

private slots:
    //void handleNewProjectClicked();
    //void handleDeleteProjectClicked();
    void setButtonAndMenuStates();
    void OpenDatabase(QString t_dbfile);

    void on_actionExit_triggered();

    void on_actionOpen_Database_triggered();

    void on_actionClose_Database_triggered();

    void on_actionClosed_Projects_triggered();

    void on_actionStatus_Bar_triggered();

    void on_actionFilter_triggered();

private:
    Ui::MainWindow *ui;

    FilterDataDialog *m_filterdialog;

    // view state
    int m_current_page;
    QList<int> m_page_history;
    PNSqlQueryModel* m_current_model;
};

#endif // MAINWINDOW_H
