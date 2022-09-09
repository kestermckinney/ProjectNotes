#include "trackeritemcommentsview.h"
#include "pndatabaseobjects.h"
#include <QHeaderView>

TrackerItemCommentsView::TrackerItemCommentsView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewTrackerItemComments");
}


TrackerItemCommentsView::~TrackerItemCommentsView()
{
    if (m_date_updated_delegate) delete m_date_updated_delegate;
    if (m_updated_by_delegate) delete m_updated_by_delegate;
}

void TrackerItemCommentsView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);

        // setup model lists

        // projects list panel delagets
        m_updated_by_delegate = new PNComboBoxDelegate(this, global_DBObjects.teamsmodel(), 1, 3);
        m_date_updated_delegate = new PNDateEditDelegate(this);

        setItemDelegateForColumn(4, m_updated_by_delegate);
        setItemDelegateForColumn(2, m_date_updated_delegate);

        // verticalHeader()->setVisible(true);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

