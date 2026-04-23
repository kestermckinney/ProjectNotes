// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
#include "trackeritemsview.h"
#include "databaseobjects.h"

TrackerItemsView::TrackerItemsView(QWidget* parent) : TableView(parent)
{
    setObjectName("tableViewTrackerItems");
    setHasOpen(true);
}

TrackerItemsView::~TrackerItemsView()
{
    if (m_actionItemTypeDelegate) delete m_actionItemTypeDelegate;
    if (m_identifiedByDelegate) delete m_identifiedByDelegate;
    if (m_dateIdentifiedDelegate) delete m_dateIdentifiedDelegate;
    if (m_assignedToDelegate) delete m_assignedToDelegate;
    if (m_priorityDelegate) delete m_priorityDelegate;
    if (m_statusDelegate) delete m_statusDelegate;
    if (m_dateDueDelegate) delete m_dateDueDelegate;
    if (m_dateDateUpdatedDelagate) delete m_dateDateUpdatedDelagate;
    if (m_dateResolvedDelegate) delete m_dateResolvedDelegate;
    if (m_meetingDelegate) delete m_meetingDelegate;
    if (m_projectDelegate) delete m_projectDelegate;
    if (m_internalDelegate) delete m_internalDelegate;
    if (m_itemNameDelegate) delete m_itemNameDelegate;
    if (m_descriptionDelegate) delete m_descriptionDelegate;

}

void TrackerItemsView::setModel(QAbstractItemModel *model)
{
    if (model)
    {
        TableView::setModel(model);

        // see setbuttonitems for visible columns
        setColumnHidden(17, true);
        setColumnHidden(18, true);
        setColumnHidden(19, true);
        setColumnHidden(20, true);

        // setup model lists
        m_itemPriority.setStringList(DatabaseObjects::item_priority);
        m_itemType.setStringList(DatabaseObjects::item_type);
        m_itemStatus.setStringList(DatabaseObjects::item_status);

        DatabaseObjects* dbo = qobject_cast<SqlQueryModel*>(dynamic_cast<SortFilterProxyModel*>(model)->sourceModel())->getDBOs();

        // projects list panel delegates
        m_actionItemTypeDelegate = new ComboBoxDelegate(this, &m_itemType);
        m_identifiedByDelegate = new SqlComboBoxDelegate(this, dbo->teamsmodelproxy(), 1, 3);
        m_dateIdentifiedDelegate = new DateEditDelegate(this);
        m_assignedToDelegate = new SqlComboBoxDelegate(this, dbo->teamsmodelproxy(), 1, 3);
        m_priorityDelegate = new ComboBoxDelegate(this, &m_itemPriority);
        m_statusDelegate = new ComboBoxDelegate(this, &m_itemStatus);
        m_dateDueDelegate = new DateEditDelegate(this);
        m_dateDateUpdatedDelagate = new DateEditDelegate(this);
        m_dateResolvedDelegate = new DateEditDelegate(this);
        m_meetingDelegate = new SqlComboBoxDelegate(this, dbo->trackeritemsmeetingsmodelproxy(), 2, 0);
        m_projectDelegate = new SqlComboBoxDelegate(this, dbo->projectslistmodelproxy());
        m_internalDelegate = new CheckBoxDelegate(this);

        m_itemNameDelegate = new PlainTextEditDelegate(this);
        m_descriptionDelegate = new PlainTextEditDelegate(this);

        // assign delegates to columns
        setItemDelegateForColumn(2, m_actionItemTypeDelegate);
        setItemDelegateForColumn(3, m_itemNameDelegate);
        setItemDelegateForColumn(4, m_identifiedByDelegate);
        setItemDelegateForColumn(5, m_dateIdentifiedDelegate);
        setItemDelegateForColumn(6, m_descriptionDelegate);
        setItemDelegateForColumn(7, m_assignedToDelegate);
        setItemDelegateForColumn(8, m_priorityDelegate);
        setItemDelegateForColumn(9, m_statusDelegate);
        setItemDelegateForColumn(10, m_dateDueDelegate);
        setItemDelegateForColumn(11, m_dateDateUpdatedDelagate);
        setItemDelegateForColumn(12, m_dateResolvedDelegate);
        setItemDelegateForColumn(13, m_meetingDelegate);
        setItemDelegateForColumn(14, m_projectDelegate);
        setItemDelegateForColumn(15, m_internalDelegate);
    }
    else
    {
        TableView::setModel(model);
    }
}

void TrackerItemsView::slotNewRecord()
{
    QSortFilterProxyModel* sortmodel = dynamic_cast<QSortFilterProxyModel*>(this->model());
    SqlQueryModel* currentmodel = dynamic_cast<SqlQueryModel*>(sortmodel->sourceModel());

    QVariant fk_value1 = dynamic_cast<TrackerItemsModel*>(currentmodel)->getFilter(14); // get the project id

    dynamic_cast<TrackerItemsModel*>(currentmodel)->newRecord(&fk_value1);
}

