#ifndef PNBASEPAGE_H
#define PNBASEPAGE_H

#include <QWidget>
#include "pntableview.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class PNBasePage : public QWidget
{
    Q_OBJECT
public:
    explicit PNBasePage(QWidget *parent = nullptr);

    virtual void setupModels( Ui::MainWindow *t_ui ) { Q_UNUSED(t_ui)};
    virtual void newRecord();
    virtual void copyItem();
    virtual void openItem();
    virtual void deleteItem();
    virtual void setPageTitle();

    PNSortFilterProxyModel*  getCurrentModel() { return m_current_model; };
    PNTableView* getCurrentView() { return m_current_view; };
    void setCurrentModel( PNSortFilterProxyModel* t_current_model ) { m_current_model = t_current_model; };
    void setCurrentView( PNTableView* t_current_view ) { m_current_view = t_current_view; };
    virtual void setButtonAndMenuStates();
    virtual void toFirst(bool t_open = true);
    const QString getTableName() { return m_table_name; }
    void setTableName(const QString& t_table_name) { m_table_name = t_table_name; }
    void buildPluginMenu(PNPluginManager* t_pm, Ui::MainWindow* t_ui);

public slots:
    void slotPluginMenu(PNPlugin* t_plugin);

private:
    PNSortFilterProxyModel* m_current_model = nullptr;
    PNTableView* m_current_view = nullptr;

    QString m_table_name;
};

#endif // PNBASEPAGE_H
