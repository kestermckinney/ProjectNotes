#include "trackeritemcommentsview.h"
#include "databaseobjects.h"
#include <QHeaderView>

TrackerItemCommentsView::TrackerItemCommentsView(QWidget* parent) : TableView(parent)
{
    setObjectName("tableViewTrackerItemComments");
}


TrackerItemCommentsView::~TrackerItemCommentsView()
{
    if (m_dateUpdatedDelegate) delete m_dateUpdatedDelegate;
    if (m_updatedByDelegate) delete m_updatedByDelegate;
    if (m_commentsDelegate) delete m_commentsDelegate;
}

void TrackerItemCommentsView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(5, true);
        setColumnHidden(6, true);
        setColumnHidden(7, true);
        setColumnHidden(8, true);
        setColumnHidden(9, true);

        // setup model lists

        DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(dynamic_cast<SortFilterProxyModel*>(model)->sourceModel())->getDBOs();

        // projects list panel delagets
        m_updatedByDelegate = new SqlComboBoxDelegate(this, dbo->teamsmodelproxy(), 1, 3);
        m_dateUpdatedDelegate = new DateEditDelegate(this);
        m_commentsDelegate = new PlainTextEditDelegate(this);

        setItemDelegateForColumn(4, m_updatedByDelegate);
        setItemDelegateForColumn(2, m_dateUpdatedDelegate);
        setItemDelegateForColumn(3, m_commentsDelegate);
    }
    else
    {
        TableView::setModel(model);
    }
}

void TrackerItemCommentsView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<TrackerItemCommentsModel*>(currentmodel)->getFilter(1); // get the project id

    dynamic_cast<TrackerItemCommentsModel*>(currentmodel)->newRecord(&fk_value1);
}
