// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ALLITEMSVIEW_H
#define ALLITEMSVIEW_H

#include "trackeritemsview.h"
#include "sqlcomboboxdelegate.h"
#include "dateeditdelegate.h"

class AllItemsView : public TrackerItemsView
{
public:
    AllItemsView(QWidget* parent = nullptr);
    ~AllItemsView();

    void setModel(QAbstractItemModel *model) override;

public slots:
    void slotNewRecord() override;

private:
    SqlComboBoxDelegate* m_allIdentifiedByDelegate = nullptr;
    SqlComboBoxDelegate* m_allAssignedToDelegate   = nullptr;
    SqlComboBoxDelegate* m_allProjectDelegate      = nullptr;
    DateEditDelegate*    m_allUpdatedDelegate      = nullptr;
};

#endif // ALLITEMSVIEW_H
