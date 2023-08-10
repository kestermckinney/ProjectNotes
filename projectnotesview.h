// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PROJECTNOTESVIEW_H
#define PROJECTNOTESVIEW_H

#include "pntableview.h"
#include "pndateeditdelegate.h"
#include "pncheckboxdelegate.h"
#include <QObject>


class ProjectNotesView : public PNTableView
{
public:
    ProjectNotesView(QWidget* t_parent = nullptr);
    ~ProjectNotesView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    // projects list panel delegates
    PNDateEditDelegate* m_meeting_date_delegate = nullptr;
    PNCheckBoxDelegate* m_internal_item_delegate = nullptr;
};


#endif // PROJECTNOTESVIEW_H
