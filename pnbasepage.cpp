#include "pnbasepage.h"
#include "pndatabaseobjects.h"
#include "pnsqlquerymodel.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QApplication>

PNBasePage::PNBasePage(QWidget *parent) : QWidget(parent)
{

}

void PNBasePage::setPageTitle()
{

}

void PNBasePage::newRecord()
{
    int lastrow = ((QSortFilterProxyModel*)getCurrentView()->model())->sourceModel()->rowCount(QModelIndex());

    ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->newRecord();

    //  check if column is visible
    int col = 1;
    while (!getCurrentView()->isVisible())
        col++;

    QModelIndex index = ((QSortFilterProxyModel*)getCurrentView()->model())->sourceModel()->index(lastrow, col);
    QModelIndex sort_index = ((QSortFilterProxyModel*)getCurrentView()->model())->mapFromSource(index);

    getCurrentView()->selectionModel()->select(sort_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    getCurrentView()->scrollTo(sort_index, QAbstractItemView::PositionAtCenter);
}

void PNBasePage::deleteItem()
{
    QModelIndexList qi = getCurrentView()->selectionModel()->selectedRows();

    for (int i = qi.count() - 1; i >= 0; i--)
    {
        ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->deleteRecord(getCurrentModel()->mapToSource(qi[i]));
    }
}

void PNBasePage::copyItem()
{
    int lastrow = ((PNSqlQueryModel*)getCurrentModel())->rowCount(QModelIndex());

    QModelIndexList qi = getCurrentView()->selectionModel()->selectedRows();

    for (int i = qi.count() - 1; i >= 0; i--)
    {
        ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->copyRecord(getCurrentModel()->mapToSource(qi[i]));
    }

    //  check if column is visible
    int col = 1;
    while (!getCurrentView()->isVisible())
        col++;

    QModelIndex index = ((QSortFilterProxyModel*)getCurrentView()->model())->sourceModel()->index(lastrow, col);
    QModelIndex sort_index = ((QSortFilterProxyModel*)getCurrentView()->model())->mapFromSource(index);

    getCurrentView()->selectionModel()->select(sort_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    getCurrentView()->scrollTo(sort_index, QAbstractItemView::PositionAtCenter);
}

void PNBasePage::openItem()
{
    QModelIndexList qi = getCurrentView()->selectionModel()->selectedRows();

    for (int i = qi.count() - 1; i >= 0; i--)
    {
        ((PNSqlQueryModel*)getCurrentModel()->sourceModel())->openRecord(getCurrentModel()->mapToSource(qi[i]));
    }
}

void PNBasePage::setButtonAndMenuStates()
{

}

 void PNBasePage::toFirst(bool t_open)
 {
    Q_UNUSED(t_open);
 }

 void PNBasePage::buildPluginMenu(PNPluginManager* t_pm, Ui::MainWindow* t_ui)
 {
     // clear any other plugin items
     QAction* mi = t_ui->menuPlugins->actions().last();

     while ( mi->text().compare("View Console") != 0 )
     {
         t_ui->menuPlugins->removeAction(mi);
         mi = t_ui->menuPlugins->actions().last();
     }

    t_ui->menuPlugins->addSeparator();

     // add globally available plugins
     for ( PNPlugin* p : t_pm->getPlugins())
     {
         if (p->hasPNPluginMenuEvent() && p->isEnabled())
         {
             QAction* act = t_ui->menuPlugins->addAction(p->getPNPluginName(), [p, this](){slotPluginMenu(p);});
             act->setIcon(QIcon(":/icons/add-on.png"));
         }
     }

     t_ui->menuPlugins->addSeparator();

    // add menus relevant to the current table
     for ( PNPlugin* p : MainWindow::getPluginManager()->getPlugins())
     {
         if (p->hasDataRightClickEvent(getTableName()) && p->isEnabled())
         {
             QAction* act = t_ui->menuPlugins->addAction(p->getPNPluginName(), [p, this](){slotPluginMenu(p);});
             act->setIcon(QIcon(":/icons/add-on.png"));
         }
     }
 }

 void PNBasePage::slotPluginMenu(PNPlugin* t_plugin)
 {
     QString response;

     if (m_page_model)
     {
         QVariant keyval;
         keyval = m_page_model->data(m_page_model->index(0, 0));

         QApplication::setOverrideCursor(Qt::WaitCursor);
         QApplication::processEvents();


         PNSqlQueryModel *exportmodel = m_page_model->createExportVersion();
         exportmodel->setFilter(0, keyval.toString());
         exportmodel->refresh();

         QDomDocument* xdoc = global_DBObjects.createXMLExportDoc(exportmodel, t_plugin->getChildTablesFilter());
         QString xmlstr = xdoc->toString();

         // call the menu plugin with the data structure
         response = t_plugin->callDataRightClickEvent(xmlstr);

         delete xdoc;

         QApplication::restoreOverrideCursor();
         QApplication::processEvents();
     }

     if (!response.isEmpty())
     {
         QApplication::setOverrideCursor(Qt::WaitCursor);
         QApplication::processEvents();

         QDomDocument doc;
         doc.setContent(response);

         if (!global_DBObjects.importXMLDoc(doc))
             QMessageBox::critical(this, tr("Plugin Response Failed"), "Parsing XML file failed.");

         QApplication::restoreOverrideCursor();
         QApplication::processEvents();

         // make sure page is updated if there was a response
         toFirst(false);
     }
 }

