// Copyright (C) 2022, 2023 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TRACKERITEMSVIEW_H
#define TRACKERITEMSVIEW_H

#include "tableview.h"
#include "sqlcomboboxdelegate.h"
#include "checkboxdelegate.h"
#include "dateeditdelegate.h"
#include "plaintexteditdelegate.h"
#include "comboboxdelegate.h"
#include <QObject>
#include <QStringListModel>

class TrackerItemsView : public TableView
{
public:
    TrackerItemsView(QWidget* parent = nullptr);
    ~TrackerItemsView();

    void setModel(QAbstractItemModel *model) override;

private:
    QStringListModel m_itemType;
    QStringListModel m_itemStatus;
    QStringListModel m_itemPriority;

    // projects list panel delegates
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

    PlainTextEditDelegate* m_itemNameDelegate = nullptr;
    PlainTextEditDelegate* m_descriptionDelegate = nullptr;

public slots:
    void slotNewRecord() override;
};


#endif // TRACKERITEMSVIEW_H
