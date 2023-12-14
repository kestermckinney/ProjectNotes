// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TRACKERITEMSVIEW_H
#define TRACKERITEMSVIEW_H

#include "pntableview.h"
#include "pncomboboxdelegate.h"
#include "pncheckboxdelegate.h"
#include "pndateeditdelegate.h"
#include "pnplaintexteditdelegate.h"
#include "comboboxdelegate.h"
#include <QObject>
#include <QStringListModel>

class TrackerItemsView : public PNTableView
{
public:
    TrackerItemsView(QWidget* t_parent = nullptr);
    ~TrackerItemsView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    QStringListModel m_item_type;
    QStringListModel m_item_status;
    QStringListModel m_item_priority;

    // projects list panel delegates
    //PNComboBoxDelegate* m_unfiltered_people_delegate = nullptr;
    ComboBoxDelegate* m_action_item_type_delegate = nullptr;
    PNComboBoxDelegate* m_identified_by_delegate = nullptr;
    ComboBoxDelegate* m_priority_delegate = nullptr;
    PNDateEditDelegate* m_date_identified_delegate = nullptr;
    PNComboBoxDelegate* m_assigned_to_delegate = nullptr;
    ComboBoxDelegate* m_status_delegate = nullptr;
    PNDateEditDelegate* m_date_due_delegate = nullptr;
    PNDateEditDelegate* m_date_date_updated_delagate = nullptr;
    PNDateEditDelegate* m_date_resolved_delegate = nullptr;
    PNComboBoxDelegate* m_meeting_delegate = nullptr;
    PNComboBoxDelegate* m_project_delegate = nullptr;
    PNCheckBoxDelegate* m_internal_delegate = nullptr;
    //PNComboBoxDelegate* m_client_delegate = nullptr;
    NotEditableDelegate* m_not_editable_delegate = nullptr;
    PNPlainTextEditDelegate* m_item_name_delegate = nullptr;
    PNPlainTextEditDelegate* m_description_delegate = nullptr;
};


#endif // TRACKERITEMSVIEW_H
