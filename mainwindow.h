#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pndatabaseobjects.h"
#include "pnsettings.h"
#include "filterdatadialog.h"

#include <QMainWindow>
#include <QStringListModel>
#include <QStack>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *t_parent = nullptr);
    ~MainWindow();

    void navigateToPage(QWidget* t_widget);
    void navigateForward();
    void navigateBackward();
    bool navigateAtEnd() { return (m_navigation_location == (m_navigation_history.count() - 1)); }
    bool navigateAtStart() { return (m_navigation_location <= 0); }
    void navigateClearHistory() { m_navigation_location = -1; m_navigation_history.clear(); }
    QWidget* navigateCurrentPage() { return (m_navigation_location == -1 ? nullptr : m_navigation_history.at(m_navigation_location) ); }

private slots:
    //void handleNewProjectClicked();
    //void handleDeleteProjectClicked();
    void setButtonAndMenuStates();
    void openDatabase(QString t_dbfile);

    void on_actionExit_triggered();

    void on_actionOpen_Database_triggered();

    void on_actionClose_Database_triggered();

    void on_actionClosed_Projects_triggered();

    void on_actionStatus_Bar_triggered();

    void on_actionFilter_triggered();

    void on_actionClients_triggered();

    void on_actionPeople_triggered();

    void on_actionProjects_triggered();

    void on_actionBack_triggered();

    void on_actionForward_triggered();

private:
    Ui::MainWindow *ui;

    FilterDataDialog *m_filterdialog;

    // view state
    int m_current_page;
    QList<int> m_page_history;
    PNSqlQueryModel* m_current_model = nullptr;
    PNTableView* m_current_view = nullptr;

    QStack<QWidget*> m_navigation_history;
    int m_navigation_location = -1;
};

#endif // MAINWINDOW_H
