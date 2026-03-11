// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef NOTESACTONITEMSVIEW_H
#define NOTESACTONITEMSVIEW_H

#include "tableview.h"
#include "sqlcomboboxdelegate.h"
#include "checkboxdelegate.h"
#include "dateeditdelegate.h"
#include "comboboxdelegate.h"
#include "plaintexteditdelegate.h"
#include <QObject>
#include <QStringListModel>

class NotesActionItemsView : public TableView
{
public:
    NotesActionItemsView(QWidget* parent = nullptr);
    ~NotesActionItemsView();

    void setModel(QAbstractItemModel *model) override;

private:
    QStringListModel m_itemType;//(DatabaseObjects::item_type);
    QStringListModel m_itemStatus;//DatabaseObjects::item_status;
    QStringListModel m_itemPriority;//DatabaseObjects::item_priority;

    // projects list panel delegates
//    SqlComboBoxDelegate* m_unfilteredPeopleDelegate = nullptr;
    ComboBoxDelegate* m_actionItemTypeDelegate = nullptr;
    SqlComboBoxDelegate* m_identifiedByDelegate = nullptr;
    ComboBoxDelegate* m_priorityDelegate = nullptr;
    DateEditDelegate* m_dateIdentifiedDelegate = nullptr;
    SqlComboBoxDelegate* m_assignedToDelegate = nullptr;
    ComboBoxDelegate* m_statusDelegate = nullptr;
    DateEditDelegate* m_dateDueDelegate = nullptr;
    DateEditDelegate* m_dateDateUpdatedDelagate = nullptr;
    DateEditDelegate* m_dateResolvedDelegate = nullptr;
    SqlComboBoxDelegate* m_meetingDelegate = nullptr;
    SqlComboBoxDelegate* m_projectDelegate = nullptr;
    CheckBoxDelegate* m_internalDelegate = nullptr;
    SqlComboBoxDelegate* m_clientDelegate = nullptr;
    PlainTextEditDelegate* m_itemNameDelegate = nullptr;
    PlainTextEditDelegate* m_itemDescriptionDelegate = nullptr;

public slots:
    void slotNewRecord() override;
};


#endif // NOTESACTONITEMSVIEW_H
