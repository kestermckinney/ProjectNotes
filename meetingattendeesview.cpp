#include "meetingattendeesview.h"
#include "pndatabaseobjects.h"

MeetingAttendeesView::MeetingAttendeesView(QWidget* t_parent) : PNTableView(t_parent)
{

}

MeetingAttendeesView::~MeetingAttendeesView()
{
    if (m_unfiltered_people_delegate) delete m_unfiltered_people_delegate;
}

void MeetingAttendeesView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(3, true);

        // setup model lists

        // projects list panel delagets
        m_unfiltered_people_delegate = new PNComboBoxDelegate(this, global_DBObjects.teamsmodel(), 1, 3);

        setItemDelegateForColumn(2, m_unfiltered_people_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

