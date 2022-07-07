#ifndef PEOPLELISTVIEW_H
#define PEOPLELISTVIEW_H

#include "pntableview.h"
#include "pncomboboxdelegate.h"

class PeopleListView : public PNTableView
{
public:
    PeopleListView(QWidget* t_parent = nullptr);
    ~PeopleListView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    PNComboBoxDelegate* m_unfiltered_clients_delegate =  nullptr;
};

#endif // PEOPLELISTVIEW_H
