#ifndef TRACKERITEMCOMMENTSVIEW_H
#define TRACKERITEMCOMMENTSVIEW_H

#include "pntableview.h"

#include <QObject>
#include <pndateeditdelegate.h>
#include <pncomboboxdelegate.h>

class TrackerItemCommentsView : public PNTableView
{
public:
    TrackerItemCommentsView(QWidget* t_parent = nullptr);
    ~TrackerItemCommentsView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    // projects list panel delegates
    PNComboBoxDelegate* m_updated_by_delegate = nullptr;
    PNDateEditDelegate* m_date_updated_delegate = nullptr;

};

#endif // TRACKERITEMCOMMENTSVIEW_H