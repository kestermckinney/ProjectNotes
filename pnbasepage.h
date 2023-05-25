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
    virtual void deleteItem();
    virtual void setPageTitle();

    PNSortFilterProxyModel*  getCurrentModel() { return m_current_model; };
    PNTableView* getCurrentView() { return m_current_view; };
    void setCurrentModel( PNSortFilterProxyModel* t_current_model ) { m_current_model = t_current_model; };
    void setCurrentView( PNTableView* t_current_view ) { m_current_view = t_current_view; };
    virtual void setButtonAndMenuStates();
    virtual void toFirst(bool t_open = true);

private:
    PNSortFilterProxyModel* m_current_model = nullptr;
    PNTableView* m_current_view = nullptr;
};

#endif // PNBASEPAGE_H
