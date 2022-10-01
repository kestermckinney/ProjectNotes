#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <QStack>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#include "pndatabaseobjects.h"
#include "pnsettings.h"
#include "pnbasepage.h"
#include "preferencesdialog.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *t_parent = nullptr);
    ~MainWindow();

    void navigateToPage(PNBasePage* t_widget);
    void navigateForward();
    void navigateBackward();
    bool navigateAtEnd() { return (m_navigation_location == (m_navigation_history.count() - 1)); }
    bool navigateAtStart() { return (m_navigation_location <= 0); }
    void navigateClearHistory() { m_navigation_location = -1; m_navigation_history.clear(); }
    PNBasePage* navigateCurrentPage() { return (m_navigation_location == -1 ? nullptr : m_navigation_history.at(m_navigation_location) ); }

public slots:
    void on_actionOpen_ProjectDetails_triggered();
    void on_actionOpen_ItemDetails_triggered();
    void on_actionOpen_ProjectNote_triggered();

private slots:
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
    void on_actionNew_Item_triggered();
    void on_actionCopy_Item_triggered();
    void on_actionDelete_Item_triggered();
    void on_actionInternal_Items_triggered();
    void on_actionAll_Tracker_Action_Items_triggered();
    void on_actionPreferences_triggered();
    void on_actionResolved_Tracker_Action_Items_triggered();

private:
    Ui::MainWindow *ui;   

    PreferencesDialog* m_preferences_dialog;

    // view state
    QList<int> m_page_history;

    QStack<PNBasePage*> m_navigation_history;
    int m_navigation_location = -1;
};

#endif // MAINWINDOW_H
