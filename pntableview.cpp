#include "pntableview.h"
#include "pnsettings.h"
#include "pndatabaseobjects.h"
#include "pnsqlquerymodel.h"
#include "mainwindow.h"

#include <QDebug>
#include <QMouseEvent>
#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>

PNTableView::PNTableView(QWidget *t_parent) : QTableView(t_parent)
{
    setSortingEnabled(true);

    QHeaderView *headerView = horizontalHeader();

    headerView->setSortIndicator(-1, Qt::AscendingOrder);
    headerView->setSortIndicatorShown(true);
    headerView->viewport()->installEventFilter(this);

    QHeaderView *rowView = verticalHeader();

    rowView->viewport()->installEventFilter(this);

    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(this, &QTableView::activated, this, &PNTableView::dataRowActivated);
    connect(this, &QTableView::clicked, this, &PNTableView::dataRowSelected);

    newRecord = new QAction(tr("New"), this);
    deleteRecord = new QAction(tr("Delete"), this);
    openRecord = new QAction(tr("Open"), this);
    exportRecord = new QAction(tr("XML Export..."), this);
    filterRecords = new QAction(tr("Filter Settings..."), this);
    resetColumns = new QAction(tr("Reset Columns"), this);
    copyRecord = new QAction(tr("Copy"), this);


    connect(newRecord, &QAction::triggered, this, &PNTableView::slotNewRecord);
    connect(deleteRecord, &QAction::triggered, this, &PNTableView::slotDeleteRecord);
    connect(openRecord, &QAction::triggered, this, &PNTableView::slotOpenRecord);
    connect(exportRecord, &QAction::triggered, this, &PNTableView::slotExportRecord);
    connect(filterRecords, &QAction::triggered, this, &PNTableView::slotFilterRecords);
    connect(resetColumns, &QAction::triggered, this, &PNTableView::slotResetColumns);
    connect(copyRecord, &QAction::triggered, this, &PNTableView::slotCopyRecord);
}

PNTableView::~PNTableView()
{
    disconnect(this, &QTableView::activated, this, &PNTableView::dataRowActivated);
    disconnect(this, &QTableView::clicked, this, &PNTableView::dataRowSelected);

    disconnect(newRecord, &QAction::triggered, this, &PNTableView::slotNewRecord);
    disconnect(deleteRecord, &QAction::triggered, this, &PNTableView::slotDeleteRecord);
    disconnect(openRecord, &QAction::triggered, this, &PNTableView::slotOpenRecord);
    disconnect(exportRecord, &QAction::triggered, this, &PNTableView::slotExportRecord);
    disconnect(filterRecords, &QAction::triggered, this, &PNTableView::slotFilterRecords);
    disconnect(resetColumns, &QAction::triggered, this, &PNTableView::slotResetColumns);
    disconnect(copyRecord, &QAction::triggered, this, &PNTableView::slotCopyRecord);

    delete newRecord;
    delete deleteRecord;
    delete openRecord;
    delete exportRecord;
    delete filterRecords;
    delete resetColumns;

    if (m_filterdialog)
        delete m_filterdialog;

    QHeaderView *headerView = horizontalHeader();
    headerView->removeEventFilter(this);
    QHeaderView *rowView = verticalHeader();
    rowView->removeEventFilter(this);
}


void PNTableView::slotPluginMenu(PNPlugin* t_plugin)
{
    PNSqlQueryModel* table = PNSqlQueryModel::findOpenTable(t_plugin->getTableName());

    if (!table && !t_plugin->getTableName().isEmpty())
    {
        QMessageBox::critical(this, tr("Plugin Call Failed"), QString("The table (%1)specified by the plugin does not exist.").arg(t_plugin->getTableName()));
        return;
    }

    if (table)
    {
        QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
        PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

        QModelIndexList qil = this->selectionModel()->selectedRows();

        QVariant keyval;

        auto qi = qil.begin();
        keyval = currentmodel->data(*qi);

        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();


        PNSqlQueryModel *exportmodel = currentmodel->createExportVersion();
        exportmodel->setFilter(0, keyval.toString());
        exportmodel->refresh();

        QDomDocument* xdoc = global_DBObjects.createXMLExportDoc(exportmodel, t_plugin->getChildTablesFilter());
        QString xmlstr = xdoc->toString();

        // call the menu plugin with the data structure
        t_plugin->callDataRightClickEvent(xmlstr);

        delete xdoc;

        QApplication::restoreOverrideCursor();
        QApplication::processEvents();
    }
    else
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();

        // call the menu plugin with an empty string
        t_plugin->callDataRightClickEvent(QString());

        QApplication::restoreOverrideCursor();
        QApplication::processEvents();

    }
}

void PNTableView::setModel(QAbstractItemModel *t_model)
{
    if ( t_model ) // load view settings on create and set model
    {
        int Col;
        QString Dir;

        QTableView::setModel(t_model);
        global_Settings.getTableViewState(objectName(), *this);
        global_Settings.getTableSortColumn(objectName(), Col, Dir);

        if (Col >= 0)
        {
            if (Dir == "A")
                horizontalHeader()->setSortIndicator(Col, Qt::AscendingOrder);
            else
                horizontalHeader()->setSortIndicator(Col, Qt::DescendingOrder);

            horizontalHeader()->setSortIndicatorShown(true);
        }
        else
        {
            horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
            horizontalHeader()->setSortIndicatorShown(false);
        }

        horizontalHeader()->setVisible(true);
        verticalHeader()->setVisible(true);
        verticalHeader()->setSortIndicatorShown(false);

    }
    else if ( this->model() ) // when closing or setting model to empty save the columns first on startup don't save a blank view
    {
        // do this on the close instead global_Settings.setTableViewState(objectName(), *this);
        QTableView::setModel(t_model);
    }
}

bool PNTableView::eventFilter(QObject* t_watched, QEvent *t_event)
{
    Q_UNUSED(t_watched);

    switch (t_event->type())
    {
    case QEvent::MouseButtonDblClick:
        if ( ((QMouseEvent*)t_event)->buttons().testFlag(Qt::LeftButton ) )
        {
            auto vheader = verticalHeader();

            if (vheader->geometry().contains(((QMouseEvent*)t_event)->pos()))
                slotOpenRecord();
            else
                return false;
        }
        break;
    case QEvent::MouseButtonPress:
        if ( ((QMouseEvent*)t_event)->button() != Qt::LeftButton )
            return false;
        m_pressPos = ((QMouseEvent*)t_event)->pos();
        break;

    case QEvent::MouseMove:
        if ( ((QMouseEvent*)t_event)->buttons().testFlag(Qt::LeftButton )
            && (m_pressPos - ((QMouseEvent*)t_event)->pos()).manhattanLength() > qApp->startDragDistance())
        {
            m_isMoving = true;
        }
        break;
    case QEvent::MouseButtonRelease:
    {
         if ( ((QMouseEvent*)t_event)->button() != Qt::LeftButton )
            return false;

         auto header = horizontalHeader();

         if (!header->geometry().contains(((QMouseEvent*)t_event)->pos()))
             return false;

        // If we were dragging a section, then pass the event on.
        if (m_isMoving)
        {
            global_Settings.setTableViewState(objectName(), *this);

            m_isMoving = false;
            return false;
        }

        const int indexAtCursor = header->logicalIndexAt(((QMouseEvent*)t_event)->pos());

        if (indexAtCursor == -1)
            return false; // Do nothing, we clicked outside the headers

        else if (header->sortIndicatorSection() != indexAtCursor)
        {
            header->setSortIndicator(indexAtCursor, Qt::AscendingOrder);
            header->setSortIndicatorShown(true);
            global_Settings.setTableSortColumn(objectName(), indexAtCursor, "A");
        }
        else if (header->sortIndicatorOrder() == Qt::AscendingOrder)
        {
            header->setSortIndicator(indexAtCursor, Qt::DescendingOrder);
            global_Settings.setTableSortColumn(objectName(), indexAtCursor, "D");
        }
        else
        {
            header->setSortIndicator(-1, Qt::AscendingOrder);
            header->setSortIndicatorShown(false);
            global_Settings.setTableSortColumn(objectName(), -1, "");
        }

        return true;
    }
    default:
        break;
    }
    return false;
}

void PNTableView::dataRowSelected(const QModelIndex &t_index)
{
    Q_UNUSED(t_index);

    ((QWidget*)parent())->setFocus();
    //setButtonAndMenus fuction is called this method seems to work in linuxF
}

void PNTableView::dataRowActivated(const QModelIndex &t_index)
{
    Q_UNUSED(t_index);
}

void PNTableView::contextMenuEvent(QContextMenuEvent *t_e)
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();

    if ( !sortmodel )
        return;

    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    int row = this->selectionModel()->currentIndex().row();

    bool is_new_record = currentmodel->data(currentmodel->index(row, 0)).isNull();

    QMenu *menu = new QMenu(this);

    menu->addAction(resetColumns);
    menu->addSeparator();

    if (currentmodel && !currentmodel->isReadOnly())
    {
        if (!is_new_record)
        {
            menu->addAction(newRecord);
            menu->addAction(deleteRecord);
            menu->addAction(copyRecord);
        }

        if (m_has_open && !is_new_record) menu->addAction(openRecord);

        if (!is_new_record) menu->addAction(exportRecord);

        menu->addAction(filterRecords);
        menu->addSeparator();
    }

    // check for plugins
    menu->addSeparator();
    //STOPPED HERE

    QString table = ((PNSqlQueryModel*) ((QSortFilterProxyModel*)this->model())->sourceModel())->tablename();

    for ( PNPlugin* p : MainWindow::getPluginManager()->getPlugins())
    {
        if (p->hasDataRightClickEvent(table) && p->isEnabled())
        {
            QAction* act = menu->addAction(p->getPNPluginName(), [p, this](){slotPluginMenu(p);});
        }
    }

    menu->exec(t_e->globalPos());
    delete menu;
}

void PNTableView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    currentmodel->newRecord();
}

void PNTableView::slotDeleteRecord()
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    QModelIndexList qil = this->selectionModel()->selectedRows();

    for (auto qi = qil.begin(); qi != qil.end(); qi++)
        currentmodel->deleteRecord(*qi);
}

void PNTableView::slotCopyRecord()
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    QModelIndexList qil = this->selectionModel()->selectedRows();

    for (auto qi = qil.begin(); qi != qil.end(); qi++)
        currentmodel->copyRecord(*qi);
}

void PNTableView::slotOpenRecord()
{
    if (!m_has_open) // don't allow double click to work if not specified
        return;

    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    QModelIndexList qil = this->selectionModel()->selectedRows();

    for (auto qi = qil.begin(); qi != qil.end(); qi++)
        currentmodel->openRecord(*qi);

    emit signalOpenRecordWindow();
}

void PNTableView::slotExportRecord()
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    QModelIndexList qil = this->selectionModel()->selectedRows();

    QVariant keyval;

    auto qi = qil.begin();
    keyval = currentmodel->data(*qi);

    // choose the file
    QString xmlfile = QFileDialog::getSaveFileName(this, tr("Save XML to file"), QString(), tr("XML File (*.xml)"));

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    if (!xmlfile.isEmpty())
    {
        QFile outfile(xmlfile);

        PNSqlQueryModel *exportmodel = currentmodel->createExportVersion();
        exportmodel->setFilter(0, keyval.toString());
        exportmodel->refresh();

        QDomDocument* xdoc = global_DBObjects.createXMLExportDoc(exportmodel);

        if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        {
            QMessageBox::critical(this, tr("Open Failed"), outfile.errorString());
            delete xdoc;
            QApplication::restoreOverrideCursor();
            return;
        }

        QTextStream textstream(&outfile);

        xdoc->save(textstream, 4);
        outfile.close();
        delete xdoc;
    }

    QApplication::restoreOverrideCursor();
    QApplication::processEvents();
}

void PNTableView::slotFilterRecords()
{
    if (m_filterdialog == nullptr)
        m_filterdialog = new FilterDataDialog(this);

    PNSortFilterProxyModel* curmodel = (PNSortFilterProxyModel*)this->model();

    m_filterdialog->setSourceModelView((PNSqlQueryModel*)curmodel->sourceModel(), this);
    m_filterdialog->show();
}

void PNTableView::slotResetColumns()
{
    resizeColumnsToContents();
}

