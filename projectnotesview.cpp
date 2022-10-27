#include "projectnotesview.h"

ProjectNotesView::ProjectNotesView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewProjectNotes");
    setHasOpen(true);
}

ProjectNotesView::~ProjectNotesView()
{
    if (m_meeting_date_delegate) delete m_meeting_date_delegate;
    if (m_internal_item_delegate) delete m_internal_item_delegate;
}

void ProjectNotesView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(4, true);

        // see setbuttonitems for visible columns

        // projects list panel delagets
        m_meeting_date_delegate = new PNDateEditDelegate(this);
        m_internal_item_delegate = new PNCheckBoxDelegate(this);

        // assign delegates to columns
        setItemDelegateForColumn(3, m_meeting_date_delegate);
        setItemDelegateForColumn(5, m_internal_item_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

//TODO: add html editor features to notes pages
//TODO: fix icons to be like the ones i created custom

