#include "pntableview.h"
#include "pnsettings.h"
#include "pnsqlquerymodel.h"

#include <QDebug>
#include <QMouseEvent>
#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QMenu>

PNTableView::PNTableView(QWidget *parent) : QTableView(parent)
{
    setSortingEnabled(true);

    QHeaderView *headerView = horizontalHeader();

    headerView->setSortIndicator(-1, Qt::AscendingOrder);
    headerView->setSortIndicatorShown(false);
    headerView->viewport()->installEventFilter(this);

    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(this, &QTableView::activated, this, &PNTableView::dataRowActivated);
    connect(this, &QTableView::clicked, this, &PNTableView::dataRowSelected);

    newRecord = new QAction(tr("New"), this);
    deleteRecord = new QAction(tr("Delete"), this);
    openRecord = new QAction(tr("Open"), this);
    exportRecord = new QAction(tr("XML Export..."), this);
    filterRecords = new QAction(tr("Filter Settings..."), this);


    connect(newRecord, &QAction::triggered, this, &PNTableView::slotNewRecord);
    connect(deleteRecord, &QAction::triggered, this, &PNTableView::slotDeleteRecord);
    connect(openRecord, &QAction::triggered, this, &PNTableView::slotOpenRecord);
    connect(exportRecord, &QAction::triggered, this, &PNTableView::slotExportRecord);
    connect(filterRecords, &QAction::triggered, this, &PNTableView::slotFilterRecords);
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

    delete newRecord;
    delete deleteRecord;
    delete openRecord;
    delete exportRecord;
    delete filterRecords;

    QHeaderView *headerView = horizontalHeader();
    headerView->removeEventFilter(this);
}


void PNTableView::setModel(QAbstractItemModel *model)
{
    QString storename = objectName();

    if ( model ) // load view settings on create and set model
    {
        int Col;
        QString Dir;

        QTableView::setModel(model);
        global_Settings.getTableViewState(storename, *this);
        global_Settings.getTableSortColumn(storename, Col, Dir);

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

    }
    else if ( this->model() ) // when closing or setting model to empty save the columns first on startup don't save a blank view
    {
        global_Settings.setTableViewState(storename, *this);
        QTableView::setModel(model);
    }
}

bool PNTableView::eventFilter(QObject * /*watched*/, QEvent *event)
{
    auto mouseEvent = [event] { return static_cast<QMouseEvent *>(event); };
    auto headerView = [this] { return horizontalHeader(); };

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (mouseEvent()->button() != Qt::LeftButton)
            return false;
        m_pressPos = mouseEvent()->pos();
        break;

    case QEvent::MouseMove:
        if (mouseEvent()->buttons().testFlag(Qt::LeftButton)
            && (m_pressPos - mouseEvent()->pos()).manhattanLength() > qApp->startDragDistance()) {
            m_isMoving = headerView()->sectionsMovable();
        }
        break;

    case QEvent::MouseButtonRelease: {
        QString storename = objectName();

        if (mouseEvent()->button() != Qt::LeftButton)
            return false;

        // If we were dragging a section, then pass the event on.
        if (m_isMoving) {
            m_isMoving = false;
            return false;
        }

        auto header = headerView();

        const int indexAtCursor = header->logicalIndexAt(mouseEvent()->pos());

        // TODO: FIX header sort when clicking on scroll bar and resizing
        // don't fire if someone clicks on scroll bar or resizes
        //if (!header->rect().contains(mouseEvent()->pos()))
        //    return false;

        if (indexAtCursor == -1)
            return false; // Do nothing, we clicked outside the headers
        else if (header->sortIndicatorSection() != indexAtCursor) {
            header->setSortIndicator(indexAtCursor, Qt::AscendingOrder);
            header->setSortIndicatorShown(true);
            global_Settings.setTableSortColumn(storename, indexAtCursor, "A");
        } else if (header->sortIndicatorOrder() == Qt::AscendingOrder) {
            header->setSortIndicator(indexAtCursor, Qt::DescendingOrder);
            global_Settings.setTableSortColumn(storename, indexAtCursor, "D");
        } else {
            header->setSortIndicator(-1, Qt::AscendingOrder);
            header->setSortIndicatorShown(false);
            global_Settings.setTableSortColumn(storename, -1, "");
        }
        emit header->sectionClicked(indexAtCursor);
        return true;
    }
    default:
        break;
    }
    return false;
}

void PNTableView::dataRowSelected(const QModelIndex &index)
{
    Q_UNUSED(index);
    // TODO: determine if base class should do anything
}

void PNTableView::dataRowActivated(const QModelIndex &index)
{
    Q_UNUSED(index);
    // TODO: determine if base class should do anything
}

void PNTableView::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = new QMenu(this);

    menu->addAction(newRecord);
    menu->addAction(deleteRecord);
    menu->addAction(openRecord);
    menu->addAction(exportRecord);
    menu->addAction(filterRecords);
    menu->addSeparator();

    menu->exec(e->globalPos());
    delete menu;
}

void PNTableView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    currentmodel->NewRecord();
}

void PNTableView::slotDeleteRecord()
{
    QSortFilterProxyModel* sortmodel = (QSortFilterProxyModel*) this->model();
    PNSqlQueryModel* currentmodel = (PNSqlQueryModel*) sortmodel->sourceModel();

    QModelIndexList qil = this->selectionModel()->selectedRows();

    for (auto qi = qil.begin(); qi != qil.end(); qi++)
        currentmodel->DeleteRecord(*qi);
}

void PNTableView::slotOpenRecord()
{
    // TODO: how should opening of the selected item be handled?
    QMessageBox::critical(nullptr, QObject::tr("Action Not Overriden"),
        tr("Open Record Needs Defined"), QMessageBox::Cancel);
}

void PNTableView::slotExportRecord()
{
    // TODO: standardize the export trigger
    QMessageBox::critical(nullptr, QObject::tr("Action Not Overriden"),
        tr("Export Record Needs Defined"), QMessageBox::Cancel);
}

void PNTableView::slotFilterRecords()
{
    // TODO: standardize the filter dialog call
    QMessageBox::critical(nullptr, QObject::tr("Action Not Overriden"),
        tr("Filter Record Needs Defined"), QMessageBox::Cancel);
}
