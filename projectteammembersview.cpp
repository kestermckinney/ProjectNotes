#include "projectteammembersview.h"
#include "pndatabaseobjects.h"

ProjectTeamMembersView::ProjectTeamMembersView(QWidget* t_parent) : PNTableView(t_parent)
{

}

ProjectTeamMembersView::~ProjectTeamMembersView()
{
    if (m_unfiltered_people_delegate) delete m_unfiltered_people_delegate;
    if (m_receive_status_delegate) delete m_receive_status_delegate;
}

void ProjectTeamMembersView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);
        setColumnHidden(3, true);

        // setup model lists

        // projects list panel delagets
        m_unfiltered_people_delegate = new PNComboBoxDelegate(this, global_DBObjects.unfilteredpeoplemodel());
        m_receive_status_delegate = new PNCheckBoxDelegate(this);
        //STOPPED NEEED DELEGATE CHECKBOX

        setItemDelegateForColumn(2, m_unfiltered_people_delegate);
        setItemDelegateForColumn(4, m_receive_status_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

