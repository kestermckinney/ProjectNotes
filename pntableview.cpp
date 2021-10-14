#include "pntableview.h"
#include "pnsettings.h"
#include "pnsqlquerymodel.h"

#include <QDebug>
#include <QMouseEvent>
#include <QApplication>
#include <QEvent>
#include <QHeaderView>

PNTableView::PNTableView(QWidget *parent) : QTableView(parent)
{
    setSortingEnabled(true);

    QHeaderView *headerView = horizontalHeader();

    headerView->setSortIndicator(-1, Qt::AscendingOrder);
    headerView->setSortIndicatorShown(false);
    headerView->viewport()->installEventFilter(this);

}

PNTableView::~PNTableView()
{
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

        if (indexAtCursor == -1)
            ; // Do nothing, we clicked outside the headers
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
