// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "tableview.h"
#include "appsettings.h"
#include "databaseobjects.h"
#include "databaseobjects.h"
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

TableView::TableView(QWidget *parent) : QTableView(parent)
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


    connect(this, &QTableView::activated, this, &TableView::dataRowActivated);
    connect(this, &QTableView::clicked, this, &TableView::dataRowSelected);   

    m_newRecord = new QAction(QIcon(":/icons/new-record.png"), tr("New"), this);
    m_deleteRecord = new QAction(QIcon(":/icons/delete.png"), tr("Delete"), this);
    m_openRecord = new QAction(QIcon(":/icons/folder.png"),tr("Open"), this);
    m_exportRecord = new QAction(tr("XML Export..."), this);
    m_filterRecords = new QAction(QIcon(":/icons/filter.png"), tr("Filter Settings..."), this);
    m_refreshRecords  = new QAction(tr("Refresh"), this);
    m_resetColumns = new QAction(tr("Reset Columns"), this);
    m_copyRecord = new QAction(QIcon(":/icons/copy.png"), tr("Copy"), this);


    connect(m_newRecord, &QAction::triggered, this, &TableView::slotNewRecord);
    connect(m_deleteRecord, &QAction::triggered, this, &TableView::slotDeleteRecord);
    connect(m_openRecord, &QAction::triggered, this, &TableView::slotOpenRecord);
    connect(m_exportRecord, &QAction::triggered, this, &TableView::slotExportRecord);
    connect(m_filterRecords, &QAction::triggered, this, &TableView::slotFilterRecords);
    connect(m_refreshRecords, &QAction::triggered, this, &TableView::slotRefreshRecords);
    connect(m_resetColumns, &QAction::triggered, this, &TableView::slotResetColumns);
    connect(m_copyRecord, &QAction::triggered, this, &TableView::slotCopyRecord);
}

TableView::~TableView()
{
    disconnect(this, &QTableView::activated, this, &TableView::dataRowActivated);
    disconnect(this, &QTableView::clicked, this, &TableView::dataRowSelected);

    disconnect(m_newRecord, &QAction::triggered, this, &TableView::slotNewRecord);
    disconnect(m_deleteRecord, &QAction::triggered, this, &TableView::slotDeleteRecord);
    disconnect(m_openRecord, &QAction::triggered, this, &TableView::slotOpenRecord);
    disconnect(m_exportRecord, &QAction::triggered, this, &TableView::slotExportRecord);
    disconnect(m_filterRecords, &QAction::triggered, this, &TableView::slotFilterRecords);
    disconnect(m_refreshRecords, &QAction::triggered, this, &TableView::slotRefreshRecords);
    disconnect(m_resetColumns, &QAction::triggered, this, &TableView::slotResetColumns);
    disconnect(m_copyRecord, &QAction::triggered, this, &TableView::slotCopyRecord);

    delete m_newRecord;
    delete m_deleteRecord;
    delete m_openRecord;
    delete m_exportRecord;
    delete m_filterRecords;
    delete m_refreshRecords;
    delete m_resetColumns;
    delete m_copyRecord;

    delete m_filterdialog;

    QHeaderView *headerView = horizontalHeader();
    headerView->removeEventFilter(this);
    QHeaderView *rowView = verticalHeader();
    rowView->removeEventFilter(this);
}


void TableView::slotPluginMenu(Plugin* plugin, const QString& functionname, const QString& exportname, const QString& tablefilter, const QString& parameter)
{
    QString response;

    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QModelIndexList qil = this->selectionModel()->selectedRows();

    QVariant keyval;

    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);
    keyval = currentmodel->data(qq);

    QApplication::processEvents();

    DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(currentmodel)->getDBOs();
    SqlQueryModel *exportmodel = dbo->createExportObject(currentmodel->tablename());

    exportmodel->setFilter(0, keyval.toString());
    exportmodel->refresh();


    QDomDocument* xdoc = dbo->createXMLExportDoc(exportmodel, tablefilter);
    QString xmlstr = xdoc->toString();

    // qDebug() << xmlstr;
    // qDebug() << tablefilter;

    // call the menu plugin with the data structure
    plugin->callXmlMethod(functionname, xmlstr, parameter);

    delete xdoc;
}

void TableView::setModel(QAbstractItemModel *model)
{
    if ( model ) // load view settings on create and set model
    {
        int Col;
        QString Dir;

        QTableView::setModel(model);

        global_Settings.getTableViewState(objectName(), *this);
        global_Settings.getTableSortColumn(objectName(), Col, Dir);

        if (Col >= 0)
        {
            Qt::SortOrder order = (Dir == "A") ? Qt::AscendingOrder : Qt::DescendingOrder;
            horizontalHeader()->setSortIndicator(Col, order);
            horizontalHeader()->setSortIndicatorShown(true);
            sortByColumn(Col, order);
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
    else if ( this->model() && model == nullptr ) // when closing or setting model to empty save the columns first on startup don't save a blank view
    {
        global_Settings.setTableViewState(objectName(), *this);

        QTableView::setModel(model);        
    }
}

bool TableView::eventFilter(QObject* watched, QEvent *event)
{
    Q_UNUSED(watched);

    switch (event->type())
    {
    case QEvent::MouseButtonDblClick:
        if ( ((QMouseEvent*)event)->buttons().testFlag(Qt::LeftButton ) )
        {         
            if ( this->viewport()->rect().contains(((QMouseEvent*)event)->pos()) &&
                 !this->horizontalHeader()->geometry().contains(((QMouseEvent*)event)->pos()) )
                slotOpenRecord();
            else
                return false;
        }
        break;
    case QEvent::MouseButtonPress:
        if ( ((QMouseEvent*)event)->button() != Qt::LeftButton )
            return false;
        m_pressPos = ((QMouseEvent*)event)->pos();
        break;

    case QEvent::MouseMove:
        if ( ((QMouseEvent*)event)->buttons().testFlag(Qt::LeftButton )
            && (m_pressPos - ((QMouseEvent*)event)->pos()).manhattanLength() > qApp->startDragDistance())
        {
            m_isMoving = true;
        }
        break;
    case QEvent::MouseButtonRelease:
    {
         if ( ((QMouseEvent*)event)->button() != Qt::LeftButton )
            return false;

         auto header = horizontalHeader();

         QMargins mg(4,4,4,4);
         QRect rct = header->geometry().marginsRemoved(mg); // only clickable inside rect
         if (!rct.contains(((QMouseEvent*)event)->pos(), true))
             return false;

        // If we were dragging a section, then pass the event on.
        if (m_isMoving)
        {
            m_isMoving = false;
            return false;
        }

        // don't sort if on the resizer line
        if ( header->cursor() == Qt::SplitHCursor )
            return false;

        const int indexAtCursor = header->logicalIndexAt(((QMouseEvent*)event)->pos());

        if (indexAtCursor == -1)
            return false; // Do nothing, we clicked outside the headers
        else if (header->sortIndicatorSection() != indexAtCursor)
        {
            header->setSortIndicator(indexAtCursor, Qt::AscendingOrder);
            header->setSortIndicatorShown(true);
            sortByColumn(indexAtCursor, Qt::AscendingOrder);
            global_Settings.setTableSortColumn(objectName(), indexAtCursor, "A");
        }
        else if (header->sortIndicatorOrder() == Qt::AscendingOrder)
        {
            header->setSortIndicator(indexAtCursor, Qt::DescendingOrder);
            sortByColumn(indexAtCursor, Qt::DescendingOrder);
            global_Settings.setTableSortColumn(objectName(), indexAtCursor, "D");
        }
        else
        {
            // Cycle back to unsorted — Qt ignores sortByColumn(-1), so call
            // model()->sort(-1) directly to clear the proxy model's sort order.
            header->setSortIndicator(-1, Qt::AscendingOrder);
            header->setSortIndicatorShown(false);
            if (model())
                model()->sort(-1, Qt::AscendingOrder);
            global_Settings.setTableSortColumn(objectName(), -1, "");
        }

        return true;
    }
    default:
        break;
    }
    return false;
}

void TableView::dataRowSelected(const QModelIndex &index)
{
    Q_UNUSED(index);
    emit rowSelectionChanged();
}

void TableView::dataRowActivated(const QModelIndex &index)
{
    Q_UNUSED(index);
}

void TableView::mouseMoveEvent(QMouseEvent *event)
{
    QTableView::mouseMoveEvent(event);
}

void TableView::leaveEvent(QEvent *event)
{
    QTableView::leaveEvent(event);
}

void TableView::paintEvent(QPaintEvent *event)
{
    QTableView::paintEvent(event);
}

void TableView::initViewItemOption(QStyleOptionViewItem *option) const
{
    QTableView::initViewItemOption(option);
    option->state &= ~QStyle::State_MouseOver;
}

bool TableView::viewportEvent(QEvent *event)
{
    // Suppress hover events so QAbstractItemView never sets its internal
    // hover index — this prevents the Windows native style from highlighting
    // cells on mouse-over regardless of initViewItemOption.
    if (event->type() == QEvent::HoverMove ||
        event->type() == QEvent::HoverEnter ||
        event->type() == QEvent::HoverLeave)
    {
        return QAbstractScrollArea::viewportEvent(event);
    }
    return QTableView::viewportEvent(event);
}

void TableView::sortMenu(QMenu* menu)
{
    // Extract actions from the menu
    QList<QAction*> actions = menu->actions();

    // Sort actions alphabetically by text
    std::sort(actions.begin(), actions.end(), [](QAction* a, QAction* b) {
        return a->text() < b->text();
    });

    // Clear the menu and re-add the sorted actions
    menu->clear();
    for (QAction* action : actions)
    {
        menu->addAction(action);
    }
}

void TableView::contextMenuEvent(QContextMenuEvent *e)
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());

    if ( !sortmodel )
        return;

    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    int row = this->selectionModel()->currentIndex().row();

    bool is_new_record = currentmodel->data(currentmodel->index(row, 0)).isNull();

    QMenu *menu = new QMenu(this);

    menu->addAction(m_resetColumns);
    menu->addSeparator();

    if (currentmodel && m_hasOpen && !is_new_record) menu->addAction(m_openRecord);

    if (currentmodel) menu->addAction(m_newRecord);

    if (currentmodel && !currentmodel->isReadOnly())
    {
        if (!is_new_record)
        {
            menu->addAction(m_deleteRecord);

            // Don't allow copying team members or meeting attendees
            QString modelName = currentmodel->objectName();
            if (modelName != "ProjectTeamMembersModel" && modelName != "MeetingAttendeesModel")
            {
                menu->addAction(m_copyRecord);
            }
        }

        if (!is_new_record) menu->addAction(m_exportRecord);

        menu->addAction(m_filterRecords);
        menu->addAction(m_refreshRecords);
        menu->addSeparator();
    }

    // check for plugins
    menu->addSeparator();

    QString table = dynamic_cast<SqlQueryModel*> (dynamic_cast<QSortFilterProxyModel*>(this->model())->sourceModel())->tablename();

    for ( Plugin* p : MainWindow::getPluginManager()->plugins())
    {
         for ( const PluginMenu& m : p->pythonplugin().menus())
        {
            if (m.dataexport().compare(table, Qt::CaseInsensitive) == 0)
            {
                QAction* act = new QAction(QIcon(":/icons/add-on.png"), m.menutitle(), menu);
                connect(act, &QAction::triggered, this,[p, m, this](){slotPluginMenu(p, m.functionname(), m.dataexport(), m.tablefilter(), m.parameter());});
                MainWindow::addMenuItem(menu, m.submenu(), m.menutitle(), act, 2);
            }
        }
    }

    menu->exec(e->globalPos());
    delete menu;
}

void TableView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    currentmodel->newRecord();
}

void TableView::slotDeleteRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QModelIndexList qil = this->selectionModel()->selectedRows();
    int v = verticalScrollBar()->value();
    int h = horizontalScrollBar()->value();


    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);
    currentmodel->deleteRecord(qq);

    // reposition the row select
    if (qi->row() > currentmodel->rowCount(rootIndex()))
    {
        setCurrentIndex(currentmodel->index(qi->row() - 1, qi->column()));
    }
    else
    {
        setCurrentIndex(*qi);
    }

    verticalScrollBar()->setValue(v);
    horizontalScrollBar()->setValue(h);
}

void TableView::slotCopyRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QModelIndexList qil = this->selectionModel()->selectedRows();

    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);
    currentmodel->copyRecord(qq);
}

void TableView::slotOpenRecord()
{
    if (!m_hasOpen) // don't allow double click to work if not specified
        return;

    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QModelIndexList qil = this->selectionModel()->selectedRows();
    auto qi = qil.begin();
    QModelIndex qq = sortmodel->mapToSource(*qi);
    QModelIndex kqi = currentmodel->index(qq.row(), m_keyToOpenField);
    QVariant record_id = currentmodel->data(kqi);

    emit signalOpenRecordWindow(record_id);
}

void TableView::slotExportRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

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

        DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(currentmodel)->getDBOs();
        SqlQueryModel *exportmodel = dbo->createExportObject(currentmodel->tablename());
        exportmodel->setFilter(0, keyval.toString());
        exportmodel->refresh();

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

void TableView::slotFilterRecords()
{
    if (m_filterdialog == nullptr)
        m_filterdialog = new FilterDataDialog(this);

    SortFilterProxyModel* curmodel = (SortFilterProxyModel*)this->model();

    m_filterdialog->setSourceModelView((SqlQueryModel*)curmodel->sourceModel(), this);
    m_filterdialog->show();
}

void TableView::slotRefreshRecords()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    currentmodel->refresh();
}

void TableView::slotResetColumns()
{
    resizeColumnsToContents();
}
