// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "pntableview.h"
#include "pnsettings.h"
#include "pndatabaseobjects.h"
#include "pndatabaseobjects.h"
#include "mainwindow.h"
#include <QMouseEvent>
#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QMargins>

#include "QLogger.h"
#include "QLoggerWriter.h"

using namespace QLogger;

PNTableView::PNTableView(QWidget *t_parent) : QTableView(t_parent)
{
    setSortingEnabled(true);

    QHeaderView *headerView = horizontalHeader();

    headerView->setSortIndicator(-1, Qt::AscendingOrder);
    headerView->setSortIndicatorShown(true);
    headerView->viewport()->installEventFilter(this);

    QHeaderView *rowView = verticalHeader();

    rowView->viewport()->installEventFilter(this);
    rowView->setDefaultSectionSize(20);

    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    rowView->setSelectionMode(QAbstractItemView::SingleSelection);
    rowView->setSelectionBehavior(QAbstractItemView::SelectRows);


    connect(this, &QTableView::activated, this, &PNTableView::dataRowActivated);
    connect(this, &QTableView::clicked, this, &PNTableView::dataRowSelected);

    newRecord = new QAction(QIcon(":/icons/new-record.png"), tr("New"), this);
    deleteRecord = new QAction(QIcon(":/icons/delete.png"), tr("Delete"), this);
    openRecord = new QAction(QIcon(":/icons/folder.png"),tr("Open"), this);
    exportRecord = new QAction(tr("XML Export..."), this);
    filterRecords = new QAction(QIcon(":/icons/filter.png"), tr("Filter Settings..."), this);
    refreshRecords  = new QAction(tr("Refresh"), this);
    resetColumns = new QAction(tr("Reset Columns"), this);
    copyRecord = new QAction(QIcon(":/icons/copy.png"), tr("Copy"), this);


    connect(newRecord, &QAction::triggered, this, &PNTableView::slotNewRecord);
    connect(deleteRecord, &QAction::triggered, this, &PNTableView::slotDeleteRecord);
    connect(openRecord, &QAction::triggered, this, &PNTableView::slotOpenRecord);
    connect(exportRecord, &QAction::triggered, this, &PNTableView::slotExportRecord);
    connect(filterRecords, &QAction::triggered, this, &PNTableView::slotFilterRecords);
    connect(refreshRecords, &QAction::triggered, this, &PNTableView::slotRefreshRecords);
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
    disconnect(refreshRecords, &QAction::triggered, this, &PNTableView::slotRefreshRecords);
    disconnect(resetColumns, &QAction::triggered, this, &PNTableView::slotResetColumns);
    disconnect(copyRecord, &QAction::triggered, this, &PNTableView::slotCopyRecord);

    delete newRecord;
    delete deleteRecord;
    delete openRecord;
    delete exportRecord;
    delete filterRecords;
    delete refreshRecords;
    delete resetColumns;

    if (m_filterdialog)
        delete m_filterdialog;

    QHeaderView *headerView = horizontalHeader();
    headerView->removeEventFilter(this);
    QHeaderView *rowView = verticalHeader();
    rowView->removeEventFilter(this);
}


void PNTableView::slotPluginMenu(Plugin* t_plugin, const QString& t_functionname, const QString& t_exportname)
{
    QString response;

    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    QModelIndexList qil = this->selectionModel()->selectedRows();

    QVariant keyval;

    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);
    keyval = currentmodel->data(qq);

    QApplication::processEvents();

    PNSqlQueryModel *exportmodel = currentmodel->createExportVersion();
    exportmodel->setFilter(0, keyval.toString());
    exportmodel->refresh();

    PNDatabaseObjects* dbo = qobject_cast<PNSqlQueryModel*>(model())->getDBOs();

    QDomDocument* xdoc = dbo->createXMLExportDoc(exportmodel, t_exportname);
    QString xmlstr = xdoc->toString();

    // call the menu plugin with the data structure
    t_plugin->callXmlMethod(t_functionname, xmlstr);
    //response = t_plugin->callDataRightClickEvent(xmlstr);

    delete xdoc;

    // QApplication::processEvents();

    // if (!response.isEmpty())
    // {
    //     QApplication::processEvents();

    //     QDomDocument doc;
    //     doc.setContent(response);

    //     if (!dbo->importXMLDoc(doc))
    //         QMessageBox::critical(this, tr("Plugin Response Failed"), "Parsing XML file failed.");

    //     QApplication::processEvents();
    // }
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
            if ( this->viewport()->rect().contains(((QMouseEvent*)t_event)->pos()) &&
                 !this->horizontalHeader()->geometry().contains(((QMouseEvent*)t_event)->pos()) )
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

         QMargins mg(4,4,4,4);
         QRect rct = header->geometry().marginsRemoved(mg); // only clickable inside rect
         if (!rct.contains(((QMouseEvent*)t_event)->pos(), true))
             return false;

        // If we were dragging a section, then pass the event on.
        if (m_isMoving)
        {
            global_Settings.setTableViewState(objectName(), *this);

            m_isMoving = false;
            return false;
        }

        // don't sort if on the resizer line
        if ( header->cursor() == Qt::SplitHCursor )
            return false;

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
}

void PNTableView::dataRowActivated(const QModelIndex &t_index)
{
    Q_UNUSED(t_index);
}

void PNTableView::sortMenu(QMenu* t_menu)
{
    // Extract actions from the menu
    QList<QAction*> actions = t_menu->actions();

    // Sort actions alphabetically by text
    std::sort(actions.begin(), actions.end(), [](QAction* a, QAction* b) {
        return a->text() < b->text();
    });

    // Clear the menu and re-add the sorted actions
    t_menu->clear();
    for (QAction* action : actions)
    {
        t_menu->addAction(action);
    }
}

void PNTableView::contextMenuEvent(QContextMenuEvent *t_e)
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());

    if ( !sortmodel )
        return;

    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    int row = this->selectionModel()->currentIndex().row();

    bool is_new_record = currentmodel->data(currentmodel->index(row, 0)).isNull();

    QMenu *menu = new QMenu(this);

    menu->addAction(resetColumns);
    menu->addSeparator();

    if (currentmodel && m_has_open && !is_new_record) menu->addAction(openRecord);

    if (currentmodel) menu->addAction(newRecord);

    if (currentmodel && !currentmodel->isReadOnly())
    {
        if (!is_new_record)
        {
            menu->addAction(deleteRecord);
            menu->addAction(copyRecord);
        }

        if (!is_new_record) menu->addAction(exportRecord);

        menu->addAction(filterRecords);
        menu->addAction(refreshRecords);
        menu->addSeparator();
    }

    // check for plugins
    menu->addSeparator();

    QString table = ((PNSqlQueryModel*) ((QSortFilterProxyModel*)this->model())->sourceModel())->tablename();

    for ( Plugin* p : MainWindow::getPluginManager()->plugins())
    {
        for (QMap<QString, PluginMenu>::const_iterator it = p->pythonplugin().menus().cbegin(); it != p->pythonplugin().menus().cend(); ++it)
        //for ( PluginMenu pm : p->pythonplugin().menus()) //TODO: Remove
        {
            if (!it.value().dataexport().isEmpty())
            {
                if (it.value().submenu().isEmpty())
                {
                    QAction* bact = nullptr;

                    int pastseparator = 0;

                    for (QAction* action : menu->actions())
                    {
                        if (pastseparator > 1 && action->text().compare(it.key(), Qt::CaseInsensitive) > 0)
                            bact = action;

                        if (action->isSeparator())
                            pastseparator++;
                    }

                    if (bact)
                    {
                        QAction* act = new QAction(QIcon(":/icons/add-on.png"), it.key(), this);
                        connect(act, &QAction::triggered, this,[p, it, this](){slotPluginMenu(p, it.value().functionname(), it.value().dataexport());});
                        menu->insertAction(bact, act);
                    }
                    else
                    {
                        QAction* act = menu->addAction(it.key(), [p, it, this](){slotPluginMenu(p, it.value().functionname(), it.value().dataexport());});
                        act->setIcon(QIcon(":/icons/add-on.png"));
                    }
                }
                else
                {
                    // find the submenu if it exists
                    QMenu* submenu = nullptr;

                    int pastseparator = 0;

                    for (QAction* action : menu->actions())
                    {
                        if (pastseparator > 1 && action->text().compare(it.value().submenu(), Qt::CaseInsensitive) == 0)
                        {
                            submenu = action->menu();
                        }

                        if (action->isSeparator())
                        {
                            pastseparator++;
                        }
                    }

                    // if it didn't exist create it sorted
                    if (!submenu)
                    {
                        int pastseparator = 0;

                        for (QAction* action : menu->actions())
                        {
                            if (pastseparator > 1 && action->text().compare(it.value().submenu(), Qt::CaseInsensitive) > 0)
                            {
                                submenu = new QMenu(it.value().submenu());
                                menu->insertMenu(action, submenu);
                                break;
                            }

                            if (action->isSeparator())
                                pastseparator++;
                        }
                    }

                    if (!submenu)
                        submenu = menu->addMenu(it.value().submenu());

                    QAction* act = submenu->addAction(it.key(), [p, it, this](){slotPluginMenu(p, it.value().functionname(), it.value().dataexport());});
                    act->setIcon(QIcon(":/icons/add-on.png"));
                }
            }
        }
    }

    menu->exec(t_e->globalPos());
    delete menu;
}

void PNTableView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    currentmodel->newRecord();
}

void PNTableView::slotDeleteRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    QModelIndexList qil = this->selectionModel()->selectedRows();

    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);
    currentmodel->deleteRecord(qq);
}

void PNTableView::slotCopyRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    QModelIndexList qil = this->selectionModel()->selectedRows();

    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);
    currentmodel->copyRecord(qq);
}

void PNTableView::slotOpenRecord()
{
    if (!m_has_open) // don't allow double click to work if not specified
        return;

    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    QModelIndexList qil = this->selectionModel()->selectedRows();
    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);
    QModelIndex kqi = currentmodel->index(qq.row(), m_key_to_open_field);
    QVariant record_id = currentmodel->data(kqi);

    emit signalOpenRecordWindow(record_id);
}

void PNTableView::slotExportRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    QModelIndexList qil = this->selectionModel()->selectedRows();

    QVariant keyval;

    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);
    keyval = currentmodel->data(qq);

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

        PNDatabaseObjects* dbo = qobject_cast<PNSqlQueryModel*>(currentmodel)->getDBOs();

        QDomDocument* xdoc = dbo->createXMLExportDoc(exportmodel);

        if (!outfile.open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
        {
            QMessageBox::critical(this, tr("Open Failed"), outfile.errorString());
            delete xdoc;
            QApplication::restoreOverrideCursor();
            return;
        }

        QTextStream textstream(&outfile);
        textstream.setEncoding(QStringConverter::Utf8);

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

void PNTableView::slotRefreshRecords()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    PNSqlQueryModel* currentmodel = dynamic_cast<PNSqlQueryModel*>(sortmodel->sourceModel());

    currentmodel->refresh();
}

void PNTableView::slotResetColumns()
{
    resizeColumnsToContents();
}

