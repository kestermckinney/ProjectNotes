#ifndef MEETINGATTENDEESVIEW_H
#define MEETINGATTENDEESVIEW_H

#include "pntableview.h"
#include "pncomboboxdelegate.h"
#include "pncheckboxdelegate.h"
#include <QObject>

class MeetingAttendeesView : public PNTableView
{
public:
    MeetingAttendeesView(QWidget* t_parent = nullptr);
    ~MeetingAttendeesView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    // projects list panel delegates
    PNComboBoxDelegate* m_unfiltered_people_delegate = nullptr;
};

#endif // MEETINGATTENDEESVIEW_H
