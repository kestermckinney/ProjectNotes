// Copyright (C) 2022, 2023, 2024, 2025, 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TRACKERITEMCOMMENTSVIEW_H
#define TRACKERITEMCOMMENTSVIEW_H

#include "tableview.h"

#include <QObject>
#include "dateeditdelegate.h"
#include "sqlcomboboxdelegate.h"
#include "plaintexteditdelegate.h"

class TrackerItemCommentsView : public TableView
{
public:
    TrackerItemCommentsView(QWidget* parent = nullptr);
    ~TrackerItemCommentsView();

    void setModel(QAbstractItemModel *model) override;

private:
    // projects list panel delegates
    SqlComboBoxDelegate* m_updatedByDelegate = nullptr;
    DateEditDelegate* m_dateUpdatedDelegate = nullptr;
    PlainTextEditDelegate* m_commentsDelegate = nullptr;

public slots:
    void slotNewRecord() override;
};

#endif // TRACKERITEMCOMMENTSVIEW_H
