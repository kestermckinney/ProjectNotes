// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MEETINGATTENDEESVIEW_H
#define MEETINGATTENDEESVIEW_H

#include "tableview.h"
#include "sqlcomboboxdelegate.h"
#include "checkboxdelegate.h"
#include <QObject>

class MeetingAttendeesView : public TableView
{
public:
    MeetingAttendeesView(QWidget* parent = nullptr);
    ~MeetingAttendeesView();

    void setModel(QAbstractItemModel *model) override;

private:
    // projects list panel delegates
    SqlComboBoxDelegate* m_unfilteredPeopleDelegate = nullptr;

public slots:
    void slotNewRecord() override;
};

#endif // MEETINGATTENDEESVIEW_H
