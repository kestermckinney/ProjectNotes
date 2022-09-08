#include "projectlocationsview.h"
#include "pndatabaseobjects.h"

ProjectLocationsView::ProjectLocationsView(QWidget* t_parent) : PNTableView(t_parent)
{
    setObjectName("tableViewProjectLocations");
    setHasOpen(true);
}

ProjectLocationsView::~ProjectLocationsView()
{
    if (m_file_type_delegate) delete m_file_type_delegate;
    if (m_file_button_delegate) delete m_file_button_delegate;
}

void ProjectLocationsView::setModel(QAbstractItemModel *t_model)
{
    if (t_model)
    {
        PNTableView::setModel(t_model);

        setColumnHidden(0, true);
        setColumnHidden(1, true);

        // see setbuttonitems for visible columns

        // setup model lists
        m_file_types.setStringList(PNDatabaseObjects::file_types);

        // projects list panel delagets
        m_file_type_delegate = new ComboBoxDelegate(this, &m_file_types);
        m_file_button_delegate = new PNLineEditFileButtonDelegate(this);

        // assign delegates to columns
        setItemDelegateForColumn(2, m_file_type_delegate);
        setItemDelegateForColumn(4, m_file_button_delegate);
    }
    else
    {
        PNTableView::setModel(t_model);
    }
}

//TODO: right-click open should open the file STOPPED HERE

