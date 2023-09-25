// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef NOTESACTONITEMSVIEW_H
#define NOTESACTONITEMSVIEW_H

#include "pntableview.h"
#include "pncomboboxdelegate.h"
#include "pncheckboxdelegate.h"
#include "pndateeditdelegate.h"
#include "comboboxdelegate.h"
#include <QObject>
#include <QStringListModel>

class NotesActionItemsView : public PNTableView
{
public:
    NotesActionItemsView(QWidget* t_parent = nullptr);
    ~NotesActionItemsView();

    void setModel(QAbstractItemModel *t_model) override;

private:
    QStringListModel m_item_type;//(PNDatabaseObjects::item_type);
    QStringListModel m_item_status;//PNDatabaseObjects::item_status;
    QStringListModel m_item_priority;//PNDatabaseObjects::item_priority;

    // projects list panel delegates
//    PNComboBoxDelegate* m_unfiltered_people_delegate = nullptr;
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
    PNComboBoxDelegate* m_client_delegate = nullptr;
};


#endif // NOTESACTONITEMSVIEW_H
